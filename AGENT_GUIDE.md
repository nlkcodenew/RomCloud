# RomCloud Agent Guide

Tài liệu này mô tả các quy tắc bảo trì, build và phát hành RomCloud.

## Repository

- Source: https://github.com/nlkcodenew/RomCloud
- Releases: https://github.com/nlkcodenew/RomCloud/releases
- Diagnostics: `nlkcodenew/RomCloud-diagnostics` (private)

## Thiết Bị Hỗ Trợ

| Thiết bị | Build | Tag |
| --- | --- | --- |
| TrimUI Brick Pro | `./release-brick-pro.sh` | `brick-pro-vX.Y.Z` |
| TrimUI Smart Pro S | `./release-smart-pro-s.sh` | `smart-pro-s-vX.Y.Z` |

Không dùng một binary hoặc một tag chung cho hai thiết bị.

## Cấu Trúc Chính

```text
src/                    C++ source
assets/                 fonts and images
config/                 packaged defaults
deploy/issue-relay/     Cloudflare diagnostics relay
tests/                  focused tests
tools/                  release helpers
version.json            OTA channel manifest
package.sh              three-asset package builder
```

## Quy Trình Sửa Lỗi

1. Đọc issue và log đã được relay lọc dữ liệu nhạy cảm.
2. Xác định thiết bị, phiên bản, mã `HW-xxxxxxxxxxxx` và lỗi liên quan.
3. Sửa nguyên nhân gốc, giữ thay đổi tối thiểu.
4. Chạy test liên quan và build cả hai thiết bị nếu thay đổi dùng chung.
5. Không commit `dist/`, `.wrangler/`, `wrangler.toml`, token hay toolchain.
6. Commit theo dạng `fix: ...`, `feat: ...`, `docs: ...` hoặc `release: ...`.

## Diagnostics

- Ứng dụng gửi log đã lọc tới endpoint trong `config/reporting.json`.
- GitHub token chỉ tồn tại dưới dạng Cloudflare Worker secret.
- Không thêm token vào source, database, file cấu hình, manifest hoặc ZIP.
- Trước release, quét source và toàn bộ nội dung ZIP để tìm mẫu token.

## Build Và Test

```sh
# Focused tests
node tests/test_issue_relay_worker.mjs
g++ -std=c++17 -Isrc tests/test_device_identity.cpp \
  src/platform/DeviceIdentity.cpp -o /tmp/test_device_identity
/tmp/test_device_identity

# Releases
./release-brick-pro.sh
./release-smart-pro-s.sh
```

Brick Pro xuất binary `bin/RomCloud`. Smart Pro S xuất
`bin/RomCloud-smart-pro-s` bằng TG5050 SDK.

## Quy Trình Phát Hành

Quy trình chuẩn nằm tại [docs/RELEASES.md](docs/RELEASES.md). Tóm tắt:

1. Tăng cùng một version cho cả hai nhánh `APP_VERSION`.
2. Cập nhật `version.json` và hai file release notes.
3. Build cả hai thiết bị.
4. Xác minh ZIP, manifest, SHA-256, quyền thực thi và không có secret.
5. Commit/push `main`.
6. Tạo và push hai annotated tag theo thiết bị.
7. Mỗi GitHub Release chỉ upload:
   - `manifest.json`
   - một ZIP cài đặt theo phiên bản
   - file `<ZIP>.sha256`

Không upload binary rời, Lite Installer, MPV bundle hoặc checksum tổng hợp.

## OTA

- `version.json` chứa package URL và SHA-256 riêng cho từng thiết bị.
- Client tải ZIP đầy đủ, xác minh SHA-256 và chuẩn bị cài khi restart.
- `config/settings.json` của người dùng phải được giữ nguyên.
- Thay đổi OTA phải được kiểm tra với cả luồng cài mới và nâng cấp.

## Checklist Trước Khi Push

- [ ] Test liên quan đạt
- [ ] Brick Pro build thành công
- [ ] Smart Pro S build thành công
- [ ] Mỗi thư mục release có đúng 3 asset
- [ ] Manifest, `.sha256` và ZIP khớp nhau
- [ ] Binary trong ZIP là AArch64 đúng thiết bị
- [ ] `launch.sh` và binary có quyền `0755`
- [ ] Không có token trong source hoặc ZIP
- [ ] `git diff --check` đạt
- [ ] `version.json` trỏ tới đúng tag và ZIP
