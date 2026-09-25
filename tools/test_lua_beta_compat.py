#!/usr/bin/env python3
"""Run targeted beta QEAPP/2 parser+crypto tests with ephemeral keys.

No developer private key is read. The public CI beta trust header is generated
in the checkout only for the duration of the test and removed on completion.
Never run while using a developer-provisioned QeappTrustKeyLuaBeta.h.
"""
from __future__ import annotations
import hashlib
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/'src/services'
HEADER=SRC/'QeappTrustKeyLuaBeta.h'
TESTS=ROOT/'tools/lua_beta_host'

def cmd(*args):
    result=subprocess.run([str(v) for v in args],capture_output=True,text=True,timeout=100)
    if result.returncode:
        raise RuntimeError(f'command returned {result.returncode}: {str(args[0])}: '+(result.stderr+'\n'+result.stdout)[-3200:])
    return result.stdout

def main():
    if HEADER.exists():
        raise RuntimeError('Personal beta header exists: test refuses to modify or use it')
    with tempfile.TemporaryDirectory(prefix='vqeaf-lua-beta-test-') as name:
        t=Path(name);prod=t/'prod.pem';beta=t/'beta.pem';prodH=t/'TestProd.h'
        try:
            cmd(sys.executable,ROOT/'tools/qeapp_keys.py','--private',prod,'--header',prodH,'--key-id','0x31534351')
            cmd(sys.executable,ROOT/'tools/qeapp_keys.py','--private',beta,'--header',HEADER,'--key-id','0x544c5541')
            text=t/'message.txt';text.write_text('Signed producer text!')
            lua=t/'hello.lua';lua.write_text('''function on_draw() engine.clear(0) engine.rect(1,2,3,4,65535) end\n''')
            icon=t/'icon.png'
            from PIL import Image
            Image.new('RGB',(32,32),(255,0,0)).save(icon)
            def build(name,kind,key,key_id):
                dest=t/name
                command=[sys.executable,str(ROOT/'tools/build_qeapp.py'),
                    '--id',name[:-6], '--name','Beta Test', '--version','1.0.0',
                    '--type',kind,'--sign-key',str(key),'--key-id',key_id,
                    '--icon',str(icon),'-o',str(dest)]
                if kind=='lua':command+=['--lua',str(lua),'--enable-lua-experimental']
                else:command+=['--text',str(text)]
                cmd(*command)
                return dest
            lua_ok=build('lua_ok.qeapp','lua',beta,'0x544c5541')
            text_ok=build('text_ok.qeapp','text',prod,'0x31534351')
            lua_wrong=build('lua_wrong.qeapp','lua',prod,'0x31534351')
            text_wrong=build('text_wrong.qeapp','text',beta,'0x544c5541')
            corrupt=t/'corrupt.qeapp';b=bytearray(lua_ok.read_bytes());manifest=int.from_bytes(b[8:12],'little');b[116+manifest]^=4;corrupt.write_bytes(b)
            b=t/'dual-host'
            cmd('g++','-std=c++17','-Wno-deprecated-declarations','-Werror',
                '-DQEAPP_HOST_OPENSSL=1','-DVQEAF_ENABLE_LUA=1','-DVQEAF_LUA_BETA_TRUST=1',
                '-DQEAPP_TRUST_KEY_HEADER="TestProd.h"',
                '-I'+str(t),'-I'+str(SRC),SRC/'QeappFormat.cpp',SRC/'QeappSignature.cpp',
                TESTS/'test_dual_publisher.cpp','-lcrypto','-o',b)
            print(cmd(b,lua_ok,text_ok,lua_wrong,text_wrong,corrupt).strip())
            # The exact same source's parser must REJECT beta Lua when beta feature is disabled.
            stock=t/'stock-parser';beta_parser=t/'beta-parser'
            for extra,out in (([],stock),(['-DVQEAF_ENABLE_LUA=1'],beta_parser)):
                cmd('g++','-std=c++11',*extra,'-I'+str(SRC),SRC/'QeappFormat.cpp',
                    TESTS/'test_parser.cpp','-o',out)
                print(cmd(out).strip())
            print('PASS: QEAPP/2 2-key crypto, payload integrity, stock-vs-beta manifest and icon contract')
        finally:
            # Never leave test keys or CI trust material in the checkout.
            if HEADER.exists():HEADER.unlink()

if __name__=='__main__':
    try:main()
    except Exception as exc:raise SystemExit('BETA HOST TEST FAILED: '+str(exc))
