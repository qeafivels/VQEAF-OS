# VQEAF Sans / Midnight V2 — font và tương phản (26/09/2026)

## Thiết kế font hệ thống

Bộ **VQEAF Sans** là hai tập glyph bitmap dựng sẵn từ họ DejaVu Sans đã được dự án sử dụng và được tạo bằng `tools/generate_vietnamese_glyphs.py`. Không nhúng hoặc phân phối tệp TTF, không dùng bộ máy FreeType và không cấp phát trong lúc vẽ. Thay đổi V2:

| Vai trò | Kiểu chữ | Kích thước glyph | Vị trí |
|---|---|---:|---|
| Micro / metadata | Regular | 11px, vùng mực 18px | status, mô tả danh sách, chú thích |
| Caption | Regular | 11px | nhãn launcher và phím tắt |
| Body | Bold | 13px, vùng mực 18px | tiêu đề từng ứng dụng, softkeys |
| Title | Bold | 13px | header, popup, hộp thoại |
| Clock | Bold ×2 | 26px | Home, Lock và Clock |

Cả ASCII và UTF-8 tiếng Việt NFC của Midnight dùng chung một họ glyph, gồm **229 mã Unicode ở mỗi kiểu (458 glyph)**. Phiên bản mới dựng lại toàn bộ tập Regular ở cỡ 11px và baseline 14 từ DejaVu Sans, giữ Bold 13px và baseline 16. Các ký tự tiếng Việt có dấu được bảo vệ bằng vùng mực 18px; trong một hàng danh sách cao 42px, tiêu đề bắt đầu y+3, mô tả y+22, không cắt dấu hoặc vẽ đè lên hàng dưới. Chữ quá dài dùng `...` theo độ rộng pixel và chỉ cắt tại ranh giới codepoint UTF-8. Popup cũng giới hạn chuỗi theo chiều rộng thực tế.

Đường sinh glyph có thể cấu hình biến môi trường `VQEAF_FONT_DIR` và chuẩn Linux mặc định. Mã C++ chỉ có bảng hằng và vẽ các đoạn pixel, không chuyển TTF sang runtime. Theme cũ giữ font trước đó để bảo đảm tương thích.

## Midnight V2: các màu đã chỉnh

| Vai trò | Giá trị RGB565 | Màu gần đúng | Mục đích |
|---|---:|---|---|
| Nền | `0x0863` | `#0B0F18` | xanh đen không chói |
| Card | `0x10E5` | `#151C2B` | phân cấp surfaces |
| Đang chọn | `0x21AA` | `#263652` | nền tương tác |
| Chữ chính | `0xEFBF` | `#ECF4FF` | độ rõ cao |
| Chữ phụ | `0x9D79` | `#9EADCA` | dễ đọc, phân cấp |
| Accent V2 | `0x859F` | `#84B2FF` | focus sáng hơn trên cả nền selected |
| Viền | `0x322C` | `#354664` | viền ít nổi khi không focus |

Các phép tính tương phản dựa trên RGB565 đã chuyển về sRGB; không phải kết quả đo quang học trực tiếp từ panel ST7789: chữ chính/nền **17,99:1**, chữ phụ/nền card **7,65:1**, màu accent mới/nền đang chọn **5,80:1**, chữ chính/nền đang chọn **11,48:1**. Viền không được dùng làm kênh duy nhất để báo trạng thái; hàng được chọn vẫn đổi nền và viền accent.

Các popup và hộp thoại không còn đường viền xám sáng hardcoded ở Midnight; hàng/phím được chọn dùng focus accent thống nhất, thanh cuộn áp dụng màu viền nhẹ và thanh thumb xanh sáng. Không thay đổi palette/theme bên thứ ba.

## Kiểm thử / chấp nhận trên thiết bị

- `python tools/test_midnight_theme.py`: kiểm tra giới hạn font, cả 229 codepoint hai vai trò, vùng mực, NVS và độ tương phản.
- `python tools/test_qeapp_icon_list.py`: hồi quy biểu tượng ứng dụng đã ký.
- `python tools/test_lua_qeapp_uart.py`: luồng gói Lua không bị thay đổi.
- `pio run -e vqeaf_lua_uart`: bản firmware UART cho ESP32-S3 N16R8.
- Sau khi chọn Midnight: `diag theme status` phải báo `id=5 modern_font=1`. Theme External (`id=4`) trên thẻ SD được cố ý giữ nguyên; hãy đổi thủ công sang Midnight nếu thiết bị đang sử dụng theme riêng.
- Đối chiếu Home / Applications / Settings / Theme Manager / dialog trên ST7789 và ghi nhận lỗi ký tự dấu, cắt dòng, ghosting và FPS. Ảnh mockup chỉ để thẩm định thiết kế, không thay thế ảnh chụp LCD.

## Kết quả phần cứng và CI

- Regression Windows `test_midnight_theme.py`, `test_qeapp_icon_list.py`, `test_lua_qeapp_uart.py`: PASS. Build firmware UART Lua ESP32-S3: SUCCESS; RAM tĩnh 91.944 / 327.680 byte (**28,1%**), flash app 1.703.665 / 6.553.600 byte (**26,0%**).
- GitHub native CI commit `9873e1bd`: **SUCCESS** (các commit trung gian `22661cb` và `2ba875c` từng fail vì biểu thức chính quy Python sai ở bài test mới; đã sửa và chạy lại thành công).
- Đã sao lưu app1 2MiB và otadata 8KiB trước khi nạp; file riêng tư chỉ giữ tại máy người dùng trong `local_hw_results/`. SHA-256 bản sao app1: `2c023db0e5193f0be04427ab826ba3ae63f12c3d2051e37fcf628f1ebe917655`.
- Đã nạp firmware `vqeaf_lua_uart` mới vào đúng OTA app1 tại `0x650000` qua COM3; esptool báo `Hash of data verified`. Đọc ngược **1.704.032 byte**, **EXACT_FW_MATCH True**; SHA-256 build/readback `11cd55fc2f4a388d6a43c447b44aa7e009da179afd8b492b8f05d307af94f686`; **OTADATA_UNCHANGED True**. NVS/SD/bootloader không xóa hay format.
- Trên ESP32-S3 thật sau nạp, `diag lua probe`: **PASS** (`rect=3 text=1 peak_heap=12143`); `diag app icons`: **PASS** (`total=3 expected=2 ok=2 failed=0`).
- `diag theme status`: **`id=4 name=SD card theme modern_font=0 safe_mode=1`**. Thiết bị đang **cố ý giữ theme tùy chỉnh đã lưu trên SD** và còn Safe Mode, nên chưa thể nghiệm thu font/màu mới *trên LCD vật lý*. `diag lua status` vẫn ghi `reset=Brownout crash_streak=0 boot_healthy=1`. Chỉ sau khi ổn định nguồn và vào Normal Mode mới nên chọn Midnight trong Themes rồi chụp LCD thật và đo FPS. Hình preview chỉ là mô phỏng, không phải ảnh LCD.

