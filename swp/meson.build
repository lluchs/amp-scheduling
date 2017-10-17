
if likwid.found()
	swp = shared_library('swp',
		'swp.cpp',
		dependencies: [thread_dep, likwid],
		cpp_args: ['-DLIKWID_PERFMON'],
		install: true)

	install_headers('swp.h')
endif
