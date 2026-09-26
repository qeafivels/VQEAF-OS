# Đồng bộ VQEAF-OS trên máy Windows – 26/09/2026

- Đường dẫn: D:\Program\arduino\legacy-32-classic-E524546\projects\VQEAF_OS
- Nguồn: qeafivels/VQEAF-OS, nhánh feat/qeafbrowser-parity-20260926, commit b0b634c8d5fad1dcbb90e83c87215d0da4660c9b.
- So khớp 665 tệp do Git quản lý: 102 tệp không đổi, 489 tệp được cập nhật (đã backup bản cũ), 74 tệp mới. Sau đồng bộ: 0 tệp lệch nguồn.
- Những tệp riêng trong thư mục cũ không bị xóa, gồm preview và các báo cáo cũ. Dự án QEAPP-Studio ở thư mục bên cạnh không bị sửa.
- Thư mục backup: D:\Program\arduino\legacy-32-classic-E524546\projects\VQEAF_OS_presync_backup_20260926_093505
- Danh mục file từng được thay đổi và thêm: xem sync_manifest.json trong thư mục backup.
- Build PlatformIO `pio run -e vqeaf_os`: PASS, RAM tĩnh 91.076/327.680 byte (27,8%), flash 1.556.013/6.553.600 byte (23,7%).
- Firmware bin: .pio/build/vqeaf_os/firmware.bin (1.556.384 byte).
- Kiểm thử cấu trúc Browser, Overview/zoom/thumb/cache/cookie, Recovery: PASS.
- Kiểm thử hồi quy C++ trên Windows chưa chạy được vì không có g++ trên PATH; các CI Linux của nhánh nguồn từng PASS.
- Chưa nạp firmware này lên bo ESP32-S3 trong tác vụ đồng bộ; bản local đã được biên dịch nhưng vẫn cần nghiệm thu tính năng thật.
- Cảnh báo đã biết: LittleFS trên bo từng báo lỗi, SD fallback khôi phục cookie/cache thử nghiệm thành công. Overview benchmark thực đo 19 FPS với input giả lập; chưa có số đo độ trễ D-Pad vật lý.
