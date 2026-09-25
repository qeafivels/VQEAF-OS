# VQEAF-OS v2.5.1 + Back r2 — Bản thử nghiệm (ESP32-S3)

> **Bản phát hành thử nghiệm (Pre-release).** Mã nguồn đã qua kiểm thử host và biên dịch ESP32-S3 trên GitHub Actions. **Chưa có xác nhận nạp và vận hành trên phần cứng thật.** Hãy sao lưu dữ liệu và chỉ thử trên thiết bị phát triển phù hợp.

## Điểm mới

- **v2.4.3–v2.4.4:** bổ sung kiểm tra luồng cài ứng dụng/theme và công cụ thu log Serial **115200 baud** để điều tra reset khi cài đặt.
- **v2.5.0–v2.5.1:** nâng cấp lõi render và quản lý bộ nhớ, tối ưu luồng mở ứng dụng QEAPP, vẽ icon, chuyển cảnh; bổ sung công cụ đo FPS, thời gian dựng hình, độ trễ phím và bộ nhớ qua Serial.
- **Back r2:** bổ sung bộ điều phối xác nhận **thoát ứng dụng**. Các thao tác Back nội bộ vẫn do ứng dụng xử lý; khi thực sự thoát, hộp thoại hệ thống có mặc định **No**. Hủy xác nhận không kết thúc phiên ứng dụng và cơ chế vẽ nền không được che hộp thoại.
- **Giữ đồ họa:** Back r2 chỉ bổ sung lõi/kiểm thử và thay đổi điều phối tại `src/main.cpp`; giữ nguyên theme, icon, launcher, font và renderer của bản **v2.5.1**.

## Phần cứng và giới hạn tương thích

Mã nguồn dành cho **ESP32-S3-WROOM-1 N16R8** (16 MB Flash, 8 MB PSRAM), LCD **ST7789 240×320**, microSD và bàn phím theo cấu hình GPIO có sẵn trong repository. Không tự đổi chân phần cứng.

Firmware mặc định `vqeaf_os` không có nghĩa là hỗ trợ các ứng dụng Lua tương tác từ Studio. `.qeapp` phải thuộc kiểu firmware hỗ trợ và được ký bằng khóa tin cậy. IDE và các mẫu Lua thuộc repository [QEAPP-Studio](https://github.com/qeafivels/QEAPP-Studio).

## Kết quả xác minh

| Hạng mục | Kết quả |
| --- | --- |
| Kiểm thử C++ host cho Back r2 | **160/160 PASS** trong báo cáo của gói nguồn |
| Bộ nghiệm thu v2.5.1 trên PC | **8/8 giai đoạn PASS** |
| GitHub Actions host regression | **PASS**, [xem workflow](https://github.com/qeafivels/VQEAF-OS/actions/runs/36116256896) |
| GitHub Actions PlatformIO ESP32-S3 (`vqeaf_os`) | **PASS**, [xem build và artifact](https://github.com/qeafivels/VQEAF-OS/actions/runs/36118874784) |
| Nạp và chạy thực tế trên ESP32-S3 | **CHƯA KIỂM TRA** |
| LCD, phím, microSD, WiFi, installer/theme, FPS thực và Back trên board | **CHƯA KIỂM TRA** |

**Commit nguồn đã biên dịch:** `512a51a7ab0ae57f5ce4c56ded47ad6f06612d9e`. Tag này trỏ tới đúng commit đã dùng tạo artifact CI; các commit README/CI về sau trên `main` không làm thay đổi firmware trong bản đính kèm.

## Tệp đính kèm và SHA-256

- **`VQEAF-OS_v2.5.1_Back-r2_firmware.bin`**: ảnh ứng dụng firmware, **1.503.984 byte**. SHA-256: `e68c66fc1200ee3ae897a3fe91c19c4ee366fd39f5880cb6de66924e486e5c87`.
- **`VQEAF-OS_v2.5.1_Back-r2_partitions.bin`**: bảng phân vùng, **3.072 byte**. SHA-256: `bd0f7954aca2ef7d925ee21aaa1f3dc8822d1d6ce5cbbd26a135e5886bfff6ce`.
- **`VQEAF-OS_v2.5.1_Back-r2_CI-artifact.zip`**: nguyên bản artifact từ [CI run #36118874784](https://github.com/qeafivels/VQEAF-OS/actions/runs/36118874784), có `firmware.bin`, `firmware.elf`, `partitions.bin` và log build. SHA-256: `7184f79147b9a4123dbe23be480ecb205fbcd1c4c6ae10ae9224a515c7f03685`.
- **`SHA256SUMS.txt`**: kiểm tra toàn vẹn tệp phát hành.

**Lưu ý nạp firmware:** tệp `firmware.bin` riêng **không phải** ảnh flash hoàn chỉnh và gói CI không có bootloader. Sử dụng **PlatformIO theo `platformio.ini` và đúng board N16R8** để build/nạp theo quy trình chuẩn, không tự suy đoán địa chỉ flash hoặc ghi `partitions.bin` lên thiết bị đang sử dụng. Sao lưu firmware và dữ liệu SD trước khi thử.

## Các bước nghiệm thu còn lại

1. Sao lưu dữ liệu, build/nạp bằng PlatformIO cho board N16R8, mở Serial Monitor ở **115200 baud**.
2. Kiểm tra khởi động, màu/icon trên ST7789 và điều khiển phím thật.
3. Thử mở nhiều ứng dụng, Back nội bộ, hộp thoại thoát mặc định No, hủy và xác nhận thoát; theo dõi watchdog/panic/reset.
4. Thử ký hợp lệ/không hợp lệ khi cài `.qeapp`, import theme `.vqeaf`, microSD/WiFi và quan sát các chỉ số FPS, heap/PSRAM cùng độ trễ từ log.

[Chi tiết nguồn v2.5.1 + Back r2](https://github.com/qeafivels/VQEAF-OS/blob/main/docs/RELEASE_V251_BACK_R2_GITHUB.md) · [Studio độc lập](https://github.com/qeafivels/QEAPP-Studio)
