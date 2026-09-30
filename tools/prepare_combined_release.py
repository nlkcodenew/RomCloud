#!/usr/bin/env python3
import hashlib
import json
import pathlib
import shutil


ROOT = pathlib.Path(__file__).resolve().parents[1]
DIST = ROOT / "dist"
RELEASE = DIST / "release"
VERSION_FILE = ROOT / "version.json"


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main():
    data = json.loads(VERSION_FILE.read_text(encoding="utf-8"))
    version = data["version"]
    brick_name = f"RomCloud-brick-pro-v{version}.zip"
    smart_name = f"RomCloud-smart-pro-s-v{version}.zip"
    brick_zip = DIST / "brick-pro" / brick_name
    smart_zip = DIST / "smart-pro-s" / smart_name
    if not brick_zip.is_file() or not smart_zip.is_file():
        raise SystemExit("Build both device packages before preparing the combined release.")

    brick_hash = sha256(brick_zip)
    smart_hash = sha256(smart_zip)
    data["package_sha256"] = brick_hash
    data["SMART_PRO_S_package_sha256"] = smart_hash
    VERSION_FILE.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    RELEASE.mkdir(parents=True, exist_ok=True)
    expected = {
        brick_name,
        f"{brick_name}.sha256",
        smart_name,
        f"{smart_name}.sha256",
        "ota-manifest.json",
    }
    for path in RELEASE.iterdir():
        if path.is_file() and path.name not in expected:
            path.unlink()
    for archive, name, digest in (
        (brick_zip, brick_name, brick_hash),
        (smart_zip, smart_name, smart_hash),
    ):
        shutil.copy2(archive, RELEASE / name)
        (RELEASE / f"{name}.sha256").write_text(f"{digest}  {name}\n", encoding="utf-8")
    shutil.copy2(VERSION_FILE, RELEASE / "ota-manifest.json")
    print(f"Prepared shared release v{version} in {RELEASE}")
    print(f"Brick Pro: {brick_hash}")
    print(f"Smart Pro S: {smart_hash}")


if __name__ == "__main__":
    main()
