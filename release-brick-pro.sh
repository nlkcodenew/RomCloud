#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

VERSION=$(grep '"version"' version.json | head -n 1 | awk -F'"' '{print $4}')
TAG="brick-pro-v${VERSION}"

TARGET_DEVICE=brick-pro ./build.sh
TARGET_DEVICE=brick-pro ./package.sh

echo "Brick Pro release artifacts are ready in dist/."
echo "Recommended tag: ${TAG}"
