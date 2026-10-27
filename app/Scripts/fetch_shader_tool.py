#!/usr/bin/env python3
"""Download the prebuilt shader translator (ShaderTool/pgmsl.cc built against Google's Tint).

It turns the game's WGSL shaders into Metal Shading Language at build time. The
prebuilt binary is attached to this project's releases (macOS 13+, Apple Silicon);
set PGMSL_PREBUILT_URL to use another copy, or build it yourself with
ShaderTool/build_pgmsl.sh from a Dawn checkout. The archive is checked against a
pinned SHA-256 before use. Tint/Dawn: BSD-3-Clause and Apache-2.0 (licenses inside).

PGMSL_PREBUILT_URL may be an http(s):// URL, a file:// URL or a plain local path.
If it is not set (or fails), the script looks for pgmsl-macos-arm64.tar.gz in
several local folders (build-tools/ next to app/, build-tools/ inside app/, Tools/).
"""
import hashlib
import os
import sys
import time
import tarfile
import urllib.request
from pathlib import Path

NAME = 'pgmsl-macos-arm64.tar.gz'
SHA256 = 'c80005c5fe22653cc7b2dd10a8276202df8e8fbfcb6d6d1a3e6599c2240105a1'

root = Path(__file__).resolve().parents[1]
target = root / 'Tools/pgmsl'
if (target / 'pgmsl').exists():
    print('Shader translator already present.')
    sys.exit(0)

archive = root / 'Tools/pgmsl.tar.gz'
archive.parent.mkdir(parents=True, exist_ok=True)


def candidates():
    """Yield (label, source) where source is a URL string or a local Path."""
    env = (os.environ.get('PGMSL_PREBUILT_URL') or '').strip()
    if env:
        if '://' in env:
            yield env, env
        else:
            yield env, Path(env).expanduser()
    for folder in [root.parent / 'build-tools', root / 'build-tools', root / 'Tools', root.parent]:
        yield str(folder / NAME), folder / NAME


def fetch(source):
    """Return the bytes of a local file or URL, or None if it is not available."""
    if isinstance(source, Path):
        return source.read_bytes() if source.is_file() else None
    last = None
    for attempt in range(1, 4):
        try:
            with urllib.request.urlopen(source, timeout=300) as response:
                return response.read()
        except Exception as error:  # 404, SSL, timeout, missing file:// ...
            last = error
            print(f'  attempt {attempt}/3 failed: {error}', flush=True)
            if isinstance(error, FileNotFoundError) or 'No such file' in str(error):
                break
            time.sleep(3 * attempt)
    print(f'  giving up on {source}: {last}', flush=True)
    return None


print('Looking for the shader translator...', flush=True)
data = None
tried = []
for label, source in candidates():
    print(f'- trying {label}', flush=True)
    tried.append(label)
    blob = fetch(source)
    if blob is None:
        continue
    digest = hashlib.sha256(blob).hexdigest()
    if digest != SHA256:
        print(f'  checksum mismatch ({digest[:12]}...), skipping this copy', flush=True)
        continue
    data = blob
    print('  found a valid copy.', flush=True)
    break

if data is None:
    sys.exit(
        'Shader translator not found.\n'
        f'Upload {NAME} to the repo at build-tools/{NAME} (same level as the app/ folder),\n'
        'or set PGMSL_PREBUILT_URL to a download link of that file.\n'
        'Checked:\n  ' + '\n  '.join(tried)
    )

archive.write_bytes(data)
sys.path.insert(0, str(Path(__file__).parent))
from safe_extract import extract
with tarfile.open(archive) as package:
    extract(package, root / 'Tools')
os.chmod(target / 'pgmsl', 0o755)
print('Shader translator ready.')
