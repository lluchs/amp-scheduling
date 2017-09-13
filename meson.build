project('async-scheduling', 'c')

thread_dep = dependency('threads')

include = include_directories('.')
ultmigration = static_library('ultmigration', 'ultmigration.c', 'ultmigration.s')

subdir('test')
subdir('tools')
