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
| Overview tile, zoom x1..x8, minimap | Có | Chưa tương đương | Port renderer theo VQEAF UI, không import LovyanGFX |
| JPEG/PNG thumbnail + LRU PSRAM/LittleFS CRC | Có | Chưa có trong nội dung Browser native | Tái dùng decoder OS, giới hạn budget/cache |
| Pixel scroll và quán tính D-pad | Có | Hiện tại scroll theo dòng | Cổng fixed-point và kiểm tra FPS |
| Cookie phiên và history/bookmark bền | Có | Bookmark bền; history phiên; chưa port cookie | Thêm store scoped theo domain |
| Bàn phím/nguồn WiFi | Có | Dùng bộ nhập liệu và quản lý WiFi OS | Test phím thật 240x320 |
| Giao diện S60 đậm/pixel-perfect | Có | Dùng theme/chrome OS thay vì LovyanGFX | Mẫu giao diện phù hợp OS |
| Download | Có | Có, HTTPS xác minh và route installer/theme OS | SD tháo nóng, lỗi nguồn/timeout |
| NTP clock | Có | Dùng status/time service OS | Test NTP sau mất mạng |

## Ngưỡng chấp nhận

1. `python tools/test_qeafbrowser_os.py`: forward/back stack, URL/TLS, bookmark, installer routing.
2. `python tools/test_lua_transition_noblank.py` và `python tools/test_backguard_v251.py`: không hồi quy Back r2/Lua.
3. `pio run -e vqeaf_os` và `pio run -e vqeaf_lua_beta`: cả hai profile biên dịch.
4. Hardware ESP32-S3: duyệt 20 trang, 100 lượt Back/Forward, 30 phút D-pad scroll, mở/thoát liên tục, không watchdog, không giảm heap liên tục; chụp LCD và log Serial 115200; kiểm tra SD rút/lắp và TLS sai hostname.

**Lưu ý:** Các gate Python hiện là kiểm tra cấu trúc, không thay thế simulator và kiểm chứng HTTP/ảnh/TFT thực. Không phát hành firmware production trước khi chạy đầy đủ kiểm thử phần cứng.