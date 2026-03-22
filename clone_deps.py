#!/usr/bin/env python3
"""
clone_deps.py - Automatically clone Breakout game dependencies into libs directory
"""

import os
import subprocess
import sys
from pathlib import Path

# Dependency configuration: repository URL, branch/tag, target directory name
DEPENDENCIES = [
    {
        "url": "https://github.com/SFML/SFML.git",
        "version": "3.0.2",
        "dir": "SFML"
    },
    {
        "url": "https://github.com/erincatto/box2d.git",
        "version": "v2.4.2",
        "dir": "box2d"
    },
    {
        "url": "https://github.com/ocornut/imgui.git",
        "version": "v1.91.9",
        "dir": "imgui"
    },
    {
        "url": "https://github.com/SFML/imgui-sfml.git",
        "version": "master",    # 3.0 corresponds to master branch
        "dir": "imgui-sfml"
    }
]

def clone_repo(url, version, target_dir, depth=1):
    """Clone the repository to the specified directory; skip if the directory already exists."""
    if target_dir.exists():
        print(f"Directory {target_dir} already exists, skipping clone.")
        return True

    print(f"Cloning {url} (version {version}) to {target_dir}...")
    try:
        # Use shallow clone with --depth 1 and --branch
        subprocess.run(
            ["git", "clone", "--depth", str(depth), "--branch", version, url, str(target_dir)],
            check=True,
            capture_output=True,
            text=True
        )
        print(f"Successfully cloned {target_dir}")
        return True
    except subprocess.CalledProcessError as e:
        print(f"Failed to clone {url}: {e.stderr}", file=sys.stderr)
        return False

def main():
    # Get the script's directory and determine the libs directory path
    script_dir = Path(__file__).resolve().parent
    libs_dir = script_dir / "libs"

    # Create the libs directory if it does not exist
    libs_dir.mkdir(exist_ok=True)

    # Clone dependencies one by one
    success = True
    for dep in DEPENDENCIES:
        target = libs_dir / dep["dir"]
        if not clone_repo(dep["url"], dep["version"], target):
            success = False
            # Optionally decide whether to continue cloning other libraries; here we continue but mark failure
            # If you want to exit immediately on error, uncomment break

    if success:
        print("\nAll dependencies cloned successfully!")
    else:
        print("\nFailed to clone some dependencies. Please check network connection or version information.", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
