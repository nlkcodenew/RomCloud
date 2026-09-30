# Cài Đặt RomCloud

Tài liệu này áp dụng cho **TrimUI Brick Pro** và **TrimUI Smart Pro S**.

## 1. Chọn Đúng Bản Cho Thiết Bị

Mở trang [RomCloud Releases](https://github.com/nlkcodenew/RomCloud/releases)
và chọn tag tương ứng:

| Thiết bị | Tag | File cần tải |
| --- | --- | --- |
| TrimUI Brick Pro | `vX.Y.Z` | `RomCloud-brick-pro-vX.Y.Z.zip` |
| TrimUI Smart Pro S | `vX.Y.Z` | `RomCloud-smart-pro-s-vX.Y.Z.zip` |

Mỗi release còn có `manifest.json` và file `.zip.sha256` để kiểm tra phát hành.
Người dùng bình thường **chỉ cần tải một file ZIP** trong bảng trên. Không tải
`Source code (zip)` hoặc `Source code (tar.gz)` do GitHub tự tạo.

## 2. Cài Vào Thẻ Nhớ

1. Tắt máy và tháo thẻ nhớ, hoặc kết nối thẻ với máy tính.
2. Giải nén file ZIP vừa tải vào thư mục gốc của thẻ.
3. Sau khi giải nén, xác nhận có đường dẫn:

   ```text
   /mnt/SDCARD/Apps/RomCloud/launch.sh
   /mnt/SDCARD/Apps/RomCloud/bin/RomCloud
   ```

4. Tháo thẻ an toàn, lắp lại vào máy và khởi động.
5. Mở **Apps → RomCloud**.

Không chép nguyên file ZIP vào `Apps/RomCloud`; phải giải nén để thư mục `Apps`
trong ZIP hòa vào thư mục `Apps` trên thẻ nhớ.

## 3. Cập Nhật OTA

1. Kết nối Wi-Fi.
2. Trong RomCloud, mở màn hình **Cập nhật**.
3. Chọn cập nhật khi có phiên bản mới.
4. Ứng dụng tải ZIP đúng thiết bị và xác minh SHA-256.
5. Khởi động lại theo hướng dẫn để hoàn tất cài đặt.

OTA giữ lại `config/settings.json`. Không tắt máy hoặc tháo thẻ trong lúc đang
tải hay cài đặt.

## 4. Kiểm Tra SHA-256 Tùy Chọn

File `<tên ZIP>.sha256` chứa checksum chính thức. Có thể kiểm tra trên máy tính:

```sh
sha256sum -c RomCloud-brick-pro-vX.Y.Z.zip.sha256
```

Thay tên file bằng bản Smart Pro S nếu cần. Nếu checksum không khớp, xóa file
và tải lại từ GitHub Releases.

## 5. Xử Lý Sự Cố

- **App không xuất hiện:** kiểm tra đúng đường dẫn `Apps/RomCloud/config.json`.
- **App không chạy:** xác nhận đã tải đúng ZIP cho thiết bị.
- **ZIP có thêm một thư mục lồng:** di chuyển thư mục `Apps` về gốc thẻ nhớ.
- **OTA thất bại:** kiểm tra Wi-Fi, dung lượng trống và thử tải ZIP thủ công.
- **Cần báo lỗi:** mở mục chẩn đoán trong ứng dụng và cung cấp mã thiết bị băm
  `HW-xxxxxxxxxxxx`; không gửi token, mật khẩu hoặc mã định danh phần cứng thô.
