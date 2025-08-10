# 🛠️ Animal Crossing Vita - Tools

**Asset Integration for AC-Decomp Vita Port**

## 🎯 **Key Understanding: No Asset Conversion Needed!**

After reviewing the original AC-Decomp project structure, we discovered that **AC-Decomp handles assets at compile-time**, not runtime:

```c
// How AC-Decomp handles assets (from their source files)
u8 inv_mwin_nwaku_tex[] = {
#include "assets/inv_mwin/inv_mwin_nwaku_tex.inc"
};

u16 inv_mwin_w1_tex_rgb_ci4_pal[] ATTRIBUTE_ALIGN(32) = {
#include "assets/inv_mwin_w1_tex_rgb_ci4_pal.inc"
};
```

**What this means**:
- Assets are **baked into the executable** during compilation
- No runtime asset loading system needed
- No texture format conversion required
- Our platform wrapper just needs to handle the GameCube → VitaGL API translation

## 📋 **What We Actually Need**

### **1. Original Game Files (User Provided)**
Place in `/dump/` directory:
- `main.dol` (rename to `static.dol`)
- `forest_1st.arc`
- `forest_2nd.arc` 
- `foresta.rel.szs`

### **2. AC-Decomp Build Process**
The original AC-Decomp project:
1. Extracts assets from game files → `assets/` directory
2. Includes assets at compile-time using `#include` statements
3. Builds complete game executable

### **3. Our Vita Integration**
Our approach:
1. Use AC-Decomp's existing asset system (no changes needed)
2. Compile AC-Decomp source with our platform wrapper
3. Platform wrapper translates GameCube API calls to VitaGL

## 🔧 **Build Process**

```bash
cd vita/
./build_vita.sh demo     # Test platform layer
./build_vita.sh full     # Build with AC-Decomp integration
```

## ✅ **Simplified Architecture**

```
AC-Decomp Source Files → Platform Wrapper → VitaGL → PlayStation Vita
     (includes assets)      (API translation)   (rendering)
```

**No asset conversion pipeline needed!** 🎉

This dramatically simplifies our project and eliminates a major source of complexity. 