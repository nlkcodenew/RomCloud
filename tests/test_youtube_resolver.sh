#!/bin/sh
set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
TEST_DIR=$(mktemp -d)
trap 'rm -rf "$TEST_DIR"' EXIT
mkdir -p "$TEST_DIR/scripts" "$TEST_DIR/bin"
cp "$ROOT/scripts/youtube_search.sh" "$TEST_DIR/scripts/youtube_search.sh"
sed -i 's/\r$//' "$TEST_DIR/scripts/youtube_search.sh"
cat > "$TEST_DIR/bin/yt-dlp-glibc" <<'EOF'
#!/bin/sh
case " $* " in
    *player_client=android*) echo 'https://example.test/video?c=ANDROID' ;;
    *) echo 'https://example.test/video?c=VISIONOS'; echo 'https://example.test/audio?c=VISIONOS' ;;
esac
EOF
cat > "$TEST_DIR/bin/curl" <<'EOF'
#!/bin/sh
case " $* " in
    *c=ANDROID*) printf 206 ;;
    *) printf 403 ;;
esac
EOF
chmod +x "$TEST_DIR/bin/yt-dlp-glibc" "$TEST_DIR/bin/curl"
PATH="$TEST_DIR/bin:$PATH" "$TEST_DIR/scripts/youtube_search.sh" url Y6DGORE8EiI 360 > "$TEST_DIR/result"
test "$(cat "$TEST_DIR/result")" = 'https://example.test/video?c=ANDROID'
grep -q 'video HTTP 403' /tmp/romcloud_youtube_error.log
grep -q 'video HTTP 206' /tmp/romcloud_youtube_error.log
PATH="$TEST_DIR/bin:$PATH" "$TEST_DIR/scripts/youtube_search.sh" url Y6DGORE8EiI 360 android > "$TEST_DIR/result"
test "$(cat "$TEST_DIR/result")" = 'https://example.test/video?c=ANDROID'
if grep -q 'video HTTP 403' /tmp/romcloud_youtube_error.log; then exit 1; fi
echo 'youtube resolver tests passed'
