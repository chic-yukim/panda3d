# Author: Younguk Kim (bluekyu)
# Date  : 2017-08-12

cmake_minimum_required(VERSION 3.6)
project(panda3d)

# === configure ====================================================================================
set_property(GLOBAL PROPERTY USE_FOLDERS ON)    # Project Grouping

get_filename_component(panda3d_build_outputdir ${PROJECT_BINARY_DIR} NAME)
set(panda3d_build_outputdir "${panda3d_build_outputdir}-built")
set(panda3d_build_outputdir_path "${PROJECT_SOURCE_DIR}/${panda3d_build_outputdir}")

# cache options
if(WIN32)
    set(panda3d_thirdparty_base "" CACHE PATH "The path of thirdparty")
endif()
set(panda3d_build_threads "4" CACHE STRINGS "The number of threads")
set(panda3d_build_optimize "4" CACHE STRINGS "Optimization level (1-4)")

# package options
option(panda3d_build_everything "Build with everything options" OFF)
set(panda3d_build_default_packages "direct" "gl" "eigen" "zlib" "png" "jpeg" "squish"
    "openal" "vorbis" "assimp" "egg" "freetype" "pandatool" "sse2"
)

find_package(PythonInterp REQUIRED)
find_package(PythonLibs REQUIRED)

# setup build command
set(panda3d_build_command ${PYTHON_EXECUTABLE} makepanda/makepanda.py
    --outputdir \"${panda3d_build_outputdir}\"
    --threads ${panda3d_build_threads}
    --optimize ${panda3d_build_optimize}
)

if(WIN32)
    if(MSVC_VERSION GREATER 1920)
        message(FATAL_ERROR "Unknown Visual Studio.")
    elseif(MSVC_VERSION GREATER_EQUAL 1910)    # vs2017
        set(panda3d_build_msvc_version "14.1")
        set(panda3d_build_default_windows_sdk "10.0")
    elseif(MSVC_VERSION GREATER_EQUAL 1900)    # vs2015
        set(panda3d_build_msvc_version "14")
        set(panda3d_build_default_windows_sdk "10.0")
    elseif(MSVC_VERSION GREATER_EQUAL 1800)    # vs2013
        set(panda3d_build_msvc_version "12")
        set(panda3d_build_default_windows_sdk "8.1")
    elseif(MSVC_VERSION GREATER_EQUAL 1700)    # vs2012
        set(panda3d_build_msvc_version "11")
        set(panda3d_build_default_windows_sdk "7.1")
    elseif(MSVC_VERSION GREATER_EQUAL 1600)    # vs2010
        set(panda3d_build_msvc_version "10")
        set(panda3d_build_default_windows_sdk "7.1")
    else()
        message(FATAL_ERROR "Unsupported Visual Studio.")
    endif()
    set(panda3d_build_windows_sdk ${panda3d_build_default_windows_sdk} CACHE "Windows SDK version" STRINGS)

    set(panda3d_build_command ${panda3d_build_command}
        --msvc-version=${panda3d_build_msvc_version}
        --windows-sdk=${panda3d_build_windows_sdk}
        --override PHAVE_MINMAX_H=UNDEF
    )
endif()

# apply package options
if(panda3d_build_everything)
    set(panda3d_build_command ${panda3d_build_command} --everything)
else()
    set(panda3d_build_command ${panda3d_build_command} --nothing)
    foreach(pkg_id ${panda3d_build_default_packages})
        set(panda3d_build_command ${panda3d_build_command} --use-${pkg_id})
    endforeach()
endif()

set(pdna3d_git_dir_path "${PROJECT_SOURCE_DIR}/.git")
if(EXISTS "${pdna3d_git_dir_path}")
    if(EXISTS "${pdna3d_git_dir_path}/HEAD")
        file(STRINGS "${pdna3d_git_dir_path}/HEAD" panda3d_git_HEAD_contents)
        string(REGEX MATCH "^[0-9a-f]+$" panda3d_git_HEAD_hash ${panda3d_git_HEAD_contents})
        if(NOT panda3d_git_HEAD_hash)
            string(REGEX REPLACE "^ref: (.*)$" "\\1" panda3d_git_HEAD_ref_path ${panda3d_git_HEAD_contents})
            file(STRINGS "${pdna3d_git_dir_path}/${panda3d_git_HEAD_ref_path}" panda3d_git_HEAD_hash)
        endif()

        if(panda3d_git_HEAD_hash)
            message(STATUS "[panda3d] git-commit: ${panda3d_git_HEAD_hash}")
            set(panda3d_build_command ${panda3d_build_command} --git-commit ${panda3d_git_HEAD_hash})
        else()
            message(WARNING "[panda3d] Cannot find git-commit hash.")
        endif()
    else()
        message(ERROR "[git-utils] HEAD file does NOT exist in .git directory!")
    endif()
endif()
# ==================================================================================================

# === target =======================================================================================
if(EXISTS "${panda3d_thirdparty_base}")
    add_custom_target(panda3d ${CMAKE_COMMAND} -E env
        MAKEPANDA_THIRDPARTY=\"${panda3d_thirdparty_base}\"
        ${panda3d_build_command}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
else()
    add_custom_target(panda3d ${panda3d_build_command} WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endif()
# ==================================================================================================

# === test =========================================================================================
# runtime test
enable_testing()
add_subdirectory("${PROJECT_SOURCE_DIR}/makepanda/test_cpp")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(panda3d_build_rpath "PATH=%PATH%;${panda3d_build_outputdir_path}/bin")
else()
    set(panda3d_build_rpath "LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${panda3d_build_outputdir_path}/lib")
endif()

add_test(NAME panda3d_test_cpp
    COMMAND ${CMAKE_COMMAND} -E env "${panda3d_build_rpath}" $<TARGET_FILE:panda3d_test>
)
# ==================================================================================================

# === install ======================================================================================
install(DIRECTORY "${panda3d_build_outputdir_path}/" DESTINATION "."
        PATTERN "tmp" EXCLUDE)
# ==================================================================================================
