#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

VERSION=$(grep '"SMART_PRO_S_version"' version.json | head -n 1 | awk -F'"' '{print $4}')
TAG="v${VERSION}"

./build-smart-pro-s.sh
TARGET_DEVICE=smart-pro-s ./package.sh

echo "Smart Pro S release artifacts are ready in dist/smart-pro-s/."
echo "Shared release tag: ${TAG}"
