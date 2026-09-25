#!/usr/bin/env python3
"""System-wide compiled-host visual/logic soak. Host clock != ESP32 clock.

Intended to run AFTER tools/run_full_system_review.py, so the 15 independent
OS gates and production C++ screens can be included in one photo contact sheet.
Physical LCD, GPIO signal and oscillator measurement require a board.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import json, os, re, subprocess, tempfile, time

R=Path(__file__).resolve().parents[1]
P=R/"preview"
O=R/"build_reports"/"system_stress"
P.mkdir(exist_ok=True)
O.mkdir(parents=True,exist_ok=True)

config=(R/"platformio.ini").read_text()
m=re.search(r"^board_build\.f_cpu\s*=\s*(\d+)L?\s*$",config,re.M)
assert m, "PlatformIO clock explicitly configured"
configured_hz=int(m.group(1))
assert configured_hz==240_000_000, "Unexpected board CPU clock setting"
# Boot/microcontroller PLL confirmation is not simulated.
compiled_output=""
with tempfile.TemporaryDirectory(prefix="vqeaf-full-system-host-") as td:
    exe=Path(td)/"sketch_visual"
    cmd=["g++","-std=c++11","-fpermissive","-O2","-Wall","-Wextra","-Werror",
         "-Itools/vqeaf_host/reference_stubs","-Itools/host_stubs",
         "-Iinclude","-Isrc","-Isrc/core",
         "tools/legacy_host/test_full_system_visual_soak.cpp",
         "src/core/SymbianUI.cpp","src/core/VqeafIconRenderer.cpp",
         "tools/host_stubs/host_globals.cpp",
         "-o",str(exe)]
    p=subprocess.run(cmd,cwd=R,text=True,capture_output=True,timeout=140)
    (O/"compile.log").write_text(p.stdout+p.stderr)
    if p.returncode:
        print(p.stdout+p.stderr,flush=True)
        raise SystemExit(p.returncode)
    before=time.monotonic()
    p=subprocess.run([str(exe),str(P)+"/"],cwd=R,text=True,
                     capture_output=True,timeout=140)
    run_s=time.monotonic()-before
    compiled_output=p.stdout+p.stderr
    (O/"host_soak.log").write_text(compiled_output)
    print(compiled_output,flush=True)
    if p.returncode:
        raise SystemExit(p.returncode)

number=re.search(
    r"HOST_STRESS edits=(\d+) undo=(\d+) save_restore=(\d+) corrupt_rejected=(\d+)"
    r" model_seconds=([.\d]+) redraws=(\d+) redraw_seconds=([.\d]+)"
    r" redraw_median_ms=([.\d]+) redraw_p95_ms=([.\d]+)"
    r" full_screen_fills=(\d+)",compiled_output)
assert number, "Host compiled benchmark summary absent"
edits,undos,restores,rejected,model_seconds,redraws,redraw_seconds,median,p95,fullfills=number.groups()
assert int(edits)==20_000 and int(redraws)==240
assert int(restores)==5000 and int(rejected)>1000
assert int(fullfills)==0

stems=["sketch_01_empty_green","sketch_02_strokes_green",
       "sketch_03_loaded_night","sketch_04_eraser_night"]
for s in stems:
    im=Image.open(P/(s+".ppm")).convert("RGB")
    assert im.size==(240,320),s
    im.save(P/(s+".png"))

prev=R/"build_reports"/"system_full"/"report.json"
assert prev.exists(),"Run the 15 full OS regression gates before visual soak"
full=json.loads(prev.read_text())
assert full["status"]=="PASS" and len(full["checks"])==15

# Actual C++ rendering in all screen captures, but several app/menu screens
# intentionally build named UI states rather than execute installed apps.
gallery=[
 ("HOME / actual system renderer","v23_home_host_raster.png"),
 ("MENU / actual system renderer","v23_menu_host_raster.png"),
 ("BROWSER / keyboard","browser_keyboard.png"),
 ("WIFI / masked input","wifi_keyboard_masked.png"),
 ("EVENTS / notification widgets","system_notifications.png"),
 ("INSTALL / verified UI state","installer_revisit_open.png"),
 ("INSTALLED / open popup","installed_apps_open_menu.png"),
 ("STATUS / strong WIFI","statusbar_large_green.png"),
 ("SKETCH / page 1 empty","sketch_01_empty_green.png"),
 ("SKETCH / ink + graphite","sketch_02_strokes_green.png"),
 ("SKETCH / restore + night","sketch_03_loaded_night.png"),
 ("SKETCH / erase + night","sketch_04_eraser_night.png")
]
fontfile="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
font=ImageFont.truetype(fontfile,14) if Path(fontfile).exists() else ImageFont.load_default()
scale=2; w=240*scale; h=320*scale
sheet=Image.new("RGB",(4*(w+18)+18,3*(h+64)+65),"#17222B")
d=ImageDraw.Draw(sheet)
for i,(label,filename) in enumerate(gallery):
    image=Image.open(P/filename).convert("RGB")
    assert image.size==(240,320),(filename,image.size)
    x=18+i%4*(w+18);y=16+i//4*(h+64)
    d.text((x,y),label,font=font,fill="#E4F3F8")
    sheet.paste(image.resize((w,h),Image.Resampling.NEAREST),(x,y+24))
d.text((18,sheet.height-28),
    "HOST CAPTURE - REAL C++ DRAW PATH - NOT LCD PHOTOS; PC REFERENCE FONT; CPU 240 MHz IS CONFIG ONLY",
    font=font,fill="#FFDC98")
out=P/"vqeaf_full_system_12screens_stress.png"
sheet.save(out)

report={
 "commit":os.getenv("GITHUB_SHA","local_checkout"),
 "status":"PASS",
 "environment":"Linux host / RGB565 TFT framebuffer shim + production OS source + CI PlatformIO",
 "physical_board_connected":False,
 "lcd_photos":False,
 "image_renderer":"Real production C++ SymbianUI + SketchpadRenderer; reference PC glyph bitmap",
 "configured_cpu_hz":configured_hz,
 "measured_esp32_cpu_hz":None,
 "measured_esp32_fps":None,
 "host_render_median_ms":float(median),
 "host_render_p95_ms":float(p95),
 "host_render_seconds_for_240":float(redraw_seconds),
 "host_elapsed_wall_s":round(run_s,3),
 "edit_cycles":int(edits),"undo_cycles":int(undos),
 "save_reload_roundtrips":int(restores),"corrupted_payloads_rejected":int(rejected),
 "host_full_lcd_fills_during_240_redraws":int(fullfills),
 "os_regression_gates_passed":len(full["checks"]),
 "os_regression_gates_total":15,
 "host_captured_screens":len(gallery),
 "hardware_not_tested":[
   "Actual oscillator and thermal frequency throttling",
   "ESP32-S3 SPI TFT transport and glass pixel response",
   "Physical keypad scanning / press-to-photon latency",
   "microSD write endurance, power-cut and physical hotplug",
   "USB VBUS sensing and BLE/WiFi radio coexistence over hours",
   "Actual audio I2S output without confirmed codec/amplifier",
   "True runtime FPS, clock and heap/PSRAM telemetry"
 ]
}
(O/"report.json").write_text(json.dumps(report,ensure_ascii=False,indent=2)+"\n")
(O/"REPORT_VN.md").write_text(
 f"# VQEAF-OS — Kiểm thử hệ điều hành toàn diện (host + CI)\n\n"
 f"Commit: \`{report['commit']}\`\n\n"
 f"- **15/15** bài kiểm thử OS hiện có: PASS.\n"
 f"- **20.000** lượt chỉnh sửa bản vẽ; **{undos}** Undo; "
 f"**{restores}** lần tuần tự hóa/khôi phục: PASS.\n"
 f"- **{rejected}** payload hỏng checksum bị từ chối, không thay thế dữ liệu cũ: PASS.\n"
 f"- **240** lần dựng lại giao diện C++ cùng chủ đề S60/Night, "
 f"**{fullfills}** lần tô lại toàn LCD: PASS.\n"
 f"- Đã lưu **12** màn hình framebuffer 240x320; font PC là shim tham chiếu.\n"
 f"- Thời gian vẽ *máy chủ CI* (không phải ESP32): trung vị "
 f"**{float(median):.3f}ms**, p95 **{float(p95):.3f}ms**; "
 f"phụ thuộc CPU runner và không dự đoán FPS trên ST7789.\n"
 f"- Cấu hình CPU ESP32-S3 từ platformio.ini: **{configured_hz//1_000_000} MHz**. "
 f"Không có đo xung nhịp thật hoặc throttle / điện áp / nhiệt.\n"
 f"- Ảnh: \`preview/vqeaf_full_system_12screens_stress.png\`.\n\n"
 "Các bước *chưa thực hiện*: nạp board thực, đo FPS trên LCD, thời gian phản hồi phím "
 "và chạy tải trên chip, đo heap/PSRAM trong nhiều giờ, USB/microSD thật. "
 "Để đo thật: build \`vqeaf_perf_diag\`, chạy \`tools/RUN_V251_DEVICE_BENCHMARK.bat\` "
 "qua Serial 115200 và chụp trực tiếp LCD.\n",
 encoding="utf8")
print("PASS: full 15-gate host OS + extended 20k edit / 240 raster stress")
print("OUTPUT:",out)
