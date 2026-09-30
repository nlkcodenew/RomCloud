# RomCloud Release Process

RomCloud publishes separate release channels for each supported device. Both
channels use the same application version but must have separate tags and ZIPs.

## Device Channels

| Device | Tag format | Installation ZIP |
| --- | --- | --- |
| TrimUI Brick Pro | `brick-pro-vX.Y.Z` | `RomCloud-brick-pro-vX.Y.Z.zip` |
| TrimUI Smart Pro S | `smart-pro-s-vX.Y.Z` | `RomCloud-smart-pro-s-vX.Y.Z.zip` |

Do not use a shared generic tag because the two devices use different build
toolchains and binaries.

## Release Assets

Each GitHub Release must contain exactly three uploaded assets:

1. `manifest.json`
2. One device-specific, versioned installation ZIP
3. The matching `<installation ZIP>.sha256`

GitHub also displays its automatically generated source archives. End users
only need the versioned installation ZIP.

The ZIP contains the complete `Apps/RomCloud` directory, including the app,
launcher, media tools, libraries, assets, and default configuration. Do not
publish separate binaries, Lite installers, or media bundles.

## Build Commands

```sh
./release-brick-pro.sh
./release-smart-pro-s.sh
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
7. Commit and push `main`, then create and push both annotated tags.
8. Create one GitHub Release per tag and upload exactly the three assets.

## OTA Rules

- OTA selects the channel compiled into the running binary.
- The full ZIP is downloaded and checked against `package_sha256` from
  `version.json` before installation.
- User `config/settings.json` is preserved during package installation.
- GitHub credentials must never be stored in the app, manifest, ZIP, or SD
  card. Diagnostics use the HTTPS relay configured in `config/reporting.json`.

## Current Release

- Brick Pro: `brick-pro-v2.2.2`
- Smart Pro S: `smart-pro-s-v2.2.2`
