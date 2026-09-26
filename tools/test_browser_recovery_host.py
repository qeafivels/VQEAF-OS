#!/usr/bin/env python3
"""Cold restart simulation in separate host processes, including corruption."""
import shutil
import subprocess
import tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="qb_recovery_") as directory:
    binary=Path(directory)/"qb_recovery_host"
    subprocess.run(["g++","-std=c++11","-O2","-Wall","-Wextra","-Werror",
                    str(root/"tools/browser_recovery_host.cpp"),"-o",str(binary)],
                   check=True)
    def invoke(action,expect):
        result=subprocess.run([str(binary),action,directory],capture_output=True,text=True)
        assert (result.returncode==0)==expect,(action,result.stdout,result.stderr)
        print(result.stdout.strip(), "EXPECTED" if expect else "REJECTED AS EXPECTED")
    invoke("stage",True)
    invoke("verify",True) # New process is a simulated cold reboot.
    pixel=Path(directory)/"thumb"
    raw=bytearray(pixel.read_bytes());raw[-11]^=0x7F;pixel.write_bytes(raw)
    invoke("verify",False)
    invoke("stage",True)
    cookie=Path(directory)/"cookie"
    raw=bytearray(cookie.read_bytes());cookie.write_bytes(raw[:3])
    invoke("verify",False)
    invoke("stage",True)
    (Path(directory)/"html").unlink()
    invoke("verify",False)
    invoke("stage",True)
    (Path(directory)/"marker").unlink()
    invoke("verify",False)
print("PASS browser host cold-start restore / malformed and truncated rejection")
