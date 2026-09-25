# Qeafbrowser native port trong VQEAF-OS v2.5.1 (giai đoạn 1)

Nguồn tham khảo: `nectvety-software/legacy-32-classic-E524546/projects/Qeafbrowser_v1.7` (README trong thư mục hiện mô tả đến v2.2).

## Mô hình tích hợp

- **Một firmware / một vòng loop:** dùng `BrowserApp`, `BrowserService`, `AppContext`, `VqeafUI`, keypad, WiFi, TFT_eSPI và StorageService sẵn có của OS. **Không** nhúng `main.cpp`, GPIO, driver LovyanGFX, mount SD độc lập, boot loop hoặc sơ đồ phân vùng từ dự án trình duyệt standalone.
- **TLS:** giữ `TrustedTls::configure`, không nhập `setInsecure()` của bản standalone. Chặn HTTPS redirect xuống HTTP.
- **Bộ nhớ:** 84 dòng, 24 liên kết, 16 URL lịch sử, 12 bookmark; dùng PSRAM và giới hạn body 32 KiB đang có. Không tải nguyên bản firmware standalone vào bộ nhớ.
- **Giao diện:** giữ chrome/softkeys/menu hệ điều hành, không đè status và footer của VQEAF, không thay theme hoặc icon OS.
- **Launcher:** Browser đang là một ứng dụng native trong VQEAF, không cài một firmware thứ hai và không bypass chính sách QEAPP/2.

## Các tính năng đã nối vào nguồn OS

- `mtt:start` (Speed Dial hoạt động kể cả ngoại tuyến); `mtt:history` (16 mục phiên hiện tại), `mtt:bookmark`, `mtt:help`, `mtt:about`.
- Menu Options: Speed Dial, History, Bookmarks, Bookmark page, Help. Trang chủ của Browser trong OS mặc định mở Speed Dial.
- Bookmark tối đa 12 URL: cấp từ PSRAM, lưu vào `/System/Apps/Data/qeafbrowser_bookmarks.txt` trên SD bằng `StorageService::writeAtomic`. Nạp lại bằng `recoverAtomicFile`; nếu không có thẻ SD, bookmark chỉ sống trong RAM cho đến khi khởi động lại.
- Thêm cú pháp WML `<card title>`, `<anchor><go href=...>` vào parser văn bản hiện có.
- Tiếp tục dùng luồng tải, kiểm tra URL và chuyển file tải về App Installer/Theme Manager của OS (không tự thực thi dữ liệu tải).

## Chưa nhập ở giai đoạn này

Đây là **port chức năng native**, không phải sao chép nguyên firmware Qeafbrowser từ repo nguồn. Chưa có: Opera-style full-page overview với thumbnail JPEG/PNG, zoom x1..x8, quán tính D-pad, trình render S60 pixel-perfect từ LovyanGFX, cookie store bản standalone và engine WML/HTML hoàn chỉnh 400 dòng. Chưa cam kết tương đương phiên bản standalone hoặc tương thích tất cả website.

Lý do không nhúng trực tiếp: standalone và OS hiện cùng sở hữu LCD/keypad/mount SD, dùng hai thư viện renderer khác nhau; việc ghép hai `main.cpp` không thể hoạt động an toàn và có thể tái tạo lỗi nhấp nháy. Trước khi tái sử dụng toàn bộ source của bên thứ ba, cũng cần xác minh điều kiện cấp phép.

## Kiểm thử

Chạy trên checkout của nhánh:

```powershell
py -3 tools/test_qeafbrowser_os.py
py -3 tools/test_lua_transition_noblank.py
py -3 tools/test_lua_frame_present.py
py -3 tools/test_backguard_v251.py
pio run -e vqeaf_os
```

CI: `.github/workflows/qeafbrowser-native-ci.yml` biên dịch **cả stock OS lẫn Lua beta** bằng khóa Lua tạm cho CI. **Firmware Lua beta CI không được nạp lên máy cá nhân** vì dùng khóa không khớp ứng dụng đã ký.

### Acceptance trên thiết bị thật (chưa chạy)

1. Khởi động và mở Browser không WiFi: thấy Speed Dial; Home/Back về Launcher và Back r2 không reset ngoài ý muốn.
2. Bật WiFi: vào web HTTPS dùng CA hợp lệ; URL chuyển hướng tương đối, hostname sai/hết hạn, HTTPS downgrade bị chặn.
3. Mở WML có `anchor/go` để xác minh điều hướng URL và title.
4. Lưu bookmark, rút/lắp SD an toàn, reboot và xác nhận đã phục hồi; nếu mất thẻ thì không format.
5. Cuộn trang, nhập URL bằng T9, tải `.qeapp` có chữ ký qua installer và `.vqeaf` qua theme manager.
6. Dùng Serial 115200 và quay video LCD 60 FPS để so sánh hiện tượng nhấp nháy Browser/Home/Lua trước-sau; ghi heap/PSRAM và Watchdog/reset reasons.

Giữ nhánh thử nghiệm tách khỏi main cho tới khi toàn bộ target CI và thử nghiệm phần cứng đạt.
