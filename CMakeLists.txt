# Author: Younguk Kim (bluekyu)

cmake_minimum_required(VERSION 3.6)
project(panda3d)

# === configure ====================================================================================
set_property(GLOBAL PROPERTY USE_FOLDERS ON)    # Project Grouping

include(cmake/git-utils.cmake)

include(ProcessorCount)
ProcessorCount(process_count)
if(process_count EQUAL 0)
    set(process_count "1")
endif()

get_filename_component(panda3d_build_outputdir ${PROJECT_BINARY_DIR} NAME)
set(panda3d_build_outputdir "built-${panda3d_build_outputdir}")
set(panda3d_build_outputdir_path "${PROJECT_SOURCE_DIR}/${panda3d_build_outputdir}")

# cache options
if(WIN32)
    set(panda3d_thirdparty_base "" CACHE PATH "The path of thirdparty")
endif()
set(panda3d_build_threads "${process_count}" CACHE STRING "The number of threads")
set(panda3d_build_optimize "4" CACHE STRING "Optimization level (1-4)")
option(panda3d_build_verbose "Verbose" OFF)

# package options
option(panda3d_build_everything "Build with everything options" OFF)
option(panda3d_build_eigen "Build with --use-eigen options" ON)
option(panda3d_build_pandatool "Build with --use-pandatool options" OFF)
option(panda3d_build_assimp "Build with --use-assimp options" OFF)
option(panda3d_build_maya "Build with --use-maya options" OFF)
option(panda3d_build_max "Build with --use-max options" OFF)
option(panda3d_build_python "Build with --use-python options" OFF)
option(panda3d_enable_fixed_function "Unset 'SUPPORT_FIXED_FUNCTION=UNDEF'" OFF)
option(panda3d_enable_pstats "Set 'DO_PSTATS=1'" OFF)

set(panda3d_build_packages "direct" "gl" "openal" "vorbis" "zlib" "png" "jpeg" "tiff" "openexr"
    "squish" "egg" "freetype" "harfbuzz" "directcam" "vision" "x11" "sse2"
)

if(panda3d_build_eigen)
    set(panda3d_build_packages ${panda3d_build_packages} "eigen")
endif()

if(panda3d_build_python)
    set(panda3d_build_packages ${panda3d_build_packages} "python")
endif()

if(panda3d_build_pandatool)
    set(panda3d_build_packages ${panda3d_build_packages} "pandatool")
endif()

if(panda3d_build_assimp)
    set(panda3d_build_packages ${panda3d_build_packages} "assimp")
    if(NOT panda3d_build_pandatool)
        message(WARNING "pandatool is disabled.")
    endif()
endif()

if(panda3d_build_maya)
    set(panda3d_build_packages ${panda3d_build_packages}
        "maya6" "maya65" "maya7" "maya8" "maya85" "maya2008" "maya2009" "maya2010" "maya2011" "maya2012" "maya2013"
        "maya20135" "maya2014" "maya2015" "maya2016" "maya20165" "maya2017" #"maya2018"
    )
    if(NOT panda3d_build_pandatool)
        message(WARNING "pandatool is disabled.")
    endif()
endif()

if(panda3d_build_max)
    set(panda3d_build_packages ${panda3d_build_packages}
        "max6" "max7" "max8" "max9" "max2009" "max2010" "max2011" "max2012" "max2013" "max2014"
    )
    if(NOT panda3d_build_pandatool)
        message(WARNING "pandatool is disabled.")
    endif()
endif()

find_package(PythonInterp REQUIRED)
find_package(PythonLibs REQUIRED)

# setup build command
set(panda3d_build_command ${PYTHON_EXECUTABLE} makepanda/makepanda.py
    --outputdir \"${panda3d_build_outputdir}\"
    --threads ${panda3d_build_threads}
    --optimize ${panda3d_build_optimize}
)
if(${panda3d_build_verbose})
    set(panda3d_build_command ${panda3d_build_command} --verbose)
endif()

if(WIN32)
    if(MSVC_VERSION GREATER_EQUAL 1930)
        message(FATAL_ERROR "Unknown Visual Studio.")
    elseif(MSVC_VERSION GREATER_EQUAL 1920)    # vs2019
        set(panda3d_build_msvc_version "14.2")
        set(panda3d_build_default_windows_sdk "10.0")
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
    set(panda3d_build_windows_sdk ${panda3d_build_default_windows_sdk} CACHE STRING "Windows SDK version")

    set(panda3d_build_command ${panda3d_build_command}
        --msvc-version=${panda3d_build_msvc_version}
        --windows-sdk=${panda3d_build_windows_sdk}
    )
endif()

if(panda3d_enable_pstats)
    set(panda3d_build_command ${panda3d_build_command} --override DO_PSTATS=1)
endif()

if(NOT panda3d_enable_fixed_function)
    set(panda3d_build_command ${panda3d_build_command} --override SUPPORT_FIXED_FUNCTION=UNDEF)
endif()

# apply package options
if(panda3d_build_everything)
    set(panda3d_build_command ${panda3d_build_command} --everything)
else()
    set(panda3d_build_command ${panda3d_build_command} --nothing)
    foreach(pkg_id ${panda3d_build_packages})
        set(panda3d_build_command ${panda3d_build_command} --use-${pkg_id})
    endforeach()
endif()

get_git_head_hash(${CMAKE_CURRENT_SOURCE_DIR} panda3d_GIT_COMMIT)
if (panda3d_GIT_COMMIT)
    message("[panda3d] git commit: ${panda3d_GIT_COMMIT}")
    set(panda3d_build_command ${panda3d_build_command} --git-commit ${panda3d_GIT_COMMIT})
endif()

# setup environment variables
if(EXISTS "${panda3d_thirdparty_base}")
    set(panda3d_build_env MAKEPANDA_THIRDPARTY=\"${panda3d_thirdparty_base}\")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(panda3d_build_env ${panda3d_build_env}
        CC=\"${CMAKE_C_COMPILER}\"
        CXX=\"${CMAKE_CXX_COMPILER}\"
    )
endif()
# ==================================================================================================

# === target =======================================================================================
if(panda3d_build_env)
    add_custom_target(panda3d ${CMAKE_COMMAND} -E env ${panda3d_build_env}
        ${panda3d_build_command}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
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
