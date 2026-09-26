# Khôi phục hỗ trợ Lua .qeapp trên VQEAF-OS N16R8 (26/09/2026)

## Nguyên nhân

Bản `vqeaf_os_uart` vừa được nạp cho thiết bị chỉ bật trình duyệt/ứng dụng khai báo; **không có** `VQEAF_ENABLE_LUA=1`. Lõi Lua 5.4.8 và đường chạy `PackageApp -> LuaApp` đã có ở bản beta nhưng không được biên dịch vào profile sản phẩm này, vì vậy một ứng dụng `type=lua` dù đã cài vẫn không thể khởi chạy.

## Khôi phục và tích hợp

- Bổ sung profile **`vqeaf_lua_uart`** kế thừa `vqeaf_lua_beta` và điều hướng Serial đến UART0/CH340 (COM3, 115200). Giữ nguyên `vqeaf_os` và `vqeaf_os_uart` mặc định; không bật chạy mã Lua không ký trong profile thường.
- Cài bộ Lua **5.4.8 chính thức, đối chiếu SHA-256 trước khi dùng**, qua `python tools/bootstrap_lua.py --firmware-root .`; bản build này có đủ 26 tệp lõi `.c`.
- Giữ **đúng public trust header từ bộ ký Lua QEAPP Studio hiện có** chỉ trên máy Windows. Đã xác minh một gói Lua `.qeapp` thực do Studio tạo: định dạng QEAPP/2, hash manifest/icon/payload, chữ ký ECDSA P-256 và ID nhà xuất bản Lua `0x544C5541` đều PASS. Không thay khóa cũ, không đưa package cá nhân, public header dành riêng cho thiết bị hoặc private PEM lên GitHub.
- Profile Lua vẫn yêu cầu receipt cài đặt đã được xác minh và kiểm tra lại SHA-256 của `payload.txt` trước mỗi lần khởi chạy. Lua beta key chỉ ký Lua; khóa text/web production vẫn hoạt động.
- Thêm `diag lua status` (chỉ đọc) và `diag lua probe` (chạy **mã thử nghiệm cố định**, không đọc/ghi file, không dùng cookie, không tải ứng dụng không ký), kèm `tools/lua_uart_hardware_lab.py`.
- Đường vẽ Lua sử dụng framebuffer offscreen 240×270 trong PSRAM + bản lưu khung trước; tối đa khoảng 20 lần cập nhật/giây. Sandbox giới hạn nguồn <=64 KiB, heap mặc định 192 KiB, tối đa 75.000 lệnh hoặc 65 ms và 512 lệnh vẽ mỗi callback. Không mở API Lua io/os/package/load hoặc module native.

## Kết quả kiểm thử ESP32-S3 thật

Bo ESP32-S3 N16R8 + ST7789 240×320, UART0 qua CH340 COM3, thẻ SD được gắn. `pio run -e vqeaf_lua_uart` **SUCCESS** với RAM tĩnh **91.896/327.680 B (~28,0%)** và flash **1.698.909/6.553.600 B (~25,9%)**.

Bản Lua UART có theo dõi Safe Mode đã được **nạp vào đúng OTA app1 tại 0x650000**. Trước khi ghi đã sao lưu app1 hiện tại (2 MiB) và otadata (8 KiB) vào `local_hw_results/` **chỉ trên máy Windows**. Không định dạng SD, NVS, bootloader, partition table hoặc thay đổi các trường ký. Sau nạp đã **đọc ngược và đối chiếu đủ 1.699.280 byte**: SHA-256 file và flash đều là `46737b07756fc7c61bcdec31c2916d58650d745e5139e0bb3ff8efcd2bb54d02`, khớp từng byte. **otadata không thay đổi**.

Qua Serial thật, `diag lua status`: **enabled=1**, PSRAM còn **8.332.323 B**, **1 ứng dụng Lua đã ký** có trong danh mục cài đặt, `vm_running=0`. `diag lua probe` trong Recovery chạy Lua VM với chương trình thử cố định trên ESP32-S3 và trả **PASS**, `rect=3`, `text=1`, đỉnh heap Lua **12.563 B**. Đây là xác minh **runtime Lua chạy thật**, không phải minh chứng rằng gói `.qeapp` riêng đã được mở thành công trên LCD.

## Chặn hiện tại: Safe Mode và nguồn điện

Ngay khi lấy trạng thái, máy trả **safe_mode=1, screen=Recovery, crash_streak=0, boot_healthy=1, key_a_low=0, key_down_low=0**, lý do reset cuối cùng **Brownout**. Không có bằng chứng crash-loop đang diễn ra (`crash_streak=0`), nhưng Brownout chỉ ra đã xảy ra sự cố điện áp ở lần reset được ghi nhận. **Safe Mode chủ động chặn chạy ứng dụng Lua**, dù mã Lua và 1 gói đã ký hiện diện trên bo. Không tự động xóa cờ Safe Mode từ xa hoặc bỏ qua cổng kiểm tra chữ ký.

Sau khi kiểm tra cáp và nguồn USB ổn định, tại màn hình **Recovery** chọn **Start normal mode** (từ đầu danh sách nhấn DOWN một lần, START để chọn). Nếu Safe Mode tái xuất hiện hoặc phát sinh Brownout, dừng thử ứng dụng và kiểm tra nguồn/điện áp cùng log Serial trước. Sau khi vào Normal Mode, mở **Applications** rồi chạy ứng dụng Lua đã ký để nghiệm thu giao diện/điều khiển thực tế.

## Phạm vi đã và chưa được chứng minh

- **PASS:** xác thực package Lua Studio thực trên máy Windows; tải đúng Lua 5.4.8; kiểm thử cấu trúc; biên dịch firmware CH340; nạp và readback flash; xác nhận runtime Lua và callback vẽ/phím thử nghiệm **trên thiết bị thật**.
- **Chưa nghiệm thu:** thao tác người dùng mở và chơi/chạy **gói Lua đã cài** qua Launcher/Applications khi Safe Mode đã tắt; hình ảnh thật, D-Pad vật lý, độ ổn định nguồn và FPS dài hạn.
- Không công bố firmware cá nhân hóa (chứa public trust key riêng), mẫu ứng dụng cá nhân, raw serial hoặc ảnh flash lên kho công khai. Profile Lua tiếp tục được ghi rõ là beta; PR chưa gộp production.

### Lệnh lặp lại (sau khi sử dụng public header đúng khóa ký đang dùng)

```powershell
python tools/bootstrap_lua.py --firmware-root .
python tools/test_lua_qeapp_uart.py
pio run -e vqeaf_lua_uart
python tools/lua_uart_hardware_lab.py --port COM3 --boot-settle 0
```
