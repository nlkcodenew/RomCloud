#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

SDK_ROOT="${SDK_ROOT:-/tmp/tg5050-sdk-romcloud/sdk_tg5050_linux_v1.0.0}"
CXX="$SDK_ROOT/host/bin/aarch64-none-linux-gnu-g++"
SYSROOT="$SDK_ROOT/host/aarch64-buildroot-linux-gnu/sysroot"

if [ ! -x "$CXX" ]; then
    echo "TG5050 SDK compiler not found: $CXX" >&2
    exit 1
fi

echo "=== Compiling RomCloud for TrimUI Smart Pro S (TG5050 SDK) ==="
mkdir -p bin

"$CXX" \
    -std=c++17 \
    -O3 \
    -Wall -Wextra \
    -DROMCLOUD_TARGET_SMART_PRO_S=1 \
    -Isrc \
    -I"$SYSROOT/usr/include" \
    -I"$SYSROOT/usr/include/SDL2" \
    src/main.cpp \
    src/app/Application.cpp \
    src/ui/UIManager.cpp \
    src/ui/CoverManager.cpp \
    src/ui/BoxartScraper.cpp \
    src/ui/QrRenderer.cpp \
    src/ui/qrcodegen.cpp \
    src/network/HttpClient.cpp \
    src/network/WebServer.cpp \
    src/auth/AuthManager.cpp \
    src/sync/DriveSyncEngine.cpp \
    src/download/DownloadManager.cpp \
    src/ota/UpdateManager.cpp \
    src/input/InputManager.cpp \
    src/filesystem/FileSystemManager.cpp \
    src/platform/PlatformInfo.cpp \
    src/platform/DeviceIdentity.cpp \
    src/logging/Logger.cpp \
    src/logging/IssueLogger.cpp \
    src/iptv/IPTVManager.cpp \
    src/iptv/TikTokManager.cpp \
    src/config/AppConfig.cpp \
    src/database/DatabaseManager.cpp \
    src/database/RomIndexer.cpp \
    src/sync/UploadManager.cpp \
    src/backup/BackupManager.cpp \
    src/rom/RomDetector.cpp \
    src/rom/RomOrganizer.cpp \
    src/localsend/LocalSendManager.cpp \
    -L"$SYSROOT/usr/lib" \
    -Wl,-rpath-link,"$SYSROOT/usr/lib" \
    -lSDL2 \
    -lSDL2_image \
    -lSDL2_ttf \
    -lsqlite3 \
    -lcurl \
    -lssl \
    -lcrypto \
    -lpthread \
    -ldl \
    -lm \
    -o bin/RomCloud-smart-pro-s

"$SDK_ROOT/host/bin/aarch64-none-linux-gnu-strip" --strip-unneeded bin/RomCloud-smart-pro-s
echo "=== Build Successful: bin/RomCloud-smart-pro-s ==="
ls -lh bin/RomCloud-smart-pro-s
file bin/RomCloud-smart-pro-s
