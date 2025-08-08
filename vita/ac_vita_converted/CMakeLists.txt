cmake_minimum_required(VERSION 3.19)

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

# VitaGL setup - use direct library approach for Docker
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(VITAGL REQUIRED vitaGL)

# Include VitaGL directly
include_directories(/usr/local/vitasdk/arm-vita-eabi/include)
link_directories(/usr/local/vitasdk/arm-vita-eabi/lib)
set(VITAGL_LIBRARIES vitaGL pthread)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wno-unused-variable -Wno-unused-function -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-unused-variable -Wno-unused-function -O2")

# Include directories
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  # ${VITAGL_INCLUDE_DIRS} # This line is removed as per the new_code
)

# Find all source files (automatically generated)
file(GLOB_RECURSE SOURCES 
  "*.c"
  "*.cpp"
)

# Exclude test and example files
list(FILTER SOURCES EXCLUDE REGEX ".*/test_.*")
list(FILTER SOURCES EXCLUDE REGEX ".*/example_.*")

# Create the executable
add_executable(AnimalCrossingVita ${SOURCES})

# Link libraries
target_link_libraries(AnimalCrossingVita
  ${VITAGL_LIBRARIES}
  SceLibKernel_stub
  SceDisplay_stub  
  SceGxm_stub
  SceSysmodule_stub
  SceCtrl_stub
  SceTouch_stub
  SceAudio_stub
  SceAudioout_stub
  ScePgf_stub
  ScePower_stub
  SceCommonDialog_stub
  SceRtc_stub
  pthread
  m
)

# Build .vpk
vita_create_self(AnimalCrossingVita.self AnimalCrossingVita)
vita_create_vpk(AnimalCrossingVita.vpk ${VITA_TITLEID} AnimalCrossingVita.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE ../sce_sys/icon0.png sce_sys/icon0.png
  FILE ../sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE ../sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE ../sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)
