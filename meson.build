project('async-scheduling', 'c', default_options: ['c_std=c11'])

thread_dep = dependency('threads')

include = include_directories('.')
ultmigration = shared_library('ultmigration',
	'ultmigration.c', 'ultmigration.s',
	dependencies: thread_dep,
	install: true)

install_headers('ultmigration.h')

subdir('test')
subdir('tools')
