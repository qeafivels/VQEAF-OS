#!/usr/bin/env python3
"""Read-only audit of local .qeapp files. Verify signed contents and publisher
policy identical to QEAPP/2 firmware. Never touch device, never re-sign apps.
"""
import argparse,hashlib,json,re,struct
from pathlib import Path
from cryptography.hazmat.primitives.asymmetric import ec,utils
from cryptography.hazmat.primitives.hashes import SHA256
ROOT=Path(__file__).resolve().parents[1]
def key(path):
 txt=path.read_text(encoding="utf8")
 match=re.search(r'QEAPP_TRUST_PUBKEY\[65\]=\{([^}]+)\}',txt,re.S)
 if not match:raise ValueError("Missing trust public key")
 return ec.EllipticCurvePublicKey.from_encoded_point(ec.SECP256R1(),
  bytes(int(x,16) for x in re.findall(r'0x[0-9a-fA-F]{2}',match.group(1))))
TRUST={0x31534351:key(ROOT/'src/services/QeappTrustKey.h'),
       0x544c5541:key(ROOT/'src/services/QeappTrustKeyLuaBeta.h')}
def audit(p):
 result={'filename':p.name,'bytes':p.stat().st_size,'status':'REJECT','reason':None}
 try:
  b=p.read_bytes()
  if b[:8]!=b'QEAPP2\r\n' or len(b)<192:raise ValueError("Not supported QEAPP/2")
  m,i,s=struct.unpack_from('<III',b,8)
  if not 0<m<=2048 or i not in(0,2048) or s>256*1024 or len(b)!=116+m+i+s+76:
   raise ValueError("Package length/section bounds invalid")
  meta={}
  for row in b[116:116+m].decode('utf8').splitlines():
   if '=' in row:
    k,v=row.split('=',1);meta[k]=v
  typ=meta.get('type','');result['type']=typ;result['id']=meta.get('id','')
  if b[-76:-68]!=b'QSIGP256':raise ValueError("Missing signed trailer")
  kid=struct.unpack_from('<I',b,-68)[0]
  result['key_id']=f"0x{kid:08x}"
  off=116
  for length,stored,name in [(m,b[20:52],'manifest'),(i,b[52:84],'icon'),(s,b[84:116],'payload')]:
   if hashlib.sha256(b[off:off+length]).digest()!=stored:
    raise ValueError(name+' SHA-256 mismatch')
   off+=length
  if kid not in TRUST:raise ValueError("Unknown publisher key")
  if typ=='lua' and kid!=0x544c5541:
   raise ValueError("Lua app declares wrong key ID for firmware; signature with declared key not verified; re-export using existing Lua-beta publisher")
  r=int.from_bytes(b[-64:-32],'big');t=int.from_bytes(b[-32:],'big')
  TRUST[kid].verify(utils.encode_dss_signature(r,t),b[:-76],ec.ECDSA(SHA256()))
  if typ!='lua' and kid==0x544c5541:
   raise ValueError("Lua beta key cannot publish other app types")
  if typ=='lua' and not (1<=s<=65536):raise ValueError("Lua source exceeds runtime bounds")
  if typ=='text' and not s:raise ValueError("Missing signed text payload")
  if typ=='web' and s:raise ValueError("Unexpected web payload")
  if typ not in ('lua','web','text'):raise ValueError("Unsupported app type")
  result['status']='PASS';result['reason']='Signature + manifest + payload + firmware trust policy verified'
 except Exception as e:
  result['reason']=str(e)[:180] or type(e).__name__
 return result
if __name__=='__main__':
 pa=argparse.ArgumentParser();pa.add_argument('--directory',type=Path,required=True)
 pa.add_argument('--out',type=Path,default=ROOT/'local_hw_results/local_qeapp_compat.json')
 args=pa.parse_args()
 rows=[audit(p) for p in sorted(args.directory.rglob('*.qeapp'))]
 args.out.parent.mkdir(parents=True,exist_ok=True)
 args.out.write_text(json.dumps(rows,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
 for r in rows:print(r['status'],r['filename'],r.get('type','?'),r.get('key_id','?'),r['reason'])
 print("PASS count",sum(r['status']=='PASS' for r in rows),"REJECT count",sum(r['status']!='PASS' for r in rows))
