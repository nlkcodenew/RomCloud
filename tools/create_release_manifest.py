#!/usr/bin/env python3
import argparse
import datetime
import hashlib
import json
import os
import zipfile


def sha256_bytes(data):
    return hashlib.sha256(data).hexdigest()


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--zip", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--app", required=True)
    parser.add_argument("--device", required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--tag", required=True)
    args = parser.parse_args()

    files = []
    with zipfile.ZipFile(args.zip) as archive:
        for info in sorted(archive.infolist(), key=lambda item: item.filename):
            if info.is_dir():
                continue
            data = archive.read(info.filename)
            files.append({
                "path": info.filename,
                "sha256": sha256_bytes(data),
                "size": info.file_size,
            })

    asset_name = os.path.basename(args.zip)
    manifest = {
        "version": args.version,
        "app": args.app,
        "device": args.device,
        "built": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "release_tag": args.tag,
        "prerelease": False,
        "files": files,
        "remove": [],
        "release_asset": {
            "name": asset_name,
            "sha256": sha256_file(args.zip),
            "size": os.path.getsize(args.zip),
        },
    }
    os.makedirs(os.path.dirname(os.path.abspath(args.output)), exist_ok=True)
    with open(args.output, "w", encoding="utf-8", newline="\n") as handle:
        json.dump(manifest, handle, ensure_ascii=False, indent=2)
        handle.write("\n")


if __name__ == "__main__":
    main()
