# VQEAF-OS v2.5.1 — biểu tượng thanh trạng thái 18 px

Màn hình ST7789 240 × 320, chiều cao thanh tiêu đề 27 px.

- WiFi: bốn cột có độ rộng 3 px, độ cao lần lượt 6/9/12/16 px. Nhóm được đặt trong ô 18 × 16 px, hiển thị mức tín hiệu từ RSSI thật khi kết nối.
- Pin: hình viền rộng 18 px, cao tối đa 14 px, được căn giữa theo vùng 18 × 16 px. Không tô mức sạc giả vì cấu hình phần cứng đã xác nhận chưa có ADC đo pin.
- Vị trí (gốc 0): WiFi x=194, y=5; pin x=217, y=5. Giữa hai ô cách 5 px, bên phải chừa 5 px; không lấn vùng đồng hồ giữa x=80–159 hoặc đường chia y=25–26.
- Khi RSSI đổi: chỉ xóa/vẽ lại vùng phải 80 × 25 px, không làm trống toàn màn hình, không cấp phát framebuffer bổ sung.
- Các trạng thái USB/microSD vẫn được xử lý bằng notification; không tự ý vẽ biểu tượng giả hoặc chiếm thêm ô trên thanh trạng thái.

## Kiểm tra

Chạy `python tools/test_statusbar_icons.py`. Bộ kiểm thử biên dịch trực tiếp `src/core/SymbianUI.cpp` với bộ đệm RGB565 mô phỏng, xác minh giới hạn hình học, thay đổi RSSI cao/thấp, không vẽ đè đồng hồ/vùng nội dung và không có `fillScreen` khi làm mới.

Ảnh trước khi nạp thiết bị: `preview/vqeaf_large_statusbar_comparison.png` (S60 Green/Night, sóng khỏe/yếu); dữ liệu lấy từ renderer C++ thực trên máy tính nhưng phông chữ mô phỏng, **không phải ảnh LCD thật**.

Bộ kiểm thử toàn OS có thêm gate `statusbar_18px_raster_geometry`; cần kiểm tra lại LCD vật lý trước khi merge/phát hành.
