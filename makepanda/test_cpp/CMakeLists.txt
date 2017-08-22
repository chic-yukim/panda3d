# Author: Younguk Kim (bluekyu)
# Date  : 2017-08-18

cmake_minimum_required(VERSION 3.6)
project(panda3d_test)

# === configure ====================================================================================
if(WIN32)
    set(panda3d_import_location "${PROJECT_SOURCE_DIR}/${panda3d_build_outputdir}/lib/lib")
else()
    set(panda3d_import_location "")
endif()
set(panda3d_components p3framework panda pandaexpress p3dtool p3dtoolconfig p3direct p3interrogatedb)
foreach(component_name ${panda3d_components})
    list(APPEND panda3d_libraries "${panda3d_import_location}${component_name}${CMAKE_IMPORT_LIBRARY_SUFFIX}")
endforeach()
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

target_include_directories(${PROJECT_NAME} PRIVATE "${panda3d_build_outputdir}/include")
target_link_libraries(${PROJECT_NAME} PRIVATE ${panda3d_libraries})
# ==================================================================================================
