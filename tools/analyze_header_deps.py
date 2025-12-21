#!/usr/bin/env python3
"""
Analyze header dependencies across actor files.

This script scans all actor source files and counts which headers are included
most frequently. This helps identify which headers are blocking the most actors
from being compiled for the PC port.

Usage:
    python3 tools/analyze_header_deps.py
"""

import os
import re
from collections import Counter, defaultdict
from pathlib import Path

# Configuration
ACTOR_DIR = "src/actor"
INCLUDE_PATTERN = re.compile(r'#include\s+[<"]([^>"]+)[>"]')

def scan_includes(filepath):
    """Extract all #include statements from a C file."""
    includes = []
    try:
        with open(filepath, 'r', errors='ignore') as f:
            for line in f:
                match = INCLUDE_PATTERN.search(line)
                if match:
                    includes.append(match.group(1))
    except Exception as e:
        print(f"Warning: Could not read {filepath}: {e}")
    return includes

def categorize_header(header):
    """Categorize a header by its path/type."""
    if header.startswith('m_'):
        return 'game_m_*'
    elif header.startswith('ac_'):
        return 'actor_ac_*'
    elif header.startswith('dolphin/'):
        return 'dolphin'
    elif header.startswith('libc64/') or header.startswith('libultra/'):
        return 'libultra'
    elif header.startswith('sys_'):
        return 'sys_*'
    elif header.startswith('game/'):
        return 'game/'
    elif '/' in header:
        return header.split('/')[0] + '/'
    else:
        return 'other'

def main():
    # Find project root (parent of tools/)
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    actor_path = project_root / ACTOR_DIR

    if not actor_path.exists():
        print(f"Error: Actor directory not found: {actor_path}")
        return 1

    # Collect data
    header_counts = Counter()
    header_to_actors = defaultdict(list)
    actor_files = list(actor_path.glob("*.c"))

    print(f"Scanning {len(actor_files)} actor files in {actor_path}...")
    print()

    for actor_file in sorted(actor_files):
        includes = scan_includes(actor_file)
        actor_name = actor_file.name
        for header in includes:
            header_counts[header] += 1
            header_to_actors[header].append(actor_name)

    # Print summary
    print("=" * 70)
    print("HEADER DEPENDENCY HEATMAP")
    print("=" * 70)
    print()
    print("Headers ranked by how many actor files include them.")
    print("Focus on stubbing/fixing high-count headers first to unblock more actors.")
    print()

    # Top headers overall
    print("-" * 70)
    print("TOP 40 HEADERS BY USAGE")
    print("-" * 70)
    print(f"{'Header':<50} {'Count':>6}")
    print("-" * 70)

    for header, count in header_counts.most_common(40):
        print(f"{header:<50} {count:>6}")

    print()

    # Category breakdown
    print("-" * 70)
    print("HEADERS BY CATEGORY")
    print("-" * 70)

    category_counts = defaultdict(Counter)
    for header, count in header_counts.items():
        cat = categorize_header(header)
        category_counts[cat][header] = count

    # Sort categories by total impact
    cat_totals = {cat: sum(counts.values()) for cat, counts in category_counts.items()}

    for cat in sorted(cat_totals.keys(), key=lambda c: -cat_totals[c]):
        total = cat_totals[cat]
        headers = category_counts[cat]
        print(f"\n{cat} (total: {total} includes across {len(headers)} headers):")
        for header, count in headers.most_common(10):
            print(f"  {header:<48} {count:>4}")
        if len(headers) > 10:
            print(f"  ... and {len(headers) - 10} more")

    print()

    # m_* headers are likely the biggest blockers - detail them
    print("-" * 70)
    print("DETAILED: m_*.h HEADERS (likely biggest blockers)")
    print("-" * 70)

    m_headers = [(h, c) for h, c in header_counts.items() if h.startswith('m_')]
    m_headers.sort(key=lambda x: -x[1])

    print(f"{'Header':<40} {'Count':>6}  {'Sample Actors'}")
    print("-" * 70)

    for header, count in m_headers[:25]:
        actors = header_to_actors[header][:3]
        actors_str = ', '.join(actors)
        if len(header_to_actors[header]) > 3:
            actors_str += f' (+{len(header_to_actors[header]) - 3} more)'
        print(f"{header:<40} {count:>6}  {actors_str}")

    print()

    # Summary stats
    print("-" * 70)
    print("SUMMARY STATISTICS")
    print("-" * 70)
    print(f"Total actor files scanned:     {len(actor_files)}")
    print(f"Unique headers found:          {len(header_counts)}")
    print(f"Total #include statements:     {sum(header_counts.values())}")
    print(f"Average includes per actor:    {sum(header_counts.values()) / len(actor_files):.1f}")

    # Find actors with fewest dependencies (easiest to port)
    print()
    print("-" * 70)
    print("ACTORS WITH FEWEST DEPENDENCIES (easiest to port)")
    print("-" * 70)

    actor_dep_counts = []
    for actor_file in actor_files:
        includes = scan_includes(actor_file)
        actor_dep_counts.append((actor_file.name, len(includes), includes))

    actor_dep_counts.sort(key=lambda x: x[1])

    print(f"{'Actor File':<40} {'# Includes':>10}")
    print("-" * 70)
    for actor, count, includes in actor_dep_counts[:20]:
        print(f"{actor:<40} {count:>10}")

    print()
    print("Done!")
    return 0

if __name__ == "__main__":
    exit(main())
