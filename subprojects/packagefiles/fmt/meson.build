project('fmt', 'cpp')

fmt_dep = declare_dependency(
    compile_args: [
        '-DFMT_STATIC_THOUSANDS_SEPARATOR=1',
        '-DFMT_USE_LONG_DOUBLE=0',
        '-DFMT_USE_DOUBLE=0',
        '-DFMT_USE_FLOAT=0'
    ],
    include_directories: 'include',
    sources: 'src/format.cc'
)