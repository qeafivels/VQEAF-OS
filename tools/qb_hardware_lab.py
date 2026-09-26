#!/usr/bin/env python3
"""Read-only ESP32-S3 serial FPS capture, or opt-in isolated reboot recovery.
Does not flash firmware, control keys or collect cookie values.
Usage:
  python tools/qb_hardware_lab.py --port COM3 --mode baseline --seconds 30
  python tools/qb_hardware_lab.py --port COM3 --mode roundtrip --seconds 60
Open the Browser Overview and operate D-Pad during the roundtrip capture.
"""
import argparse
import json
import re
import statistics
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

try:
    import serial
except ImportError:
    print("Install pyserial: python -m pip install pyserial", file=sys.stderr)
    raise SystemExit(2)

WHITELIST = (
    "[QB][PERF]", "[QB][RECOVERY]", "[VQEAF][FPS]", "[VQEAF][PERF]",
    "[S3DIAG][SD]", "[VQEAF][BUILD]", "[VQEAF][CORE][BROWSER]",
)
FIELDS = re.compile(r"([A-Za-z][A-Za-z0-9_]*)=(-?[A-Za-z0-9_./]+)")

def parse(line):
    return dict(FIELDS.findall(line))

def metrics(lines):
    def group(prefix):
        return [parse(s) for s in lines if s.startswith(prefix)]
    qb, fps, perf = group("[QB][PERF]"), group("[VQEAF][FPS]"), group("[VQEAF][PERF]")
    def values(records,key, positive=False):
        out=[]
        for d in records:
            try:
                value=float(d[key])
                if not positive or value>0:out.append(value)
            except (KeyError,ValueError):
                pass
        return out
    def summary(v):
        if not v:return None
        return dict(samples=len(v),min=min(v),median=statistics.median(v),max=max(v))
    return {
        "browser_animation_fps":summary(values(qb,"anim_fps",True)),
        "browser_perf_windows":len(qb),
        "input_dispatch_avg_us":summary(values(fps,"input_dispatch_avg_us",True)),
        "input_dispatch_p95_upper_us":summary(values(fps,"input_dispatch_p95_le_us",True)),
        "input_event_windows":sum(values(fps,"input_events")),
        "ui_navigation_avg_us":summary(values(fps,"nav_avg_us",True)),
        "loop_avg_us":summary(values(perf,"avg_loop_us",True)),
        "loop_max_us":summary(values(perf,"max_loop_us",True)),
        "free_psram_bytes":summary(values(perf,"psram",True)),
    }

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--port",default="COM3")
    ap.add_argument("--baud",type=int,default=115200)
    ap.add_argument("--mode",choices=["baseline","roundtrip","verify"],default="baseline")
    ap.add_argument("--seconds",type=int,default=30)
    ap.add_argument("--output",type=Path,default=Path("qb_hardware_results"))
    ap.add_argument("--keep-fixtures",action="store_true",help="Leave synthetic recovery artifacts for inspection")
    args=ap.parse_args()
    args.output.mkdir(parents=True,exist_ok=True)
    stamp=datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    lines=[]
    with serial.Serial(args.port,args.baud,timeout=.25,write_timeout=2) as port:
        # Opening CH340 may already toggle DTR; don't blindly write USB reset.
        pending=bytearray()
        def gather(seconds,expect=None):
            found=False
            stop=time.monotonic()+seconds
            while time.monotonic()<stop:
                payload=port.read(2048)
                pending.extend(payload)
                while b"\n" in pending:
                    raw,_,tail=pending.partition(b"\n");pending[:]=tail
                    line=raw.decode("utf-8",errors="replace").strip()
                    if any(line.startswith(p) for p in WHITELIST):
                        lines.append(line)
                        print(line)
                        if expect and expect in line:found=True
                if found:return True
            return found
        if args.mode=="roundtrip":
            port.write(b"diag qb stage\n")
            if not gather(12,"[QB][RECOVERY] stage=PASS"):
                print("INCONCLUSIVE: no staged diagnostic fixture; board may run older firmware")
            else:
                port.write(b"diag qb reboot\n")
                if not gather(14,"[QB][RECOVERY] reboot=REQUESTED"):
                    print("INCONCLUSIVE: opt-in reboot not acknowledged")
                else:
                    # UART CH340 can stop producing bytes during restart.
                    boot=gather(25,"[VQEAF][BUILD]")
                    port.write(b"diag qb verify\n")
                    verified=gather(12,"[QB][RECOVERY] verify=PASS")
                    print("RECOVERY", "PASS" if verified and boot else "INCONCLUSIVE",
                          "boot_seen=",boot,"verify_pass=",verified)
                    if verified and not args.keep_fixtures:
                        port.write(b"diag qb cleanup\n")
                        gather(5,"[QB][RECOVERY] cleanup=COMPLETE")
        elif args.mode=="verify":
            port.write(b"diag qb verify\n")
            gather(12,"[QB][RECOVERY] verify=")
        if args.seconds>0:
            print("Operate the physical keypad in Browser Overview now, when testing browser FPS.")
            gather(args.seconds)
    report={
        "timestamp_utc":stamp,"port":args.port,"mode":args.mode,
        "seconds_requested":args.seconds,"metrics":metrics(lines),
        "recovery_lines":[s for s in lines if s.startswith("[QB][RECOVERY]")],
        "note":"FPS counts changed document animation frames, not LCD photon rate; "+
               "input_dispatch metrics exclude hardware interrupt-to-photon delay."
    }
    log=args.output/("qb_"+args.mode+"_"+stamp+".log")
    summary=log.with_suffix(".json")
    log.write_text("\n".join(lines)+"\n",encoding="utf-8")
    summary.write_text(json.dumps(report,indent=2),encoding="utf-8")
    print("SERIAL_LOG",log)
    print("METRICS_JSON",summary)
    print("METRICS",json.dumps(report["metrics"],indent=2))
    if args.mode=="roundtrip" and not any("[QB][RECOVERY] verify=PASS" in s for s in lines):
        return 1
    return 0

if __name__=="__main__":
    raise SystemExit(main())
