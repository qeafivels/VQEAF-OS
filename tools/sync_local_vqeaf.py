#!/usr/bin/env python3
"""Safely synchronize tracked VQEAF-OS files into a standalone legacy project.
Preview is default; --apply backs up overwritten files and never deletes extras.
The source must be a CLEAN Git checkout of the intended VQEAF-OS branch.
Raw flash backups, serial logs, local SDK assets, and sibling projects are excluded.
"""
import argparse
import hashlib
import json
import shutil
import subprocess
from datetime import datetime
from pathlib import Path

def git(source, *args):
    return subprocess.check_output(["git", "-C", str(source), *args])

def digest(path):
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            h.update(block)
    return h.digest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--target", type=Path, required=True)
    parser.add_argument("--backup-parent", type=Path,
                        help="Defaults to target's parent; backup remains outside project")
    parser.add_argument("--apply", action="store_true", help="Actually copy, otherwise preview only")
    args = parser.parse_args()
    source = args.source.resolve()
    target = args.target.resolve()
    if not source.is_dir() or not target.is_dir():
        parser.error("Source and target must already exist")
    if source == target or source in target.parents or target in source.parents:
        parser.error("Source and target must be separate folders")
    if git(source, "status", "--porcelain", "--untracked-files=no").strip():
        parser.error("Source has uncommitted tracked changes; commit/stash them first")
    branch = git(source, "branch", "--show-current").decode().strip()
    commit = git(source, "rev-parse", "HEAD").decode().strip()
    tracked = [x.decode("utf-8") for x in git(source, "ls-files", "-z").split(b"\0") if x]
    changes = []
    for name in tracked:
        rel = Path(name)
        src, dst = source / rel, target / rel
        if not src.is_file():
            parser.error("Tracked source file missing: " + name)
        if not dst.is_file() or digest(src) != digest(dst):
            changes.append((name, src, dst, dst.is_file()))
    report = {"source_commit": commit, "source_branch": branch, "target": str(target),
              "updated": [n for n, _, _, exists in changes if exists],
              "created": [n for n, _, _, exists in changes if not exists],
              "unchanged": len(tracked) - len(changes),
              "notice": "Extra target files, sibling projects and target build artifacts untouched"}
    print(json.dumps({"commit": commit, "branch": branch, "changed": len(report["updated"]),
                      "created": len(report["created"]), "unchanged": report["unchanged"],
                      "mode": "APPLY" if args.apply else "PREVIEW"}, indent=2))
    if not args.apply:
        return 0
    base = (args.backup_parent or target.parent).resolve()
    if target == base or target in base.parents:
        parser.error("Backup parent must be outside the target folder")
    backup = base / ("VQEAF_OS_presync_backup_" + datetime.now().strftime("%Y%m%d_%H%M%S"))
    backup.mkdir(parents=True, exist_ok=False)
    for name, src, dst, exists in changes:
        if exists:
            old = backup / "overwritten" / name
            old.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(dst, old)
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
        if digest(src) != digest(dst):
            raise RuntimeError("Hash mismatch after writing: " + name)
    report["backup"] = str(backup)
    (backup / "sync_manifest.json").write_text(
        json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    print("Sync verified. Backup and manifest:", backup)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

