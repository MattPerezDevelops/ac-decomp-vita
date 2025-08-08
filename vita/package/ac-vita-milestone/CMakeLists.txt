cmake_minimum_required(VERSION 3.5)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VITASDK})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.cmake" CACHE PATH "toolchain file")
    else()
        message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
    endif()
endif()

project(AnimalCrossingVitaDemo)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "AC Vita Demo")
set(VITA_TITLEID  "DEMO00001")
set(VITA_VERSION  "01.00")

# Include our platform directory
include_directories(platform)

# Simple demo executable
add_executable(AnimalCrossingVitaDemo
    demo_main.c
    platform/gx_to_vitagl_wrapper.c
    platform/enhanced_texture_renderer.c
)

target_link_libraries(AnimalCrossingVitaDemo
    vitaGL
    vitashark
    SceLibKernel_stub
    SceDisplay_stub
    SceGxm_stub
    SceSysmodule_stub
    SceCtrl_stub
    ScePower_stub
    SceAudio_stub
    m
)

vita_create_self(AnimalCrossingVitaDemo.self AnimalCrossingVitaDemo)
vita_create_vpk(AnimalCrossingVitaDemo.vpk ${VITA_TITLEID} AnimalCrossingVitaDemo.self
    VERSION ${VITA_VERSION}
    NAME ${VITA_APP_NAME}
)
