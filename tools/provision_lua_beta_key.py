#!/usr/bin/env python3
"""Generate a dedicated local beta publisher, or use an existing private PEM.

No private keys or personalized public headers are committed or sent to CI.
The stock text/web publisher is never replaced. Key ID must match Studio's
experimental Lua QEAPP/2 key ID 0x544C5541.
"""
from __future__ import annotations
import argparse
import os
from pathlib import Path
import stat
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ec

BETA_ID = 0x544C5541
ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / 'src/services/QeappTrustKeyLuaBeta.h'

def _outside_repo(path: Path) -> Path:
    path = path.expanduser().resolve()
    if path == ROOT or ROOT in path.parents:
        raise ValueError('Private key must be outside the repository')
    return path

def provision(private: Path | None = None, existing: Path | None = None,
              header: Path = DEST):
    if (private is None) == (existing is None):
        raise ValueError('Choose exactly one of --private or --existing-private')
    if header.exists():
        raise FileExistsError('Existing beta public key header; refusing to rotate trust silently')
    if existing is not None:
        existing = _outside_repo(existing)
        key = serialization.load_pem_private_key(existing.read_bytes(), password=None)
    else:
        private = _outside_repo(private)
        if private.exists():
            raise FileExistsError('Existing private key; use --existing-private instead')
        key = ec.generate_private_key(ec.SECP256R1())
        private.parent.mkdir(parents=True, exist_ok=True)
        data = key.private_bytes(serialization.Encoding.PEM,
            serialization.PrivateFormat.PKCS8, serialization.NoEncryption())
        # O_EXCL prevents overwriting and POSIX mode 600 protects fresh files.
        fd = os.open(private, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'wb') as f:
            f.write(data)
        if os.name == 'posix':
            private.chmod(stat.S_IRUSR | stat.S_IWUSR)
    if not isinstance(key, ec.EllipticCurvePrivateKey) or not isinstance(key.curve, ec.SECP256R1):
        raise ValueError('Expected ECDSA secp256r1/P-256 private key')
    blob = key.public_key().public_bytes(serialization.Encoding.X962,
                                         serialization.PublicFormat.UncompressedPoint)
    if len(blob) != 65 or blob[0] != 4:
        raise ValueError('Unexpected public point')
    lines = ['#pragma once', '#include <stdint.h>',
             '// GENERATED PUBLIC trust key for VQEAF Lua beta; never store a private PEM in firmware.',
             f'static const uint32_t QEAPP_TRUST_KEY_ID=0x{BETA_ID:08x}u;',
             'static const uint8_t QEAPP_TRUST_PUBKEY[65]={',
             ','.join(f'0x{n:02x}' for n in blob), '};', '']
    header.parent.mkdir(parents=True, exist_ok=True)
    header.write_text('\n'.join(lines), encoding='utf-8')
    print('Generated beta PUBLIC trust header:', header)
    print('Lua beta key ID: 0x544C5541; stock text/web key is unchanged')
    print('Never commit the private PEM or the personalized beta header')
    return 0

if __name__ == '__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    group=ap.add_mutually_exclusive_group(required=True)
    group.add_argument('--private', type=Path, help='Create a NEW private key OUTSIDE source control')
    group.add_argument('--existing-private', type=Path, help='Use an EXISTING private PEM, e.g. previously used by Studio')
    ap.add_argument('--header', type=Path, default=DEST)
    args=ap.parse_args()
    try:
        raise SystemExit(provision(args.private,args.existing_private,args.header))
    except (ValueError,OSError,TypeError) as exc:
        raise SystemExit('Key setup failed: '+str(exc))
