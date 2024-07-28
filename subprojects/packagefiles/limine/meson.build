project('limine', 'c')

limine_includes = include_directories('.')

limine_exec = executable(
    'limine',
    'limine.c',
    include_directories: limine_includes
)

limine_dep = declare_dependency(
	include_directories: limine_includes
)
