#!/usr/bin/env python3
"""
Generate the PC actor DLF table (m_actor_dlftbls_pc.c).

This script reads a JSON configuration of approved actors and generates
the actor table with real profiles for approved actors and NULL stubs
for unapproved ones.

Usage:
    python3 tools/generate_actor_table.py [--output FILE] [--config FILE]

Default output: src/pc/m_actor_dlftbls_pc.c
Default config: tools/approved_actors.json
"""

import argparse
import json
import re
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict, Optional

# Configuration
DEFAULT_OUTPUT = "src/pc/m_actor_dlftbls_pc.c"
DEFAULT_CONFIG = "tools/approved_actors.json"
M_ACTOR_H = "include/m_actor.h"

@dataclass
class ProfileEntry:
    """An entry in the profile enum."""
    index: int
    name: str  # e.g., "PLAYER" (without mAc_PROFILE_ prefix)
    full_name: str  # e.g., "mAc_PROFILE_PLAYER"


@dataclass
class ApprovedActor:
    """An approved actor configuration."""
    profile_id: int
    profile_name: str  # e.g., "mAc_PROFILE_PLAYER"
    profile_symbol: str  # e.g., "Player_Profile"
    header: str  # e.g., "ac_player.h" or "m_player_call.h"
    macro_name: str  # e.g., "Player" for P(Player)


def parse_profile_enum(m_actor_h_path: Path) -> List[ProfileEntry]:
    """Parse the actor_profile_table enum from m_actor.h."""
    profiles = []

    try:
        content = m_actor_h_path.read_text(errors='ignore')
    except Exception as e:
        print(f"Error reading {m_actor_h_path}: {e}")
        return profiles

    # Find the enum block
    enum_match = re.search(
        r'enum\s+actor_profile_table\s*\{([^}]+)\}',
        content,
        re.DOTALL
    )

    if not enum_match:
        print("Warning: Could not find actor_profile_table enum")
        return profiles

    enum_body = enum_match.group(1)

    # Extract each enum value
    index = 0
    for line in enum_body.split('\n'):
        line = line.strip()
        # Skip empty lines and comments
        if not line or line.startswith('//') or line.startswith('/*'):
            continue

        # Match enum entry like "mAc_PROFILE_PLAYER,"
        match = re.match(r'(mAc_PROFILE_\w+)\s*,?', line)
        if match:
            full_name = match.group(1)
            if full_name == 'mAc_PROFILE_NUM':
                break  # End of actual profiles
            name = full_name.replace('mAc_PROFILE_', '')
            profiles.append(ProfileEntry(
                index=index,
                name=name,
                full_name=full_name
            ))
            index += 1

    return profiles


def load_approved_actors(config_path: Path) -> List[ApprovedActor]:
    """Load the approved actors from JSON config."""
    try:
        with open(config_path, 'r') as f:
            data = json.load(f)
    except FileNotFoundError:
        print(f"Config file not found: {config_path}")
        print("Creating default config...")
        create_default_config(config_path)
        with open(config_path, 'r') as f:
            data = json.load(f)

    actors = []
    for entry in data.get('approved_actors', []):
        actors.append(ApprovedActor(
            profile_id=entry['id'],
            profile_name=entry['profile_name'],
            profile_symbol=entry['profile_symbol'],
            header=entry['header'],
            macro_name=entry['macro_name']
        ))

    return actors


def create_default_config(config_path: Path):
    """Create a default approved_actors.json with basic actors."""
    default_config = {
        "description": "Approved actors for PC port. Add actors here as they are ported.",
        "approved_actors": [
            {
                "id": 0,
                "profile_name": "mAc_PROFILE_PLAYER",
                "profile_symbol": "Player_Profile",
                "header": "m_player_call.h",
                "macro_name": "Player"
            },
            {
                "id": 3,
                "profile_name": "mAc_PROFILE_FIELDM_DRAW",
                "profile_symbol": "Fieldm_Draw_Profile",
                "header": "ac_fieldm_draw.h",
                "macro_name": "Fieldm_Draw"
            },
            {
                "id": 4,
                "profile_name": "mAc_PROFILE_FIELD_DRAW",
                "profile_symbol": "Field_Draw_Profile",
                "header": "ac_field_draw.h",
                "macro_name": "Field_Draw"
            },
            {
                "id": 180,
                "profile_name": "mAc_PROFILE_DUMMY",
                "profile_symbol": "Dummy_Profile",
                "header": "ac_dummy.h",
                "macro_name": "Dummy"
            }
        ]
    }

    config_path.parent.mkdir(parents=True, exist_ok=True)
    with open(config_path, 'w') as f:
        json.dump(default_config, f, indent=2)
    print(f"Created default config: {config_path}")


