@echo off

SET PANDA_PYTHON_PATH=thirdparty\win-python3.5-x64\python.exe 
SET THREADS=8
SET OPTIMIZE=4
SET MSVC_VERSION=14.1
SET WINDOWS_SDK=10.0.15063.0

SET EXTRA_INPUT=--nothing --use-direct --use-gl --use-eigen --use-openal --use-vorbis --use-zlib --use-png --use-jpeg --use-squish --use-freetype --use-assimp --use-egg --use-pandatool --use-sse2

%PANDA_PYTHON_PATH% makepanda\makepanda.py --threads %THREADS% --optimize %OPTIMIZE% --msvc-version=%MSVC_VERSION% --windows-sdk=%WINDOWS_SDK% %EXTRA_INPUT%
