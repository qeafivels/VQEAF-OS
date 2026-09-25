#!/usr/bin/env python3
"""QEAPP installed-app revisit menu gate: run production C++11 policy and
inspect OS routing, then compile the signed-install host acceptance suite.
This never claims a physical ESP32-S3 launch has been observed.
"""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
ui=(root/"src/apps/Apps.cpp").read_text()
main=(root/"src/main.cpp").read_text()
policy=(root/"src/apps/InstallerMenuPolicy.h").read_text()
installer=(root/"src/services/AppInstallerService.cpp").read_text()
assert '"Details", "Open"' in ui and '"Reset app data", "Uninstall"' in ui
assert '"Details", "Update"' in ui and '"Details", "Install / Update"' in ui
assert "InstallerMenuPolicy::primary(" in ui
assert ui.count("InstallerMenuPolicy::canOpen(")>=2, "both popup and softkey must use same guard"
assert "ctx.pendingPackageId=selectedMeta.id" in ui
assert "return ScreenId::PackageApp;" in ui
assert "if(choice==7 && installedTab)" in ui, "uninstall must remain a distinct action"
assert "if(installedTab && verified)" in ui, "installed list OK should open app"
assert "ctx.installer.get(selectedMeta.id,old)" in ui
assert "Qeapp::compareVersion(selectedMeta.version,old.version)>0" in ui
assert "if(!willUpdate)feedback=\"Already installed - choose Open\";" in ui
assert "if (s == ScreenId::PackageApp)" in main and "appInstaller.get(" in main, "launch-time signature revalidation"
assert "verifyInstalled(id,meta,error)" in installer
source=r"""
#include "apps/InstallerMenuPolicy.h"
#include <cassert>
#include <cstdio>
using namespace InstallerMenuPolicy;
int main(){
  // First opening in Inbox: user inspects the signed package, then installs.
  assert(primary(false,false,false,false,false)==Primary::Verify);
  assert(primary(false,true,false,false,false)==Primary::Verify);
  assert(primary(false,true,true,false,false)==Primary::Install);
  assert(!canOpen(false,true,false,false));
  // Revisit after successful install: Open, never repeat Install.
  assert(primary(false,true,true,true,false)==Primary::Open);
  assert(canOpen(false,true,true,false));
  // Revisit newer signed package: preserve Update, not Open.
  assert(primary(false,true,true,true,true)==Primary::Update);
  assert(!canOpen(false,true,true,true));
  // Installed list always exposes Open label, but revalidates before launch.
  assert(primary(true,false,false,false,false)==Primary::Open);
  assert(!canOpen(true,false,false,false));
  assert(canOpen(true,true,true,false));
  // Rejected signature can never turn into a launch route.
  assert(!canOpen(false,false,false,true,false));
  assert(!canOpen(true,false,true,false));
  puts("PASS: installer Open/Install/Update/Verify matrix + verified launch guard");
}
"""
with tempfile.TemporaryDirectory(prefix="qeapp-menu-") as folder:
    test=Path(folder)/"menu.cpp"
    exe=Path(folder)/"menu"
    test.write_text(source)
    subprocess.run(["g++","-std=c++11","-Wall","-Wextra","-Werror","-Isrc",str(test),"-o",str(exe)],check=True,cwd=root)
    subprocess.run([str(exe)],check=True,cwd=root)
print("PASS: OS installer/App list routing and per-launch signature contract")
