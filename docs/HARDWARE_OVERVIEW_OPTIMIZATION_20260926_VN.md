# Báo cáo tối ưu Overview và nạp firmware ESP32-S3 thật — 26/09/2026

## Phạm vi thay đổi

- Bộ lập kế hoạch **dirty-region không cấp phát heap**: vẽ lại hai tile khi chuyển focus, chỉ thanh tiến độ khi scroll không đổi focus, toàn bộ lưới khi zoom hoặc chuyển cửa sổ.
- Loại bỏ thao tác xóa khung tile dư thừa sau khi đã xóa nền toàn bộ.
- Bộ đếm FPS không ghi nhận tick bị bỏ qua khi không có pixel thay đổi.
- Giữ nguyên giới hạn redraw 30 Hz, parser/cache/cookie và cơ chế fallback SD; không cấp phát thêm framebuffer.
- Bổ sung profile `vqeaf_os_uart` cho bo dùng CH340 UART0; profile `vqeaf_os` native USB CDC không đổi. Bổ sung test planner và kiểm tra tích hợp trong GitHub CI.

## So sánh trực tiếp trên ESP32-S3 N16R8, màn hình ST7789 240×320

Phép đo cùng một thiết bị, profile chẩn đoán COM3/115200 và cùng công việc focus 4 tile zoom x4, **32 lần vẽ mỗi chế độ**:

| Cơ chế | Thời gian vẽ trung bình |
|---|---:|
| Vẽ lại toàn bộ Overview cho thao tác đổi focus | **53.038 ms** |
| Chỉ vẽ các tile bẩn và scrollbar | **16.298 ms** |

Thao tác **đổi focus** giảm thời gian vẽ khoảng **69,3%**, tỷ lệ **3,25 lần** giữa hai phương án đo cùng điều kiện. Đây là thời gian LCD cho đầu vào **giả lập**, không phải FPS thao tác phím vật lý.

64 lần dựng toàn bộ x1–x8 sau tối ưu: trung bình **53.038 ms**, P95 **53.752 ms**, tối đa **60.243 ms**, thông lượng **18,8 FPS**. Không khẳng định cả trình duyệt đạt 30/60 FPS; những thao tác buộc vẽ toàn bộ vẫn tốn ~53 ms. Vẫn thiếu phép đo D-Pad-to-photon thực tế (`input_events=0`).

Đã chạy kiểm thử cấu trúc tính năng Overview/zoom/cache/cookie, recovery và kiểm thử planner/đường dẫn repaint; GitHub Linux host/regression và firmware stock CI trên các commit triển khai trước đó **SUCCESS**.

## Khôi phục trạng thái trên bo thật

Trên firmware chẩn đoán cùng nhánh, fixture tổng hợp tách biệt `stage=PASS` (HTML, cookie giả, thumbnail), ESP.restart được xác nhận, sau boot `verify=PASS html=PASS cookie=PASS thumb=SD_FALLBACK_PASS`, cuối cùng `cleanup=COMPLETE`. LittleFS trên bo hiện lỗi mount, **chỉ chứng minh được tầng SD fallback**; không format vùng flash để tránh mất dữ liệu cũ.

## Nạp firmware production vào ESP32-S3 (đã hoàn tất)

- Bo kiểm tra qua CH340 COM3, ESP32-S3 flash 16 MiB. Phân vùng app1 đang hoạt động tại **0x650000**. Trước thử nghiệm đã sao lưu **2.097.152 byte app1 và 8 KiB otadata** vào `local_hw_results/` riêng trên máy Windows; đã có bản sao 16 MiB flash từ lượt kiểm thử trước. Những bản sao có thể chứa dữ liệu riêng tư **không đưa lên GitHub**.
- Build sản phẩm: `pio run -e vqeaf_os_uart` **SUCCESS**, RAM tĩnh **91.284/327.680 byte (27,9%)**, flash app **1.569.893/6.553.600 byte (24,0%)**.
- Tệp nhị phân tại thư mục đích `.pio/build/vqeaf_os_uart/firmware.bin`, **1.570.256 byte**. Đã ghi **chỉ app1 tại 0x650000**, không ghi NVS, bootloader, partition table, otadata hoặc SD. Esptool xác nhận `Hash of data verified` và reset thiết bị.
- **Đã đọc ngược 1.570.256 byte từ chính flash**, so sánh toàn bộ: `READBACK_EXACT_MATCH True`, SHA-256 ảnh build và readback đều **`901ec0cd4fb4de7f4acdaa52e5c6bf7bcbcbad6fd8b00f1b68f7e92b4a86cc9c`**.
- Sau khi nạp, qua đúng cổng COM3 nhận lệnh `diag help` và `diag sd status`, xác nhận firmware **đang chạy**, thẻ SD **mounted**, `errors=0`, dung lượng **1858 MiB**. Log banner khởi động production không thu được vì mở Serial sau khởi động; không khẳng định đã đo đầy đủ trạng thái mọi dịch vụ ở boot production.
- Không nạp profile diagnostic thành firmware cuối: trạng thái để lại trên bo là **`vqeaf_os_uart` production**.

## Vấn đề chưa hoàn tất trước khi phát hành

- Chưa xác minh độ trễ vật lý D-Pad→pixel, live HTTPS/JPEG/PNG qua Wi-Fi và độ ổn định toàn bộ hệ điều hành dài hạn.
- Sửa lỗi mount LittleFS phải có chiến lược bảo toàn phân vùng trước; SD fallback hiện hoạt động.
- Cookie đang được lưu plaintext trên SD; chưa dùng cho thông tin đăng nhập nhạy cảm cho đến khi kiểm toán.
- Nhánh tính năng / Pull Request #10 tiếp tục **Draft**; kết quả build, nạp, readback và nhận lệnh Serial thật không có nghĩa tất cả chức năng OS đều đã nghiệm thu.
