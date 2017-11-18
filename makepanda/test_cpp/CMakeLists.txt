# Author: Younguk Kim (bluekyu)
# Date  : 2017-08-18

cmake_minimum_required(VERSION 3.6)
project(panda3d_test)

# === configure ====================================================================================
set(panda3d_INCLUDE_DIR "${panda3d_build_outputdir_path}/include")
set(panda3d_DEFAULT_COMPONENTS p3framework panda pandaexpress p3dtool p3dtoolconfig p3direct p3interrogatedb)

if(WIN32)
    set(panda3d_lib_prefix "lib")
else()
    set(panda3d_lib_prefix "")
endif()
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
        "${panda3d_build_outputdir_path}/lib/${panda3d_lib_prefix}${component_name}${panda3d_lib_suffix}${lib_suffix}"
    )
endforeach()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package(Threads REQUIRED)
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
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${panda3d_LIBRARY}
    $<$<PLATFORM_ID:Linux>:Threads::Threads>
)
# ==================================================================================================
