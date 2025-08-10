# 🎮 Animal Crossing Vita Port - Build Details

**Quick build guide for the PlayStation Vita port.**

## 🚀 **Build Commands**

```bash
./build_vita.sh demo     # Test platform layer (current target)
./build_vita.sh full     # Build with AC-Decomp integration
./build_vita.sh clean    # Clean build directory
```

## 🔧 **Current Issues**

### **Platform Compilation Errors** (Active)
From last build attempt:
- `vglInitExtended()` return value handling  
- Missing `glNormal3f()` function
- Duplicate function definitions
- Type errors in platform wrapper

### **Next Actions**
1. Fix platform compilation errors in `platform/ac_vita_platform.c`
2. Test demo build success
3. Attempt full AC-Decomp integration build
4. Add missing GameCube API functions as needed

## 📁 **Platform Files**

```
platform/
├── ac_vita_platform.h          # GameCube API → Vita wrapper declarations
├── ac_vita_platform.c          # Platform wrapper implementation  
├── vita_main_demo.c             # Demo mode (platform test)
└── vita_main_ac_game.c          # Full game mode (AC-Decomp integration)
```

## 🎯 **Integration Strategy**

- **Zero root project changes**: All Vita code in `vita/` directory
- **Reference upstream**: Build system points to `../ac-decomp-upstream/`
- **Platform wrapper**: Translates GameCube SDK calls to VitaGL/Vita SDK
- **Iterative development**: Add GameCube API functions as compilation reveals them

## 📊 **Build System**

**CMake Integration**:
- Includes AC-Decomp headers: `../ac-decomp-upstream/include/`
- Compiles AC-Decomp source: `../ac-decomp-upstream/src/*.c`
- Links VitaGL and Vita libraries
- Packages as VPK for PlayStation Vita

**Docker Environment**:
- VitaSDK with VitaGL, VitaShaRK, SceShaccCgExt
- Consistent cross-platform builds
- Isolated from host system dependencies 