# VQEAF-OS v2.5.1 Lua beta — khắc phục nhấp nháy vùng ứng dụng

**Phạm vi:** chỉ vùng ứng dụng Lua 240×270 tại LCD y=29..298. Không thay launcher, theme, icon, font, Back r2 hoặc GPIO gốc; không thay đổi firmware stock `vqeaf_os`. Bản này thuộc nhánh beta `feat/qeapp-lua-compat-v251`, **chưa xác nhận loại bỏ nhấp nháy trên LCD thiết bị thực**.

## Nghi vấn được xác định từ mã nguồn

Trước bản sửa, callback `engine.clear()`, `engine.rect()`, `engine.text()` được nối trực tiếp vào `TFT_eSPI`, nên một khung hình Lua có thể hiện từng pha **xóa sạch rồi vẽ lại** trên ST7789. Vòng lặp gọi `update()` và `render()` khoảng mỗi 50 ms. Khi hủy xác nhận Back r2, `enterScreen(screen, false, true)` gọi `ui.clearContent()` trước khi gọi lại `render()`, tạo thêm một khoảng trống có thể quan sát. Đây là nguyên nhân có cơ sở trong **luồng mã nguồn**; mức độ đóng góp đối với hiện tượng trên thiết bị cần đo bằng video và Serial.

## Cơ chế sau khi sửa

1. `TFT_eSprite` RGB565 240×270 đặt trong PSRAM (129.600 byte) nhận **mọi thao tác vẽ Lua**; không còn tô đen LCD giữa một callback. Một bản snapshot RGB565 cùng dung lượng trong PSRAM để so sánh hoàn chỉnh từng frame.
2. Chỉ sau khi cả `on_update()` và `on_draw()` thành công mới được `pushSprite(0,29)` **một lần** lên vùng nội dung. Nếu pixel không đổi (kể cả script vẫn gọi `engine.clear()`), bỏ qua toàn bộ giao dịch SPI; giảm số lần ghi SPI và tải CPU.
3. Trên Back r2, không gọi lại Lua callback khi popup đang hiện. Nếu người dùng chọn **No**, giữ nguyên VM + snapshot, bỏ qua clearContent, ép xuất lại sprite một lần để che phần popup bị đè trên nội dung. Chọn **Yes**/lỗi: hủy sprite và snapshot, giải phóng PSRAM.
4. Nếu thiếu PSRAM hoặc thất bại khi cấp bộ đệm, **thông báo lỗi trên Applications**; không lùi về luồng vẽ trực tiếp gây nhấp nháy hoặc cố ghi quá vùng nhớ.
5. Mỗi 5 giây Serial 115200 in `[VQEAF][LUA][FRAME] flush=... same=... avg_spi_us=... free_psram=...` phục vụ chẩn đoán trên thiết bị. Khi nội dung tĩnh, `same` nên tăng nhanh hơn `flush`; `avg_spi_us` phản ánh thời gian flush nhưng không phải độ trễ từ phím đến điểm ảnh.

Hai bộ đệm cộng lại khoảng **253 KiB** PSRAM, bên cạnh VM Lua 192 KiB và các thành phần OS khác. Không thay đổi phép ký, giới hạn gói Lua 64 KiB hay trust key.

## Test bắt buộc

```powershell
python tools/test_lua_frame_present.py
python tools/test_lua_beta_compat.py
python tools/test_backguard_v251.py
python tools/test_v15_signature.py
python tools/bootstrap_lua.py
python tools/test_lua_beta_vm.py
pio run -e vqeaf_os
# Chỉ với public header beta được provision hợp lệ, KHÔNG commit header cá nhân:
pio run -e vqeaf_lua_beta
```

GitHub Actions nhánh beta tạo **khóa ephemeral** để kiểm thử build; artifact đó không dùng để chạy app đã ký bằng PEM cá nhân. Không đổi khóa trong production, không đưa private PEM hoặc header cá nhân lên GitHub.

### Nghiệm thu màn hình trên ESP32-S3 N16R8 (CHƯA CHẠY)

1. Sao lưu Flash/SD và giữ firmware beta cũ để rollback. Chỉ build/nạp sau khi CI PASS và kiểm tra public key beta từ đúng PEM trên PC. Serial: `pio device monitor -p COM3 -b 115200` (COM3 chỉ là ví dụ).
2. Quay **video 60 FPS** lúc mở app Lua đơn giản có `on_draw()` gọi `engine.clear()` + chuyển động một sprite qua màn hình trong ≥30 giây. Chụp video đối chứng từ beta cũ cùng môi trường; tránh ánh sáng nhấp nháy/auto exposure khi so sánh.
3. Để app tĩnh 10 giây: theo dõi `flush/same`. Sau đó di chuyển D-pad, quan sát không xuất hiện nền đen giữa các nét vẽ. So sánh màu RGB565/vị trí 29px của status bar và softkey. Nếu vẫn nhấp nháy cả **Home/Launcher** thì đây là lỗi khác: kiểm tra nguồn/backlight PWM, nguồn TFT và đường vẽ của hệ thống, không gán nguyên nhân cho Lua.
4. Mở popup Back → chọn **No** nhiều lần; VM vẫn sống, không có đen trống khi khôi phục; **Yes** dọn PSRAM. Lặp mở/thoát 100 lần, theo dõi `free_psram` và reset, 10 phút liên tục.
5. Gửi ảnh/video màn hình thật cùng log Serial để phân biệt *chớp do xóa trước khi vẽ*, *LCD tearing*, *nguồn/backlight*, hay *reset watchdog*. Test host/CI không chứng minh LCD đã hết nhấp nháy.

## Giới hạn

`TFT_eSprite::pushSprite` là truyền blocking; có thể còn **tearing** do tấm nền ST7789 không đồng bộ VSYNC, đặc biệt nếu frame động chiếm toàn màn hình. Chỉ tối ưu tiếp vùng dirty tile/DMA sau khi đo `avg_spi_us` và video trên đúng board. Đừng tự tăng LCD SPI speed, đổi GPIO hoặc sửa Retro-Go UI khi chưa có bằng chứng.
