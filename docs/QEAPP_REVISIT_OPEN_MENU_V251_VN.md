# QEAPP Installed app/game: Open on subsequent visits

After a signed `.qeapp` is installed, opening the same package from Files, Downloads or the App inbox again shows **Open**, rather than asking to reinstall the same or an older version. In **App installer → Installed apps**, the middle softkey and **Options → Open** also launch the selected installation. The Applications menu retains **Open selected** for installed apps and games.

The source package is signature-checked on selection. The matching installed application is reverified using `AppInstallerService::get()`, and `ScreenId::PackageApp` re-verifies immediately before launch. No install transaction, filesystem write, or auto-upgrade occurs when choosing Open.

A signed package with a higher version offers **Update** and uses the normal confirmation flow. Fresh signed packages offer **Install**. A damaged installed package or invalid source signature does not receive an executable Open action; recovery must be handled separately. **Uninstall** remains explicit under **Installed apps → Options → Uninstall** with confirmation; **Reset app data** is a distinct action.

Supported application types still depend on the selected firmware profile: stock firmware and experimental Lua-enabled QEAPP games are not interchangeable. The menu option does not add execution support for unsupported formats.

## Host regression

```sh
python tools/test_installer_revisit_open.py
python tools/test_v24_app_manager.py
pio run -e vqeaf_os
```

The first gate compiles and executes the actual C++11 menu decision policy for fresh install, installed revisit, newer signed updates and invalid-signature rejection. It also checks OS routing and the independently verified installed-app path. The signed installer host gate tests package creation and upgrade independently. On-device relaunch and SD-removal acceptance still require the physical ESP32-S3 board.
