import importlib.util
from pathlib import Path
s=importlib.util.spec_from_file_location("qeapp_log",Path(__file__).with_name("qeapp_launch_log_readonly.py"))
mod=importlib.util.module_from_spec(s);s.loader.exec_module(mod)
rows=[
 ("t1","[VQEAF][LUA][STATUS] enabled=1 safe_mode=1"),
 ("t2","[VQEAF][QEAPP][LAUNCH_FAIL] reason=SAFE_MODE"),
 ("t3","[VQEAF][LUA] launch rejected id=test reason=Lua canvas failed"),
 ("t4","[S3DIAG][SD] status mounted errors=0"),
 ("t5","Brownout detector was triggered")
]
r=mod.analyze(rows)["events"]
assert r["safe_mode"]==2,r
assert r["qeapp_fail"]==1,r
assert r["lua_runtime"]==2,r
assert r["power_reset"]==1,r
print("PASS: QEAPP read-only parser classification, synthetic fixture only")
