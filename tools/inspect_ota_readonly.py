#!/usr/bin/env python3
"""Decode ESP32-S3 ESP-IDF OTA selection metadata from a READ-ONLY flash dump.
Accept either 8192-byte otadata or an existing full 16 MiB flash backup.
This estimates selected boot candidate, NOT a proof of currently executing slot.
"""
from pathlib import Path
import argparse,struct,zlib,json,hashlib
def parse(blob):
    if len(blob)==0x2000: data=blob; origin="otadata_only"
    elif len(blob)==0x1000000: data=blob[0xe000:0x10000]; origin="full_16mb_backup"
    else:raise ValueError("expected 8192-byte otadata or full 16 MiB flash dump")
    result=[]
    for index in range(2):
        rec=data[index*0x1000:index*0x1000+32]
        seq,state,crc=struct.unpack_from("<III",rec,0)[0],struct.unpack_from("<I",rec,24)[0],struct.unpack_from("<I",rec,28)[0]
        check=zlib.crc32(rec[:4],0xffffffff)&0xffffffff
        valid=seq not in (0,0xffffffff) and crc==check
        # 0xffffffff is OTA_IMG_UNDEFINED and may be a valid legacy state.
        state_names={0:"NEW",1:"PENDING_VERIFY",2:"VALID",3:"INVALID",4:"ABORTED",0xffffffff:"UNDEFINED"}
        boot_candidate=valid and state in (0xffffffff,0,2)
        result.append(dict(copy=index,ota_seq=seq,ota_state=state_names.get(state,hex(state)),
           crc_stored=hex(crc),crc_calculated=hex(check),crc_valid=crc==check,
           select_entry_valid=valid,boot_candidate=bool(boot_candidate),
           slot_by_seq="ota_"+str((seq-1)%2) if valid else None))
    good=[x for x in result if x["boot_candidate"]]
    candidate=max(good,key=lambda x:x["ota_seq"]) if good else None
    return dict(origin=origin,records=result,preferred_ota_candidate=candidate["slot_by_seq"] if candidate else None,
                warning="Metadata only; bootloader may fall back if image invalid. Does not prove running slot. Dump taken earlier may be stale.")
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("dump",type=Path)
    p.add_argument("--out",type=Path)
    a=p.parse_args(); data=a.dump.read_bytes()
    report=parse(data);report["source_sha256"]=hashlib.sha256(data).hexdigest()
    if a.out:
        a.out.parent.mkdir(parents=True,exist_ok=True)
        a.out.write_text(json.dumps(report,indent=2)+"\n",encoding="utf8")
    print(json.dumps(report,indent=2))
if __name__=="__main__":main()
