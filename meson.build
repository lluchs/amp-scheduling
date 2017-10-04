project('async-scheduling', 'c', default_options: ['c_std=c11'])

thread_dep = dependency('threads')

include = include_directories('.')
ultmigration = shared_library('ultmigration',
	'ultmigration.c', 'ultmigration.s',
	dependencies: thread_dep,
	install: true)

shared_library('ultmigration_dummy',
	'ultmigration_dummy.c',
	install: true)

install_headers('ultmigration.h')

subdir('tools')
subdir('pmc')
subdir('test')
