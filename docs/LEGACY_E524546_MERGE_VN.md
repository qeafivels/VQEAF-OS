# Tích hợp tính năng E524546-OS vào VQEAF-OS (giai đoạn 1)

Nguồn đối chiếu: `nectvety-software/legacy-32-classic-E524546` nhánh `master`, commit
`efec5b5dc659e88e15e7802b9e525d5b67b9fa6c`.
Nơi đến: `qeafivels/VQEAF-OS` nhánh `main` tại thời điểm tách
`1093405aaf3c5119ec991e61c0e133a1e74ac5bb`.
Các bản firmware cũ trong monorepo gồm **nhiều hệ điều hành độc lập**,
không phải một bản cập nhật tương thích nhị phân. Chỉ chuyển từng tính năng,
**không ghi đè runtime, driver, package signer hay GPIO** của VQEAF-OS.

## Ma trận 15 tính năng E524546 Doodle OS

| Tính năng tham khảo | VQEAF-OS | Hành động trong đợt này |
|---|---|---|
| WiFi | Quản lý thật (quét/kết nối) | Giữ lõi WiFi hiện tại, không nhập demo scan giả |
| Files | Có SD manager | Giữ chức năng thật |
| Bluetooth | Có BLE scanner | Không biến thành classic BT hoặc audio |
| Terminal | Có Shell | Giữ lệnh OS hiện tại |
| Notes | Có ghi chú văn bản | Giữ Notes; thêm Sketchpad 3 trang vẽ riêng |
| LoRa | Chưa xác nhận module | Không hiển thị phần cứng hoặc gửi tin giả |
| IR | Chưa xác nhận LED/receiver | Không hiển thị hành động giả |
| Browser | Có Qeafbrowser thật | Không thay bằng bookmark demo offline |
| Settings | Có thông số thật | Giữ cấu hình thiết bị |
| Radio | Chưa xác nhận FM module | Không quảng cáo chức năng giả |
| Music | Có dịch vụ nhạc, yêu cầu cấu hình âm thanh thật | Giữ nền phát thật |
| Paint | Thiếu trình vẽ native | **MỚI:** Sketchpad với nét mực, chì, tẩy, Undo, lưu SD |
| Retro | Có Pixel Snake native | Giữ, không tự sao chép emulator khác |
| Chat | Thiếu backend xác nhận | Chưa tích hợp (demo legacy là mô phỏng) |
| VM | Có Lua 5.4 thử nghiệm, ứng dụng cần chữ ký đúng | Không mở quyền `io/os/require` tùy ý hoặc chấp nhận Lua 5.1 nhị phân |

## Tính năng thực sự bổ sung

1. **Sketchpad native** tại Explorer > Media, Explorer > Applications hoặc Applications:
   - 3 trang, D-Pad di chuyển đồng thời vẽ khi giữ, OPTION đổi Ink/Pencil/Eraser,
     B hoàn tác một nét, START lưu, A trở lại danh sách (xác nhận nếu chưa lưu).
   - Tối đa 160 nét/trang. Trình bày kiểu giấy vở được dựng bằng RGB565,
     không tải bitmap nặng hay thêm full-screen framebuffer.
   - Dữ liệu mới tại `/Documents/Sketchpad/pageN.qsk`: định dạng riêng
     có độ dài chính xác, FNV-1a để phát hiện hỏng file, kiểm tra giới hạn và
     `StorageService::writeAtomic` hỗ trợ backup khi mất nguồn.
2. **Nhập dữ liệu cũ có kiểm soát:** sao chép thủ công từng `trangN.dat` từ
   LittleFS của hệ cũ (sau khi sao lưu flash!) vào thư mục
   `/Documents/Sketchpad/legacy/` trên microSD mới. Chọn trang trống,
   OPTION -> Import. Giới hạn 8192 byte, 160 nét, số và tọa độ hợp lệ.
   Chuyển tọa độ phần viền hệ cũ vào viewport mới mà không đè thanh trạng thái.
   Không nhập đè trang mới đang có; giữ tệp cũ nguyên vẹn.
3. **Notebook Blue theme**: `themes/notebook_legacy.vqeaf` cùng bản sao
   `sd/System/Themes/notebook_legacy.vqeaf`. Chỉ dùng palette, không mang
   giao diện/vật liệu/các tệp font hay thư viện Lua bên thứ ba qua VQEAF.

## Hợp đồng an toàn, sở hữu mã nguồn và tính tương thích

- Thực hiện chức năng theo hành vi quan sát được, mã C++ mới viết độc lập.
  Không sao chép vendor LuaS30 5.1.5, PochitaOS, artwork hay SDK vì monorepo
  nguồn không công bố giấy phép chung áp dụng cho toàn bộ thành phần.
- VQEAF-OS sử dụng TFT_eSPI, N16R8 và sơ đồ GPIO gốc, không thêm LovyanGFX
  song song hoặc nạp một firmware thứ hai để làm mất launcher/app data.
- Không bỏ kiểm tra chữ ký QEAPP/2, không nhúng private key. VQEAF Lua beta
  khác với LuaS30 5.1 (không hỗ trợ trực tiếp toàn bộ `engine.*` legacy).
- Không khẳng định LoRa/IR/FM/audio output có phần cứng khi chưa xác nhận BOM.

## Kiểm thử và thao tác

```sh
python tools/test_legacy_merge.py     # model C++11, corrupt input, launcher/SD/theme
python tools/run_full_system_review.py # bộ kiểm thử OS đã có
pio run -e vqeaf_os                 # firmware stock
# Lua beta cần bootstrap chính thức và public key beta phù hợp:
# python tools/bootstrap_lua.py --firmware-root .
# pio run -e vqeaf_lua_beta
```

Để nạp trên board thật: sao lưu NVS, flash và SD trước; kiểm tra build, dùng
`pio run -e vqeaf_os -t upload` rồi `pio device monitor -b 115200`.
Ghi nhận thời gian khởi động, mở Sketchpad, tạo/lưu/tải lại 3 trang, rút SD
an toàn khi idle, gỡ/lắp lại và theo dõi heap/PSRAM. Chưa thực hiện nạp
mạch, đo FPS hoặc chụp LCD vật lý trong CI.
