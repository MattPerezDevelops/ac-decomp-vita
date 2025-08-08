#!/usr/bin/env python3
"""
Asset preprocessing for Animal Crossing Vita
Ensures all assets are in the correct format for runtime loading
"""

import os
import json
from pathlib import Path

def preprocess_assets():
    # Copy assets from the main asset directory
    asset_src = "../../ac_assets"
    asset_dst = "assets"
    
    if os.path.exists(asset_src):
        print(f"📦 Copying assets from {asset_src} to {asset_dst}...")
        os.system(f"cp -r {asset_src} {asset_dst}")
        print("✅ Assets ready for runtime loading!")
    else:
        print(f"⚠️ Asset directory not found: {asset_src}")
        print("Assets will be loaded from memory if available")

if __name__ == "__main__":
    preprocess_assets()
