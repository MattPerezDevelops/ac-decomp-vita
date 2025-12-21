#!/usr/bin/env python3
"""
Analyze actor profiles to determine which are linker-ready.

This script parses actor source files to find ACTOR_PROFILE definitions and
checks if the required function pointers (ct, dt, mv, dw, sv) are defined
locally or are just extern declarations.

Prioritizes actors used in TITLE_DEMO scene for the PC port.

Usage:
    python3 tools/analyze_actor_profiles.py
"""

import os
import re
from collections import defaultdict
from pathlib import Path
from dataclasses import dataclass
from typing import Optional, List, Set, Dict, Tuple

# Configuration
ACTOR_DIR = "src/actor"
INCLUDE_DIR = "include"

# TITLE_DEMO actors (from src/data/scene/title_demo.c)
# These are the actors spawned in the title demo scene
TITLE_DEMO_PROFILES = {
    "mAc_PROFILE_PLAYER",
    "mAc_PROFILE_EFFECTBG",
    "mAc_PROFILE_BIRTH_CONTROL",
    "mAc_PROFILE_NPC",
    "mAc_PROFILE_STRUCTURE",
    "mAc_PROFILE_TOOLS",
    "mAc_PROFILE_HANDOVERITEM",
    "mAc_PROFILE_EFFECT_CONTROL",
    "mAc_PROFILE_WEATHER",
    "mAc_PROFILE_ANIMAL_LOGO",
    "mAc_PROFILE_QUEST_MANAGER",
    "mAc_PROFILE_UKI",
    # Also needed for any outdoor scene
    "mAc_PROFILE_FIELDM_DRAW",
    "mAc_PROFILE_FIELD_DRAW",
    "mAc_PROFILE_DUMMY",  # Fallback actor
}

# Patterns
PROFILE_DEF_PATTERN = re.compile(
    r'ACTOR_PROFILE\s+(\w+)\s*=\s*\{([^}]+)\}',
    re.MULTILINE | re.DOTALL
)
FUNCTION_DEF_PATTERN = re.compile(r'^(?:static\s+)?(?:void|int)\s+(\w+)\s*\([^)]*\)\s*\{', re.MULTILINE)
EXTERN_FUNC_PATTERN = re.compile(r'extern\s+(?:void|int)\s+(\w+)\s*\(', re.MULTILINE)
NONE_PROC_PATTERN = re.compile(r'NONE_ACTOR_PROC|none_proc\d*|NULL|mActor_NONE_PROC\d*|^\s*$')

@dataclass
class ProfileInfo:
    """Information about an actor profile."""
    name: str               # e.g., "Player_Profile"
    source_file: str        # e.g., "ac_player.c"
    profile_id: Optional[str] = None  # e.g., "mAc_PROFILE_PLAYER"

    # Function pointers extracted from profile
    ct_proc: Optional[str] = None
    dt_proc: Optional[str] = None
    mv_proc: Optional[str] = None
    dw_proc: Optional[str] = None
    sv_proc: Optional[str] = None

    # Which functions are defined locally
    local_functions: Set[str] = None
    extern_functions: Set[str] = None

    # Analysis result
    is_linkable: bool = False
    missing_functions: List[str] = None

    def __post_init__(self):
        if self.local_functions is None:
            self.local_functions = set()
        if self.extern_functions is None:
            self.extern_functions = set()
        if self.missing_functions is None:
            self.missing_functions = []


def extract_profile_id_from_struct(struct_body: str) -> Optional[str]:
    """Extract the profile ID (first field) from the profile struct body."""
    # First field should be like: mAc_PROFILE_PLAYER
    match = re.search(r'(mAc_PROFILE_\w+)', struct_body)
    if match:
        return match.group(1)
    return None


def extract_function_pointers(struct_body: str) -> Tuple[str, str, str, str, str]:
    """Extract the 5 function pointers from a profile struct.

    Profile struct fields:
    { id, part, flags, npc_id, obj_bank, class_size, ct, dt, mv, dw, sv }
    """
    # Clean up the struct body
    body = struct_body.replace('\n', ' ').replace('\r', ' ')

    # Split by comma and find function pointer fields
    # The last 5 fields are: ct_proc, dt_proc, mv_proc, dw_proc, sv_proc
    parts = [p.strip() for p in body.split(',')]

    if len(parts) >= 5:
        # Last 5 are the procs
        sv = parts[-1].rstrip('}').strip()
        dw = parts[-2].strip()
        mv = parts[-3].strip()
        dt = parts[-4].strip()
        ct = parts[-5].strip()
        return ct, dt, mv, dw, sv

    return None, None, None, None, None


