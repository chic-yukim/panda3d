# Panda3D in CRSF

This build version is used in CRSF SDK.
(https://github.com/bluekyu/panda3d)


## Thrid-party Licenses
See more detail: https://www.panda3d.org/manual/index.php/Third-party_dependencies_and_license_info

And third-party license files are in 'thirdparty-licenses' directory.

- Eigen: MPL2
- OpenAL: LGPL
- libvorbis: BSD
- ZLib: zlib
- libpng: libpng
- libjpeg: libjpeg
- libjpeg-turbo: libjpeg-turbo
- libsquish: MIT
- Freetype: FreeType
- Assimp: assimp



# Building Panda3D

## Windows

We currently build using the Microsoft Visual Studio 2015 and 2017.
Visual Studio 2015 and 2017 are compatible, so you can use any one.
(for example, you can use VS2017 with VS2015 binary version.)

You will also need to have the third-party dependency libraries available for
the build scripts to use. These are available from the URL,
depending on whether you are on a 32-bit or 64-bit system:
https://www.panda3d.org/forums/viewtopic.php?f=9&t=18775

Note that it is VS2015 version, but you can built it with VS2017 as mentioned above.

After acquiring these dependencies, you may simply build Panda3D from the
command prompt using the following command:

```bash
.\build.bat
```


# Customizing

## Build Command
We use below command and this is written in 'build.bat' file.
```
thirdparty\win-python3.5-x64\python.exe makepanda\makepanda.py --threads 8 --optimize 4 --msvc-version=14 --windows-sdk=10.0.14393.0 --nothing --use-direct --use-gl --use-eigen --use-openal --use-vorbis --use-zlib --use-png --use-jpeg --use-squish --use-freetype --use-assimp --use-egg --use-pandatool --use-sse2
```


## Modified Codes
### vector_src.h
Below lines are added.
```
#undef EXPCL
#undef EXPTP
#define EXPCL
#define EXPTP
```

### dtoolbase.h
Modify below to use Boost above 1.60
(Use Windows Vista/Windows Server 2008 API version)
```
//#define _WIN32_WINNT 0x0502   // original code
#define _WIN32_WINNT 0x0600
```


### Third-party
#### fcollada
In FCollada/FUtils/Platforms.h, modify below to use Boost above 1.60
(Use Windows Vista/Windows Server 2008 API version)
```
#define _WIN32_WINNT 0x0600
```
