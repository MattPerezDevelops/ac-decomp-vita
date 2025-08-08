/**
 * AC Vita Platform Late Declarations
 * Functions that need AC-Decomp types to be defined first
 */

#pragma once

// This header should be included AFTER AC-Decomp headers have defined their types

#ifdef __cplusplus
extern "C" {
#endif

// Functions that need AC-Decomp types (ACTOR, mActor_name_t, etc.)
void mCoBG_BgCheckControll(xyz_t* actor_revpos, ACTOR* actorx, f32 range, f32 ground_dist, s16 attr_wall, s16 rev_type, s16 check_type);

#ifdef __cplusplus
}
#endif 