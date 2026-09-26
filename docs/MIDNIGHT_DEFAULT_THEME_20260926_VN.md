# VQEAF Midnight: giao diện tối hiện đại mặc định (ESP32-S3, 26/09/2026)

## Mục tiêu và thiết kế

Built-in **VQEAF Midnight**: nền charcoal/navy `#0B0F18`, card `#151C2B`, hàng focus `#263652`, chữ `#ECF4FF`, chữ phụ `#9EADCA`, accent xanh-tím `#658CFF` và đường viền `#354664`. Toàn bộ palette được biên dịch thành 13 hằng RGB565; các dải wallpaper nhẹ được vẽ bằng primitive, không lưu ảnh màn hình hoặc tạo thêm framebuffer.

Font hệ thống mặc định riêng của giao diện Midnight là **bitmap DejaVu Sans Latin + Unicode tiếng Việt NFC** đã có sẵn trong dự án: cả ASCII lẫn ký tự Việt được đo/vẽ bằng **cùng** hai bộ glyph regular/bold `UiVietnameseFont.h`, thay cho cách trộn TFT_eSPI cổ điển (ASCII) với bitmap UTF-8 trước đây. Thêm `drawScaled()` integer 2× cho đồng hồ Home/Lock/Clock (không cần FreeType, TTF, SD, hoặc cấp phát trong lúc vẽ). Không phân phối tệp font. Các theme cũ vẫn dùng cách hiển thị chữ quen thuộc.

Lớp màn hình áp dụng cùng typography/palette: header và đồng hồ status, Home clock/status/shortcut/hint, grid caption, danh sách ứng dụng, thông báo, popup/menu, hộp thoại, khóa màn hình, Clock và transition mở ứng dụng. Font được đo theo độ rộng pixel, cắt theo nguyên codepoint UTF-8 khi cần.

## Tương thích và mặc định

- Đăng ký **ThemeId::ModernDark=5** thay vì thay đổi các mã đã lưu trong NVS: Classic=0, Black=1, Lime=2, AMOLED Red=3, External=4.
- Cài mới dùng Midnight mặc định ngay từ `SymbianUI` và `SystemSettings`, tránh nháy palette cũ trong boot.
- **NVS migration một lần `themeRev=4`**: chuyển các theme tích hợp của firmware trước thành Midnight để đáp ứng yêu cầu thay mặc định trên bo đã dùng; **theme nhập từ thẻ SD (External=4) vẫn giữ nguyên**. Có thể chọn lại theme cũ bất cứ lúc nào trong Themes; lựa chọn mới không tiếp tục bị ghi đè sau khi đã có revision 4.
- Midnight đứng đầu danh sách 5 theme built-in. Settings ←/→ quay vòng cả năm theme, **Reset appearance** trở về Midnight.
- `diag theme status` qua COM3 115200 in `id`, `name`, `modern_font`, `safe_mode`, không đọc thông tin cá nhân.

## Đo lường kiểm thử và nạp

`tools/test_midnight_theme.py` trên Windows: PASS các kiểm tra mã theme/NVS/typography, Unicode NFC tiếng Việt, wallpaper không cấp phát, vẽ đồng hồ nguyên tỷ lệ và telemetry. Tỷ lệ tương phản từ RGB565 theo công thức sRGB: body **17,99:1**, header **16,47:1**, secondary **8,69:1**, selected **11,48:1**, popup **15,91:1**. Kiểm thử icon đã ký `test_qeapp_icon_list.py` vẫn PASS. Build `pio run -e vqeaf_lua_uart` **SUCCESS**; vẫn dùng Lua 5.4.8 và public trust key trên máy người dùng (không commit lên GitHub).

Trước nạp, đã sao lưu độc lập app1 (2MiB) và otadata (8KiB) trên **Windows chỉ cục bộ** tại `local_hw_results/pre_midnight_*`, không upload bản sao flash hoặc private keys. SHA-256 app1 backup: `5306ca2ee58065f68d86d98248f9688c1103bf3ddf08e0c5cf5beebed22f6f3b`. SHA-256 otadata: `1948f69d226fea36612358041ed24eda23c2f013c0f9759f14ee8284eeeb1767`.

Đã **nạp** bản `vqeaf_lua_uart` mới **chỉ tại OTA app1 offset 0x650000** qua CH340 COM3, không chạm bootloader, phân vùng khác hoặc microSD. Esptool xác nhận hash ngay khi ghi. Sau đó đọc ngược đủ **1.703.824 byte** và đối chiếu từng byte với `firmware.bin`: **PASS**. SHA-256 bản build và readback đều `1d81f80f90bed765551599831a8bc264fa5832c984853c2ce016ace1d1cfa30f`; `OTADATA_UNCHANGED True`.

### Những bước nghiệm thu vẫn cần

**Đã đọc thành công UART sau flash**: `diag theme status` trả `id=4 name=SD card theme modern_font=0 safe_mode=1`. Điều này chứng minh firmware mới đang hoạt động và lệnh chẩn đoán mới được tích hợp, nhưng thiết bị **đang cố ý giữ theme SD cá nhân** từ NVS thay vì tự áp đặt Midnight; đó là hành vi migration đã chọn để không làm mất tùy chỉnh người dùng. Midnight đã là mặc định cho cài mới và cho các theme tích hợp cũ được nâng cấp, nhưng thiết bị cụ thể hiện cần người dùng **chọn chủ đề Midnight đầu tiên trong Themes** sau khi kiểm tra nguồn và thoát Safe Mode. Khi ấy `diag theme status` phải trả `id=5 name=VQEAF Midnight modern_font=1`. Một số lần mở COM3 ngay sau reset trả `WriteFile failed / Access denied` hoặc cổng mất tạm thời; lý do Brownout trước đây chưa được loại trừ. Chưa có ảnh LCD vật lý xác nhận pixel hoặc số liệu FPS/soak. Không tự tắt Safe Mode từ xa, không tự sửa/cắt xóa NVS để áp đặt theme trên cấu hình External của người dùng.

Lưu ý hình mockup Midnight (nếu được gửi riêng) chỉ là **minh họa thiết kế dựa trên palette** chứ không phải ảnh LCD thực.
