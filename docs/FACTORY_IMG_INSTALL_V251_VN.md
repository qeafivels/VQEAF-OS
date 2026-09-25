# VQEAF-OS v2.5.1 + Back r2: gói cài mặc định `.img`

**Đối tượng:** thiết bị tự chế đúng **ESP32-S3-WROOM-1 N16R8 (Flash 16 MiB, PSRAM 8 MiB)** với sơ đồ chân trong `include/BoardConfig.h`. Màn hình ST7789 240×320. File `.img` này là **ảnh Flash nguyên khối để cài mới về trạng thái trống**, không phải ảnh thẻ microSD, bộ cài chạy trong VQEAF-OS, gói OTA hoặc tệp phù hợp cho mọi bo ESP32-S3.

> **CẢNH BÁO MẤT DỮ LIỆU:** `.img` chứa **16.777.216 byte**, trong đó các vùng không được định nghĩa là thành phần khởi động/firmware được điền **0xFF**. Ghi tệp này từ địa chỉ `0x0` sẽ **ghi đè NVS, dữ liệu chọn OTA, OTA slot còn lại, LittleFS/SPIFFS và coredump**. Sao lưu **toàn bộ Flash 16 MiB** và thẻ microSD trước khi tiếp tục. Không sử dụng nếu board khác, sử dụng Secure Boot/Flash Encryption, hoặc không chắc bố cục phân vùng hiện tại.

## 1. Chọn đúng gói trên GitHub Release

