
#define _GNU_SOURCE

#include "ultmigration.h"

#include <assert.h>
#include <pthread.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <semaphore.h>

/* global initialization */

static int klt_count = 0;
static int initialized = 0;
pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;


struct thread_pool_info;

/* current ULT */
struct current_thread_info {
	struct thread_pool_info *pool_thread;
	uintptr_t stack;
	char aux_thread[4096];
	sem_t exit_sem;
};
static __thread struct current_thread_info *current;

/* thread pool for ULT execution */

#define THREAD_POOL_SIZE 1

#define STOP_THREAD ((void*)(uintptr_t)(-1))

struct thread_pool_info {
	struct current_thread_info *queue[8];
	uintptr_t stack;
	pthread_t thread;
	int cpu;
} __attribute__((aligned(64)));

// We have ULT_TYPE_MAX types of threads. For each type, there is a pool of
// THREAD_POOL_SIZE threads.
static struct thread_pool_info pool[ULT_TYPE_MAX][THREAD_POOL_SIZE];

static inline void __monitor(const void *address)
{
	/* "monitor %eax, %ecx, %edx;" */
	__asm volatile(".byte 0x0f, 0x01, 0xc8;"
	               :: "a" (address), "c" (0), "d"(0));
}

static inline void __mwait(const void *address)
{
	/* "mwait %eax, %ecx;" */
	__asm volatile(".byte 0x0f, 0x01, 0xc9;"
	               :: "a" (address), "c" (0));
}

struct current_thread_info *ult_pick_next_thread(struct thread_pool_info *pool_thread) {
	struct current_thread_info *next = NULL;
	int i;

	/* poll until a ULT is scheduled to run on this thread */
	while (1) {
mwait_retry:
		/* check all the queue entries for a non-null entry */
		for (i = 0; i < 8; i++) {
			/*next = __atomic_exchange_n(&pool_thread->queue[i],
			                           next,
			                           __ATOMIC_SEQ_CST);*/
			next = __atomic_load_n(&pool_thread->queue[i],
			                       __ATOMIC_SEQ_CST);
			if (next) {
				/* does not need to be atomic, we are the only
				 * writer */
				__atomic_store_n(&pool_thread->queue[i],
				                 NULL,
				                 __ATOMIC_SEQ_CST);
				return next;
			}
		}
		/* if none was found, sleep until the cache line changes */
		__monitor(pool_thread->queue);
		for (i = 0; i < 8; i++) {
			if (pool_thread->queue[i] != NULL) {
				goto mwait_retry;
			}
		}
		__mwait(pool_thread->queue);
		/* try again */
	}
}

void ult_set_pool_thread_affinity(struct thread_pool_info *pool_thread) {
	cpu_set_t cpus;
	CPU_ZERO(&cpus);
	CPU_SET(pool_thread->cpu, &cpus);
	sched_setaffinity(0, sizeof(cpus), &cpus);
}

extern void *ult_pool_thread_entry(void *param);

static void ult_initialize(void) {
	int i, t;
	char *cpu_list, *end;

	/* analyze the processor topology */
	char *slow_cpu = getenv("SLOW_CPU");
	char *fast_cpu = getenv("FAST_CPU");
	assert(slow_cpu != NULL && fast_cpu != NULL && "SLOW_CPU or FAST_CPU environment variable not set");

	/* create a thread pool */
	for (t = 0; t < ULT_TYPE_MAX; t++) {
		switch (t) {
			case ULT_FAST: cpu_list = fast_cpu; break;
			case ULT_SLOW: cpu_list = slow_cpu; break;
		}
		for (i = 0; i < THREAD_POOL_SIZE; i++) {
			// Read cpu_list as list of comma-separated integers.
			pool[t][i].cpu = strtol(cpu_list, &end, 10);
			assert(end != cpu_list && "SLOW_CPU/FAST_CPU contained invalid data");
			cpu_list = end;
			if (*cpu_list == ',') cpu_list++;

			pthread_create(&pool[t][i].thread,
				       NULL,
				       ult_pool_thread_entry,
				       &pool[t][i]);
		}
	}
}

static void ult_uninitialize(void) {
	/* this function is only called when the last ULT calls
	 * ult_unregister_klt, so we do not have to care about existing ready
	 * queue contents */

	int i, t;

	/* send the threads a message and wait for them to stop */
	for (t = 0; t < ULT_TYPE_MAX; t++) {
		for (i = 0; i < THREAD_POOL_SIZE; i++) {
			__atomic_store_n(&pool[t][i].queue[0],
					 STOP_THREAD,
					 __ATOMIC_SEQ_CST);
		}
	}
	for (t = 0; t < ULT_TYPE_MAX; t++) {
		for (i = 0; i < THREAD_POOL_SIZE; i++) {
			pthread_join(pool[t][i].thread, NULL);
		}
	}
}

void ult_register_asm(struct current_thread_info *thread,
                      struct thread_pool_info *first_klt);

void ult_register_klt(void) {
	pthread_mutex_lock(&init_mutex);
	klt_count++;
	if (!initialized) {
		ult_initialize();
		initialized = 1;
	}
	pthread_mutex_unlock(&init_mutex);

	/* allocate a second stack for this kernel-level thread */
	struct current_thread_info *thread =
			malloc(sizeof(struct current_thread_info));
	memset(thread, 0, sizeof(*thread));
	sem_init(&thread->exit_sem, 0, 0);

	/* store the pointer to the current thread in TLS so that it is always
	 * directly available via %fs */
	current = thread;

	/* migrate this user-level thread to the thread pool and let this
	 * kernel-level thread block until ult_unregister_klt migrates the ULT
	 * back */
	ult_register_asm(current, &pool[0][0]);
}

void ult_wait_for_unregister(struct current_thread_info *thread) {
	sem_wait(&thread->exit_sem);
}

void ult_signal_unregister(struct current_thread_info *thread) {
	sem_post(&thread->exit_sem);
}

void ult_unregister_asm(struct current_thread_info *thread);

void ult_unregister_klt(void) {
	if (current == NULL) {
		return;
	}
	/* migrate the thread to its original kernel-level thread */
	ult_unregister_asm(current);

	pthread_mutex_lock(&init_mutex);
	klt_count--;
	if (initialized && klt_count == 0) {
		ult_uninitialize();
		initialized = 0;
	}
	pthread_mutex_unlock(&init_mutex);
}

void ult_migrate_asm(struct current_thread_info *ult,
                     struct thread_pool_info *next);

void ult_migrate(enum ult_thread_type type) {
	assert(type >= 0 && type < ULT_TYPE_MAX);
	if (current == NULL) {
		return;
	}
	/* TODO: determine suitable threads in the thread pool for the phase */
	// TODO: Use more than one thread?
	struct thread_pool_info *next = &pool[type][0];
	if (next == current->pool_thread) {
		return;
	}
	ult_migrate_asm(current, next);
}

int ult_registered(void) {
	return current != NULL;
}
