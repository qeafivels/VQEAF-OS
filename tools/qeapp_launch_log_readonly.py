#!/usr/bin/env python3
"""Read-only QEAPP launch capture, never sends serial bytes or writes device Flash."""
import argparse,datetime as dt,json,re,time
from pathlib import Path
import serial
CATEGORIES={
 "safe_mode":re.compile(r"safe_mode=1|reason=SAFE_MODE|Safe Mode blocks Lua",re.I),
 "signature":re.compile(r"signature|receipt|sha.?256|hash mismatch|unsigned|trust key",re.I),
 "sd_storage":re.compile(r"sd_unmounted|SD.*(removed|NOT_MOUNTED|error)|payload unreadable|Cannot open",re.I),
 "lua_runtime":re.compile(r"\[VQEAF\]\[LUA\].*(started|launch rejected|callback rejected|STATUS|FRAME|PROBE)|Lua.*(error|failed)",re.I),
 "qeapp_fail":re.compile(r"\[VQEAF\]\[QEAPP\]\[LAUNCH_FAIL\]|\[QEAPP\]\[INSTALL\]",re.I),
 "power_reset":re.compile(r"Brownout|rst:0x|ESP-ROM:|Guru Meditation|watchdog|PREVIOUS_RESET",re.I),
}
def analyze(rows):
 matches={k:[] for k in CATEGORIES}
 for ts,line in rows:
  for name,pat in CATEGORIES.items():
   if pat.search(line):matches[name].append({"utc":ts,"line":line[:400]})
 return {"serial_lines":len(rows),"events":{k:len(v) for k,v in matches.items()},
         "details":matches,"limitations":"Log-only: no launch path conclusion if app not opened during capture. Timestamp is Windows receive time."}
def main():
 pa=argparse.ArgumentParser()
 pa.add_argument("--port",default="COM3")
 pa.add_argument("--seconds",type=int,default=120)
 pa.add_argument("--out",type=Path,default=Path("local_hw_results/qeapp_launch_capture"))
 args=pa.parse_args();args.out.mkdir(parents=True,exist_ok=True)
 raw=args.out/"serial_timestamped.log"
 print("READ ONLY; opening CH340 could still electrically reset the board.",flush=True)
 port=serial.Serial(port=None,baudrate=115200,timeout=.25)
 port.dtr=False;port.rts=False;port.port=args.port
 rows=[];error=None
 try:
  port.open()
  with raw.open("w",encoding="utf8") as dst:
   t0=time.monotonic()
   while time.monotonic()-t0<args.seconds:
    try:payload=port.readline()
    except serial.SerialException as exc:
     error=type(exc).__name__;print("PORT_INTERRUPTED",error,flush=True);break
    if not payload:continue
    ts=dt.datetime.now(dt.timezone.utc).isoformat()
    line=payload.decode(errors="replace").rstrip("\r\n")
    dst.write(ts+" "+line+"\n");dst.flush()
    rows.append((ts,line))
    if any(p.search(line) for p in CATEGORIES.values()):print(ts,line[:220],flush=True)
 except serial.SerialException as exc:error=type(exc).__name__
 finally:
  if port.is_open:port.close()
 report=analyze(rows);report.update(complete=error is None,error=error,port=args.port)
 (args.out/"triage.json").write_text(json.dumps(report,indent=2,ensure_ascii=False)+"\n",encoding="utf8")
 print("RESULT",json.dumps({"complete":report["complete"],"serial_lines":len(rows),"event_counts":report["events"],"error":error}),flush=True)
 print("SAVED",args.out,flush=True)
if __name__=="__main__":main()
