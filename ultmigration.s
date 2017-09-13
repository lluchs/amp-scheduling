
.global ult_pool_thread_entry
	/* rdi = current thread_pool_info */
ult_pool_thread_entry:
	/* push callee-saved registers to the stack */
	sub $64, %rsp
	rdfsbase %rax
	mov %rax, 56(%rsp)
	rdgsbase %rax
	mov %rax, 48(%rsp)
	mov %rbp, 40(%rsp)
	mov %rbx, 32(%rsp)
	mov %r12, 24(%rsp)
	mov %r13, 16(%rsp)
	mov %r14, 8(%rsp)
	mov %r15, 0(%rsp)
	/* save stack pointer to the thread struct so that we can restart from
	 * here when a user level thread returns control */
	mov %rsp, 64(%rdi)

	push %rdi
	call ult_set_pool_thread_affinity
	pop %rdi

pick_next_thread:
	/* pick the next thread to execute */
	push %rdi
	call ult_pick_next_thread
	pop %rdi

	cmp $0xffffffffffffffff, %rax
	je stop_pool_thread
	/* valid next thread, switch to it */

	/* register the kernel level thread in the ULT struct */
	mov %rdi, (%rax)

	/* restore the thread state */

	/* load the stack pointer */
	mov 8(%rax), %rsp
	/* restore callee-saved registers */
restore_thread:
	mov 56(%rsp), %rax
	wrfsbase %rax
	mov 48(%rsp), %rax
	wrgsbase %rax
	mov 40(%rsp), %rbp
	mov 32(%rsp), %rbx
	mov 24(%rsp), %r12
	mov 16(%rsp), %r13
	mov 8(%rsp), %r14
	mov 0(%rsp), %r15
	add $64, %rsp
	/* return to the caller */
	ret


stop_pool_thread:
	/* restore callee-saved registers */
	mov 56(%rsp), %rax
	wrfsbase %rax
	mov 48(%rsp), %rax
	wrgsbase %rax
	mov 40(%rsp), %rbp
	mov 32(%rsp), %rbx
	mov 24(%rsp), %r12
	mov 16(%rsp), %r13
	mov 8(%rsp), %r14
	mov 0(%rsp), %r15
	add $64, %rsp
	/* return to the caller */
	ret


.global ult_migrate_asm
ult_migrate_asm:
	/* push callee-saved registers to the stack */
	sub $64, %rsp
	rdfsbase %rax
	mov %rax, 56(%rsp)
	rdgsbase %rax
	mov %rax, 48(%rsp)
	mov %rbp, 40(%rsp)
	mov %rbx, 32(%rsp)
	mov %r12, 24(%rsp)
	mov %r13, 16(%rsp)
	mov %r14, 8(%rsp)
	mov %r15, 0(%rsp)

	/* save stack pointer in user-level thread struct */
	mov %rsp, 8(%rdi)

	/* switch stack (required if signals interrupt this thread) */
	mov (%rdi), %rdx /* current kernel-level thread */
	mov 64(%rdx), %rsp

	/* insert thread into destination ready list */
	mov $7, %rcx
	mov $0, %rax
1:
	add $1, %rcx
	and $7, %rcx
	lock cmpxchg %rdi, 24(%rsi, %rcx, 8) /* wakes up the destination */
	jnz 1b

	/* let this kernel-level thread wait for the next ULT */
	mov %rdx, %rdi
	jmp pick_next_thread

.global ult_register_asm
ult_register_asm:
	/* push callee-saved registers to the stack */
	sub $64, %rsp
	rdfsbase %rax
	mov %rax, 56(%rsp)
	rdgsbase %rax
	mov %rax, 48(%rsp)
	mov %rbp, 40(%rsp)
	mov %rbx, 32(%rsp)
	mov %r12, 24(%rsp)
	mov %r13, 16(%rsp)
	mov %r14, 8(%rsp)
	mov %r15, 0(%rsp)

	/* save stack pointer in user-level thread struct */
	mov %rsp, 8(%rdi)

	/* switch stack */
	mov %rdi, %rax
	add $0x1010, %rax
	mov %rax, %rsp

	/* align stack */
	and $0xfffffffffffffff0, %rsp
	sub $8, %rsp

	/* insert thread into destination ready list */
	mov $7, %rcx
	mov $0, %rax
1:
	add $1, %rcx
	and $7, %rcx
	lock cmpxchg %rdi, 24(%rsi, %rcx, 8) /* wakes up the destination */
	jnz 1b

	/* wait for the ULT to finish */
	push %rdi
	call ult_wait_for_unregister
	pop %rdi

	/* restore the thread */
	mov 8(%rdi), %rsp
	jmp restore_thread

.global ult_unregister_asm
ult_unregister_asm:
	/* push callee-saved registers to the stack */
	sub $64, %rsp
	rdfsbase %rax
	mov %rax, 56(%rsp)
	rdgsbase %rax
	mov %rax, 48(%rsp)
	mov %rbp, 40(%rsp)
	mov %rbx, 32(%rsp)
	mov %r12, 24(%rsp)
	mov %r13, 16(%rsp)
	mov %r14, 8(%rsp)
	mov %r15, 0(%rsp)

	/* save stack pointer in user-level thread struct */
	mov %rsp, 8(%rdi)

	/* switch stack (required if signals interrupt this thread) */
	mov (%rdi), %rdx /* current kernel-level thread */
	mov 64(%rdx), %rsp

	push %rdx
	call ult_signal_unregister
	pop %rdi

	jmp pick_next_thread

