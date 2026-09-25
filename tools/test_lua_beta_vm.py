#!/usr/bin/env python3
"""Compile the exact firmware Lua runtime with the hash-verified official Lua C library.
Host-only: not a claim of ESP32-S3/PSRAM or LCD acceptance.
"""
from __future__ import annotations
import subprocess
import tempfile
from pathlib import Path
from bootstrap_lua import CORE
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/'lib/VqeafLua54/src'

def run(args):
    p=subprocess.run(list(map(str,args)),capture_output=True,text=True,timeout=120)
    if p.returncode:
        raise RuntimeError('FAILED: '+str(args[0])+'\n'+(p.stderr+p.stdout)[-4500:])

if __name__ == '__main__':
    missing=CORE-set(p.name for p in SRC.glob('*.c'))
    if missing:
        raise SystemExit('NOT_RUN: bootstrap official Lua 5.4.8 first; missing '+','.join(sorted(missing)))
    with tempfile.TemporaryDirectory(prefix='qe-lua-vm-host-') as dir:
        t=Path(dir);objects=[]
        for name in sorted(CORE):
            obj=t/(name+'.o');run(['gcc','-O1','-std=c99','-I'+str(SRC),'-c',SRC/name,'-o',obj]);objects.append(obj)
        vm=t/'vm.o';test=t/'test.o';exe=t/'test_vm'
        flags=['-DVQEAF_ENABLE_LUA=1','-I'+str(SRC),'-I'+str(ROOT/'src/lua'),'-std=c++17']
        run(['g++',*flags,'-c',ROOT/'src/lua/QeLuaRuntime.cpp','-o',vm])
        run(['g++',*flags,'-c',ROOT/'tools/lua_beta_host/test_runtime.cpp','-o',test])
        run(['g++',vm,test,*objects,'-lm','-o',exe])
        run([exe]);print('PASS Lua 5.4.8 host runtime tests; ESP32/PSRAM/LCD NOT_RUN')