def generate_table(profiles: List[ProfileEntry],
                   approved: List[ApprovedActor],
                   output_path: Path) -> str:
    """Generate the m_actor_dlftbls_pc.c file content."""

    # Build lookup of approved actors by index
    approved_by_id: Dict[int, ApprovedActor] = {a.profile_id: a for a in approved}

    # Collect unique headers
    headers = sorted(set(a.header for a in approved))

    # Generate the file
    lines = []

    # Header comment
    lines.append("/**")
    lines.append(" * @file m_actor_dlftbls_pc.c")
    lines.append(" * @brief PC-specific actor profile table")
    lines.append(" *")
    lines.append(" * AUTO-GENERATED by tools/generate_actor_table.py")
    lines.append(" * Do not edit manually - edit tools/approved_actors.json instead.")
    lines.append(" *")
    lines.append(f" * Approved actors: {len(approved)}/{len(profiles)}")
    lines.append(" */")
    lines.append("")

    # Includes
    lines.append('#include "m_actor_dlftbls.h"')
    lines.append('#include "m_actor.h"')
    lines.append("")
    lines.append("/* Headers for approved actors */")
    for header in headers:
        lines.append(f'#include "{header}"')
    lines.append("")

    # Count variable
    lines.append("/* Number of actor profiles */")
    lines.append("int actor_dlftbls_num = mAc_PROFILE_NUM;")
    lines.append("")

    # Macros
    lines.append("/* Stub entry (NULL profile) */")
    lines.append("#define S {0, 0, NULL, NULL, NULL, NULL, 0, 0, 0, 0}")
    lines.append("")
    lines.append("/* Profile entry macro */")
    lines.append("#define P(actor) {0, 0, NULL, NULL, NULL, &actor##_Profile, 0, 0, 0, 0}")
    lines.append("")

    # Table
    lines.append("/*")
    lines.append(" * The actor profile table")
    lines.append(" *")
    lines.append(" * Approved actors:")
    for a in approved:
        lines.append(f" *   [{a.profile_id}] = {a.profile_name} -> {a.profile_symbol}")
    lines.append(" */")
    lines.append("ACTOR_DLFTBL actor_dlftbls[mAc_PROFILE_NUM] = {")

    # Generate entries in groups of 10
    for i in range(0, len(profiles), 10):
        group = profiles[i:i+10]
        group_start = i
        group_end = min(i + 9, len(profiles) - 1)

        # Build entries for this group
        entries = []
        comments = []
        for p in group:
            if p.index in approved_by_id:
                actor = approved_by_id[p.index]
                entries.append(f"P({actor.macro_name})")
                comments.append(f"{p.index}={p.name}")
            else:
                entries.append("S")

        # Format line
        entry_str = ", ".join(entries)
        comment_actors = [c for c in comments]

        if comment_actors:
            comment = " /* " + ", ".join(comment_actors) + " */"
        else:
            comment = ""

        lines.append(f"    /* {group_start:3d}-{group_end:3d} */ {entry_str},{comment}")

    lines.append("};")
    lines.append("")

    # Static assert
    lines.append("/* Verify array size at compile time */")
    lines.append('_Static_assert(sizeof(actor_dlftbls) / sizeof(actor_dlftbls[0]) == mAc_PROFILE_NUM,')
    lines.append('               "actor_dlftbls must have exactly mAc_PROFILE_NUM entries");')
    lines.append("")

    # Cleanup macros
    lines.append("#undef S")
    lines.append("#undef P")
    lines.append("")

    # Init/cleanup functions
    lines.append("/*")
    lines.append(" * Initialize the actor dlftbls.")
    lines.append(" */")
    lines.append("void actor_dlftbls_init(void) {")
    lines.append("    /* Table is initialized statically */")
    lines.append("}")
    lines.append("")
    lines.append("/*")
    lines.append(" * Cleanup actor dlftbls.")
    lines.append(" */")
    lines.append("void actor_dlftbls_cleanup(void) {")
    lines.append("    /* Nothing to clean up */")
    lines.append("}")
    lines.append("")

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="Generate PC actor DLF table"
    )
    parser.add_argument(
        "--output", "-o",
        default=DEFAULT_OUTPUT,
        help=f"Output file (default: {DEFAULT_OUTPUT})"
    )
    parser.add_argument(
        "--config", "-c",
        default=DEFAULT_CONFIG,
        help=f"Config file (default: {DEFAULT_CONFIG})"
    )
    parser.add_argument(
        "--dry-run", "-n",
        action="store_true",
        help="Print output instead of writing file"
    )

    args = parser.parse_args()

    # Find project root
    script_dir = Path(__file__).parent
    project_root = script_dir.parent

    m_actor_h = project_root / M_ACTOR_H
    config_path = project_root / args.config
    output_path = project_root / args.output

    print(f"Reading profile enum from: {m_actor_h}")
    profiles = parse_profile_enum(m_actor_h)
    print(f"Found {len(profiles)} profiles")

    print(f"Loading approved actors from: {config_path}")
    approved = load_approved_actors(config_path)
    print(f"Loaded {len(approved)} approved actors")

    # Validate approved actors
    profile_indices = {p.index for p in profiles}
    for a in approved:
        if a.profile_id not in profile_indices:
            print(f"Warning: Approved actor ID {a.profile_id} ({a.profile_name}) not found in enum")

    # Generate table
    content = generate_table(profiles, approved, output_path)

    if args.dry_run:
        print("\n--- Generated content ---")
        print(content)
    else:
        print(f"Writing to: {output_path}")
        output_path.parent.mkdir(parents=True, exist_ok=True)
        with open(output_path, 'w') as f:
            f.write(content)
        print("Done!")

    # Summary
    print(f"\nSummary:")
    print(f"  Total profiles: {len(profiles)}")
    print(f"  Approved actors: {len(approved)}")
    print(f"  Stub entries: {len(profiles) - len(approved)}")

    return 0


if __name__ == "__main__":
    exit(main())
