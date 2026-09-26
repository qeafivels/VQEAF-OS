# Kiểm thử phím điều khiển và giảm nhấp nháy — VQEAF-OS, 26/09/2026

## Lỗi xác nhận từ mã nguồn

1. **Themes / START và OPTION → Apply:** hàm `ThemesApp::apply()` xóa nguyên màn hình ST7789 trước khi hàm gọi vẽ lại trang. Đã loại bỏ cả hai lần clear cho theme built-in và theme SD: caller chịu trách nhiệm repaint một lần.
2. **Popup / D-pad UP/DOWN:** `SymbianUI::popupMenu()` từng đổ kín và vẽ lại popup 214px rộng sau mỗi lần đổi một hàng focus; có thể tạo nhấp nháy trên SPI LCD. Thêm cache giới hạn trên mỗi instance. Khi offset và menu không đổi, chỉ vẽ lại **hàng focus cũ + mới**; chỉ vẽ toàn bộ khi mới mở/chuyển trang/cuộn offset. Invalidate cache khi nội dung, lưới, footer hoặc chrome thay đổi.
3. **Midnight / START/Back screen transitions:** bỏ hiệu ứng sọc vẽ rồi xóa 5 lần trên SPI khi giao diện Midnight hoạt động. Theme cũ không bị thay đổi.
4. **Phím vật lý:** bổ sung lệnh COM3 đọc GPIO `diag keys` cho **MENU/UP/A/LEFT/START/RIGHT/OPTION/DOWN/B/SELECT**; chỉ đọc trạng thái, không bấm giả, không ghi cấu hình. Công cụ `tools/keypad_uart_lab.py` hướng dẫn người dùng nhấn và thả lần lượt 10 phím trên phần cứng để xác nhận tín hiệu không nhiễu hoặc kẹt.

## Kiểm thử đã thực hiện

- `tools/test_navigation_flicker.py`: PASS toàn bộ điều kiện liên quan đến focus redraw, popup scrolling, key-modal routing, cache invalidation, theme apply và tránh blank Lua compositor.
- `tools/keypad_uart_lab.py --selftest`: PASS bộ giải mã trạng thái của cả mười nút, gồm zero/release.
- `tools/test_midnight_theme.py`, `tools/test_qeapp_icon_list.py`, `tools/test_lua_qeapp_uart.py`: PASS. `test_grid_nav.py`: PASS.
- Một số bộ kiểm thử giả lập C++ tích hợp bàn phím khác chưa chạy trên Windows vì không có `g++` trong PATH. Workflow Linux native cho mã firmware tại commit `7a4ac224`: **SUCCESS**, nhưng đó không thay thế thao tác bấm thật trên thiết bị.
- Biên dịch PlatformIO `vqeaf_lua_uart`: **SUCCESS**. RAM tĩnh **91.968 B / 327.680 B (28,1%)**, flash ứng dụng **1.704.273 B / 6.553.600 B (~26,0%)**.

## Nạp bo ESP32-S3 và rollback

- Trước khi nạp, sao lưu độc lập OTA app1 2 MiB và otadata 8 KiB trên máy Windows dưới `local_hw_results/pre_navfix_*_PRIVATE.bin`. Không upload hay commit các bản sao chứa dữ liệu riêng.
- Nạp đúng OTA app1 ở `0x650000` bằng esptool qua CH340 COM3, firmware **1.704.640 byte**, công cụ in `Hash of data verified`.
- Đọc ngược đúng **1.704.640 byte**: `EXACT_FW_MATCH True`; SHA256 ảnh firmware và flash **`756230e1bb6d2b2588e98815bfe7783a734a5c74f89b9332b10d22dfdc9b1e1f`**; `OTADATA_UNCHANGED True`. Không format NVS hoặc SD; không ghi bootloader/bảng phân vùng.

## Nghiệm thu thực tế chưa hoàn tất

Sau lệnh reset của esptool, CH340 COM3 có lúc **biến mất khỏi danh sách cổng** rồi lần truy vấn tiếp theo trả `GetOverlappedResult failed / Access denied`. Do đó **chưa thu được kết quả `diag keys` thực từ firmware mới và chưa bấm thử đủ 10 nút trên bo hoặc có ảnh quay LCD để xác minh hết nhấp nháy**. Không được suy diễn PASS vật lý chỉ từ kiểm thử mã nguồn hoặc hash flash.

Khi cổng COM3 ổn định và người dùng ở cạnh bo:

1. Mở `python -m serial.tools.miniterm COM3 115200`, nhập `diag keys`. Khi thả hết nút kỳ vọng `pressed_mask=0x000`; khi giữ riêng một nút, kỳ vọng đúng một bit (MENU=001, UP=002, A=004, LEFT=008, START=010, RIGHT=020, OPTION=040, DOWN=080, B=100, SELECT=200). Không gửi lệnh khi miniterm và script đang tranh chấp COM3.
2. Đóng miniterm rồi chạy `python tools/keypad_uart_lab.py --port COM3`. Tool yêu cầu người dùng giữ lần lượt các phím và báo PASS/FAIL nhấn và thả; không mô phỏng nút nào.
3. Test bằng mắt Home, Launcher, Themes, Apps Inbox/Installed, Settings, WiFi và popup: D-pad UP/DOWN/LEFT/RIGHT, START, A/B, OPTION, MENU ngắn/dài, SELECT dài. Theo dõi nền sáng/chớp, hình focus, thẻ icon và log panic/brownout. Nếu COM3 mất ngay sau reset, kiểm tra cáp/nguồn/driver CH340 và không kết luận là lỗi UI.
4. `diag theme status` **đã xác nhận** Midnight `id=5 modern_font=1`; riêng `safe_mode=1` vẫn cần kiểm tra nguồn điện và xử lý Recovery trước khi thử giao diện tương tác. `reset=Brownout` biểu thị lần reset gần nhất, không chứng minh sụt áp đang diễn ra.

Đây là sửa các đường dựng hình được chứng minh ở mã nguồn + kiểm thử tự động + xác minh flash, **không phải báo cáo đã kiểm thử hoàn toàn phím vật lý/FPS màn hình ST7789**.
