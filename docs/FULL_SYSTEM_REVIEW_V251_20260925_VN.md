# Báo cáo kiểm thử toàn hệ điều hành VQEAF-OS v2.5.1 + Qeafbrowser
Ngày: 25/09/2026. Nhánh: `feat/system-vkeyboard-usb-sd-notices`.

## Phạm vi và giới hạn

Chạy source C++ giao diện thực của OS với TFT RGB565 framebuffer shim trên Linux (khác màn LCD thật và font TFT thật); chạy regression C++/Python dịch vụ và build PlatformIO N16R8 stock + Lua experimental.

**Không có bo ESP32-S3 vật lý kết nối:** không tuyên bố đo FPS/SPI, thực nghiệm cắm/rút USB Type-C chỉ sạc, rút thẻ SD đang cấp nguồn, hoặc tải website qua WiFi thật. Các ảnh kèm CI là raster sinh từ source C++, không phải ảnh chụp màn hình LCD hay chụp website đã tải thành công.

## Kết quả host regression

Lần chạy `36146138973`, 13/13 host gates PASS:
1. Compile production SymbianUI + TextKeyboard trong host C++11.
2. Render RGB565 240×320: Home/Menu, Browser URL keyboard, D-pad focus, WiFi password và Notifications.
3. Kiểm palette, icon, layout, glyph từ source production.
4. Mô phỏng phím QWERTY/TLD/ẩn mật khẩu/T9/cancel/maxlength bằng TextKeyboard.cpp thật.
5. USB attach/detach debounce và mô hình thông báo microSD.
6. Route Qeafbrowser nội bộ/Bookmark và tích hợp OS.
7. Quy tắc phím Back.
8. Lua frame/no-blank.
9. Lua transition/no-blank.
10. TLS đã xác minh và lưu trữ.
11. Kiểm tra độ ổn định/bộ nhớ/SD bằng host mocks.
12. Installer/reset regression.
13. Full v2.5.1 acceptance (--full, gồm app manager/firmware host link).

Cả `pio run -e vqeaf_os` và `pio run -e vqeaf_lua_beta` đều build thành công trong lần chạy này (Lua dùng CI ephemeral public trust; đừng dùng beta CI artifact trên thiết bị có app đã ký bằng khoá người dùng).

CI run đó bị đánh FAIL ở bước cuối **chỉ vì artifact uploader bỏ qua thư mục ẩn `.pio`**. Đã sửa bằng cách xuất đúng `.pio/build/vqeaf_os/firmware.bin` ra `ci_output/VQEAF_OS_v251_default.bin`, chỉ upload production firmware, không publish key CI hoặc beta CI. Workflow được chạy lại.

## Sửa lỗi

- Bàn phím: di chuyển D-pad chỉ tô hai ô cũ/mới thay vì tẩy toàn màn hình; khi SHIFT/SYM đổi layout, tẩy các viền ô cũ vì số cột/alignment khác nhau.
- Browser: mở ô nhập URL từ Speed Dial sẽ không chứa `mtt:start`.
- USB: giữ debounce và phân biệt USB data host với nguồn VBUS; microSD không remount giữa lúc audio đang giữ file.
- Host fixtures: cập nhật tham chiếu API TFT byte order/getSwapBytes trong mock; đồng bộ legacy Music/Gallery checks với Back-modal safety; test version đọc BuildVersion.h thay vì literal cũ trong main.cpp.
- Tạo review screenshot gồm 6 màn 240×320 từ sản phẩm C++ thật và báo cáo JSON từng gate.

## Dữ liệu tải về

GitHub Actions workflow: `full-system-review-ci.yml`.
Ảnh: `preview/vqeaf_full_system_host_capture.png`; ảnh đơn: `v23_home_host_raster.png`, `v23_menu_host_raster.png`, `browser_keyboard.png`, `browser_keyboard_focus.png`, `wifi_keyboard_masked.png`, `system_notifications.png`.
Báo cáo: `build_reports/system_full/report.json` trong workflow artifact.

## Gate phần cứng trước khi merge/release

Trên ESP32-S3 WROOM1 N16R8, Serial 115200:
- Boot/reboot 30 lần; quan sát watchdog/brownout và heap/PSRAM/free stack.
- WiFi thật + HTTPS chứng thực CA + Bookmark quay về browser.
- Quay 60fps LCD để kiểm việc repaint D-pad/SHIFT, chuyển cảnh và font.
- Type-C data host có nguồn độc lập (để nhận notification khi rút); không khẳng định phát hiện dây sạc nếu chưa có VBUS-sense.
- SD idle rút/lắp 10 chu kỳ; thử lỗi installer QEAPP/theme, đảm bảo không rút trong lúc ghi.
- Đo FPS, p95 latency, peak RAM/PSRAM bằng firmware perf_diag và Serial. Chưa có số phần cứng.
