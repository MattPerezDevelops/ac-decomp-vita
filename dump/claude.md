# Plan: Animal Crossing PC Port - Three-Layer Abstraction Strategy

## Executive Summary

This document outlines a complete strategy to port Animal Crossing from the GameCube decompilation to PC, using a three-layer abstraction that separates game logic, porting layer, and host implementation.

**Why PC Port Instead of N64 Port:**
- N64 has 8MB RAM limit; the game needs ~12MB (insurmountable without major cuts)
- PC has unlimited memory, eliminating the core blocker
- The existing emu64 system already parses N64 display lists perfectly
- PC port allows modern graphics (OpenGL/Vulkan) with higher resolutions
- Result is more portable (Windows, Linux, Mac, potentially consoles)

---

## Analysis of N64 Port Work (To Be Shelved)

### What Was Created (159 files changed/added)

#### 1. N64 Compatibility Headers (`include/n64_compat/`)
~25 header files that stub/replace GameCube headers:
- `dolphin/os.h` - Maps OSGetTime, OSAllocFromHeap, etc. to N64 equivalents
- `dolphin/gx.h` - GX texture/blend constants (stubbed)
- `dolphin/types.h` - Base type definitions (u8, s16, etc.)
- `dolphin/pad.h`, `dolphin/dvd.h`, `dolphin/mtx.h` - Stubs
- `card/` - Memory card stubs
- `MSL_C/` - Math/string compatibility

#### 2. N64 Runtime Implementation (`src/n64/`)
~15 C/ASM files implementing N64 hardware interfaces:

| File | Purpose | Lines |
|------|---------|-------|
| `n64_memory.c` | Custom heap allocator for 8MB constraint | 316 |
| `n64_timing.c` | OSGetTime using MIPS Count register | ~100 |
| `n64_controller.c` | PIF controller communication | ~150 |
| `n64_audio.c` | Audio Interface (AI) programming | ~200 |
| `gc_stubs.c` | 300+ GameCube function stubs | 301 |
| `boot.s` | N64 ROM header + bootstrap | ~200 |
| `emu64_n64.c` | Graphics passthrough (key file) | ~150 |
| `graph_n64.c` | Adapted graphics system | ~100 |

#### 3. Modified Source Files (~64 files)
- `include/types.h` - Added TARGET_N64 conditionals
- `include/graph.h` - N64 display list compatibility
- `src/graph.c` - Graphics system adaptations
- Various model files - Removed GameCube-specific pragmas

#### 4. Build System
- `Makefile.n64_full` - Complete N64 build (172KB)
- `config/N64/*.ld` - 5 linker script variants
- `configure_n64*.py` - Build configuration scripts

### Why N64 Port Failed

**The Insurmountable Problem:**
```
Section Sizes:
.text   = 2.28 MB (must be in RAM)
.rodata = 3.67 MB (could stay in ROM via KSEG1)
.data   = 5.63 MB (must be in RAM)
.bss    = 1.18 MB (must be in RAM)
───────────────────
RAM needed: 9.09 MB minimum
N64 has:    8.00 MB maximum

Gap: 1.09 MB OVER LIMIT even with all optimizations
```

Even with KSEG1 ROM access for .rodata, we're still 1MB over. This would require:
- Major content cuts (unacceptable)
- Streaming/DMA complexity (months of work)
- Reducing vertex data to .rodata (massive code changes)

**Decision: Pivot to PC Port**

---

