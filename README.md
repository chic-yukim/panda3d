# Panda3D for Render Pipeline C++

This repository is used to develop [Render Pipeline C++](https://github.com/bluekyu/render_pipeline_cpp)



## Build Status
| Build Status                       | About                       |
| :--------------------------------: | :-------------------------: |
| [![azure-badge]][azure-link]       | Visual Studio 2015 and 2017 |
| [![appveyor-badge]][appveyor-link] | Visual Studio 2017 Preview  |

[azure-badge]: https://dev.azure.com/bluekyu/rpcpp-devops/_apis/build/status/panda3d/panda3d "Azure build status"
[azure-link]: https://dev.azure.com/bluekyu/rpcpp-devops/_build/latest?definitionId=3 "Azure build link"
[appveyor-badge]: https://ci.appveyor.com/api/projects/status/dti693iydj981tu5/branch/master?svg=true "AppVeyor build status"
[appveyor-link]: https://ci.appveyor.com/project/bluekyu/panda3d/branch/master "AppVeyor build link"

You can download built files from each Build Page.



##### Note
- These builds uses only partial third-parties, not everything. So, some third-parties may be omitted.
- Windows Debug: Optimize option is 1 with MixForDebug in [panda3d-thirdparty](https://github.com/bluekyu/panda3d-thirdparty)
- Windows Release: Optimize option is 4 with Release in [panda3d-thirdparty](https://github.com/bluekyu/panda3d-thirdparty)



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
- libtiff: libtiff



# Building Panda3D
## Windows

I currently build using the Microsoft Visual Studio 2015 and 2017 on Windows 10. (default is VS2017.)
Visual Studio 2015 and 2017 are binary compatible, so you can use any one.
(for example, you can use VS2017 with VS2015 binary version.)

You will also need to have the third-party dependency libraries available for
the build scripts to use. These are available from the URL,
depending on whether you are on a 32-bit or 64-bit system:
https://www.panda3d.org/forums/viewtopic.php?f=9&t=18775

Note that those libraries are VS2015 version, but you can build with VS2017 as the mentioned above.
OR, you can build from my [panda3d-thirdparty](https://github.com/bluekyu/panda3d-thirdparty) repository.

After acquiring these dependencies, you may simply build Panda3D from CMake.

## Linux

See [README](https://github.com/panda3d/panda3d) in official repository. OR, you can use CMake.



# Customizing

See github-compare for all differences:
https://github.com/panda3d/panda3d/compare/master...bluekyu:master
