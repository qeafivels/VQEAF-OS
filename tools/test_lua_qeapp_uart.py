#!/usr/bin/env python3
"""QEAPP Studio Lua bridge regression. Optional real app/public key verifies
the *existing* publisher without uploading user apps or private signing keys.
"""
from pathlib import Path
import argparse
import hashlib
import re
import struct
ROOT=Path(__file__).resolve().parents[1]

def structural():
    ini=(ROOT/"platformio.ini").read_text(encoding="utf-8")
    main=(ROOT/"src/main.cpp").read_text(encoding="utf-8")
    signer=(ROOT/"src/services/QeappSignature.cpp").read_text(encoding="utf-8")
    vm=(ROOT/"src/lua/QeLuaRuntime.cpp").read_text(encoding="utf-8")
    checks={
      "Lua UART profile inherits signed Lua beta":
        "[env:vqeaf_lua_uart]" in ini and "extends = env:vqeaf_lua_beta" in ini,
      "Stock production profile leaves Lua disabled":
        "-D VQEAF_ENABLE_LUA=1" in ini and "[env:vqeaf_os_uart]" in ini,
      "Lua profile retains real UART0 COM3 boot output":
        "-D ARDUINO_USB_CDC_ON_BOOT=0" in ini,
      "Installed payload is checked against verified receipt before VM":
        'sourceHash.finish(digest)' in main and
        'equalHash(digest,signedHeader+84)' in main and
        'appInstaller.get(appCtx.pendingPackageId' in main,
      "On-device feature banner is observable":
        "[VQEAF][LUA] runtime=ENABLED" in main,
      "Trusted publisher accepts Lua only and preserves stock publisher":
        "Lua beta key cannot sign text/web apps" in signer and
        "Lua apps require Lua beta publisher key" in signer,
      "Lua callback has CPU, time, graphics and PSRAM budgets":
        "kInstructionsPerCall" in vm and "kDrawCallsPerCallback" in vm and
        "MALLOC_CAP_SPIRAM" in vm,
      "VM cannot open unrestricted Lua OS/IO/package libraries":
        "luaopen_os" not in vm and "luaopen_io" not in vm and
        "luaopen_package" not in vm,
    }
    for name,ok in checks.items():
        if not ok:raise AssertionError(name)
        print("PASS",name)

def verify_user_sample(path,key_header):
    from cryptography.hazmat.primitives.asymmetric import ec,utils
    from cryptography.hazmat.primitives.hashes import SHA256
    header=key_header.read_text(encoding="utf-8")
    match=re.search(r'(?:QEAPP_LUA_BETA_PUBKEY|QEAPP_TRUST_PUBKEY)\[65\]\s*=\s*\{([^}]+)\}',header,re.S)
    if match is None:raise ValueError("Expected local public key header")
    pub=bytes(int(x,16) for x in re.findall(r'0x[0-9a-fA-F]+',match.group(1)))
    assert len(pub)==65 and pub[0]==4
    public=ec.EllipticCurvePublicKey.from_encoded_point(ec.SECP256R1(),pub)
    b=path.read_bytes()
    assert b[:8]==b'QEAPP2\r\n', "not QEAPP/2"
    manifest,icon,payload=struct.unpack_from("<III",b,8)
    assert 0<manifest<=2048 and icon in (0,2048) and 0<payload<=65536
    assert len(b)==116+manifest+icon+payload+76
    assert hashlib.sha256(b[116:116+manifest]).digest()==b[20:52]
    assert hashlib.sha256(b[116+manifest:116+manifest+icon]).digest()==b[52:84]
    assert hashlib.sha256(b[116+manifest+icon:-76]).digest()==b[84:116]
    meta=b[116:116+manifest].decode("utf-8")
    assert "type=lua" in meta.splitlines()
    assert b[-76:-68]==b"QSIGP256"
    key_id=struct.unpack_from("<I",b,-68)[0]
    assert key_id==0x544C5541
    r=int.from_bytes(b[-64:-32],"big")
    s=int.from_bytes(b[-32:],"big")
    signature=utils.encode_dss_signature(r,s)
    public.verify(signature,b[:-76],ec.ECDSA(SHA256()))
    print("PASS existing QEAPP Studio Lua .qeapp authenticity, hashes, manifest, source budget:",
          path.name,"bytes",len(b))

if __name__=="__main__":
    p=argparse.ArgumentParser()
    p.add_argument("--package",type=Path)
    p.add_argument("--public-header",type=Path)
    a=p.parse_args()
    structural()
    if a.package or a.public_header:
        if not a.package or not a.public_header:p.error("Both optional arguments required")
        verify_user_sample(a.package,a.public_header)
