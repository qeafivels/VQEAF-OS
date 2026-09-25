#!/usr/bin/env python3
"""Raster-test the real SymbianUI statusbar C++ and publish a cropped visual comparison.

PC-side TFT framebuffer & test WiFi are mocks; output is not an LCD photograph.
"""
import subprocess, tempfile
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

R=Path(__file__).resolve().parents[1]
P=R/"preview"
P.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix="vqeaf-statusbar-") as tmp:
    binary=Path(tmp)/"statusbar_test"
    cmd=["g++","-std=c++11","-fpermissive","-O2","-Wall","-Wextra","-Werror",
         "-DVQEAF_HOST_WIFI_TEST",
         "-Itools/vqeaf_host/reference_stubs","-Itools/host_stubs",
         "-Iinclude","-Isrc","-Isrc/core",
         "tools/vqeaf_host/test_statusbar_icons.cpp",
         "src/core/SymbianUI.cpp","src/core/VqeafIconRenderer.cpp",
         "tools/host_stubs/host_globals.cpp",
         "-o",str(binary)]
    subprocess.run(cmd,cwd=R,check=True)
    subprocess.run([str(binary),str(P)+"/"],cwd=R,check=True)

labels=[("Strong WiFi / S60 Green","statusbar_large_green"),
        ("Strong WiFi / Night","statusbar_large_night"),
        ("Weak WiFi / S60 Green","statusbar_low_green"),
        ("Weak WiFi / Night","statusbar_low_night")]
fpath="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
font=ImageFont.truetype(fpath,19) if Path(fpath).exists() else ImageFont.load_default()
scale=4
w,h=240*scale,27*scale
canvas=Image.new("RGB",(2*(w+30)+30,2*(h+56)+20),"#162221")
d=ImageDraw.Draw(canvas)
for i,(title,name) in enumerate(labels):
    img=Image.open(P/(name+".ppm")).convert("RGB")
    assert img.size==(240,320)
    img.save(P/(name+".png"))
    left=30+(i%2)*(w+30)
    top=18+(i//2)*(h+56)
    d.text((left,top),title,font=font,fill="#E9F4ED")
    canvas.paste(img.crop((0,0,240,27)).resize((w,h),Image.Resampling.NEAREST),
                 (left,top+30))
canvas.save(P/"vqeaf_large_statusbar_comparison.png")
print("PASS: actual OS WiFi+battery 18px layout / no clock collision / partial redraw")
print("OUTPUT:",P/"vqeaf_large_statusbar_comparison.png")
