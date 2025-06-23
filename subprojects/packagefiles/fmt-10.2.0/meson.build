project('fmt', 'cpp', license: 'BSD', default_options: ['cpp_std=c++14'])

args = [
    '-DFMT_STATIC_THOUSANDS_SEPARATOR=1',
    '-DFMT_USE_LONG_DOUBLE=0',
    '-DFMT_USE_DOUBLE=0',
    '-DFMT_USE_FLOAT=0',
    '-DFMT_HEADER_ONLY',
]

fmt_dep = declare_dependency(compile_args : args, include_directories : include_directories('include'))

if meson.version().version_compare('>=0.54.0')
    meson.override_dependency('fmt', fmt_dep)
endif
