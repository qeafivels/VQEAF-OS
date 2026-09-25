# QEAPP-Studio — publication layout

The desktop IDE and two application samples are maintained under `developer/QEAPP-Studio/`, separate from the ESP32-S3 firmware at this repository's root.

```text
VQEAF-OS/
├── src/, include/, platformio.ini            # Firmware stays unchanged
└── developer/QEAPP-Studio/                   # Companion desktop IDE
    ├── AGENTS.md, PROMPT.md, SKILLS.md
    ├── agents/, docs/agents/
    ├── studio/, runtime/, engine/, tools/
    ├── tests/, projects/
    ├── samples/
    │   ├── pocket-focus/                     # Lua app + text guide + tests
    │   └── pocket-calculator/                # Lua app + text guide + tests
    └── run_studio.bat
```

## Publication status

This initial branch commit establishes the integration path only. The full desktop Studio source and sample code have **not** been uploaded in this commit. To finish publishing the generated, verified source ZIP, extract `QEAPP_Studio_v074_Pocket_Samples_GitHub_Ready.zip` supplied with this work and execute `PUBLISH_GITHUB.bat` from its root on a Windows PC with authenticated Git access. The script updates this feature branch without force-pushing and leaves the firmware graphics and core unchanged.

The interactive Lua sample apps are host-VM examples. The shipping firmware supports signed text/web QEAPP packages; experimental Lua installation on ESP32-S3 still needs hardware verification.