[Release v2.5.1 + Back r2](https://github.com/qeafivels/VQEAF-OS/releases/tag/v2.5.1-back-r2) được đánh dấu **Pre-release** vì chưa có nghiệm thu LCD, phím, SD, WiFi, FPS thực tế trên thiết bị.

| Trường hợp | Tệp/phương pháp | Có xóa dữ liệu? |
| --- | --- | --- |
| Cài mới/khôi phục **sạch hoàn toàn** cho N16R8 | `VQEAF-OS_v2.5.1_Back-r2_factory.img` (gói cài nguyên khối mặc định cho **cài mới**) | **Có.** Ghi lại toàn bộ 16 MiB. |
| Cập nhật máy hiện có; muốn giữ NVS/OTA/FS | Build và Upload profile `vqeaf_os` trong PlatformIO theo đúng source tag, sau khi sao lưu | Tùy sơ đồ hiện tại; **không** dùng `.img` toàn khối. |
| Gỡ lỗi/hồi quy / tải riêng application binary | `VQEAF-OS_v2.5.1_Back-r2_firmware.bin` (chỉ app, không có bootloader) | Không tự đủ để nạp sạch. |

`.img` phải có tệp JSON đi kèm `...factory.img.json` và `...IMG_SHA256SUMS.txt` để đối chiếu. Gói `.img` được ghép từ **chính tag đã qua CI**: bootloader đúng cấu hình, bảng phân vùng đã biên dịch, Arduino `boot_app0.bin` và `firmware.bin` tương ứng tại địa chỉ kiểm tra. Không đưa private key hoặc dữ liệu người dùng vào ảnh.

## 2. Kiểm tra trước khi cài

1. Tải `.img`, `.img.json` và `IMG_SHA256SUMS.txt` tại GitHub Release. Trên Windows PowerShell, kiểm tra `Get-FileHash .\VQEAF-OS_v2.5.1_Back-r2_factory.img -Algorithm SHA256` và so với JSON / file checksum.
2. Đảm bảo `esptool` phát hiện **ESP32-S3 và Flash 16 MB**, không sử dụng Secure Boot/Flash Encryption khi ghi ảnh không ký. Đóng mọi Serial Monitor đang chiếm cổng.
3. Sao lưu MicroSD riêng. Dùng `COM5` chỉ là **ví dụ**; thay bằng cổng phát hiện được:

```powershell
py -3 -m pip install "esptool>=4.5,<5"
py -3 -m esptool --chip esp32s3 --port COM5 flash_id
py -3 -m esptool --chip esp32s3 --port COM5 --baud 460800 read_flash 0x0 0x1000000 VQEAF_Backup_16MiB.bin
Get-FileHash .\VQEAF_Backup_16MiB.bin -Algorithm SHA256
```

**DỪNG** nếu `read_flash` thất bại hoặc chưa bảo quản an toàn được bản sao lưu. Khi cần giữ NVS/OTA, không sử dụng `.img` mặc định kiểu factory.

## 3. Cài mới toàn khối từ `.img` bằng esptool

Chỉ thực hiện nếu đã kiểm tra đúng thiết bị **và chấp nhận xóa sạch dữ liệu**:

```powershell
py -3 -m esptool --chip esp32s3 --port COM5 --baud 460800 --before default_reset --after hard_reset write_flash 0x0 VQEAF-OS_v2.5.1_Back-r2_factory.img
```

Không tự thêm `erase_flash`: việc ghi toàn bộ `.img` đã xóa các dữ liệu cũ trong các vùng cần ghi. Không đổi offset `0x0`, không dùng cho bo khác và không ghép thêm `partitions.bin` hay `firmware.bin` khi đã nạp nguyên khối.

## 4. Dùng PlatformIO để cập nhật hoặc tự tạo `.img`

```powershell
git clone https://github.com/qeafivels/VQEAF-OS.git
cd VQEAF-OS
# Để tái hiện mã firmware Release, checkout tag v2.5.1-back-r2 trước khi build.
git checkout v2.5.1-back-r2
py -3 -m pip install platformio pillow
pio run -e vqeaf_os
pio device list
# Không cần .img cho cập nhật thông thường:
pio run -e vqeaf_os -t upload --upload-port COM5
pio device monitor -p COM5 -b 115200
```

Để **tự đóng gói `.img`**, dùng script `tools/build_factory_img.py` trên nhánh `main` hiện hành (sau khi nó được bổ sung), trỏ `--project-root` tới checkout của tag đã build. Trên bản dựng tag cũ không có script này:

```powershell
# Ví dụ bố trí: D:\VQEAF-main\tools\build_factory_img.py
# và D:\VQEAF-v251-tag\ là source tag đã build bằng PlatformIO.
py -3 D:\VQEAF-main\tools\build_factory_img.py `
  --project-root D:\VQEAF-v251-tag `
  --build-dir D:\VQEAF-v251-tag\.pio\build\vqeaf_os `
  --output D:\VQEAF_Release\VQEAF-OS_v2.5.1_Back-r2_factory.img

py -3 D:\VQEAF-main\tools\build_factory_img.py `
  --verify-only --output D:\VQEAF_Release\VQEAF-OS_v2.5.1_Back-r2_factory.img
```

Script không tự biên dịch hoặc nạp. Khi không tìm được Arduino `boot_app0.bin`, chỉ định đường dẫn chính xác bằng `--boot-app0`, **không** dùng ảnh/SDK từ board khác. Có thể thêm `--expected-firmware-sha256` và `--expected-partitions-sha256` để chỉ xuất `.img` nếu hai thành phần trùng Release gốc.

## 5. Kiểm tra và thu log

Sau khi khởi động, mở Serial 115200 và kiểm tra lần lượt splash/Home, icon và màu sắc, các nút D-pad, Back nội bộ, hộp thoại `Close application?` mặc định **No**, thử hủy/đồng ý thoát. Thử SD, WiFi, cài `.qeapp` ký hợp lệ và theme `.vqeaf`; theo dõi crash, reset và FPS thực tế. Các bài test GitHub Actions chưa thay thế nghiệm thu này.

## 6. Cấu trúc `.img` (cố định cho board N16R8 này)

| Vùng Flash | Nội dung ảnh sạch |
| --- | --- |
| `0x000000` | Bootloader ESP32-S3 theo profile `vqeaf_os` |
| `0x008000` | `partitions.bin` (cùng SHA-256 bản build đã đối chiếu) |
| `0x009000–0x00DFFF` | NVS trống `0xFF` |
| `0x00E000` | Arduino `boot_app0.bin` trong phân vùng `otadata` |
| `0x010000` | `firmware.bin` (app0) |
| `0x650000–0xC8FFFF` | app1 trống `0xFF` |
| `0xC90000–0xFEFFFF` | LittleFS/SPIFFS trống `0xFF` |
| `0xFF0000–0xFFFFFF` | Coredump trống `0xFF` |

Không dùng ảnh `.img` này như ảnh thẻ SD, không mở bằng installer `.qeapp` trong hệ điều hành. Nếu chỉ cần nâng cấp, chọn PlatformIO để duy trì dữ liệu.