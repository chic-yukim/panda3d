# Author: Younguk Kim (bluekyu)
# Date  : 2017-08-12

cmake_minimum_required(VERSION 3.6)
project(panda3d)

if(WIN32)
    file(GLOB panda3d_thirdparty_python_dir "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/win-python*")
    if(EXISTS ${panda3d_thirdparty_python_dir})
        list(INSERT CMAKE_PROGRAM_PATH 0 ${panda3d_thirdparty_python_dir})
    endif()
endif()

find_package(PythonInterp REQUIRED)
find_package(PythonLibs REQUIRED)

# cache and options
set(panda3d_USE_OPTIONS "png" "jpeg" "squish" "assimp" "openal" "vorbis" "pandatool")
foreach(option_id ${panda3d_USE_OPTIONS})
    option(panda3d_use_${option_id} "Use ${option_id}" ON)
endforeach()
option(panda3d_build_minimal "Build with minimal options" OFF)
set(panda3d_build_THREADS "4" CACHE "The number of threads" STRINGS)
set(panda3d_build_OPTIMIZE "4" CACHE "Optimization level" STRINGS)

# setup build command
set(panda3d_build_command ${PYTHON_EXECUTABLE} makepanda/makepanda.py --threads ${panda3d_build_THREADS} --optimize ${panda3d_build_OPTIMIZE})

if(WIN32)
    set(panda3d_build_MSVC_VERSION "14.1" CACHE "MSVC version" STRINGS)
    set(panda3d_build_WINDOWS_SDK "10.0" CACHE "Windows SDK version" STRINGS)
    set(panda3d_build_command ${panda3d_build_command} --msvc-version=${panda3d_build_MSVC_VERSION} --windows-sdk=${panda3d_build_WINDOWS_SDK})
endif()

set(panda3d_build_EXTRA_INPUT --nothing --use-direct --use-gl --use-eigen --use-zlib --use-freetype --use-egg --use-sse2)

# apply options
if(NOT panda3d_build_minimal)
    foreach(option_id ${panda3d_USE_OPTIONS})
        if(panda3d_use_${option_id})
            set(panda3d_build_EXTRA_INPUT ${panda3d_build_EXTRA_INPUT} --use-${option_id})
        endif()
    endforeach()
endif()

set(panda3d_build_command ${panda3d_build_command} ${panda3d_build_EXTRA_INPUT})

add_custom_target(panda3d ALL ${panda3d_build_command}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
