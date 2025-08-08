# 🎮 Animal Crossing PlayStation Vita Port

**Status**: ✅ **FOUNDATION COMPLETE + INSTALLATION WORKING**  
**VPK**: `VPK/AnimalCrossingVita-v1.2-installation-fix.vpk` (115KB)  
**Next**: Test on Vita hardware → GX API wrapper implementation

## 🚀 Quick Start

### **Fast Development** (⚡ ~5 seconds)
```bash
# For rapid iteration during development
./dev_build.sh

# First run: ~25 seconds (builds dev Docker image)
# Subsequent runs: ~5 seconds (reuses image)
```

### **Production Build** (~2 minutes)
```bash
# For final/release builds with full validation
./build_docker.sh

# Result: build/AnimalCrossingVita.vpk (115KB)
# Organized: VPK/AnimalCrossingVita-v1.4-vertex-arrays.vpk
```

## 📋 What Works

- ✅ **Complete build system** (Docker + CMake + VitaSDK)
- ✅ **All dependencies** (VitaGL + VitaShaRK + SceShaccCgExt)
- ✅ **Graphics rendering** (VitaGL → sceGxm → SGX543MP4+)
- ✅ **Vita integration** (Controller input, LiveArea assets)

## 🎯 Next Phase

**Goal**: Create GameCube GX → VitaGL wrapper
- Implement `GXBegin()`, `GXPosition3f32()`, `GXEnd()`
- Map GX primitives to OpenGL equivalents
- Test with simple Animal Crossing scene

## 📖 Documentation

- **Complete Progress**: `VITA_PORT_SAVEPOINT.md`
- **Build Details**: `BUILD_SUCCESS_SUMMARY.md`
- **Original Roadmap**: Check parent directory for roadmap files

## 🔧 Technical Foundation

### VitaGL Architecture Understanding
**CRITICAL**: VitaGL provides complete OpenGL API compatibility. Standard OpenGL functions like `glBegin()`, `glEnd()`, `glClear()` ARE VitaGL functions that get translated to sceGxm automatically.

### Build Configuration
```dockerfile
# Optimized for Animal Crossing
HAVE_SBRK=1 HAVE_SHARK=1 HAVE_GLSL_SUPPORT=1
MATH_SPEEDHACK=1 DRAW_SPEEDHACK=1
CIRCULAR_VERTEX_POOL=1 SHARED_RENDERTARGETS=1
```

### Test Application
Current VPK renders:
- Blue background (VitaGL clearing)
- RGB triangle (OpenGL immediate mode)
- START button exits (Vita controller integration) 