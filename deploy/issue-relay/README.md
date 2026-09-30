# RomCloud GitHub Issue relay

Relay nhận log đã lọc từ RomCloud và tạo Issue trong repo chẩn đoán private.
GitHub token chỉ được lưu trong Cloudflare Worker secret, không nằm trong app,
source, release ZIP hoặc thẻ nhớ.

## Thiết lập

1. Tạo repo private, khuyến nghị `nlkcodenew/RomCloud-diagnostics`.
2. Tạo fine-grained PAT chỉ có quyền **Issues: Read and write** trên repo đó.
3. Cài Wrangler, đăng nhập và tạo KV:

   ```sh
   npm install --global wrangler
   wrangler login
   wrangler kv namespace create REPORTS
   ```

4. Copy `wrangler.toml.example` thành `wrangler.toml`, điền KV ID và repo.
5. Lưu token bằng secret rồi deploy:

   ```sh
   wrangler secret put GITHUB_TOKEN
   wrangler deploy
   ```

6. Điền URL HTTPS kết thúc bằng `/report` vào `config/reporting.json`, sau đó
   build lại release.

Worker lọc lại token, mật khẩu, IP nội bộ, MAC, serial, chip ID và machine-id;
giới hạn 10 request/10 phút theo IP băm và chống trùng report trong 30 ngày.
Không commit `wrangler.toml` hoặc bất kỳ token nào.

## Deployment hiện tại

- Worker: `https://romcloud-issue-relay.issue-relay.workers.dev/report`.
- Repo nhận log: `nlkcodenew/RomCloud-diagnostics` (private).
- E2E ngày 2026-09-30: tạo Issue `#1`; request lặp được KV nhận diện là trùng.
