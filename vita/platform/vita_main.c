/**
 * Animal Crossing Vita - Single Clean Implementation
 * Minimal main entry point for AC-Decomp integration
 */

#include "ac_vita_platform_minimal.h"
#include <psp2/kernel/processmgr.h>

int main(int argc, char *argv[]) {
    // Initialize our platform
    ac_vita_platform_init();
    
    // AC-Decomp would call its main game logic here
    // For now, just initialize and exit cleanly
    
    // Cleanup
    ac_vita_platform_cleanup();
    
    // Exit gracefully
    sceKernelExitProcess(0);
    return 0;
} 