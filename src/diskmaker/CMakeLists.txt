project(diskmaker CXX)

add_executable(
    diskmaker
    diskmaker.cpp
    fat32.cpp
)
target_compile_features(diskmaker PRIVATE cxx_std_17)
target_link_libraries(diskmaker parted)