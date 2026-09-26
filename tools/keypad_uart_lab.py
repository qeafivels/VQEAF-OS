#!/usr/bin/env python3
"""Read-only physical ESP32-S3 keypad lab, CH340 COM3 / UART0 at 115200.

Nothing in this tool synthesizes presses or alters firmware/SD/NVS. Real
buttons must be pressed by a human while this script captures raw GPIO masks.
"""
import argparse
import re
import time

NAMES=("MENU","UP","A","LEFT","START","RIGHT","OPTION","DOWN","B","SELECT")
LINE=re.compile(r"\[VQEAF\]\[KEYS\] pressed_mask=0x([0-9A-Fa-f]{1,3})")
def decode(payload):
    frames=[]
    for m in LINE.finditer(payload):
        v=int(m.group(1),16)
        if v<1024:frames.append(v)
    return frames

def sample(port,seconds=1.2):
    port.reset_input_buffer()
    end=time.monotonic()+seconds
    frames=[]
    while time.monotonic()<end:
        port.write(b"diag keys\n")
        stop=time.monotonic()+0.14
        buffer=bytearray()
        while time.monotonic()<stop:
            buffer.extend(port.read(256))
        frames.extend(decode(buffer.decode("ascii",errors="ignore")))
    return frames

def exercise(port):
    assert any(v==0 for v in sample(port,1.0)),(
        "Release ALL keys first. Baseline should report 0x000.")
    passed=[]
    for i,name in enumerate(NAMES):
        input(f"\nRelease all keys. ENTER then hold ONLY {name} for 2 seconds: ")
        frames=sample(port,2.1)
        desired=1<<i
        good=sum(v==desired for v in frames)
        leaks=sum(v & ~desired != 0 for v in frames)
        ok=good>=2 and leaks==0
        print(f"{name:7s}: {'PASS' if ok else 'FAIL'} "
              f"frames={len(frames)} exact={good} unwanted={leaks}")
        passed.append(ok)
        input(f"Release {name}, then press ENTER: ")
        released=sample(port,.6)
        if not any(v==0 for v in released):
            print(f"RELEASE FAIL: {name} still active or serial lost")
            passed[-1]=False
    print("\nPHYSICAL RESULT",sum(passed),"/",len(NAMES),"passed")
    return all(passed)

def main():
    a=argparse.ArgumentParser()
    a.add_argument("--port",default="COM3")
    a.add_argument("--baud",type=int,default=115200)
    a.add_argument("--selftest",action="store_true")
    args=a.parse_args()
    if args.selftest:
        payload="noise\n[VQEAF][KEYS] pressed_mask=0x000 MENU=0\n"
        assert decode(payload)==[0]
        for i in range(10):
            assert decode(f"[VQEAF][KEYS] pressed_mask=0x{1<<i:03X}\n")==[1<<i]
        assert decode("[VQEAF][KEYS] pressed_mask=0xFFF")==[]
        print("PASS serial GPIO parser, release state, all 10 distinct buttons")
        return
    import serial
    with serial.Serial(args.port,args.baud,timeout=.1) as port:
        port.dtr=False;port.rts=False
        if not exercise(port):raise SystemExit(1)
if __name__=="__main__":main()
