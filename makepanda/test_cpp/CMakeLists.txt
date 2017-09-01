# Author: Younguk Kim (bluekyu)
# Date  : 2017-08-18

cmake_minimum_required(VERSION 3.6)
project(panda3d_test)

# === configure ====================================================================================
set(panda3d_includes "${panda3d_build_outputdir_path}/include")
set(panda3d_components p3framework panda pandaexpress p3dtool p3dtoolconfig p3direct p3interrogatedb)

# TODO: for static
# for shared
list(GET CMAKE_FIND_LIBRARY_SUFFIXES 0 lib_suffix)

foreach(component_name ${panda3d_components})
    list(APPEND panda3d_libraries "${panda3d_build_outputdir_path}/lib/lib${component_name}${lib_suffix}")
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

target_include_directories(${PROJECT_NAME} PRIVATE "${panda3d_includes}")
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${panda3d_libraries}
    $<$<PLATFORM_ID:Linux>:Threads::Threads>
)
# ==================================================================================================
