#!/usr/bin/env python3
"""Generate or verify a SHA-256 manifest of a DVSI test-vector directory.

Does NOT redistribute DVSI content — only records filenames + hashes.

Usage:
    # Create a manifest from a local DVSI mirror:
    python3 tools/dvsi_manifest.py --generate <dvsi_dir> <out.sha256>

    # Verify a local mirror matches a committed manifest:
    python3 tools/dvsi_manifest.py --verify <dvsi_dir> <manifest.sha256>

    # Examples:
    python3 tools/dvsi_manifest.py \\
        --generate "/mnt/share/P25-IQ-Samples/DVSI Vectors/tv-std" \\
        test_vectors/dvsi_manifest/tv-std.sha256

    python3 tools/dvsi_manifest.py \\
        --verify "/mnt/share/P25-IQ-Samples/DVSI Vectors/tv-std" \\
        test_vectors/dvsi_manifest/tv-std.sha256

Manifest format (standard GNU sha256sum output, two-space separator):
    <64-char-hex>  <path-relative-to-dvsi-dir>
"""
import argparse
import hashlib
import sys
from pathlib import Path

CHUNK = 1 << 20  # 1 MiB read buffer

def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        while True:
            chunk = f.read(CHUNK)
            if not chunk:
                break
            h.update(chunk)
    return h.hexdigest()

def walk_files(root: Path):
    for p in sorted(root.rglob("*")):
        if p.is_file():
            yield p

def generate(dvsi_dir: Path, out_path: Path) -> int:
    if not dvsi_dir.is_dir():
        print(f"ERROR: {dvsi_dir} is not a directory", file=sys.stderr)
        return 1
    print(f"Generating manifest for {dvsi_dir}", file=sys.stderr)
    files = list(walk_files(dvsi_dir))
    if not files:
        print("ERROR: no files under source directory", file=sys.stderr)
        return 1

    lines = []
    for i, p in enumerate(files, 1):
        rel = p.relative_to(dvsi_dir).as_posix()
        h = sha256_file(p)
        lines.append(f"{h}  {rel}\n")
        if i % 50 == 0 or i == len(files):
            print(f"  [{i}/{len(files)}] {rel}", file=sys.stderr)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    # Header block: same '#' convention as our CSV files, ignored by sha256sum -c.
    header = [
        f"# DVSI test vector manifest generated from local mirror.\n",
        f"# Source directory (at generation time): {dvsi_dir}\n",
        f"# File count: {len(files)}\n",
        f"# Each line: <sha256-hex>  <relative-path-under-source>\n",
        f"# No DVSI file content is included; only filenames + hashes.\n",
        f"# Verify with: python3 tools/dvsi_manifest.py --verify <local-dir> <this-file>\n",
        f"#\n",
    ]
    out_path.write_text("".join(header) + "".join(lines))
    print(f"Wrote {out_path} ({len(files)} files)", file=sys.stderr)
    return 0

def parse_manifest(manifest_path: Path):
    """Yield (sha256_hex, relative_path) tuples, skipping comment/header lines."""
    for line in manifest_path.read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        # Standard sha256sum format: hex + two spaces + path
        parts = line.split("  ", 1)
        if len(parts) != 2:
            print(f"WARN: skipping malformed line: {line!r}", file=sys.stderr)
            continue
        yield parts[0], parts[1]

def verify(dvsi_dir: Path, manifest_path: Path) -> int:
    if not dvsi_dir.is_dir():
        print(f"ERROR: {dvsi_dir} is not a directory", file=sys.stderr)
        return 1
    if not manifest_path.is_file():
        print(f"ERROR: {manifest_path} not found", file=sys.stderr)
        return 1

    expected = dict(parse_manifest(manifest_path))
    print(f"Verifying {len(expected)} files against {manifest_path}", file=sys.stderr)

    missing = []
    mismatched = []
    extra = []
    matched = 0

    for rel, exp_hash in expected.items():
        p = dvsi_dir / rel
        if not p.is_file():
            missing.append(rel)
            continue
        actual = sha256_file(p)
        if actual != exp_hash:
            mismatched.append((rel, exp_hash, actual))
        else:
            matched += 1

    # Check for local files not in manifest
    local_set = {p.relative_to(dvsi_dir).as_posix() for p in walk_files(dvsi_dir)}
    extra = sorted(local_set - set(expected.keys()))

    print(f"\nResults:", file=sys.stderr)
    print(f"  Matched:     {matched}/{len(expected)}", file=sys.stderr)
    print(f"  Mismatched:  {len(mismatched)}", file=sys.stderr)
    print(f"  Missing:     {len(missing)}", file=sys.stderr)
    print(f"  Extra local: {len(extra)}", file=sys.stderr)

    if mismatched:
        print(f"\nMISMATCHED (first 10 of {len(mismatched)}):", file=sys.stderr)
        for rel, exp, act in mismatched[:10]:
            print(f"  {rel}\n    expected {exp}\n    actual   {act}", file=sys.stderr)
    if missing:
        print(f"\nMISSING (first 10 of {len(missing)}):", file=sys.stderr)
        for rel in missing[:10]:
            print(f"  {rel}", file=sys.stderr)
    if extra:
        print(f"\nEXTRA LOCAL (first 10 of {len(extra)}):", file=sys.stderr)
        for rel in extra[:10]:
            print(f"  {rel}", file=sys.stderr)

    if mismatched or missing:
        return 1
    if extra:
        print("\nNote: extra local files aren't an error, but your copy has more "
              "content than the manifest. If the extras should be tracked, regenerate "
              "the manifest.", file=sys.stderr)
    return 0

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    grp = ap.add_mutually_exclusive_group(required=True)
    grp.add_argument("--generate", nargs=2, metavar=("DVSI_DIR", "OUT_MANIFEST"),
                     help="Generate a manifest from a local DVSI mirror.")
    grp.add_argument("--verify", nargs=2, metavar=("DVSI_DIR", "IN_MANIFEST"),
                     help="Verify a local mirror against a committed manifest.")
    args = ap.parse_args()

    if args.generate:
        return generate(Path(args.generate[0]), Path(args.generate[1]))
    else:
        return verify(Path(args.verify[0]), Path(args.verify[1]))

if __name__ == "__main__":
    sys.exit(main())
