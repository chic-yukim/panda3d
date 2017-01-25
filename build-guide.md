# Build version
8b67ab31f511cb209d0ba3828c02ce3941ef36d8 - ok
f94db0661e076998d7cb3b0e827b31d1aae02d02 - ok
6f168446d05767371f43cf9e1c6cbcf0ca5a2f29 - ok
8381118a976184836cfccdd828ea315f44edf783
82b77286836d50f47526e5c9875b8ca3e891b38f - fail
29ea65bb3ff2352319580806349aad9715b36588 - fail


# Build Options
makepanda\makepanda.py --threads 8 --msvc-version=12 --windows-sdk=8.1 --everything --no-python --no-artoolkit --no-eigen --no-ffmpeg --no-fmodex --no-openal --no-openssl --no-squish --no-vrpn --no-fftw --no-opencv --no-awesomium --no-rocket --no-ode --no-maya2016

## Minimal Version
makepanda\makepanda.py --threads 8 --msvc-version=14 --windows-sdk=10 --nothing --use-direct --use-gl --use-zlib --use-png --use-jpeg --use-freetype --use-pandatool --use-sse2

## Windows 10 SDK
In makepanda/makepandacore.py, modify SDK version for `SDK["MSPLATFORM"]`.


## License Issue
* ffmpeg
* fmodex
* openal



# Modified Code
## vector_src.h
Below lines are added.
```
#undef EXPCL
#undef EXPTP
#define EXPCL
#define EXPTP
```

## mouseWatcher.h
`__declspec(noinline)` code is added.
```
__declspec(noinline) INLINE bool has_mouse() const;

__declspec(noinline) INLINE const LPoint2 &get_mouse() const;
```

## dtoolbase.h
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