def is_none_proc(proc_name: str) -> bool:
    """Check if a proc is a null/none proc."""
    if proc_name is None:
        return True
    return bool(NONE_PROC_PATTERN.search(proc_name))


def analyze_actor_file(filepath: Path) -> List[ProfileInfo]:
    """Analyze a single actor source file."""
    profiles = []

    try:
        content = filepath.read_text(errors='ignore')
    except Exception as e:
        print(f"Warning: Could not read {filepath}: {e}")
        return profiles

    # Find all defined functions
    local_functions = set(FUNCTION_DEF_PATTERN.findall(content))
    extern_functions = set(EXTERN_FUNC_PATTERN.findall(content))

    # Find all ACTOR_PROFILE definitions
    for match in PROFILE_DEF_PATTERN.finditer(content):
        profile_name = match.group(1)
        struct_body = match.group(2)

        profile = ProfileInfo(
            name=profile_name,
            source_file=filepath.name,
            local_functions=local_functions.copy(),
            extern_functions=extern_functions.copy()
        )

        # Extract profile ID
        profile.profile_id = extract_profile_id_from_struct(struct_body)

        # Extract function pointers
        ct, dt, mv, dw, sv = extract_function_pointers(struct_body)
        profile.ct_proc = ct
        profile.dt_proc = dt
        profile.mv_proc = mv
        profile.dw_proc = dw
        profile.sv_proc = sv

        # Check if all required functions are available
        profile.missing_functions = []
        all_defined = True

        for proc_name, proc_label in [
            (ct, 'ct_proc'), (dt, 'dt_proc'), (mv, 'mv_proc'),
            (dw, 'dw_proc'), (sv, 'sv_proc')
        ]:
            if is_none_proc(proc_name):
                continue  # NULL/NONE procs are fine

            # Clean up function reference (handle casts)
            clean_name = proc_name
            if '(' in clean_name:
                # Handle casts like (mActor_proc)func_name
                paren_match = re.search(r'\)(\w+)', clean_name)
                if paren_match:
                    clean_name = paren_match.group(1)
            if '&' in clean_name:
                clean_name = clean_name.replace('&', '').strip()

            # Check if function is defined locally
            if clean_name not in local_functions:
                if clean_name in extern_functions:
                    profile.missing_functions.append(f"{proc_label}={clean_name} (extern)")
                    all_defined = False
                else:
                    # Could be from an included header - mark as potentially missing
                    profile.missing_functions.append(f"{proc_label}={clean_name} (unknown)")
                    all_defined = False

        profile.is_linkable = all_defined
        profiles.append(profile)

    return profiles


def get_profile_to_name_mapping(include_path: Path) -> Dict[str, str]:
    """Map profile IDs to human-readable names from m_actor.h."""
    mapping = {}
    m_actor_h = include_path / "m_actor.h"

    if not m_actor_h.exists():
        return mapping

    try:
        content = m_actor_h.read_text(errors='ignore')
        # Find enum values like mAc_PROFILE_PLAYER
        for match in re.finditer(r'(mAc_PROFILE_\w+)', content):
            profile_id = match.group(1)
            # Extract the name part
            name = profile_id.replace('mAc_PROFILE_', '')
            mapping[profile_id] = name
    except Exception:
        pass

    return mapping


