#!/usr/bin/env python3
"""Verify local QEAPP signature then run its Lua on the exact firmware VM on Windows.
This is a HOST integration gate; it cannot read the device's microSD over COM3.
"""
import argparse,sys,subprocess,tempfile,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from bootstrap_lua import CORE
from test_lua_qeapp_uart import verify_user_sample
ap=argparse.ArgumentParser(description=__doc__)
ap.add_argument('--package',required=True,type=Path)
a=ap.parse_args()
pub=ROOT/'src/services/QeappTrustKeyLuaBeta.h'
verify_user_sample(a.package,pub)
data=a.package.read_bytes();m,i,s=struct.unpack_from('<III',data,8)
script=data[116+m+i:116+m+i+s]
src=ROOT/'lib/VqeafLua54/src'
with tempfile.TemporaryDirectory(prefix='qeapp-real-host-') as temp:
 t=Path(temp)
 code=t/'verified_signed_source.lua';code.write_bytes(script)
 objects=[]
 def run(args):
  proc=subprocess.run([str(x) for x in args],capture_output=True,text=True)
  if proc.returncode:raise RuntimeError((' '.join(str(x) for x in args[:3])+'\n'+proc.stdout+'\n'+proc.stderr)[-1800:])
  if proc.stdout:print(proc.stdout.strip(),flush=True)
 for name in sorted(CORE):
  obj=t/(name+'.o');run(['gcc','-O1','-std=c99','-I'+str(src),'-c',src/name,'-o',obj]);objects.append(obj)
 flags=['-DVQEAF_ENABLE_LUA=1','-I'+str(src),'-I'+str(ROOT/'src/lua'),'-std=c++17']
 vm=t/'vm.o';driver=t/'driver.o';exe=t/'qeapp_host.exe'
 run(['g++',*flags,'-c',ROOT/'src/lua/QeLuaRuntime.cpp','-o',vm])
 run(['g++',*flags,'-c',ROOT/'tools/lua_beta_host/test_real_package_runtime.cpp','-o',driver])
 run(['g++',vm,driver,*objects,'-lm','-o',exe])
 run([exe,code])
 print('NOTE: host test does not launch a device SD app or validate real TFT/PSRAM.')
