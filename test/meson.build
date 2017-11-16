executable('simple', 'simple.c',
           link_with: ultmigration,
           dependencies: thread_dep,
           include_directories: include)

executable('ult_idle', 'ult_idle.c',
           link_with: ultmigration,
           dependencies: thread_dep,
           include_directories: include)

executable('micro', 'micro.c',
           link_with: [ultmigration, pmc],
           dependencies: thread_dep,
           include_directories: include)

executable('micro_pmc', 'micro.c', 'micro_pmc.c',
           link_with: [ultmigration, pmc],
           dependencies: thread_dep,
           include_directories: include)

executable('micro_swp', 'micro.c', 'micro_swp.c',
           link_with: [ultmigration, pmc, swp],
           dependencies: thread_dep,
           include_directories: include)