def main():
    # Find project root
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    actor_path = project_root / ACTOR_DIR
    include_path = project_root / INCLUDE_DIR

    if not actor_path.exists():
        print(f"Error: Actor directory not found: {actor_path}")
        return 1

    # Get profile name mapping
    profile_names = get_profile_to_name_mapping(include_path)

    # Analyze all actor files
    all_profiles: List[ProfileInfo] = []
    actor_files = list(actor_path.glob("*.c"))

    print(f"Analyzing {len(actor_files)} actor files in {actor_path}...")
    print()

    for actor_file in sorted(actor_files):
        profiles = analyze_actor_file(actor_file)
        all_profiles.extend(profiles)

    # Categorize profiles
    title_demo_ready = []
    title_demo_broken = []
    other_ready = []
    other_broken = []

    for profile in all_profiles:
        is_title_demo = profile.profile_id in TITLE_DEMO_PROFILES

        if profile.is_linkable:
            if is_title_demo:
                title_demo_ready.append(profile)
            else:
                other_ready.append(profile)
        else:
            if is_title_demo:
                title_demo_broken.append(profile)
            else:
                other_broken.append(profile)

    # Print results
    print("=" * 80)
    print("ACTOR PROFILE LINKABILITY REPORT")
    print("=" * 80)
    print()
    print("Profiles are 'READY' if all function pointers are defined locally.")
    print("Profiles are 'BROKEN' if they reference extern functions not in the file.")
    print()

    # TITLE_DEMO section (priority)
    print("-" * 80)
    print("TITLE_DEMO ACTORS (HIGH PRIORITY)")
    print("-" * 80)

    print("\n  READY TO LINK:")
    if title_demo_ready:
        for p in sorted(title_demo_ready, key=lambda x: x.name):
            print(f"    {p.source_file:<35} {p.name}")
    else:
        print("    (none)")

    print("\n  BROKEN (needs work):")
    if title_demo_broken:
        for p in sorted(title_demo_broken, key=lambda x: x.name):
            missing = ', '.join(p.missing_functions[:3])
            if len(p.missing_functions) > 3:
                missing += f" (+{len(p.missing_functions) - 3} more)"
            print(f"    {p.source_file:<35} {p.name}")
            print(f"      Missing: {missing}")
    else:
        print("    (none - all TITLE_DEMO actors are ready!)")

    print()

    # Missing TITLE_DEMO actors (no profile found)
    found_profiles = {p.profile_id for p in all_profiles if p.profile_id}
    missing_profiles = TITLE_DEMO_PROFILES - found_profiles
    if missing_profiles:
        print("  MISSING PROFILES (not found in src/actor/*.c):")
        for pid in sorted(missing_profiles):
            name = profile_names.get(pid, pid)
            print(f"    {pid} ({name})")
        print()

    # OTHER section (lower priority)
    print("-" * 80)
    print("OTHER ACTORS")
    print("-" * 80)

    print(f"\n  READY TO LINK ({len(other_ready)} total):")
    for p in sorted(other_ready, key=lambda x: x.name)[:15]:
        print(f"    {p.source_file:<35} {p.name}")
    if len(other_ready) > 15:
        print(f"    ... and {len(other_ready) - 15} more")

    print(f"\n  BROKEN ({len(other_broken)} total):")
    for p in sorted(other_broken, key=lambda x: x.name)[:10]:
        missing = ', '.join(p.missing_functions[:2])
        if len(p.missing_functions) > 2:
            missing += f" (+{len(p.missing_functions) - 2} more)"
        print(f"    {p.source_file:<35} {p.name}")
        print(f"      Missing: {missing}")
    if len(other_broken) > 10:
        print(f"    ... and {len(other_broken) - 10} more")

    print()

    # Summary
    print("-" * 80)
    print("SUMMARY")
    print("-" * 80)
    total_ready = len(title_demo_ready) + len(other_ready)
    total_broken = len(title_demo_broken) + len(other_broken)

    print(f"Total profiles found:      {len(all_profiles)}")
    print(f"Ready to link:             {total_ready} ({100*total_ready/len(all_profiles):.1f}%)")
    print(f"Broken (extern refs):      {total_broken}")
    print()
    print(f"TITLE_DEMO ready:          {len(title_demo_ready)}/{len(TITLE_DEMO_PROFILES)}")
    print(f"TITLE_DEMO broken:         {len(title_demo_broken)}")
    print(f"TITLE_DEMO not found:      {len(missing_profiles)}")
    print()

    # Recommendations
    print("-" * 80)
    print("RECOMMENDATIONS")
    print("-" * 80)
    if title_demo_ready:
        print("\nReady to enable now:")
        for p in title_demo_ready:
            print(f"  - {p.profile_id} ({p.name})")

    if title_demo_broken:
        print("\nNext actors to fix (TITLE_DEMO priority):")
        for p in title_demo_broken[:5]:
            print(f"  - {p.profile_id}: fix {', '.join(p.missing_functions[:2])}")

    print()
    print("Done!")
    return 0


if __name__ == "__main__":
    exit(main())