## Three-Layer Abstraction Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    LAYER 1: GAME LOGIC                      │
│         Pure C code speaking N64 concepts only              │
│   (actors, items, villagers, display list generation)       │
│                                                             │
│   Uses: osGetTime(), osCreateThread(), gsSPVertex(), etc.   │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│              LAYER 2: PORTING ABSTRACTION LAYER             │
│     Implements N64 OS/Hardware API using abstractions       │
│                                                             │
│   osCreateThread() → SDL_CreateThread()                     │
│   osGetTime()      → SDL_GetPerformanceCounter()            │
│   osSpTaskStart()  → Submit display list to GBI interpreter │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│            LAYER 3: HOST SYSTEM IMPLEMENTATION              │
│        Actual rendering/input/audio on target platform      │
│                                                             │
│   Graphics: GBI Interpreter → OpenGL/Vulkan draw calls      │
│   Input:    SDL gamepad/keyboard handling                   │
│   Audio:    SDL_Audio or OpenAL                             │
│   Window:   SDL or GLFW window management                   │
└─────────────────────────────────────────────────────────────┘
```

---

## Key Insight: emu64 Is The Foundation

The GameCube version contains a complete N64 emulation system called `emu64` that:
- **Parses 100% of N64 GBI (Graphics Binary Interface) commands**
- Understands all N64 texture formats, lighting modes, matrix operations
- Converts N64 display lists to GameCube GX calls

**For PC port, we:**
1. Keep the display list GENERATION code (Layer 1)
2. Replace the GX CONVERSION with OpenGL/Vulkan rendering (Layer 3)
3. The GBI parsing logic from emu64 becomes our interpreter (Layer 2)

This is dramatically simpler than writing a new renderer from scratch.

---

## Implementation Plan

### Phase 1: Clean Slate Setup
**Goal:** Start fresh with proper PC build system

1. **Shelve N64 work:**
   ```bash
   git stash -u -m "N64 port work - shelved for PC port"
   ```

2. **Create PC port branch:**
   ```bash
   git checkout -b pc-port
   ```

3. **Set up CMake build system** (cross-platform):
   ```
   CMakeLists.txt
   cmake/FindSDL2.cmake
   cmake/platform_config.cmake
   ```

### Phase 2: Layer 2 - Porting Abstraction Layer
**Goal:** Create N64-style API that calls SDL/platform functions

#### 2.1 Core OS Functions (`src/pc/os_compat.c`)
```c
// Thread management
void osCreateThread(OSThread* t, void* id, void (*entry)(void*),
                    void* arg, void* sp, OSPri pri);
void osStartThread(OSThread* t);
void osYieldThread(void);

// Timing
u64 osGetTime(void);
u32 osGetCount(void);
void osSetTimer(OSTimer* t, OSTime countdown, OSTime interval,
                OSMesgQueue* mq, OSMesg msg);

// Message queues
void osCreateMesgQueue(OSMesgQueue* mq, OSMesg* msg, s32 count);
s32 osSendMesg(OSMesgQueue* mq, OSMesg msg, s32 flags);
s32 osRecvMesg(OSMesgQueue* mq, OSMesg* msg, s32 flags);
```

#### 2.2 Memory Management (`src/pc/mem_compat.c`)
```c
// Simple wrappers - PC has plenty of RAM
void* n64_malloc(size_t size) { return malloc(size); }
void n64_free(void* ptr) { free(ptr); }
void* n64_memalign(size_t align, size_t size);
```

#### 2.3 Controller Input (`src/pc/input_compat.c`)
```c
// Map SDL gamepad to N64 controller structure
typedef struct {
    u16 button;
    s8 stick_x, stick_y;
} OSContPad;

