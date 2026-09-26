# Qeafbrowser standalone -> VQEAF-OS: cổng tương thích có kiểm soát

Nguồn: `nectvety-software/legacy-32-classic-E524546/projects/Qeafbrowser_v1.7` (README của thư mục hiện ghi tới v2.2).
Đích: VQEAF-OS v2.5.1 / Back r2. Nhánh thử nghiệm: `feat/qeafbrowser-parity-20260926`.

## Nguyên tắc không làm mất chức năng

- Bảo tồn nguồn standalone và mọi tính năng có sẵn của OS. Không thay `main.cpp`, GPIO, driver TFT_eSPI, điểm mount SD hay bộ phím của hệ điều hành bằng phiên bản LovyanGFX standalone.
- Giữ nguyên HTTP/HTTPS xác minh chứng chỉ qua `TrustedTls`, chống HTTPS downgrade, `StorageService::writeAtomic`, đường đi tải tệp tới trình cài ứng dụng có chữ ký và quản lý theme. Không nhập đường `setInsecure()` từ standalone.
- Không dùng full-screen framebuffer thứ hai để hỗ trợ chuyển cảnh. Hồ sơ RAM/PSRAM cần đo trên bo thật trước khi tăng giới hạn vùng hiển thị.
- Tránh giả định các tính năng được tích hợp chỉ vì có trong repository nguồn.

## Ma trận tương thích / nhiệm vụ còn lại

| Tính năng | Standalone | VQEAF-OS trên nhánh này | Cần kiểm thử/port tiếp |
|---|---|---|---|
| Speed Dial, History, Bookmark, Help, About | Có | Có native; bookmark lưu atomic qua SD | Kiểm thử hồi phục SD/reboot |
| URL, redirect tương đối, HTTP/HTTPS | Có | Có; HTTPS dùng CA đáng tin cậy | TLS thực trên bo và redirect bất thường |
| Quay lại / tiến tới | Có | **Back + Forward 12 URL**, cấp PSRAM một lần; không đổi stack khi tải lỗi | Kiểm thử web có redirect và offline |
| HTML/WML, liên kết và focus keypad | Parser lớn hơn | Parser đơn giản native; WML anchor/go | Port từng phần WML/card và test fixture |
| Overview tile, zoom x1..x8, minimap | Có | Đã có thử nghiệm lưới preview 3×3, zoom x1..x8, con trỏ/crosshair, thanh cuộn; renderer native OS | Kiểm tra fidelity ảnh/tile với standalone và tương tác phím thật |
| JPEG/PNG thumbnail + LRU PSRAM/LittleFS CRC | Có | Có thử nghiệm JPEG/PNG HTTPS vào 3 tile PSRAM RGB565 64×48; cache LittleFS CRC32, SD fallback khi LittleFS không mount | Thử ảnh thật, trường hợp decode lỗi, 96 KiB/ảnh, cache 24 tệp, gỡ SD |
| Pixel scroll và quán tính D-pad | Có | Đã nối cuộn theo pixel Q8/quán tính D-pad, hạn chế repaint đến 30 Hz; không thêm framebuffer | Đo FPS và độ trễ phím Serial 115200 trên thiết bị |
| Cookie phiên và history/bookmark bền | Có | Bookmark bền; history phiên; cookie host-only/path/secure giới hạn 12, ghi atomic lên microSD | Chưa đầy đủ RFC6265, expiry/PSL/SameSite; bảo vệ dữ liệu nhạy cảm và nghiệm thu khôi phục SD |
| Bàn phím/nguồn WiFi | Có | Dùng bộ nhập liệu và quản lý WiFi OS | Test phím thật 240x320 |
| Giao diện S60 đậm/pixel-perfect | Có | Dùng theme/chrome OS thay vì LovyanGFX | Mẫu giao diện phù hợp OS |
| Download | Có | Có, HTTPS xác minh và route installer/theme OS | SD tháo nóng, lỗi nguồn/timeout |
| NTP clock | Có | Dùng status/time service OS | Test NTP sau mất mạng |

## Ngưỡng chấp nhận

1. `python tools/test_qeafbrowser_os.py`: forward/back stack, URL/TLS, bookmark, installer routing. Chạy thêm `python tools/test_browser_feature_parity.py` và `g++ -std=c++11 -O2 -Wall -Wextra -Werror tools/browser_parity_host.cpp -o qb-test && ./qb-test` trên Linux/CI để kiểm tra motion, giới hạn zoom, cookie và CRC cache.
2. `python tools/test_lua_transition_noblank.py` và `python tools/test_backguard_v251.py`: không hồi quy Back r2/Lua.
3. `pio run -e vqeaf_os` và `pio run -e vqeaf_lua_beta`: cả hai profile biên dịch.
4. Hardware ESP32-S3: duyệt 20 trang, 100 lượt Back/Forward, 30 phút D-pad scroll, mở/thoát liên tục, không watchdog, không giảm heap liên tục; chụp LCD và log Serial 115200; kiểm tra SD rút/lắp và TLS sai hostname.

**Lưu ý:** Đây là triển khai thử nghiệm, không phải sao chép pixel-perfect firmware standalone. Unit tests kiểm chứng logic motion/cookie/CRC; gate Python kiểm tra cấu trúc. Giải mã JPEG/PNG, HTTPS, tương tác LCD và FPS chưa được xác nhận trên thiết bị thật. Cookie lưu trên microSD là dữ liệu plaintext; không dùng đăng nhập nhạy cảm trước khi có audit và chính sách mã hóa/xóa dữ liệu. Chưa hỗ trợ hoàn chỉnh expires, public-suffix, SameSite, cookie nhiều header Set-Cookie trên cùng phản hồi hoặc HTML/WML 400 dòng như bản nguồn. Không phát hành firmware production trước khi nghiệm thu phần cứng và Lua beta.
## Kiểm thử hồi quy theo tính năng mới

