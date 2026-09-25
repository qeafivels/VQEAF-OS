# VQEAF OS v2.5.1 + Back r2 — bản mã nguồn thử nghiệm

**Phạm vi:** nâng cấp lõi theo gói `VQEAF_OS_v251_Back_r2_Device_Test_Candidate_Full_Source.zip`, giữ nguyên toàn bộ tài nguyên và mã renderer so với gói gốc v2.5.1 khi bổ sung Back r2. Cập nhật v2.5.1 trước Back r2 có các sửa đổi icon/hiệu năng riêng so với v2.4.2.

**Phiên bản xuất bản:** nhánh `release/v2.5.1-back-r2`, chưa xác nhận là firmware phát hành lên ESP32-S3 thật. Không đưa mã QEAPP-Studio vào repository OS.

## Các mốc phiên bản
- v2.4.3: hồi quy installer, keyboard và theme.
- v2.4.4: thu nhật ký reset và chẩn đoán lỗi khi cài ứng dụng/theme.
- v2.5.0: tối ưu render, RGB565 và chuyển cảnh; có cấu hình so sánh A/B.
- v2.5.1: tối ưu launch, icon ký số, cache và số đo FPS/độ trễ.
- Back r2: thêm bộ `OsBackConfirm` để xác nhận **thoát ứng dụng**, mặc định chọn **No**, xử lý Back nội bộ app trước, tiếp tục session khi hủy; chặn repaint đồng hồ và tác vụ che popup.

## Giới hạn thay đổi khi bổ sung Back r2
Gói Back r2 thêm bốn tệp: `src/core/OsBackConfirm.h`, `tools/backguard_host/test_os_back_confirm.cpp`, `tools/test_backguard_v251.py`, `docs/CORE_BACK_CONFIRM_V251_ONLY.md`, và **chỉ sửa `src/main.cpp`** so với gói v2.5.1. Icon, renderer, theme, launcher, font, màn hình và sprite từ v2.5.1 được giữ nguyên.

## Kết quả xác minh tại thời điểm chuyển Git
- Kiểm thử C++ host Back: **160 kiểm tra PASS**; static integration PASS.
- `python tools/verify_v251.py`: **8/8 giai đoạn PASS** trên PC (RGB565, bộ đo, parser tổng hợp, chữ ký, kiểm tra fallback).
- `python -m compileall -q tools`: PASS.
- Upload Git: đối chiếu Git blob cho **592/592** tệp có mặt trong ZIP r2 (ngoại trừ README đã cập nhật riêng), bảo toàn 10 tệp chỉ có ở nhánh `main`, bao gồm CI, hợp đồng Studio và tài liệu lịch sử.
- `pio run -e vqeaf_os`: **chưa thực hiện tại máy dùng để chuẩn bị release** do thiếu PlatformIO toolchain; xem [CI](https://github.com/qeafivels/VQEAF-OS/actions) để biết kết quả build từ GitHub.
- ESP32-S3, LCD ST7789, SD, WiFi, phím bấm, FPS thực và thời gian phản hồi: **CHƯA KIỂM TRA trên thiết bị thật**.

## Kiểm thử bắt buộc trước khi hợp nhất vào main
```bash
python tools/test_backguard_v251.py
python tools/verify_v251.py
pio run -e vqeaf_os
```
Trên ESP32-S3 N16R8:
```bash
pio run -e vqeaf_os -t upload
pio device monitor --baud 115200
```
Test Snake/Browser/Music/Gallery: Back nội bộ giữ nguyên hành vi; chỉ khi app thực sự thoát mới hiện hộp thoại `VQEAF OS / Close application?`, mặc định `No`. Hủy không làm mất tiến trình. Đồng hồ, preview, icon không vẽ lên modal. Không giả định firmware có hỗ trợ Lua beta nếu chưa có bản runtime và chữ ký phù hợp.

## Ranh giới repository
- Firmware và đồ họa: `qeafivels/VQEAF-OS`.
- Studio và hai ứng dụng mẫu: `qeafivels/QEAPP-Studio`.

**Không sửa `main` hoặc phát hành firmware.bin chỉ dựa trên host tests.**

## CI PlatformIO: Arduino FS header discovery

GitHub Actions first target build of the release branch failed because PlatformIO LDF (deep+, strict) listed `FS` and `SD_MMC` but compiled `SD_MMC.cpp` **without** the Arduino `FS/src` include directory (`FS.h: No such file or directory`). This is a build-system dependency issue, not proof that Back r2 caused a firmware compile regression. A follow-up commit adds only a framework-resolved FS include directory through `extra_scripts = pre:tools/pio_fs_sdmmc_dependency.py` and a dependency-free host smoke for that path. A successful **new** target CI build must still be verified; physical board verification remains outstanding.
