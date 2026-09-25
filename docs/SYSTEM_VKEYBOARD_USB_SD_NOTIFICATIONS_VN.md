# VQEAF-OS: Bàn phím ảo toàn hệ thống và thông báo kết nối

## Bàn phím ảo

`TextKeyboard` mặc định của OS được thay bằng bố cục Qeafbrowser 4 hàng QWERTY, có hàng số, phím SHIFT, SYM, SPACE, DEL, DONE và ba phím nhanh `.com/.net/.org` (ẩn ở chế độ mật khẩu). WiFi, trình duyệt, ghi chú, tìm kiếm và các màn hình gọi `ctx.keyboard` được sử dụng chung giao diện, không sao chép trình nhập liệu vào từng ứng dụng.

D-pad di chuyển, START chọn ký tự, OPTION đổi hoa/thường, SELECT đổi ký tự, B xoá, MENU hoàn tất, A huỷ; T9 bàn phím số tiếp tục hoạt động. Độ dài nhập tối đa 159 ký tự. `draw()` chỉ vẽ lại vùng nhập liệu, không xoá toàn bộ viewport trước khi vẽ (tránh nhấp nháy thêm).

## Thông báo USB Type-C

Sử dụng **HWCDC::isPlugged()** từ USB Serial/JTAG tích hợp ESP32-S3 (board profile hiện tại `ARDUINO_USB_MODE=1`, `ARDUINO_USB_CDC_ON_BOOT=1`). Sau 750 ms ổn định, OS tạo Notification Center event:

- `USB connected / Computer connected via Type-C`
- `USB disconnected / Type-C data connection lost`

Đây là **trạng thái USB có host**, KHÔNG phải máy đo điện áp dây VBUS. Dây chỉ sạc, adapter điện không enumeration hoặc nguồn USB cắt điện toàn thiết bị khi rút **không thể phát hiện chính xác bằng firmware này**. Không gán GPIO mới hoặc đọc điện áp 5V trực tiếp từ ESP32-S3. Nếu phần cứng có mạch chia áp/comparator riêng nối VBUS vào GPIO an toàn thì có thể thêm chức năng đo nguồn Type-C sau khi đối chiếu sơ đồ chân thật.

## Thông báo thẻ nhớ

Giữ nguyên `StorageService.tick()` và cơ chế chống remount khi phát âm thanh/file còn mở:
- Khi khởi động có thẻ SD: `microSD detected / Memory card ready`.
- Khi SD bị rút và phát hiện: `microSD removed / Storage offline; reinsert to retry`.
- Khi gắn thẻ lại và mount thành công: `microSD mounted / Filesystem ready again`.

Thời gian kiểm tra SD là 8 giây khi đã mount; khi lỗi chờ 5 giây rồi retry có giới hạn. Thông báo nằm trong Notification Center và Serial 115200, **không sử dụng popup toàn màn hình gây nhấp nháy**. Khi thẻ bị rút giữa hoạt động, luồng OS hiện tại tự rời màn hình phụ thuộc vào thẻ. Không kiểm tra việc rút nóng lúc OS đang ghi dữ liệu.

## Kiểm thử

```powershell
py -3 tools/test_system_keyboard_usb_sd.py
py -3 tools/test_qeafbrowser_os.py
pio run -e vqeaf_os
```

Hardware acceptance:
1. Nhập WiFi bằng QWERTY, SHIFT/SYM/SPACE/DEL, xác minh mật khẩu không hiện văn bản và không có `.com`.
2. Nhập URL có `.com`, mở từ Browser và điều hướng Back. Thử nhập dài, T9 và phím hủy.
3. **Máy dùng nguồn riêng:** cắm Type-C data vào máy tính và rút, quan sát USB Notification Center cùng Serial. Nếu OS cấp nguồn duy nhất từ Type-C sẽ tắt khi rút, nên không thể hiển thị thông báo rút.
4. Gắn/rút thẻ SD khi không ghi/đọc, đối chiếu `[S3DIAG][SD] event=REMOVED/MOUNTED`; quan sát Notification Center; xác minh ứng dụng và theme vẫn hoạt động sau mount.
5. Quay video LCD để phân tích nhấp nháy thật; host mock/CI không xác minh được chất lượng hình ảnh hoặc tốc độ SPI.

Các test chạy trong GitHub Actions chứng thực hành vi phần mềm và build, KHÔNG thay thế thử USB/VBUS hay SD phần cứng.
