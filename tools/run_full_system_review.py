#!/usr/bin/env python3
"""Full HOST/CI VQEAF system regression and genuine C++ RGB565 UI screenshots.

Runs compiled firmware modules and host shims, NOT the ESP32-S3 hardware.
Compiling the real embedded target is performed separately by the workflow.
"""
import json,subprocess,sys,time
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
R=Path(__file__).resolve().parents[1]
O=R/"build_reports"/"system_full"
O.mkdir(parents=True,exist_ok=True)
P=R/"preview"
P.mkdir(exist_ok=True)
results=[]
def run(name,cmd,timeout=900):
  print("RUN",name,flush=True)
  started=time.monotonic()
  try:
    p=subprocess.run([str(x) for x in cmd],cwd=R,text=True,
        stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=timeout)
    out=p.stdout
    good=p.returncode==0
    code=p.returncode
  except subprocess.TimeoutExpired as e:
    out=str(e)
    good=False
    code=124
  (O/(name+".log")).write_text(out,encoding="utf8",errors="replace")
  results.append({"test":name,"status":"PASS" if good else "FAIL","exit_code":code,
                  "duration_s":round(time.monotonic()-started,2)})
  print(results[-1]["status"],name,out[-950:],flush=True)
  return good

def sheet():
  images=[
    ("HOME / production SymbianUI.cpp",P/"v23_home_host_raster.png"),
    ("APP MENU / production SymbianUI.cpp",P/"v23_menu_host_raster.png"),
    ("BROWSER QWERTY / production TextKeyboard.cpp",P/"browser_keyboard.png"),
    ("D-PAD FOCUS / production TextKeyboard.cpp",P/"browser_keyboard_focus.png"),
    ("WiFi PASSWORD / masked input",P/"wifi_keyboard_masked.png"),
    ("SYSTEM NOTIFICATIONS / production UI widgets",P/"system_notifications.png"),
  ]
  images=[(n,Image.open(f).convert("RGB")) for n,f in images if f.exists()]
  if not images: return
  fpath="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
  font=ImageFont.truetype(fpath,12) if Path(fpath).exists() else ImageFont.load_default()
  scale=2
  w,h=240*scale,320*scale
  page=Image.new("RGB",(3*(w+18)+18,2*(h+74)+70),"#1d2924")
  d=ImageDraw.Draw(page)
  for i,(title,im) in enumerate(images):
    x=18+(i%3)*(w+18);y=15+(i//3)*(h+74)
    d.text((x,y),title,font=font,fill="#f2f7ed")
    page.paste(im.resize((w,h),Image.Resampling.NEAREST),(x,y+26))
  d.text((18,page.height-19),
    "C++ HOST RGB565 OUTPUT - NO PHYSICAL DEVICE - MOCK TFT DOES NOT RASTERIZE TEXT GLYPHS",
    font=font,fill="#fbdba2")
  page.save(P/"vqeaf_full_system_host_capture.png")
  for title,im in images:
    assert im.size==(240,320),(title,im.size)
  print("OUTPUT preview/vqeaf_full_system_host_capture.png",flush=True)

compile_command=[
  "g++","-std=c++11","-fpermissive","-Wall","-Wextra","-Werror",
  "-Itools/vqeaf_host/preview_stubs","-Itools/host_stubs",
  "-Iinclude","-Isrc","-Isrc/core",
  "tools/vqeaf_host/test_system_raster.cpp",
  "src/core/SymbianUI.cpp","src/core/VqeafIconRenderer.cpp",
  "src/core/TextKeyboard.cpp","src/services/NotificationService.cpp",
  "tools/host_stubs/host_globals.cpp",
  "-o",str(O/"system_raster")]
gates=[
  ("compile_real_cpp_gui_and_keyboard",compile_command),
  ("capture_real_cpp_qwerty_and_notifications",[O/"system_raster",str(P)+"/"]),
  ("reference_rgb565_layout_and_glyph_tests",[sys.executable,"tools/test_ui_v23.py"]),
  ("actual_browser_keyboard_events",[sys.executable,"tools/test_browser_keyboard_events.py"]),
  ("usb_sd_debounce_and_os_integration",[sys.executable,"tools/test_system_keyboard_usb_sd.py"]),
  ("qeafbrowser_native_routes",[sys.executable,"tools/test_qeafbrowser_os.py"]),
  ("back_key_invariants",[sys.executable,"tools/test_backguard_v251.py"]),
  ("lua_frame_no_blank",[sys.executable,"tools/test_lua_frame_present.py"]),
  ("lua_transition_no_blank",[sys.executable,"tools/test_lua_transition_noblank.py"]),
  ("storage_and_verified_tls",[sys.executable,"tools/test_v20_storage_tls.py"]),
  ("memory_and_sd_stability",[sys.executable,"tools/test_v19_stability.py"]),
  ("installer_reset_regression",[sys.executable,"tools/test_v244_install_reset.py"]),
  ("full_v251_acceptance",[sys.executable,"tools/verify_v251.py","--full"]),
]
for name,cmd in gates:
  if not run(name,cmd):
    break
sheet()
report={
 "version":"VQEAF-OS v2.5.1 native Qeafbrowser + universal keyboard",
 "branch":"feat/system-vkeyboard-usb-sd-notices",
 "environment":"Linux host: actual C++ OS modules + Arduino/TFT mocks",
 "real_device_connected":False,
 "live_usb_vbus_test":"NOT_RUN - host link events only; charge-only cable cannot be detected",
 "live_micro_sd_hotplug":"NOT_RUN - requires physical board and safe idle card",
 "actual_lcd_photo":"NOT_AVAILABLE",
 "ui_capture":"REAL C++ host raster / fonts are host shim, not physical LCD",
 "status":"PASS" if len(results)==len(gates) and all(r["status"]=="PASS" for r in results) else "FAIL",
 "checks":results
}
(O/"report.json").write_text(json.dumps(report,indent=2,ensure_ascii=False)+"\n")
sys.exit(0 if report["status"]=="PASS" else 1)
