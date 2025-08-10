cmake_minimum_required(VERSION 3.19)

# Vita SDK setup
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(AnimalCrossingVita)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Animal Crossing")
set(VITA_TITLEID  "ACRS00001")
set(VITA_VERSION  "01.00")

# Compiler flags optimized for AC-Decomp
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wno-unused-variable -Wno-unused-function -O2 -include ${CMAKE_CURRENT_SOURCE_DIR}/platform/ac_vita_platform_minimal.h")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-unused-variable -Wno-unused-function -O2 -include ${CMAKE_CURRENT_SOURCE_DIR}/platform/ac_vita_platform_minimal.h")

# Include directories  
include_directories(
  platform/                               # Our platform wrapper
  ../ac-decomp-upstream/include/          # AC-Decomp GameCube SDK headers
  /usr/local/vitasdk/arm-vita-eabi/include/  # VitaGL headers
)

# Link directories
link_directories(/usr/local/vitasdk/arm-vita-eabi/lib/)

# AC-Decomp Integration Control
option(AC_INCLUDE_DECOMP "Include AC-Decomp source files" ON)
option(AC_DEMO_MODE "Build demo mode instead of full game" OFF)

if(AC_DEMO_MODE)
    message(STATUS "🎮 Building AC-Decomp Vita Demo")
    
    # Demo mode - clean minimal implementation
    set(SOURCES
        platform/vita_main.c
        platform/ac_vita_minimal_impl.c
    )
    
else()
    message(STATUS "🎮 Building Complete Animal Crossing Game")
    
    # Base platform sources (clean implementation)
    set(SOURCES
        platform/ac_simple_test.c        # Simple test demonstrating VitaGL wrappers
        platform/ac_vita_minimal_impl.c  # Essential VitaGL wrappers only
    )
    
    if(AC_INCLUDE_DECOMP)
        message(STATUS "Including AC-Decomp game logic...")
        
        # Scale test: Test our mass fixes on many more AC-Decomp files
        set(AC_DECOMP_TARGET_SOURCES
            # Core system files (previously working)
            "../ac-decomp-upstream/src/PreRender.c"
            "../ac-decomp-upstream/src/TwoHeadArena.c"  
            "../ac-decomp-upstream/src/THA_GA.c"
            
            # Graphics and effects  
            "../ac-decomp-upstream/src/c_keyframe.c"
            "../ac-decomp-upstream/src/ev_cherry_manager.c"
            
            # Game system files
            "../ac-decomp-upstream/src/game/m_actor.c"
            "../ac-decomp-upstream/src/game/m_view.c"
            "../ac-decomp-upstream/src/game/m_actor_shadow.c"
            "../ac-decomp-upstream/src/game/m_actor_dlftbls.c"
            
            # Actor system
            "../ac-decomp-upstream/src/actor/ac_airplane.c"
            "../ac-decomp-upstream/src/actor/ac_animal_logo.c"
            
            # System files
            "../ac-decomp-upstream/src/system/sys_math.c"
            "../ac-decomp-upstream/src/system/sys_math3d.c"
            "../ac-decomp-upstream/src/system/sys_math_atan.c"
            
            # Scalability test - add more diverse AC-Decomp files 
            "../ac-decomp-upstream/src/effect/ef_ami_mizu.c"
            "../ac-decomp-upstream/src/effect/ef_anahikari.c"
            "../ac-decomp-upstream/src/bg_item/bg_cherry_item.c"
            "../ac-decomp-upstream/src/furniture/ac_ari_isu01.c"
            "../ac-decomp-upstream/src/furniture/ac_ari_kitchen01.c"
            "../ac-decomp-upstream/src/furniture/ac_ari_table01.c"
            
            # MASSIVE SCALE TEST - Test pipeline fixes on many more files
            "../ac-decomp-upstream/src/effect/ef_ase.c"
            "../ac-decomp-upstream/src/effect/ef_ase2.c"
            "../ac-decomp-upstream/src/effect/ef_ase_ch.c"
            "../ac-decomp-upstream/src/furniture/ac_ari_reizou01.c"
            "../ac-decomp-upstream/src/furniture/ac_din_amber.c"
        )
        
        # Add targeted AC-Decomp sources to build
        list(APPEND SOURCES ${AC_DECOMP_TARGET_SOURCES})
        
        list(LENGTH AC_DECOMP_TARGET_SOURCES AC_FILE_COUNT)
        message(STATUS "🎯 Running AC-style VitaGL wrapper test (${AC_FILE_COUNT} AC files)")
    endif()
endif()

# Create executable
add_executable(AnimalCrossingVita ${SOURCES})

# Preprocessor definitions
target_compile_definitions(AnimalCrossingVita PRIVATE 
    AC_VITA_PORT=1
    TARGET_VITA=1
    HAVE_VITAGL=1
)

if(AC_DEMO_MODE)
    target_compile_definitions(AnimalCrossingVita PRIVATE AC_DEMO_MODE=1)
endif()

# Link libraries
target_link_libraries(AnimalCrossingVita
  vitaGL
  vitashark
  SceShaccCgExt
  taihen_stub
  SceShaccCg_stub
  mathneon
  pthread
  SceLibKernel_stub
  SceDisplay_stub  
  SceGxm_stub
  SceSysmodule_stub
  SceCtrl_stub
  SceTouch_stub
  SceAudio_stub
  ScePgf_stub
  ScePower_stub
  SceCommonDialog_stub
  SceRtc_stub
  SceAppMgr_stub
  m
)

# Build .vpk
vita_create_self(AnimalCrossingVita.self AnimalCrossingVita)
vita_create_vpk(AnimalCrossingVita.vpk ${VITA_TITLEID} AnimalCrossingVita.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
) 