| Thành phần | Bài kiểm thử đã viết | Phạm vi chưa chứng minh |
|---|---|---|
| Overview / zoom | Host unit của `BrowserMotion` và source gate lựa chọn menu, grid 3×3, x1..x8 | Pixel-perfect/độ mượt màn hình thật |
| Cuộn pixel/quán tính | Host unit clamp, mục tiêu và settling, gate tick 30 Hz | Độ trễ phím, FPS trung bình/p95 LCD |
| JPEG/PNG thumbnail | Source gate URL https/image-alt, callback decoder JPEG + PNG | Decode nội dung thực/tính đúng màu trên ST7789 |
| PSRAM + LittleFS / SD | Host unit record FNV-64/CRC32 kích thước/tamper, source gate 3 tile và quota | LRU, lỗi nguồn, mount LittleFS cũ, rút thẻ SD thực |
| Cookie | Host unit domain/path/HTTPS/Max-Age=0 và round trip | Nhiều Set-Cookie header, browser-grade RFC6265/Expires/SameSite |

Lệnh chẩn đoán thiết bị: `pio run -e vqeaf_perf_diag -t upload`, `pio device monitor -b 115200` và đọc log `[QB][PERF]`. Giữ lại bản sao firmware đang dùng; không nạp CI Lua beta với khóa thử nghiệm lên thiết bị có ứng dụng đã ký.

## Kết quả thử nghiệm trên ESP32-S3 thật (26/09/2026)

Bo mạch ESP32-S3 N16R8 nối qua CH340 COM3/115200. Kiểm tra `esptool flash_id` xác nhận ESP32-S3, flash 16 MB; test chỉ ghi phân vùng app1 đang hoạt động 0x650000, không ghi NVS/partition table/bootloader. Đã sao lưu đầy đủ **16 MiB flash gốc** và phân vùng app1 vào máy Windows cục bộ, KHÔNG đưa bản sao chứa dữ liệu người dùng lên GitHub. Chẩn đoán UART dùng cấu hình thử `vqeaf_perf_diag_uart`, production vẫn giữ native USB CDC.

| Phép đo/kiểm tra | Kết quả thực tế và giới hạn |
|---|---|
| LCD renderer Overview | 64 lần vẽ thật lên ST7789, zoom x1..x8, dữ liệu trang 72 dòng và đầu vào chuyển động **giả lập trong firmware**; render trung bình **52,394 µs/frame**, p95 **53,099 µs**, cao nhất **59,652 µs**; thông lượng liên tiếp đo được **19 FPS**. Đây là tốc độ vẽ thực tế, **không phải** FPS trình duyệt khi người dùng cuộn trang thực hay giới hạn 30 Hz. |
| Độ trễ đầu vào | `input_events=0` trong các cửa sổ ghi log; **chưa thể đo** phím vật lý đến hiển thị hoặc input-dispatch p95. Thời gian xử lý điều hướng giả lập/OS khi chạy bench ghi ~117–118 ms, KHÔNG thay thế độ trễ phím. |
| SD trước kiểm thử | `diag sd rw`: **PASS**, ghi/đọc 4096 byte, kiểm CRC32; xóa scratch thành công. |
| Cookie/cache sau reset thật | `diag qb stage`: **PASS** cho HTML và cookie thử nghiệm, thumbnail SD_FALLBACK_PASS. Sau lệnh `diag qb reboot` (ESP.restart), `diag qb verify`: **PASS** HTML, cookie, CRC + pixel thumbnail; `boot_uptime_ms=4248` (chứng cứ cold boot). Đã gọi `diag qb cleanup` và nhận `cleanup=COMPLETE`. |
| LittleFS | Mount LittleFS trên bo báo **corrupted dir pair**, không format hoặc thay đổi phân vùng cũ; tự chuyển sang cache thumbnail SD. Kiểm chứng cold reboot hiện áp dụng **SD fallback**, chưa chứng minh LittleFS tier trên chính bo này. |
| Dữ liệu người dùng | Fixture dùng cookie `qb_probe=synthetic` và URL `.invalid`; không chạm file cookie/bookmark người dùng, không truy cập website thật. |

**Diễn giải:** thông lượng tile native ~19 FPS thấp hơn hạn mức vẽ lại 30 Hz, nên giới hạn hiện thời đến từ thời gian vẽ nhiều khung preview lên SPI LCD chứ không nhất thiết từ vòng lặp tính toán Q8. Cần đo riêng khả năng tái sử dụng tile/partial-redraw nếu muốn 30 FPS. Dữ liệu cookie thực nhạy cảm chỉ có thể nghiệm thu sau security review (plaintext SD hiện chưa phù hợp đăng nhập nhạy cảm).

**Trạng thái trả lại thiết bị (đã xác minh):** đã dọn fixture bằng `cleanup=COMPLETE`, nạp lại toàn bộ 1.581.056 byte app1 từ **bản sao firmware gốc**, đọc lại app1 qua esptool và so SHA-256; **khớp chính xác** `2c850ecc76fe3571b56dafc513ce420da0f7fef9afdbacf9f2d3d7aebf5b70bc`. ESP32-S3 khởi động lại firmware gốc, log cho thấy SD layout hợp lệ và theme `Blue S60` được khôi phục. Lần quan sát khởi động gốc thấy `safe_mode=1`/Browser UNAVAILABLE (có thể do trạng thái phím lúc khởi động); cần xác nhận bằng cách khởi động bình thường không giữ phím recovery. Sao lưu 16 MiB và raw serial được giữ **chỉ trên máy Windows**, không đưa lên GitHub. Không gộp PR trước khi người dùng xác nhận test phím thực và audit cookie.
