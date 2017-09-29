cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

executable('amdpstate', 'amdpstate.c', 'msrtools.c', 'amdccx.c',
           dependencies: m_dep)
