#!/usr/bin/env python3
"""Read-only postflash diagnostics on existing trusted firmware over COM3.
Runs the built-in RAM-only Lua synthetic probe. Never installs apps or writes device storage.
"""
import argparse,json,time,datetime as dt
from pathlib import Path
import serial
pa=argparse.ArgumentParser(description=__doc__)
pa.add_argument("--port",default="COM3");pa.add_argument("--out",type=Path,
 default=Path("local_hw_results/postflash_uart_acceptance.json"))
args=pa.parse_args()
port=serial.Serial(port=None,baudrate=115200,timeout=.3,write_timeout=2)
port.dtr=False;port.rts=False;port.port=args.port;port.open()
checks=[
 ("diag lua status","[VQEAF][LUA][STATUS]",3.0),
 ("diag sd status","[S3DIAG][SD]",3.0),
 ("diag clock","[VQEAF][CLOCK]",3.0),
 ("diag keys","[VQEAF][KEYS]",3.0),
 ("diag theme status","[VQEAF][THEME][STATUS]",3.0),
 ("diag app icons","[VQEAF][QEAPP][ICONS] result=",7.0),
 ("diag lua probe","[VQEAF][LUA][PROBE]",9.0)]
report={"utc":dt.datetime.now(dt.timezone.utc).isoformat(),"port":args.port,"results":{}}
try:
 for cmd,prefix,timeout in checks:
  port.reset_input_buffer();port.write((cmd+"\n").encode());port.flush()
  lines=[];start=time.monotonic();seen=False
  while time.monotonic()-start<timeout:
   data=port.readline()
   if not data:continue
   line=data.decode(errors="replace").strip()
   if line.startswith(prefix):
    lines.append(line[:350]);seen=True
    if cmd!="diag app icons" or " result=" in line:break
  report["results"][cmd]={"seen":seen,"lines":lines}
  print(cmd,"=>",lines or "NOT_SUPPORTED_OR_NO_REPLY",flush=True)
finally:port.close()
s=report["results"]
lua=" ".join(s["diag lua status"]["lines"])
probe=" ".join(s["diag lua probe"]["lines"])
report["gate_signed_lua"]=(s["diag lua status"]["seen"] and "enabled=1" in lua and
 "signed_lua_installed=1" in lua and "safe_mode=0" in lua)
report["gate_synthetic_lua"]=(s["diag lua probe"]["seen"] and "result=PASS" in probe)
report["gate_sd"]=("mounted" in " ".join(s["diag sd status"]["lines"]))
report["gate_clock"]=s["diag clock"]["seen"]
report["gate_icons"]="result=PASS" in " ".join(s["diag app icons"]["lines"])
report["limitations"]="This checks only UART/status/host synthetic Lua. Does not measure power, D-pad presses, physical display or open a signed QEAPP interactively."
args.out.parent.mkdir(parents=True,exist_ok=True)
args.out.write_text(json.dumps(report,ensure_ascii=False,indent=2)+"\n",encoding="utf8")
print("SAVED",args.out,flush=True)
