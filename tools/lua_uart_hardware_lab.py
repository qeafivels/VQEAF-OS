#!/usr/bin/env python3
"""Read-only CH340 acceptance for the trusted Lua QEAPP/2 firmware.
Runs only a fixed embedded diagnostic; does not upload unsigned apps or read files.
"""
import argparse
import re
import sys
import time
import serial

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("--port",default="COM3")
    p.add_argument("--baud",type=int,default=115200)
    p.add_argument("--boot-settle",type=float,default=7.0)
    a=p.parse_args()
    try:
        with serial.Serial(a.port,a.baud,timeout=.20,write_timeout=2) as port:
            def gather(seconds,needle):
                end=time.monotonic()+seconds
                buf=bytearray()
                while time.monotonic()<end:
                    buf.extend(port.read(4096))
                    if len(buf)>64000:
                        buf=buf[-32000:]
                    lines=buf.decode("utf-8",errors="replace").splitlines()
                    for line in reversed(lines):
                        if needle in line and line.endswith("\r")==False:
                            return line.strip()
                return ""
            # Opening a CH340 handle may reset the board via DTR.
            gather(max(0,a.boot_settle),"[VQEAF][LUA] runtime=ENABLED")
            status=""
            for _ in range(2):
                port.write(b"diag lua status\n")
                status=gather(6,"[VQEAF][LUA][STATUS]")
                if status:break
            if not status or "enabled=1" not in status:
                print("FAIL Lua profile not observable on this port",file=sys.stderr)
                return 2
            # Exact parsed safe fields only; no URLs, private app names or raw log.
            fields=dict(re.findall(r"([a-z_]+)=(\d+)",status))
            print("LUA_STATUS", "enabled=1",
                  "psram_free="+fields.get("psram_free","unknown"),
                  "signed_lua_installed="+fields.get("signed_lua_installed","unknown"),
                  "safe_mode="+fields.get("safe_mode","unknown"),
                  "sd="+fields.get("sd","unknown"))
            if fields.get("vm_running")=="1":
                print("INCONCLUSIVE A Lua app is running; do not interrupt it")
                return 3
            port.write(b"diag lua probe\n")
            result=gather(8,"[VQEAF][LUA][PROBE]")
            if not result:
                print("INCONCLUSIVE no synthetic VM result received")
                return 3
            if "result=SKIP" in result:
                print("INCONCLUSIVE",result)
                return 3
            if "result=PASS" not in result:
                print("FAIL",result,file=sys.stderr)
                return 1
            print("PASS",result)
            if fields.get("safe_mode")=="1":
                print("WARNING Safe Mode blocks normal Lua app launch")
            if fields.get("signed_lua_installed")=="0":
                print("WARNING no signed Lua app in device catalog; package launch remains UNTESTED")
            return 0
    except serial.SerialException as e:
        print("FAIL serial connection:",e,file=sys.stderr)
        return 2

if __name__=="__main__":
    raise SystemExit(main())
