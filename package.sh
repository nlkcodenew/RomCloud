#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

TARGET_DEVICE="${TARGET_DEVICE:-brick-pro}"
case "$TARGET_DEVICE" in
  brick-pro)
    TARGET_SLUG="brick-pro"
    TARGET_NAME="TrimUI Brick Pro"
    VERSION=$(grep '"version"' version.json | head -n 1 | awk -F'"' '{print $4}')
    BINARY_SOURCE="bin/RomCloud"
    ;;
  smart-pro-s)
    TARGET_SLUG="smart-pro-s"
    TARGET_NAME="TrimUI Smart Pro S"
    VERSION=$(grep '"SMART_PRO_S_version"' version.json | head -n 1 | awk -F'"' '{print $4}')
    BINARY_SOURCE="bin/RomCloud-smart-pro-s"
    ;;
  *) echo "Unsupported TARGET_DEVICE: $TARGET_DEVICE" >&2; exit 2 ;;
esac

TAG="${TARGET_SLUG}-v${VERSION}"
ZIP_NAME="RomCloud-${TARGET_SLUG}-v${VERSION}.zip"
DIST_DIR="$SCRIPT_DIR/dist/$TARGET_SLUG"
STAGING_DIR="$SCRIPT_DIR/dist/staging-$TARGET_SLUG"

echo "=== Packaging RomCloud v${VERSION} for ${TARGET_NAME} ==="
rm -rf "$STAGING_DIR" "$DIST_DIR"
mkdir -p "$DIST_DIR" "$STAGING_DIR/Apps/RomCloud/bin" "$STAGING_DIR/Apps/RomCloud/lib"
mkdir -p "$STAGING_DIR/Apps/RomCloud/scripts" "$STAGING_DIR/Apps/RomCloud/assets/fonts"
mkdir -p "$STAGING_DIR/Apps/RomCloud/assets/icons" "$STAGING_DIR/Apps/RomCloud/assets/apps_icons"
mkdir -p "$STAGING_DIR/Apps/RomCloud/assets/player_icons" "$STAGING_DIR/Apps/RomCloud/assets/button_icons"
mkdir -p "$STAGING_DIR/Apps/RomCloud/config" "$STAGING_DIR/Apps/RomCloud/iptv"

cp config.json "$STAGING_DIR/Apps/RomCloud/"
cp icon.png "$STAGING_DIR/Apps/RomCloud/icon.png"
cp -f iconsel.png "$STAGING_DIR/Apps/RomCloud/iconsel.png" 2>/dev/null || cp icon.png "$STAGING_DIR/Apps/RomCloud/iconsel.png"
cp -f icontop.png "$STAGING_DIR/Apps/RomCloud/icontop.png" 2>/dev/null || cp icon.png "$STAGING_DIR/Apps/RomCloud/icontop.png"
cp launch.sh "$STAGING_DIR/Apps/RomCloud/"
cp "$BINARY_SOURCE" "$STAGING_DIR/Apps/RomCloud/bin/RomCloud"

for binary in yt-dlp yt-dlp-glibc mpv; do
    if [ -f "bin/$binary" ]; then
        cp "bin/$binary" "$STAGING_DIR/Apps/RomCloud/bin/"
        chmod +x "$STAGING_DIR/Apps/RomCloud/bin/$binary"
    fi
done
if [ -d scripts ]; then
    cp -r scripts/* "$STAGING_DIR/Apps/RomCloud/scripts/"
    chmod +x "$STAGING_DIR/Apps/RomCloud/scripts/"*.sh 2>/dev/null || true
fi
if [ -d lib ]; then
    cp -P lib/*.so* "$STAGING_DIR/Apps/RomCloud/lib/" 2>/dev/null || true
fi
if [ "$TARGET_DEVICE" = "smart-pro-s" ]; then
    rm -f "$STAGING_DIR/Apps/RomCloud/lib/libSDL2"*.so* 2>/dev/null || true
fi

cp assets/fonts/font.ttf "$STAGING_DIR/Apps/RomCloud/assets/fonts/"
cp assets/fonts/NotoSans-Regular.ttf "$STAGING_DIR/Apps/RomCloud/assets/fonts/" 2>/dev/null || true
cp assets/icons/*.png "$STAGING_DIR/Apps/RomCloud/assets/icons/"
cp assets/apps_icons/*.png "$STAGING_DIR/Apps/RomCloud/assets/apps_icons/" 2>/dev/null || true
cp -r assets/player_icons/* "$STAGING_DIR/Apps/RomCloud/assets/player_icons/" 2>/dev/null || true
cp assets/button_icons/*.png "$STAGING_DIR/Apps/RomCloud/assets/button_icons/" 2>/dev/null || true
cp config/settings.json config/reporting.json "$STAGING_DIR/Apps/RomCloud/config/"
cp -f config/*.conf "$STAGING_DIR/Apps/RomCloud/config/" 2>/dev/null || true
cp -f iptv/*.m3u iptv/*.m3u8 "$STAGING_DIR/Apps/RomCloud/iptv/" 2>/dev/null || true
cp -f iptv/sources.txt "$STAGING_DIR/Apps/RomCloud/iptv/" 2>/dev/null || true

chmod +x "$STAGING_DIR/Apps/RomCloud/launch.sh" "$STAGING_DIR/Apps/RomCloud/bin/RomCloud"
python3 "$SCRIPT_DIR/tools/create_zip.py" "$DIST_DIR/$ZIP_NAME" "$STAGING_DIR/Apps"
python3 "$SCRIPT_DIR/tools/create_release_manifest.py" \
    --zip "$DIST_DIR/$ZIP_NAME" \
    --output "$DIST_DIR/manifest.json" \
    --app romcloud \
    --device "$TARGET_SLUG" \
    --version "$VERSION" \
    --tag "$TAG"

cd "$DIST_DIR"
sha256sum "$ZIP_NAME" > "$ZIP_NAME.sha256"
cd "$SCRIPT_DIR"
rm -rf "$STAGING_DIR"

echo "=== Release Assets Created ==="
ls -lh "$DIST_DIR/manifest.json" "$DIST_DIR/$ZIP_NAME" "$DIST_DIR/$ZIP_NAME.sha256"
