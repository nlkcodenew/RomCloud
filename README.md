# RomCloud for TrimUI Brick Pro and Smart Pro S

**RomCloud** is a native, cloud-backed ROM library management application for the **TrimUI Brick Pro** and **TrimUI Smart Pro S** handheld game consoles.

It solves the physical storage constraint of handheld gaming by integrating **Google Drive** as an unlimited master ROM library while treating the local microSD card as an on-demand cache.

---

## Key Features

* **100% Firmware Independence:** Operates entirely inside `/mnt/SDCARD/Apps/RomCloud/`. Never touches `/rom`, `/overlay`, rootfs, or core emulators. Survives all official TrimUI firmware updates.
* **Over-The-Air (OTA) Updates:** Checks the device-specific channel in `version.json`, downloads the complete release ZIP, verifies SHA-256, preserves user settings, and installs safely on restart.
* **Native C++17 & SDL2 Hardware Acceleration:** 60 FPS UI using a 1024x768 logical canvas, automatically letterboxed on the Smart Pro S 1280x720 display.
* **Privacy-safe Diagnostics:** Displays a stable `HW-xxxxxxxxxxxx` device hash and sends sanitized crash logs through an HTTPS relay without storing a GitHub token on the handheld.
* **SQLite3 Local Metadata Database:** High-performance database with WAL journal mode storing tens of thousands of ROMs, system classifications, cover art paths, file sizes, and sync states.
* **Google OAuth 2.0 Device Flow (RFC 8628):** Zero physical keyboard typing needed. Pairs in seconds with any smartphone via high-contrast native QR code.
* **Smart Drive Sync Engine:** Scans Google Drive `/RomCloud/<SYSTEM>/...` hierarchy, matches folder names to TrimUI platform folders, preserves local ROMs, and updates metadata atomically.
* **Resilient Download & Integrity Engine:** Block-based streaming download with speed calculation and ETA, automatic storage space pre-flight validation, and streaming MD5 hash verification against Google Drive.
* **TrimUI MainUI Launcher Integration:** Full integration with TrimUI launcher with native 300x300 icon, `config.json` manifest, and auto-orienting `launch.sh`.

---

## Project Structure

```text
romcloud/
├── assets/
│   └── fonts/font.ttf
├── bin/
│   └── RomCloud
├── config/
│   └── config.json
├── data/
│   └── library.db
├── docs/
│   ├── DATABASE.md
│   ├── DRIVE_SYNC.md
│   ├── DOWNLOAD_ENGINE.md
│   ├── GOOGLE_AUTH.md
│   ├── INSTALLATION.md
│   ├── LIBRARY_UI.md
│   └── USER_MANUAL.md
├── src/
│   ├── app/
│   ├── auth/
│   ├── config/
│   ├── database/
│   ├── download/
│   ├── filesystem/
│   ├── input/
│   ├── logging/
│   ├── network/
│   ├── platform/
│   ├── sync/
│   ├── ui/
│   └── utils/
├── config.json
├── icon.png
├── launch.sh
└── README.md
```

## Build and Release

Build and package the Brick Pro channel with `./release-brick-pro.sh`. Build and
package the Smart Pro S channel with `./release-smart-pro-s.sh`.

Each device release follows the same compact layout as trimui-chiaki-ng:
`manifest.json`, one versioned installation ZIP, and its `.zip.sha256` file.
Users only need to download and extract the versioned ZIP.

See [Release Process](docs/RELEASES.md) for the canonical tag, asset, checksum,
and OTA rules.

The diagnostics endpoint is configured in `config/reporting.json`. The GitHub
token belongs only in the Cloudflare Worker secret described in
`deploy/issue-relay/README.md`, never in the app or release ZIP.

---

## Documentation

* [Documentation Index](docs/README.md)
* [Installation Guide](docs/INSTALLATION.md)
* [User Manual](docs/USER_MANUAL.md)
* [Release Process](docs/RELEASES.md)
* [SQLite Database Architecture](docs/DATABASE.md)
* [Library UI & Cover Engine](docs/LIBRARY_UI.md)
* [Google OAuth 2.0 Flow](docs/GOOGLE_AUTH.md)
* [Drive Sync Engine](docs/DRIVE_SYNC.md)
* [Download & MD5 Engine](docs/DOWNLOAD_ENGINE.md)
