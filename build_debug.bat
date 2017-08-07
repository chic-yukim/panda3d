@echo off

SET PANDA_PYTHON_PATH=thirdparty\win-python3.5-x64\python.exe
SET THREADS=8
SET OPTIMIZE=1
SET MSVC_VERSION=14.1
SET WINDOWS_SDK=10.0

SET EXTRA_INPUT=--nothing --use-direct --use-gl --use-eigen --use-egg --use-pandatool --use-sse2

%PANDA_PYTHON_PATH% makepanda\makepanda.py --outputdir built_x64_debug --threads %THREADS% --optimize %OPTIMIZE% --msvc-version=%MSVC_VERSION% --windows-sdk=%WINDOWS_SDK% %EXTRA_INPUT%
