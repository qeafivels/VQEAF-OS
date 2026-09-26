"""Structural contract: QEAPP catalog diagnosis is read-only and never bypasses signatures."""
from pathlib import Path
p=Path(__file__).resolve().parents[1]/'src/main.cpp'
code=p.read_text(encoding='utf8')
start=code.index('if(c=="diag qeapp catalog")')
end=code.index('if(c=="diag app icons")',start)
body=code[start:end]
assert 'appInstaller.at(i).info' in body
assert 'BLOCKED_SAFE_MODE' in body
assert 'ELIGIBLE_TO_VERIFY' in body
assert all(op not in body for op in ('appInstaller.install(', 'appInstaller.refresh(', 'storage.fs().open(', 'setSafeMode(', 'ESP.restart('))
assert 'diag qeapp catalog - list verified app types' in code
assert 'if (systemService.safeMode())' in code
print("PASS source-only: QEAPP catalog read-only diagnostic and safe-mode launch gate")
