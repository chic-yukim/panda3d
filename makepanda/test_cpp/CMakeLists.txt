# Author: Younguk Kim (bluekyu)

cmake_minimum_required(VERSION 3.9)
project(panda3d_test
    LANGUAGES CXX
)

# === configure ====================================================================================
set(panda3d_INCLUDE_DIR "${panda3d_build_outputdir_path}/include")
set(panda3d_DEFAULT_COMPONENTS p3framework panda pandaexpress p3dtool p3dtoolconfig p3direct p3interrogatedb)

if(${panda3d_build_optimize} GREATER_EQUAL "3")
    set(panda3d_lib_suffix "")
else()
    set(panda3d_lib_suffix "_d")
endif()


# TODO: for static
# for shared
list(GET CMAKE_FIND_LIBRARY_SUFFIXES 0 lib_suffix)

foreach(component_name ${panda3d_DEFAULT_COMPONENTS})
    list(APPEND panda3d_LIBRARY
        "${panda3d_build_outputdir_path}/lib/lib${component_name}${panda3d_lib_suffix}${lib_suffix}"
    )
endforeach()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    find_package(Eigen3 REQUIRED)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package(Threads REQUIRED)
    find_package(Eigen3 REQUIRED)
endif()
# ==================================================================================================

# === sources ======================================================================================
set(panda3d_test_sources_files
    "${PROJECT_SOURCE_DIR}/main.cpp"
)
# ==================================================================================================

# === target =======================================================================================
add_executable(${PROJECT_NAME} ${panda3d_test_sources_files})
add_dependencies(${PROJECT_NAME} panda3d)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_auto_type)
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /MP /wd4251)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${panda3d_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${panda3d_LIBRARY})

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    target_link_libraries(${PROJECT_NAME} PRIVATE Eigen3::Eigen)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads Eigen3::Eigen)
endif()
# ==================================================================================================
