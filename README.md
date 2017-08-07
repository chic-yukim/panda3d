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

We currently build using the Microsoft Visual Studio 2015 and 2017 on Windows 10. (default is VS2017.)
Visual Studio 2015 and 2017 are compatible, so you can use any one.
(for example, you can use VS2017 with VS2015 binary version.)

You will also need to have the third-party dependency libraries available for
the build scripts to use. These are available from the URL,
depending on whether you are on a 32-bit or 64-bit system:
https://www.panda3d.org/forums/viewtopic.php?f=9&t=18775

Note that those libraries are VS2015 version, but you can build with VS2017 as the mentioned above.

After acquiring these dependencies, you may simply build Panda3D from the
command prompt using the following command:

```
.\build.bat
```

### Visual Studio 2015
If you use VS2015, then you need to modify 'build.bat' file as below.
```
SET MSVC_VERSION=14
```


# Customizing

## Build Command
See 'build.bat' file.


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
In order to use Boost above 1.60
(Use Windows Vista/Windows Server 2008 API version)
```
//#define _WIN32_WINNT 0x0502   // original code
#define _WIN32_WINNT 0x0600
```

### makepanda.py
```
/DWINVER=0x600
```



### Third-party
#### fcollada
In FCollada/FUtils/Platforms.h, modify below to use Boost above 1.60
(Use Windows Vista/Windows Server 2008 API version)
```
#define _WIN32_WINNT 0x0600
```
