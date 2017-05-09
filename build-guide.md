# Build Options
```
makepanda\makepanda.py --threads 8 --optimize 4 --msvc-version=14 --windows-sdk=10 --nothing --use-direct --use-gl --use-zlib --use-png --use-jpeg --use-freetype --use-egg --use-pandatool --use-sse2
```

## Windows 10 SDK
In makepanda/makepandacore.py, modify SDK version for `SDK["MSPLATFORM"]`.


## License Issue (for closed app)
* ffmpeg
* fmodex
* openal: LGPL



# Modified Code
## vector_src.h
Below lines are added.
```
#undef EXPCL
#undef EXPTP
#define EXPCL
#define EXPTP
```

## dtoolbase.h
Modify below to use Boost 1.61.
```
//#define _WIN32_WINNT 0x0502   // original code
#define _WIN32_WINNT 0x0600
```

## Thirdparty
### fcollada
In FCollada/FUtils/Platforms.h, modify below to use Boost 1.61
```
#define _WIN32_WINNT 0x0600
```
