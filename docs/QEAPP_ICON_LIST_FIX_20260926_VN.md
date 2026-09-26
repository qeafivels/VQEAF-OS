# Sửa biểu tượng danh sách QEAPP trên ESP32-S3 — 26/09/2026

## Nguyên nhân và phạm vi sửa

Màn **App Installer → Inbox** trước đây luôn đặt `inboxLabel.manifestOk=false` khi tải lại và vẽ glyph chung `File`/`App` cho **mọi** gói `.qeapp` trong danh sách. Biểu tượng RGB565 của gói chỉ được giải mã tại trang Details; do đó người dùng không thấy ảnh ứng dụng trong danh sách dù gói đã có icon.

- Dùng **6 slot** 32×32 RGB565 (~12 KiB PSRAM) cho 6 hàng đang hiển thị, không đưa bitmap lên Arduino loopTask stack.
- Mỗi vòng xử lý **tối đa một** gói trong Inbox, cách nhau ít nhất 100 ms; gọi `inspectWithIcon()` đã có để xác thực đầy đủ chữ ký QEAPP/2, manifest, icon và payload trước khi hiển thị thumbnail.
- Sau xác thực, hiển thị tên/phiên bản và icon cho đúng hàng, không vẽ lại cả trang; khi cuộn hoặc reload/nhận thẻ SD mới sẽ làm mới trạng thái cache để không hiển thị icon sai hàng.
- Các gói không có icon, hỏng hoặc chữ ký không hợp lệ vẫn hiển thị glyph an toàn; không bỏ qua chữ ký và không sửa file trên microSD.
- Danh sách **Installed apps** và **Applications** đã có đường render `loadIcon()` kiểm tra SHA-256 với digest nhận từ receipt đã xác minh, được giữ nguyên. Bổ sung lệnh **`diag app icons`** chỉ đọc để kiểm tra khả năng đọc ảnh từ SD cho từng mục mà không tiết lộ tên/đường dẫn ứng dụng.

## Build, thử nghiệm, thiết bị thật

- Kiểm thử mới `python tools/test_qeapp_icon_list.py`: **PASS** 13 quy tắc cấu trúc/lazy-cache/chữ ký; kiểm tra icon 32×32 và hash phần icon trong một tệp `.qeapp` thực tế trên máy tính: **PASS**. `test_browser_feature_parity.py` và `test_lua_qeapp_uart.py` **PASS**.
- Bộ regression C++ cần `g++` không có trên Windows PATH; workflow GitHub Linux **VQEAF native Qeafbrowser ESP32-S3** tại commit `4803f51d` đã **SUCCESS**.
- Build địa phương `pio run -e vqeaf_lua_uart`: **SUCCESS**; RAM tĩnh **91.944/327.680 B (28,1%)**, flash ứng dụng **1.700.441/6.553.600 B (25,9%)**.
- Trước khi nạp đã sao lưu app1 hiện hành (2 MiB) và otadata (8 KiB) dưới `local_hw_results/` trên máy Windows. SHA-256 app1 trước sửa: `b8476bc4246ed49cef1114eb89657980b181b331a6cc13d14ce1a797c2dbe05a`; otadata trước sửa: `1948f69d226fea36612358041ed24eda23c2f013c0f9759f14ee8284eeeb1767`. Không commit backup vì có thể chứa dữ liệu riêng tư.
- Đã nạp **chỉ OTA app1 tại 0x650000** qua CH340 COM3 (lần đầu COM3 báo Access denied sau reset; nạp lại thành công ở baud 115200); esptool `Hash of data verified`. Firmware mới **1.700.800 B**.
- Đọc ngược 1.700.800 B từ flash: **READBACK_EXACT_MATCH True**; SHA-256 ảnh build và flash đều **`a41ecda7fd448b44db61d19e2b38e6ddf0c3c1dc7ed7bbfb2ef2045fa6066e98`**; **OTADATA_UNCHANGED True**. Không xóa/format NVS, SD, bootloader, bảng phân vùng.
- Lệnh UART thật `diag app icons`: mục 0 `signed_icon=0` (ứng dụng được cài **không kèm icon**), mục 1 và 2 `signed_icon=1 verified_pixels=1`. Tổng **`result=PASS total=3 expected=2 ok=2 failed=0`**: cả **hai** bitmap có ký số đều được đọc và xác minh hợp lệ trên bo; mục không mang icon chỉ có fallback. Nếu muốn icon riêng cho mục 0 cần build lại gói với asset hình từ QEAPP-Studio.
- `diag lua probe` trên bo: **PASS rect=3 text=1 peak_heap=12563**.
- Sau khi nạp, `diag lua status` vẫn cho **`safe_mode=1 screen=Recovery crash_streak=0 boot_healthy=1 sd=1 reset=Brownout`**. Đây là vấn đề nguồn/Recovery đã có trước thay đổi icon; hệ điều hành chưa đủ điều kiện để nghiệm thu **ảnh trực quan thực tế** của danh sách khi vào Normal Mode. Không xóa Safe Mode từ xa; kiểm tra nguồn và chọn **Start normal mode** trên thiết bị khi ổn định.

## Giới hạn nghiệm thu

Đã chứng minh đường dẫn dữ liệu (hai icon tùy biến được xác minh trên bo), bản build và nạp/readback thành công; **chưa có ảnh chụp màn hình vật lý** sau khi người dùng thoát Recovery nên không tuyên bố đã quan sát thành công pixel hiển thị trên ST7789. Không gộp PR #10 khi chưa có kiểm thử nguồn và giao diện thực.
