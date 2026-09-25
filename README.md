# VQEAF-OS

**Firmware hệ điều hành cho máy cầm tay ESP32-S3**, có launcher phong cách retro, hệ thống ứng dụng QEAPP/2 có chữ ký, theme `.vqeaf` và các công cụ chẩn đoán. Repository này chỉ quản lý **OS, mã nguồn firmware và tài nguyên thiết bị**.

**IDE phát triển ứng dụng được tách riêng:** [QEAPP-Studio](https://github.com/qeafivels/QEAPP-Studio). Không đưa `studio/`, máy ảo Lua PC hoặc các dự án mẫu Studio trở lại repository OS.

> **Mã nguồn hiện tại trên `main`: VQEAF-OS v2.5.1 + Back r2.** Host regression và **build PlatformIO ESP32-S3 trên GitHub Actions** đã PASS, có artifact firmware từ CI. **Chưa kiểm thử trực tiếp** LCD, phím, SD, WiFi hay FPS trên thiết bị. Giữ nguyên giao diện, icon, theme và renderer của v2.5.1 khi bổ sung Back r2. Xem [ghi chú phát hành](docs/RELEASE_V251_BACK_R2_GITHUB.md) và [build CI đã PASS](https://github.com/qeafivels/VQEAF-OS/actions/runs/36116252602).

## Phần cứng mục tiêu

- ESP32-S3-WROOM-1 **N16R8**: 16 MB Flash, 8 MB PSRAM.
- LCD **ST7789 240×320 dọc**, thẻ SD và bàn phím phần cứng.
- **Serial Monitor 115200 baud** để thu log.
- Giữ nguyên sơ đồ GPIO đã xác nhận trong [BoardConfig](include/BoardConfig.h), [PlatformIO](platformio.ini) và định nghĩa board [N16R8](boards/vqeaf_s3_n16r8.json). **Không suy đoán GPIO** từ ảnh hoặc thiết bị khác.

Giao diện Home/Menu, icon pixel RGB565/RLE, các theme và logic renderer thuộc firmware. Studio sử dụng ảnh/máy ảo host riêng, không được coi là ảnh LCD thực tế.

## Chức năng chính

| Thành phần | Mã nguồn/tài liệu |
| --- | --- |
| Khởi động, launcher và điều hướng | [src/main.cpp](src/main.cpp), [src/launcher](src/launcher), [hợp đồng giao diện](docs/UI_PIXEL_ATLAS_V22.md) |
| Cài và kiểm tra ứng dụng QEAPP/2 | [src/services](src/services), [hướng dẫn ký gói](docs/QEAPP_V15_SIGNING.md) |
| Qeafbrowser, tải dữ liệu và chẩn đoán | [sửa lõi v2.4.2](docs/CORE_FIX_V242_VN.md) |
| Theme hệ điều hành | [theme mẫu](themes/), [hướng dẫn theme](docs/THEMES_V12.md) |
| Game native minh họa | [Pixel Snake](games/pixel_snake/README_VN.md) |
| Build/board và kiểm thử | [platformio.ini](platformio.ini), [tools](tools/), [workflow CI](.github/workflows/platformio-build.yml) |

**Khả năng tương thích:** firmware `vqeaf_os` mặc định chỉ chạy những loại QEAPP/2 đã hỗ trợ, được ký bởi khóa đáng tin cậy. Nó **không** thực thi ứng dụng Symbian SIS, MRE/VXP, hay Lua beta từ Studio như ứng dụng native một cách tự động. Không tắt bước kiểm tra chữ ký để thử ứng dụng.

## Build trên máy phát triển

Cần [PlatformIO](https://platformio.org/) và bộ công cụ ESP32 tương thích:

```powershell
git clone https://github.com/qeafivels/VQEAF-OS.git
cd VQEAF-OS
py -3 tools\verify_v242.py
pio run -e vqeaf_os
```

Chỉ sau khi kiểm tra đúng phần cứng, sao lưu dữ liệu và build thành công mới nạp và thu log:

```powershell
pio run -e vqeaf_os -t upload
pio device monitor -b 115200
```

Cấu hình `vqeaf_snake_demo` dùng **khóa thử nghiệm khác**; không thay thế firmware sản xuất bằng bản demo hoặc đưa private key vào Git. Đọc [hướng dẫn QEAPP](docs/QEAPP_V15_SIGNING.md), [build/kiểm thử board](docs/BOARD_BUILD_V233.md) và [sửa lõi v2.4.2](docs/CORE_FIX_V242_VN.md) trước khi flash.

## Cài ứng dụng và theme

- Ứng dụng: đặt file `.qeapp` đã ký hợp lệ vào vị trí firmware hỗ trợ trên SD, ví dụ `/System/Apps/Inbox/`, sau đó xác nhận qua **App Installer**.
- Theme: dùng định dạng `.vqeaf` hợp lệ, đặt trong `/System/Themes/`, quét và **Apply** bằng giao diện hệ thống.
- Khi gặp lỗi cài đặt, sao lưu SD; ghi toàn bộ log từ lúc boot đến khi xác nhận cài và kiểm tra thông báo `corediag`.

Để tạo ứng dụng bằng IDE, mở [QEAPP-Studio](https://github.com/qeafivels/QEAPP-Studio), dùng checkout OS này làm đường dẫn ngoài:

```bat
set QEAPP_FIRMWARE_ROOT=D:\Projects\VQEAF-OS
```

Lưu ý: build/preview Lua trên PC không chứng minh rằng firmware hiện tại hỗ trợ chạy `.qeapp` Lua. Beta firmware và khóa ký phải được kiểm chứng riêng.

## Cấu trúc

```text
VQEAF-OS/
├── src/            # core, launcher, apps và dịch vụ firmware
├── include/         # cấu hình phần cứng
├── boards/          # board ESP32-S3 N16R8
├── partitions/      # layout Flash
├── sd/              # ví dụ nội dung thẻ SD
├── themes/          # theme gốc
├── games/           # ví dụ game native
├── tools/           # build, doctor, đo và test host
├── docs/            # đặc tả và báo cáo
└── platformio.ini   # các cấu hình PlatformIO
```

## Kiểm thử và tình trạng phát hành

```powershell
py -3 tools\verify_v242.py
py -3 tools\doctor_v242.py
pio run -e vqeaf_os
```

Các bài test trên host chỉ xác minh một phần logic; **không thay thế** biên dịch mục tiêu, kiểm tra SD/WiFi/HTTPS, nút bấm và FPS trên ESP32-S3 thật. Kiểm tra kết quả mới nhất tại [GitHub Actions](https://github.com/qeafivels/VQEAF-OS/actions) trước khi gắn nhãn firmware ổn định.

**Tài liệu liên quan:** [Studio repo](https://github.com/qeafivels/QEAPP-Studio) · [kiến trúc](docs/ARCHITECTURE.md) · [QEAPP/2](docs/QEAPP_V15_SIGNING.md) · [tài liệu README trước khi tách](docs/archive/README_BEFORE_SPLIT_20260925.md).

**Giấy phép:** hãy xác định rõ quyền phân phối mã nguồn và các dependency trước khi tạo bản phát hành; repository hiện chưa có tệp LICENSE ở gốc.
