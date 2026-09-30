#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

./release-brick-pro.sh
./release-smart-pro-s.sh
python3 tools/prepare_combined_release.py

VERSION=$(grep '"version"' version.json | head -n 1 | awk -F'"' '{print $4}')
echo "Combined release assets are ready in dist/release/."
echo "Shared release tag: v${VERSION}"