void osContGetReadData(OSContPad* pad);
```

### Phase 3: Layer 3 - GBI Graphics Interpreter
**Goal:** Translate N64 display lists to OpenGL/Vulkan

This is the critical component. Two approaches:

#### Option A: Adapt Existing GBI Interpreter
Use an existing open-source GBI interpreter:
- **libdragon** - Has F3DEX2 interpreter
- **GLideN64** - Plugin with complete GBI support
- **fast3d** (from SM64 PC port) - Clean, MIT licensed

#### Option B: Build from emu64 Code
The existing `src/emu64/` code already parses all commands:
```c
// emu64 already handles these:
case G_VTX:      // Load vertices
case G_TRI1:     // Draw triangle
case G_TRI2:     // Draw two triangles
case G_TEXTURE:  // Set texture parameters
case G_SETIMG:   // Set texture image
case G_LOADTEX:  // Load texture to TMEM
case G_MTX:      // Matrix operations
// ... 50+ more commands
```

Convert the GX output calls to OpenGL:
```c
// Instead of: GXBegin(GX_TRIANGLES, ...)
// Output:     glBegin(GL_TRIANGLES); glVertex3f(...); glEnd();
```

### Phase 4: Layer 1 - Game Logic Cleanup
**Goal:** Remove GameCube-specific code, keep pure game logic

#### 4.1 Remove These Systems (GameCube-only):
- `JSystem/` wrappers (JKR, JUT, etc.)
- GBA link functionality
- DVD disc operations
- GameCube memory card (replace with file I/O)
- Famicom/NES emulator (optional - could port separately)

#### 4.2 Keep These Systems (Core game):
- All actor code (`src/actor/`)
- All game logic (`src/game/`)
- All data files (`src/data/`)
- Display list generation
- jaudio_NES (audio sequencer - already N64-style)

### Phase 5: Integration & Testing
**Goal:** Get game running on PC

1. **Minimal boot:** Show title screen
2. **Input working:** Navigate menus
3. **Save system:** File-based saves
4. **Full gameplay:** Walk around, talk to villagers, etc.

---

## File Structure for PC Port

```
ac-decomp/
├── CMakeLists.txt              # Main build file
├── src/
│   ├── actor/                  # Keep as-is (Layer 1)
│   ├── game/                   # Keep as-is (Layer 1)
│   ├── data/                   # Keep as-is (Layer 1)
│   ├── emu64/                  # Adapt for Layer 3
│   └── pc/                     # NEW - PC port code
│       ├── main.c              # PC entry point
│       ├── os_compat.c         # Layer 2: OS functions
│       ├── mem_compat.c        # Layer 2: Memory
│       ├── input_compat.c      # Layer 2: Input
│       ├── audio_compat.c      # Layer 2: Audio
│       └── gbi_interpreter.c   # Layer 3: Graphics
├── include/
│   ├── pc/                     # NEW - PC headers
│   │   ├── platform.h          # Platform detection
│   │   ├── os_compat.h         # N64 OS API
│   │   └── gbi.h               # GBI command definitions
│   └── ...                     # Existing headers (cleaned)
└── assets/                     # Game assets (extracted)
```

---

## Dependencies

### Required Libraries
- **SDL2** - Window, input, audio, threading
- **OpenGL 3.3+** or **Vulkan** - Graphics
- **GLEW** or **glad** - OpenGL extension loading

### Optional
- **Dear ImGui** - Debug UI
- **stb_image** - Texture loading helpers

---

## Comparison: N64 Port vs PC Port

| Aspect | N64 Port | PC Port |
|--------|----------|---------|
| Memory | 8MB limit (blocker) | Unlimited |
| Complexity | Very high (hardware) | Moderate (SDL) |
| Graphics | Direct RSP/RDP | GBI interpreter |
| Debugging | Emulator only | Full debugger |
| Distribution | ROM file | Executable |
| Portability | N64 only | Win/Linux/Mac/+ |
| Performance | Native N64 | 60fps easy |
| Resolution | 320x240 | Any resolution |

---

## Success Criteria

1. **Compiles cleanly** with GCC/Clang on Linux and Windows
2. **Boots to title screen** with correct graphics
3. **Controller input** works (gamepad or keyboard)
4. **Audio plays** (music and sound effects)
5. **Saves work** (file-based)
6. **Core gameplay** functional (walking, talking, fishing, etc.)

---

## Estimated Timeline

| Phase | Description | Effort |
|-------|-------------|--------|
| 1 | Clean slate + build system | 1-2 days |
| 2 | Porting abstraction layer | 3-5 days |
| 3 | GBI interpreter (using existing) | 1-2 weeks |
| 4 | Game logic cleanup | 1 week |
| 5 | Integration & debugging | 2-3 weeks |

**Total: 4-6 weeks to playable state**

---

## References

### Existing PC Ports Using This Approach
- **SM64 PC Port** - Uses fast3d GBI interpreter
- **OoT PC Port (Ship of Harkinian)** - Uses libultra reimplementation
- **Zelda 64: Recompiled** - Static recompilation approach

### Code Resources
- `/home/m/CLionProjects/af` - Animal Forest N64 decomp (reference)
- `src/emu64/` - Existing GBI parser in this codebase
- SM64 `src/pc/gfx/` - Open source GBI interpreter

---

## Next Steps

1. **Shelve current N64 work** (`git stash -u`)
2. **Create pc-port branch**
3. **Set up CMake build**
4. **Start with Layer 2 OS compatibility**
5. **Integrate existing GBI interpreter**

This approach avoids the insurmountable N64 memory limit while leveraging all the existing decompilation work.


This updated plan adopts the "Ideal PC Port" three-layer abstraction strategy, focusing on removing all GameCube dependencies and replacing them with a custom, PC-native Porting Abstraction Layer (PAL).

The plan explicitly addresses the risks of graphics translation complexity and structural misalignment by incorporating external libraries and mandatory auditing steps.
🚀 Updated Plan: Clean, Portable PC Port via Abstraction

The goal is to move the core game logic to a clean C/C++ state that speaks only the N64 API (os*, gs*), which is then wrapped by PC-native implementations (SDL, OpenGL/Vulkan).
Phase 0: Project Setup & Baseline Compilation

Goal: Set up the environment to compile the existing code with the new cross-compiler toolchain, generating initial linking errors.
Step	Action	Purpose
0.1	Toolchain Setup	Configure a standard cross-platform compiler (GCC/Clang) for x86/ARM target.
0.2	Remove libultra Stubs	Delete all placeholder N64 compatibility files (src/n64/* and include/n64_compat/*) as the new PAL will replace them.
0.3	Initial Compile	Attempt to compile the entire source tree. This will generate the complete list of missing GC headers and undefined GC functions.
Phase 1: GameCube Decoupling & Code Refactoring

Goal: Achieve a state where the code compiles cleanly, calling only the N64 API (os*, gs*, vi*, ai*, etc.), and has no GC headers. This is the critical cleanup phase.
Step	Action	Risk Mitigation
1.1	Header Replacement	Systematically replace every GC header (#include <dolphin/os.h>, <gcn/gx.h>, etc.) with the equivalent N64 header (#include <ultra64.h>, <PR/gbi.h>, etc.).
1.2	Feature Removal	Audit and remove/stub all GameCube-exclusive features (e.g., GBA link cable support, DVD reading, specific memory card calls) that have no N64 equivalent. Use if (0) blocks or empty stub functions.
1.3	API Conversion	Replace all remaining GC function calls (OSThreadCreate, DVDRead, GX...) with their N64 logical equivalents (osCreateThread, osPiRead..., gsSP...).
1.4	Struct Alignment Audit	CRITICAL RISK MITIGATION. Compare critical N64 data structures (e.g., GAME, GRAPH, thread structures) against the known, working structs from the Animal Forest N64 decomp. Manually adjust structure member sizes and padding to ensure they match the N64 memory layout.
Phase 2: Abstraction Layer (PAL) & Core System Implementation

Goal: Create the Porting Abstraction Layer (PAL) that defines and implements all N64 OS/Hardware functions using PC-native code, guaranteeing a successful link.
Step	Action	Implementation
2.1	System Stubs	Create pal_system.c that defines all os* and vi* functions.
2.2	Threading & Timing	Implement N64 OS functions using a cross-platform library.
2.3	File I/O Abstraction	Implement N64 PI/SI functions using PC file I/O.
2.4	Initial Linking	Link the refactored game logic (Phase 1) against the newly created PAL (Phase 2).
Phase 3: Graphics System Implementation (The GBI Translator)

Goal: Implement the graphics portion of the PAL, intercepting N64 display lists and rendering them via a modern GPU API.
Step	Action	Risk Mitigation
3.1	GBI Translator Selection	CRITICAL CHOICE. Do not write a GBI translator from scratch. Choose an existing open-source N64 graphics framework (e.g., a lightweight RDP/RSP interpreter based on GLideN64 or similar projects).
3.2	Display List Interception	Implement the N64 RSP task submission function.
3.3	Host Rendering API	Configure the GBI translator to output to a PC rendering API.
Phase 4: Debugging and Final Port

Goal: Achieve a running, stable PC game loop.
Step	Action	Focus
4.1	Game Loop Execution	Verify the game runs past initialization and enters the graph_proc loop.
4.2	Audio Implementation	Replace N64 Audio Interface (AI) calls with a cross-platform audio library.
4.3	Input Implementation	Implement N64 SI functions for controller input.
4.4	PC Finalization	Full-screen modes, configuration files, and input mapping.