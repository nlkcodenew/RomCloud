# RomCloud Release Process

RomCloud publishes one shared GitHub Release for every application version.
Each device keeps its own binary and installation ZIP inside that release.

## Device Channels

| Device | Shared tag | Installation ZIP |
| --- | --- | --- |
| TrimUI Brick Pro | `vX.Y.Z` | `RomCloud-brick-pro-vX.Y.Z.zip` |
| TrimUI Smart Pro S | `vX.Y.Z` | `RomCloud-smart-pro-s-vX.Y.Z.zip` |

The shared tag never implies a shared binary. OTA selects the correct ZIP from
`version.json` using the device channel compiled into the app.

## Release Assets

Each GitHub Release must contain exactly five uploaded assets:

1. `ota-manifest.json`
2. `RomCloud-brick-pro-vX.Y.Z.zip`
3. `RomCloud-brick-pro-vX.Y.Z.zip.sha256`
4. `RomCloud-smart-pro-s-vX.Y.Z.zip`
5. `RomCloud-smart-pro-s-vX.Y.Z.zip.sha256`

GitHub also displays its automatically generated source archives. End users
only need the versioned installation ZIP.

The ZIP contains the complete `Apps/RomCloud` directory, including the app,
launcher, media tools, libraries, assets, and default configuration. Do not
publish separate binaries, Lite installers, or media bundles.

## Build Commands

```sh
./release.sh
```

Artifacts are written to:

```text
dist/brick-pro/
dist/smart-pro-s/
```

The Smart Pro S build uses the TG5050 SDK. The Brick Pro build uses the Zig
AArch64 target configured by `build.sh`.

## Version Checklist

Before creating tags:

1. Set both `APP_VERSION` branches in `src/ota/UpdateManager.h`.
2. Update both device channels, package URLs, SHA-256 values, dates, and
   changelogs in `version.json`.
3. Update both release-note files.
4. Build both targets and verify each ZIP against `manifest.json` and its
   `.zip.sha256` file.
5. Confirm each ZIP contains the correct AArch64 binary and executable modes.
6. Scan source and ZIP contents for credentials.
7. Run `./release.sh`, commit and push `main`, then create and push tag `vX.Y.Z`.
8. Create one GitHub Release and upload the five files in `dist/release/`.

## OTA Rules

- OTA selects the channel compiled into the running binary.
- The full ZIP is downloaded and checked against `package_sha256` from
  `version.json` before installation.
- User `config/settings.json` is preserved during package installation.
- GitHub credentials must never be stored in the app, manifest, ZIP, or SD
  card. Diagnostics use the HTTPS relay configured in `config/reporting.json`.

## Current Release

- Both devices: `v2.2.3`
