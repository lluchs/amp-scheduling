project('async-scheduling', ['c', 'cpp'], default_options: ['c_std=gnu11', 'cpp_std=gnu++14'])
cc = meson.get_compiler('c')

thread_dep = dependency('threads')
likwid = cc.find_library('likwid', required: false)

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
subdir('swp')
subdir('test')
