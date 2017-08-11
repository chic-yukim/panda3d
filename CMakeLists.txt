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

set(panda3d_build_THREADS "8" CACHE "The number of threads" STRINGS)
set(panda3d_build_OPTIMIZE "4" CACHE "Optimization level" STRINGS)

set(panda3d_build_command ${PYTHON_EXECUTABLE} makepanda/makepanda.py --threads ${panda3d_build_THREADS} --optimize ${panda3d_build_OPTIMIZE})

if(WIN32)
    set(panda3d_build_MSVC_VERSION "14.1" CACHE "MSVC version" STRINGS)
    set(panda3d_build_WINDOWS_SDK "10.0" CACHE "Windows SDK version" STRINGS)
    set(panda3d_build_command ${panda3d_build_command} --msvc-version=${panda3d_build_MSVC_VERSION} --windows-sdk=${panda3d_build_WINDOWS_SDK})
endif()

set(panda3d_build_EXTRA_INPUT --nothing --use-direct --use-gl --use-eigen --use-openal --use-vorbis --use-zlib --use-png --use-jpeg --use-squish --use-freetype --use-assimp --use-egg --use-pandatool --use-sse2)

set(panda3d_build_command ${panda3d_build_command} ${panda3d_build_EXTRA_INPUT})

add_custom_target(panda3d ALL ${panda3d_build_command}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
