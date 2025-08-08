#ifndef GFX_ALLOC_H
#define GFX_ALLOC_H

#include "gx_to_vitagl_wrapper.h"  // Use proven VitaGL wrapper for GameCube API
#include "types.h"
#include "PR/mbi.h"

#ifdef __cplusplus
extern "C" {
#endif

extern Gfx* gfxopen(Gfx* gfxp);
extern Gfx* gfxclose(Gfx* gfxp, Gfx* gfxp_new);
extern Gfx* gfxalloc(Gfx** gfxpp, size_t size);

#ifdef __cplusplus
};
#endif

#endif
