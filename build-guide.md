# Build Options
```
makepanda\makepanda.py --threads 8 --optimize 4 --msvc-version=14 --windows-sdk=10.0.14393.0 --nothing --use-direct --use-gl --use-eigen --use-vorbis --use-zlib --use-png --use-jpeg --use-squish --use-freetype --use-egg --use-pandatool --use-sse2
```


# License Issues (for closed app)
* ffmpeg: LGPL
* fmodex: free for only non-commercial
* openal: LGPL



# Modified Codes
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

## geomVertexData.h
Remove conversion warning in 'utility' header.
(It appears when utility header is included in front of geomVertexData.h)
```
// typedef pmap<const VertexTransform *, int> TransformMap;
typedef pmap<const VertexTransform *, size_t> TransformMap;
```

## Thirdparty
### fcollada
In FCollada/FUtils/Platforms.h, modify below to use Boost 1.61
```
#define _WIN32_WINNT 0x0600
```
