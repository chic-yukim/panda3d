# Panda3D in Develop Branch

This **develop** branch is used to develop [Render Pipeline C++](https://github.com/bluekyu/render_pipeline_cpp)

In this project, **master** branch is used to track upstream (https://github.com/panda3d/panda3d)



## Build Status

| OS       | Build Status             | Latest Build                                      |
| :------: | :----------------------: | :-----------------------------------------------: |
| Windows  | [![win-badge]][win-link] | vc14 ([Debug][win-debug], [Release][win-release]) |
| Linux    | [![nix-badge]][nix-link] |                                                   |

[win-badge]: https://ci.appveyor.com/api/projects/status/dti693iydj981tu5/branch/develop?svg=true "AppVeyor build status"
[win-link]: https://ci.appveyor.com/project/bluekyu/panda3d/branch/develop "AppVeyor build link"
[win-debug]: https://ci.appveyor.com/api/projects/bluekyu/panda3d/artifacts/panda3d.7z?branch=develop&job=Configuration%3A+Debug "Download latest build (Debug)"
[win-release]: https://ci.appveyor.com/api/projects/bluekyu/panda3d/artifacts/panda3d.7z?branch=develop&job=Configuration%3A+Release "Download latest build (Release)"
[nix-badge]: https://travis-ci.org/bluekyu/panda3d.svg?branch=develop "Travis build status"
[nix-link]: https://travis-ci.org/bluekyu/panda3d "Travis build link"

##### Note
- These builds are default builds, not everything. So, some files may be omitted.
- Windows Debug: Optimize option is 1.
- Windows Release: Optimize option is 4.



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
- OpenEXR: Modified BSD
- libtiff: BSD-like licence



# Building Panda3D
## Windows

We currently build using the Microsoft Visual Studio 2015 and 2017 on Windows 10. (default is VS2017.)
Visual Studio 2015 and 2017 are binary compatible, so you can use any one.
(for example, you can use VS2017 with VS2015 binary version.)

You will also need to have the third-party dependency libraries available for
the build scripts to use. These are available from the URL,
depending on whether you are on a 32-bit or 64-bit system:
https://www.panda3d.org/forums/viewtopic.php?f=9&t=18775

Note that those libraries are VS2015 version, but you can build with VS2017 as the mentioned above.
OR, you can build from my [panda3d-thirdparty](https://github.com/bluekyu/panda3d-thirdparty) repository.

After acquiring these dependencies, you may simply build Panda3D from CMake GUI.


## Linux

See [README](https://github.com/bluekyu/panda3d) in official repository. OR, you can use CMake.



# Customizing

## Build Command
See 'build.bat' file.


## Modified Codes
### Pull Requested, but Not Merged
- Support for Visual Studio 2017 (https://github.com/panda3d/panda3d/compare/master...bluekyu:feature/vs2017)



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

### dtoolbase_cc.h
Move `DEFAULT_CTOR` macro to the block of Visual Studio 2015.
```
#elif defined(_MSC_VER) && _MSC_VER >= 1900 // Visual Studio 2015
#  define DEFAULT_CTOR = default
```


### Third-party
#### fcollada
In FCollada/FUtils/Platforms.h, modify below to use Boost above 1.60
(Use Windows Vista/Windows Server 2008 API version)
```
#define _WIN32_WINNT 0x0600
```
