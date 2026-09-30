#!/bin/sh
# youtube_search.sh - Optimized YouTube search for RomCloud
# Strategy:
#  - Pagination: 6 results per page via --playlist-start offset
#  - Lean flags: skip-download, no-playlist, no-warnings
#  - Falls back to bundled yt-dlp-glibc on Linux
# Usage:
#   ./youtube_search.sh search "<query>" [page=1] [per_page=6]
#   ./youtube_search.sh url "<video_id>"

BINDIR="$(cd "$(dirname "$0")" && pwd)/.."

if [ -x "${BINDIR}/bin/yt-dlp-glibc" ] && [ "$(uname -s)" = "Linux" ]; then
    YTDLP="${BINDIR}/bin/yt-dlp-glibc"
elif [ -x "${BINDIR}/bin/yt-dlp" ]; then
    YTDLP="${BINDIR}/bin/yt-dlp"
elif command -v yt-dlp >/dev/null 2>&1; then
    YTDLP="$(command -v yt-dlp)"
else
    echo "ERROR:NOMODULE Khong tim thay yt-dlp trong bin/" >&2
    exit 1
fi

PER_PAGE=6

case "$1" in
    search)
        QUERY="$2"
        PAGE="${3:-1}"
        PER_PAGE="${4:-$PER_PAGE}"
        if [ -z "$QUERY" ]; then
            echo "ERROR:EMPTY query is empty" >&2
            exit 1
        fi

        TOTAL=$(( PAGE * PER_PAGE ))
        START=$(( (PAGE - 1) * PER_PAGE + 1 ))
        END=$(( PAGE * PER_PAGE ))

        # Clean stale PyInstaller temp folders to prevent decompression errors
        rm -rf /tmp/_MEI* 2>/dev/null

        # Lean query: minimal flags, only fetch metadata (flat-playlist is critical for speed)
        "$YTDLP" \
            --flat-playlist \
            --no-warnings \
            --skip-download \
            --socket-timeout 8 \
            --playlist-start "$START" \
            --playlist-end "$END" \
            --print '%(id)s|%(title)s|%(duration)s|%(uploader)s|%(view_count)s' \
            "ytsearch${END}:${QUERY}" 2>"/tmp/romcloud_youtube_error.log"
        STATUS=$?
        if [ "$STATUS" -ne 0 ]; then
            sed 's/^/YTDLP: /' /tmp/romcloud_youtube_error.log >&2
        fi
        exit "$STATUS"
        ;;
    url)
        VIDEO_ID="$2"
        QUALITY="${3:-auto}"
        if [ -z "$VIDEO_ID" ]; then
            echo "ERROR:NOSTREAM no video id" >&2
            exit 1
        fi

        # Clean stale PyInstaller temp folders to prevent decompression errors
        rm -rf /tmp/_MEI* 2>/dev/null

        FORMAT="bestvideo[height<=720][vcodec^=avc1]+bestaudio[acodec^=mp4a]/best[height<=720][vcodec^=avc1]/best[height<=720]"
        case "$QUALITY" in
            720) FORMAT="bestvideo[height<=720][vcodec^=avc1]+bestaudio[acodec^=mp4a]/best[height<=720][vcodec^=avc1]/best[height<=720]" ;;
            360) FORMAT="bestvideo[height<=360][vcodec^=avc1]+bestaudio[acodec^=mp4a]/best[height<=360][vcodec^=avc1]/best[height<=360]" ;;
        esac

        : > /tmp/romcloud_youtube_error.log
        echo "RomCloud resolver: video_id=${VIDEO_ID} quality=${QUALITY} format=${FORMAT}" >>/tmp/romcloud_youtube_error.log

        # Let yt-dlp select its current working client. Request separate H.264
        # video and M4A audio streams because progressive formats 18/22 are no
        # longer exposed consistently by YouTube.
        RAW_URLS=$("$YTDLP" -g \
            --cache-dir /tmp/yt_cache \
            --no-check-certificates \
            -f "$FORMAT" \
            --socket-timeout 12 \
            --retries 2 \
            "https://www.youtube.com/watch?v=${VIDEO_ID}" 2>>"/tmp/romcloud_youtube_error.log")

        V_URL=$(echo "$RAW_URLS" | sed -n '1p')
        A_URL=$(echo "$RAW_URLS" | sed -n '2p')

        if [ -n "$V_URL" ] && [ "${V_URL#http}" != "$V_URL" ]; then
            if [ -n "$A_URL" ] && [ "${A_URL#http}" != "$A_URL" ]; then
                echo "${V_URL}|${A_URL}"
            else
                echo "${V_URL}"
            fi
            exit 0
        fi

        echo "RomCloud resolver: primary extraction returned no playable URL; retrying without cache" >>/tmp/romcloud_youtube_error.log
        rm -rf /tmp/yt_cache 2>>/tmp/romcloud_youtube_error.log
        RAW_URLS=$("$YTDLP" -g \
            --cache-dir /tmp/yt_cache \
            --no-check-certificates \
            --socket-timeout 15 --retries 2 -f "$FORMAT" \
            "https://www.youtube.com/watch?v=${VIDEO_ID}" 2>>"/tmp/romcloud_youtube_error.log")
        V_URL=$(echo "$RAW_URLS" | sed -n '1p')
        A_URL=$(echo "$RAW_URLS" | sed -n '2p')

        if [ -n "$V_URL" ] && [ "${V_URL#http}" != "$V_URL" ]; then
            if [ -n "$A_URL" ] && [ "${A_URL#http}" != "$A_URL" ]; then
                echo "${V_URL}|${A_URL}"
            else
                echo "${V_URL}"
            fi
            exit 0
        fi

        sed 's/^/YTDLP: /' /tmp/romcloud_youtube_error.log >&2
        echo "ERROR:NOSTREAM Failed to extract stream URL for ${VIDEO_ID}" >&2
        exit 1
        ;;
    *)
        echo "Usage: youtube_search.sh search <query> [page] [per_page]" >&2
        echo "       youtube_search.sh url <video_id> [360|720|auto]" >&2
        exit 1
        ;;
esac
