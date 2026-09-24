# SKILLS.md — Kỹ năng và checklist phát triển QEAPP Studio

Mọi agent bắt đầu bằng `PROMPT.md` và file này; kích hoạt thêm mục kỹ năng phù hợp. Nguồn chuẩn: **code hiện tại** của firmware có ưu tiên cao hơn draft API trong bộ tài liệu.

## Skill 1 — Project scaffolding

**Trigger:** New Project, template, app/game mới. **Input:** project ID, loại, phiên bản, target firmware, quyền dự kiến. **Các bước:**

1. Chọn mode: `CURRENT_QEAPP2`, `BUILTIN_NATIVE_HANDLER` hoặc `PROPOSED_SCRIPT_RUNTIME`.
2. Hiện tại dùng `qeapp.project.json` của Studio làm metadata *trên PC*, KHÔNG ghi JSON này vào QEAPP/2: signer chuyển sang ASCII manifest whitelist.
3. Tạo `assets/`, `src/` hoặc `content.txt`, `tests/`, `README.md`, `CHANGELOG.md`; giữ `dist/` và khóa bí mật ngoài Git.
4. Chạy `python tools/qstudio.py validate <project>` và test dự án. Mẫu `snake-lua-proposal` luôn báo chưa có runtime.

**PASS:** ID, tên, version đúng hợp đồng; file asset có thực; không có symlink/path traversal; phân biệt mode rõ ràng.

## Skill 2 — Signed QEAPP build & install

**Trigger:** build, export, ký, cài, update. **Các bước:**

1. Đọc format thực trong `docs/QEAPP_V15_SIGNING.md` và script gốc; `pip install cryptography Pillow` trên PC.
2. Tạo private key P-256 **bên ngoài** repo, ghi public key và key-id phù hợp vào firmware bằng `tools/qeapp_keys.py` rồi rebuild/reflash firmware. Backup key riêng.
3. Gọi `qstudio.py build ... --sign-key PATH --firmware-root PATH`; verify `qstudio.py inspect --public-key ... --key-id ...`.
4. Test sửa 1 byte → chữ ký không hợp lệ; test downgrade, same-version, mất SD và backup recovery. Copy gói tới `/System/Apps/Inbox/`, mở App Installer và xác nhận.

**FAIL bắt buộc:** khác key-id, private/public mismatch, manifest có key ngoài whitelist, payload >256KiB, icon không đúng 32x32, unsigned QEAPP/1. **Không bypass verifier** để "sửa" lỗi cài đặt.

## Skill 3 — Game loop, UI, phím cứng

**Trigger:** game 2D, app có tương tác, màn hình 240x320. Lưu API giả lập host và adapter ESP32 trong lớp platform riêng. Dự kiến hàm `init`, `update(dt_ms)`, `render`, `key`, `pause`, `resume`, `shutdown`. Giới hạn fixed-step, không block input, clamp dt. Đặt softkey/statusbar dưới quyền OS; test MENU/Home, A/Back, START/OK, SELECT long-press >600ms và D-Pad repeat. Render RGB565 và kiểm tra clipping tại biên 0..239, 0..319.

**PASS:** 240x320 golden image, 60s stress test trên host, input determinism, không mất phím/đóng băng launcher; thiết bị thật là gate riêng.

## Skill 4 — Asset pipeline & bộ nhớ

**Trigger:** spritesheet, fonts, tilemap, âm thanh. Dùng pipeline offline PNG → RGB565 hoặc palette/RLE theo asset, không decode ảnh lớn vào internal SRAM. Đo dung lượng `.qeapp` và bitmap giải nén; xếp tài nguyên trên SD theo thư mục sandbox tương lai. Đừng tự nới payload firmware mà không đổi format/version và test toàn bộ parser.

**PASS:** lossless RGB565 round-trip tại target, palette range/chunk bounds, không OOM trong test lâu; số đo trên PC không phải số đo ESP32.

## Skill 5 — Runtime sandbox (chỉ roadmap)

**Trigger:** yêu cầu "biên dịch game/app Lua thành `.qeapp` chạy độc lập". Tạo host interpreter + platform adapter + capability broker, không sửa manifest sang `type=lua` trên QEAPP/2. Thiết kế hợp đồng ABI mới/versioned, kiểm tra bytecode/source khi load, hạn mức instruction/memory, thư mục data theo ID và không dùng `loadfile/dofile/require` tự do từ SD.

**PASS:** host unit tests, fuzz malformed bytecode/resources, watchdog, host screenshots; **chưa PASS phát hành** cho đến khi firmware parser/loader/runtime/installer có E2E trên thiết bị thật.

## Skill 6 — IDE + debugger

**Trigger:** UI IDE tương tự LuaS30-IDE. Qt/PySide6 trên PC chỉ là giao diện; gọi cùng `qstudio` CLI cho Build/Inspect/Test, không giữ private key trong workspace JSON. Có Explorer, editor, asset preview, Build log, Error diagnostics, simulator controls, screenshots và device Serial Monitor. **Stop** phải hủy process thử nghiệm; các command đầy quyền cần xác nhận.

**PASS:** workflow tạo `text-app` → validate → signed build → verify, với mock signing key từ test tạm; nút Build trả đúng CLI exit code; key không bị ghi log.

## Skill 7 — Regression và báo cáo

- Bộ host: parser, hashes, P-256, icon/endian, malicious payload, signature mismatch, power-loss stage recovery, file sizes, game loop, render golden, resource leak; suite `verify_v242.py` trong firmware.
- Bộ ESP32 (cần board): cross-build PlatformIO, 115200 serial, SD FAT mounting, WiFi/NTP/TLS, install/update/reboot, theme interaction, display 240x320, D-Pad, 10-minute runtime soak, FPS/heap low-water, screenshots chụp từ máy hoặc camera.
- Báo cáo bắt buộc ghi `host simulated` hay `hardware verified`, SHA artifact, commit SHA, trust key-id (không có private key), tool versions và giới hạn.