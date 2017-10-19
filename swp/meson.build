
if likwid.found()
	swp = shared_library('swp',
		'swp.cpp', 'swp_util.cpp',
		dependencies: [thread_dep, likwid],
		cpp_args: ['-DLIKWID_PERFMON'],
		install: true)
endif

swp_migrate = shared_library('swp_migrate',
	'swp_migrate.cpp', 'swp_util.cpp',
	dependencies: [thread_dep],
	link_with: [ultmigration],
	install: true)

install_headers('swp.h')
