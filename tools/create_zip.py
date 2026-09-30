#!/usr/bin/env python3
import argparse
import os
import stat
import zipfile


def add_path(archive, source, arcname):
    source = os.path.abspath(source)
    if os.path.isdir(source):
        for root, directories, files in os.walk(source):
            directories.sort()
            files.sort()
            relative_root = os.path.relpath(root, os.path.dirname(source))
            if not files and not directories:
                info = zipfile.ZipInfo(relative_root.rstrip("/") + "/")
                info.external_attr = (stat.S_IFDIR | 0o755) << 16
                archive.writestr(info, b"")
            for filename in files:
                file_path = os.path.join(root, filename)
                relative_path = os.path.relpath(file_path, os.path.dirname(source))
                add_file(archive, file_path, relative_path)
    else:
        add_file(archive, source, arcname)


def add_file(archive, source, arcname):
    info = zipfile.ZipInfo.from_file(source, arcname)
    normalized = arcname.replace("\\", "/")
    executable = normalized.endswith(".sh") or "/bin/" in f"/{normalized}" or normalized.endswith("/RomCloud")
    mode = 0o755 if executable else 0o644
    info.external_attr = (stat.S_IFREG | mode) << 16
    info.compress_type = zipfile.ZIP_DEFLATED
    with open(source, "rb") as handle:
        archive.writestr(info, handle.read())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output")
    parser.add_argument("paths", nargs="+")
    args = parser.parse_args()

    output = os.path.abspath(args.output)
    os.makedirs(os.path.dirname(output), exist_ok=True)
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for path in args.paths:
            add_path(archive, path, os.path.basename(path.rstrip("/")))


if __name__ == "__main__":
    main()
