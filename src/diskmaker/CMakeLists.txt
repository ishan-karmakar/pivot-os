set(CMAKE_CXX_FLAGS "")

project(
    diskmaker
    VERSION 0.1
    DESCRIPTION "Helper utility to streamline process of creating disk images"
    LANGUAGES CXX
)

add_executable(
	diskmaker
	diskmaker.cpp
)

target_compile_features(diskmaker PRIVATE cxx_std_17)