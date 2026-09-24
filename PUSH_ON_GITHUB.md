# Publish the prepared VQEAF OS v2.4.1 history

This repository currently contains **bootstrap/import files only**. It does **not yet contain the VQEAF OS firmware**.

The original verified local Git bundle contains exactly two commits:

- `1dfa5eb691b2f3ccebfca4fc3d9844776b956fff` — import VQEAF OS v2.4.0 source.
- `654f51363eeb84212e9e0f726bac292cdfbde18d` — Pixel Snake signed QEAPP/2 game template and native runtime.

If you have the complete `VQEAF_OS_v241_Git_Commit_Ready.zip` package, extract it **including `.git`**, then run:

```bash
cd VQEAF-OS
git remote set-url origin https://github.com/qeafivels/VQEAF-OS.git
git fetch origin main
git push --force-with-lease origin main
git ls-remote origin refs/heads/main
```

Only proceed while the remote is still this temporary bootstrap; `--force-with-lease` prevents overwriting any concurrent remote update. Do not use unconditional `--force`.

After successful publication, `main` must point to `654f51363eeb84212e9e0f726bac292cdfbde18d`. This temporary README, import workflow, and manifest are replaced by the actual firmware history.
