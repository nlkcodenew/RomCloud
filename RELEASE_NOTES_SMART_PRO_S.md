# RomCloud 2.2.7 — TrimUI Smart Pro S

- Kiểm tra HTTP 200/206 của video và audio trước khi mở MPV; URL bị 403 chuyển sang Android MP4.
- Khi MPV vẫn gặp HTTP 403, tự resolve lại với Android và phát lại đúng một lần.
- Ghi rõ mã HTTP từng luồng, giữ log MPV và chỉ gửi lỗi lên GitHub Issues nếu retry thất bại.
- Giữ luồng H.264 + M4A chất lượng cao cho các video đã phát ổn định.
