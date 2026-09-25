# VQEAF-OS v2.5.1 + Back r2: nhánh tương thích QEAPP-Studio Lua beta

> **Trạng thái: bản thử nghiệm trên nhánh `feat/qeapp-lua-compat-v251`.** Giữ nguyên giao diện, theme, icon, renderer và logic Back r2 gốc. Không tự thay thế firmware `main`, tag hoặc Release `v2.5.1-back-r2`. Bản `.img` đã phát hành **không hỗ trợ Lua**.

## Nguồn và hợp đồng tương thích

- Studio tham khảo: [nectvety-software/legacy-32-classic-E524546/QEAPP-Studio](https://github.com/nectvety-software/legacy-32-classic-E524546/tree/master/QEAPP-Studio). Đối chiếu cấu trúc gói signed QEAPP/2, bộ ký `tools/qstudio.py` và Lua runtime của firmware beta trong repo này; phiên bản tham chiếu GitHub lúc port: `a86fb7a3a079b891e5a14a019763425652136179`.
- Kế thừa Lua runtime `QeLuaRuntime` của upstream, sử dụng Lua **5.4.8 chính thức** qua `tools/bootstrap_lua.py` có kiểm SHA-256 archive. Giữ license MIT `LICENSE.lua` khi cài nguồn Lua; tuyệt đối không dùng MediaTek MRE SDK.
- `type=text` và `type=web`: giữ trình cài, bản phát hành và khóa tin cậy hiện tại. `type=lua`: chỉ chấp nhận trong profile **`vqeaf_lua_beta`** với chữ ký hợp lệ **ECDSA P-256** của nhà phát hành beta, key ID `0x544C5541`. Không cho khóa beta ký `text/web`, không cho khóa production ký `lua`.
- Studio yêu cầu `qeapp.project.json` `project_format:1`, `type:"lua"`, `content:"main.lua"`; build `--experimental-lua` và phải chỉ định `--firmware-root` đến checkout VQEAF-OS có `src/lua/QeLuaRuntime.cpp`. Chỉ `engine.clear`, `engine.rect`, `engine.text`, `engine.blit1`, `engine.heap_used`, `engine.heap_peak`, `engine.width=240`, `engine.height=270`, callbacks `on_draw`, `on_update(dt)`, `on_key(key,down)` được dùng trong beta. Không có sound/file/network/asset pack runtime trong phiên bản này. File icon 32x32 PNG được pack thành RGB565 LE 2048 byte và tải từ thư mục Installed sau khi xác minh chữ ký.
- Các gói Studio `type=lua` **không thể chạy** trên `.img` v2.5.1 gốc. Chúng chỉ hoạt động trên profile beta sau khi header PUBLIC beta được tạo từ **đúng private PEM đã dùng ký app** và sau khi CI + kiểm thử phần cứng đạt yêu cầu.

## Cấu hình khóa (trên PC của bạn, không tải khóa riêng lên Git)

Studio tạo key riêng trên PC; *KHÔNG* sao chép `qeapp_private.pem` vào thư mục Git, firmware, microSD hoặc GitHub Actions. Nếu đã có PEM dùng ký gói Lua, hãy dùng chính PEM đó để tạo **public header** cho firmware:

```powershell
# Tại checkout repo VQEAF-OS nhánh feat/qeapp-lua-compat-v251
py -3 -m pip install cryptography pillow platformio
py -3 tools/provision_lua_beta_key.py --existing-private "D:\\SecureKeys\\qeapp_private.pem"
# Hoặc để phát hành beta mới (gói Lua cũ phải được KÝ LẠI):
# py -3 tools/provision_lua_beta_key.py --private "D:\\SecureKeys\\qeapp_lua_beta.pem"
```

Kết quả là header PUBLIC `src/services/QeappTrustKeyLuaBeta.h` trong máy bạn, **được Git ignore**; nếu đã tồn tại script từ chối ghi đè. Khóa của firmware stock trong `src/services/QeappTrustKey.h` **không thay đổi**. Khi phân phối cho người khác, cần quyết định cơ chế cấp quyền và bảo vệ khóa trước; bộ khóa tạo trong cuộc trò chuyện không phù hợp để coi là khóa sản xuất bí mật.

## Chuẩn bị Lua và build

```powershell
# Không chỉnh GPIO hoặc chuyển profile thử nghiệm thành stock.
py -3 tools/bootstrap_lua.py
py -3 tools/test_lua_beta_compat.py   # test tạo trust header TẠM; sẽ từ chối nếu đã provision trên PC
py -3 tools/test_lua_beta_vm.py       # chỉ chạy được khi đã bootstrap official Lua 5.4.8
pio run -e vqeaf_lua_beta
```

**Thứ tự đề xuất trên PC:** chạy `bootstrap_lua.py`, `test_lua_beta_compat.py`, `test_lua_beta_vm.py` trước; **sau đó** provision header PUBLIC cá nhân và build. `test_lua_beta_compat.py` cố ý từ chối đụng đến header cá nhân đã có. Test host **không** chứng minh LCD/PSRAM/SD thực tế hoạt động; tham khảo GitHub Actions của nhánh beta.

Để nạp: sao lưu đầy đủ Flash/SD; chỉ dùng profile beta trên board đúng ESP32-S3-WROOM-1 N16R8 khi đã đạt CI build và xem kỹ thay đổi: `pio run -e vqeaf_lua_beta -t upload --upload-port COM3`, sau đó `pio device monitor -p COM3 -b 115200`. Không lấy firmware CI ephemeral-key để chạy app đã ký bằng key cá nhân của bạn. Không dùng `.img` cài sạch trước đó cho Lua beta.

## Luồng cài ứng dụng từ Studio

1. Studio build gói `type=lua`, `--experimental-lua`, `--key-id 0x544c5541`, `--sign-key` trỏ đến PEM riêng trên PC, icon PNG 32x32; preview Lua host không tương đương LCD.
2. Kiểm tra gói ký bằng Studio `qstudio inspect --public-key <public PEM>`; file `.qeapp` phải có icon RGB565 LE 2048 byte, manifest Lua và payload UTF-8 ≤64KiB.
3. Chép vào thẻ microSD tại `/System/Apps/Inbox/`; dùng `Menu > Applications > App installer` quét, xem icon + Signature verified và bấm Install. Mọi thành phần phải được ký và hash-check; không bypass chữ ký.
4. Vào `Applications` mở gói. `get()` kiểm chứng installed receipt; Lua beta trước khi khởi tạo cũng kiểm SHA256 của source so với receipt. Vùng mã tạm và heap Lua dùng PSRAM (192KiB VM heap limit).
5. D-pad/Start/Option được đưa vào Lua; Back/A/B là của OS. Khi Back thoát, **Back r2** mặc định No và giữ nguyên phiên Lua khi hủy; chỉ kết thúc VM sau khi xác nhận Yes hoặc điều hướng cưỡng bức. VM dừng khi lỗi, watchdog instruction 75.000/callback ~65ms, giới hạn 512 draw/callback và khoảng 20 FPS beta.

## Cổng kiểm thử trước khi phát hành beta thực tế

- Host: kiểm dual-key/P-256 signed type, đổi icon/payload, Lua parser stock reject, VM sandbox, drawable bounds, cấm OS key; kiểm hồi quy bộ cài stock và 160 ca Back r2.
- CI PlatformIO: build cả `vqeaf_os` (không Lua) và `vqeaf_lua_beta` (Lua test key **ephemeral**), kèm thống kê Flash/RAM; tuyệt đối không phát hành beta hardware-ready chỉ dựa trên CI.
- Trên ESP32-S3 thật: log Serial 115200 từ boot; chụp hình icon 32×32 ở App Installer + Applications; xác nhận cài và mở đúng app do **cùng khóa** ký; thử Back Yes/No, reboot 100 lần, chạy ≥10 phút, app nguồn sai, key sai, copy lỗi SD, thẻ bị tháo, WiFi, màu RGB565, OOM, timeout, FPS và thời gian phím.

**Giới hạn an toàn:** việc xác minh lại source sau `get()` giảm rủi ro thay đổi file nhưng SD/FAT vẫn có race vật lý khó triệt tiêu. Beta runtime không phải sandbox đa người dùng hoặc bộ chứng thực phần mềm tin cậy. Việc nâng cấp không bảo đảm chạy toàn bộ ứng dụng được Studio hỗ trợ trong tương lai; API ngoài danh sách hiện chưa triển khai.


## Kiểm thử CI đã xác minh (trên GitHub, chưa phải thiết bị thật)

- [QEAPP Studio Lua beta CI #36133221541](https://github.com/qeafivels/VQEAF-OS/actions/runs/36133221541): **PASS** toàn bộ bước gồm khóa ký kép/parser/installer stock/Back r2, xác minh nguồn Lua 5.4.8, chạy Lua VM thật trên host, build PlatformIO cả firmware stock `vqeaf_os` và beta `vqeaf_lua_beta` với khóa beta CI tạm.
- [Stock PlatformIO CI #36133221555](https://github.com/qeafivels/VQEAF-OS/actions/runs/36133221555): **PASS**. Chạy trên branch `feat/qeapp-lua-compat-v251`, commit `ab68bd0`.
- Artifact beta CI **chỉ chứng minh biên dịch**: được provision với khóa thử nghiệm **khác** khóa riêng của bạn và không thể xác minh gói Lua mà bạn đã ký. Không lấy artifact CI này để thay thế firmware cá nhân.

## Tạo gói beta .img tương thích khóa ứng dụng của bạn (tùy chọn)

Chỉ thực hiện **trên PC cá nhân**, sau khi đã chạy test, bootstrap nguồn Lua chính thức và provision từ **đúng PEM** dùng ký gói. Trên Windows PowerShell:

```powershell
# Đang ở checkout VQEAF-OS nhánh feat/qeapp-lua-compat-v251.
# Chỉ provision MỘT LẦN nếu header beta chưa tồn tại:
py -3 tools/provision_lua_beta_key.py --existing-private "D:\\SecureKeys\\qeapp_private.pem"
pio run -e vqeaf_lua_beta

# Tạo ảnh Flash cài sạch chỉ chứa firmware beta đã ký tin cậy bởi public key của bạn:
py -3 tools/build_factory_img.py --project-root . --build-dir .pio/build/vqeaf_lua_beta --output dist/VQEAF-OS_v251_LuaBeta_PERSONAL_factory.img
py -3 tools/build_factory_img.py --verify-only --output dist/VQEAF-OS_v251_LuaBeta_PERSONAL_factory.img
Get-FileHash dist/VQEAF-OS_v251_LuaBeta_PERSONAL_factory.img -Algorithm SHA256
```

Lưu ý: nếu đã provision rồi, **bỏ qua lệnh provision** (tool cố ý từ chối ghi đè). Không bao giờ commit `QeappTrustKeyLuaBeta.h` tùy chỉnh hoặc PEM riêng, đưa chúng lên GitHub Actions hay chia sẻ bản firmware cá nhân khi chưa quản lý quyền ký.

**CẢNH BÁO MẤT DỮ LIỆU:** ảnh `PERSONAL_factory.img` là **16 MiB toàn bộ Flash** để cài mới, sẽ ghi đè NVS, OTA, LittleFS và dữ liệu trên Flash. Sao lưu đầy đủ và xác nhận phần cứng N16R8 trước khi ghi. Muốn giữ dữ liệu ứng dụng, dùng `pio run -e vqeaf_lua_beta -t upload --upload-port COM3` sau khi sao lưu và xác minh layout, **không** nạp factory IMG.

Bước nghiệm thu còn thiếu: ảnh thực tế của icon trong App Installer/Applications, chạy đúng tệp `app_tpmc8i2i.qeapp` với cùng PEM, thao tác Back (Yes/No), SD/WiFi, Serial 115200, FPS và heap đo trên ESP32-S3 thật.
