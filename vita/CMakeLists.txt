cmake_minimum_required(VERSION 3.0)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(AnimalCrossingVita)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Animal Crossing Vita")
set(VITA_TITLEID  "ACRS00001")
set(VITA_VERSION  "02.00")

# Enable AC-Decomp integration
set(AC_BUILD_TYPE "FULL_GAME")
set(AC_INCLUDE_DECOMP ON)

# Include directories
include_directories("platform")
include_directories("../ac-decomp-upstream/include")

# Base platform sources
set(BASE_SOURCES
    platform/ac_vita_platform.c
    platform/vita_main_ac_real_game.c
)

# AC-Decomp source files - MASSIVE INTEGRATION TARGET
if(AC_INCLUDE_DECOMP)
    message(STATUS "Building complete AC-Decomp game")
    message(STATUS "Including AC-Decomp game logic from ac-decomp-upstream")
    
    # Add all AC-Decomp source files automatically
    file(GLOB_RECURSE AC_DECOMP_SOURCES 
        "../ac-decomp-upstream/src/*.c"
    )
    
    # Filter out problematic files that need special handling
    list(FILTER AC_DECOMP_SOURCES EXCLUDE REGEX ".*/(test_|example_|debug_).*")
    list(FILTER AC_DECOMP_SOURCES EXCLUDE REGEX ".*/static/.*")  # Static libs
    
    # Add AC-Decomp sources to build
    foreach(AC_FILE ${AC_DECOMP_SOURCES})
        get_filename_component(RELATIVE_FILE ${AC_FILE} NAME)
        message(STATUS "Added AC-Decomp source: ${RELATIVE_FILE}")
        list(APPEND BASE_SOURCES ${AC_FILE})
    endforeach()
    
    message(STATUS "Total AC-Decomp files: ${AC_DECOMP_SOURCES}")
    list(LENGTH AC_DECOMP_SOURCES AC_FILE_COUNT)
    message(STATUS "🚀 MASSIVE BUILD: ${AC_FILE_COUNT} AC-Decomp source files!")
endif()

# Create the executable
add_executable(AnimalCrossingVita ${BASE_SOURCES})

# Add preprocessor definitions
target_compile_definitions(AnimalCrossingVita PRIVATE 
    AC_MISSING_ASSETS_OK=1
    AC_USE_PLACEHOLDER_ASSETS=1
    AC_USE_ASSET_BRIDGE=1
)

# Link libraries (comprehensive set)
target_link_libraries(AnimalCrossingVita
    # VitaGL stack
    vitaGL
    vitashark
    SceShaccCgExt
    mathneon
    
    # VitaGL required stubs
    taihen_stub
    SceShaccCg_stub
    
    # Core Vita system libraries
    SceAppMgr_stub
    SceAppUtil_stub
    SceCtrl_stub
    SceDisplay_stub
    SceGxm_stub
    SceSysmodule_stub
    SceTouch_stub
    ScePower_stub
    
    # Audio system
    SceAudio_stub
    SceAudioout_stub
    
    # File system and memory
    SceIofilemgr_stub
    SceKernelDmacMgr_stub
    
    # Math and utility
    m
)

# Create self and vpk
vita_create_self(AnimalCrossingVita.self AnimalCrossingVita)
vita_create_vpk(AnimalCrossingVita.vpk ${VITA_TITLEID} AnimalCrossingVita.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE sce_sys/icon0.png sce_sys/icon0.png
  FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png  
  FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)

# Print build info
add_custom_target(build_info ALL
    COMMAND ${CMAKE_COMMAND} -E echo "🎮 Animal Crossing Vita v${VITA_VERSION}"
    COMMAND ${CMAKE_COMMAND} -E echo "Build Type: ${AC_BUILD_TYPE}"
    COMMAND ${CMAKE_COMMAND} -E echo "Debug: OFF"
    COMMAND ${CMAKE_COMMAND} -E echo "AC-Decomp: ${AC_INCLUDE_DECOMP}"
    VERBATIM
)
