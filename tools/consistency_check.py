#!/usr/bin/env python3
"""Cross-document consistency checker for annex_tables/*.csv.

Walks every standards/TIA-102.*/annex_tables/*.csv across the repo,
computes content hashes of the data (not the comment headers), and
reports any table that is published in multiple specs with disagreeing
data — the pattern that caused the BABA-A Annex S vs BBAC-1 Annex E
OCR typo to sit undetected until manual cross-check.

Also reports tables that COULD be cross-validated (same logical table,
same data) but have divergent names or schemas — a prompt to unify
them or document the divergence.

Strategy:

1. Read every CSV under standards/*/annex_tables/.
2. For each, strip '#'-prefixed header lines and compute a content hash
   over (column names, sorted-row content).
3. Group tables by known-equivalence (hand-specified below).
4. For each group, verify all members hash identically. Flag any
   disagreement.

Usage:
    python3 tools/consistency_check.py           # report mode
    python3 tools/consistency_check.py --strict  # exit 1 on any disagreement
"""
import argparse
import csv
import hashlib
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
STANDARDS = ROOT / "standards"

# Known equivalence groups: tables that must match bit-for-bit across documents.
# Each group is a list of (doc_id, csv_filename, optional row-key function).
# When the same table is published in multiple specs (e.g., BABA-A Annex S is
# also in BBAC-1 Annex E), list both here so any future divergence is caught.
EQUIVALENCE_GROUPS = [
    # BABA-A half-rate interleave table IS the AMBE portion of BBAC-1 Annex E.
    # The BBAC-1 version is tabulated per-burst-type (IEMI, VCH_SISCH, etc.)
    # rather than as a standalone interleave map, so they don't directly
    # hash-match — but if both were extracted to the same schema, they should.
    # Leaving this group commented until BBAC-1 has a matching CSV extraction.
    # {
    #     "name": "Half-rate interleave (BABA-A Annex S = BBAC-1 Annex E voice portion)",
    #     "members": [
    #         ("TIA-102.BABA-A", "annex_s_interleave.csv"),
    #         ("TIA-102.BBAC-1", "annex_e_halfrate_interleave.csv"),  # not yet extracted
    #     ],
    # },
]

def content_hash(csv_path: Path) -> str:
    """SHA-256 of (header_row, sorted_data_rows).

    Ignores lines beginning with '#' so that extraction metadata (which
    legitimately varies) doesn't cause false positives.
    """
    header = None
    rows = []
    with csv_path.open(newline="") as f:
        for line in f:
            if line.startswith("#"):
                continue
            # Use csv module on this single line to preserve quoting semantics.
            row = next(csv.reader([line]))
            if header is None:
                header = tuple(row)
            else:
                rows.append(tuple(row))
    rows.sort()
    h = hashlib.sha256()
    h.update(str(header).encode())
    h.update(b"|")
    for r in rows:
        h.update(str(r).encode())
        h.update(b"\n")
    return h.hexdigest()

def collect_all_csvs():
    """Return dict of (doc_id, filename) -> Path."""
    out = {}
    for doc_dir in sorted(STANDARDS.iterdir()):
        if not doc_dir.is_dir():
            continue
        tbl_dir = doc_dir / "annex_tables"
        if not tbl_dir.is_dir():
            continue
        for csv_path in sorted(tbl_dir.glob("*.csv")):
            out[(doc_dir.name, csv_path.name)] = csv_path
    return out

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--strict", action="store_true",
                    help="Exit 1 on any disagreement or missing group member.")
    args = ap.parse_args()

    all_csvs = collect_all_csvs()
    print(f"Scanned {len(all_csvs)} CSV files across {len(set(k[0] for k in all_csvs))} documents.")

    had_problem = False

    # Pass 1: explicit equivalence groups.
    if not EQUIVALENCE_GROUPS:
        print("No explicit equivalence groups defined yet.")
        print("Add groups to EQUIVALENCE_GROUPS in this script as cross-spec")
        print("duplications are identified (e.g., BABA-A Annex S vs BBAC-1 Annex E).")
    else:
        print("\n=== Equivalence group checks ===")
        for group in EQUIVALENCE_GROUPS:
            name = group["name"]
            members = group["members"]
            hashes = {}
            missing = []
            for doc_id, fname in members:
                key = (doc_id, fname)
                if key not in all_csvs:
                    missing.append(f"{doc_id}/annex_tables/{fname}")
                    continue
                hashes[key] = content_hash(all_csvs[key])
            print(f"\n[{name}]")
            if missing:
                had_problem = True
                for m in missing:
                    print(f"  MISSING: {m}")
            if len(set(hashes.values())) > 1:
                had_problem = True
                print(f"  DIVERGENCE:")
                for (doc_id, fname), h in hashes.items():
                    print(f"    {doc_id}/annex_tables/{fname}: {h[:16]}...")
            elif hashes:
                print(f"  MATCH: all {len(hashes)} members hash-equivalent")

    # Pass 2: opportunistic — report any CSVs whose content hash matches
    # across documents. Useful for catching unregistered duplication.
    print("\n=== Content-hash duplicate scan ===")
    hash_to_files = {}
    for key, path in all_csvs.items():
        h = content_hash(path)
        hash_to_files.setdefault(h, []).append(key)
    found_any = False
    for h, keys in hash_to_files.items():
        if len(keys) > 1:
            # Cross-doc duplicates only — ignore same-doc (unlikely but possible).
            docs = {k[0] for k in keys}
            if len(docs) > 1:
                found_any = True
                print(f"  Duplicate content (hash {h[:16]}...):")
                for d, f in keys:
                    print(f"    {d}/annex_tables/{f}")
    if not found_any:
        print("  No cross-document duplicates detected.")

    # Pass 3: schema-similarity heuristic — report CSVs with identical
    # column names but different content. These might be the same table
    # with divergent data (spec bug) or just coincidental schema overlap.
    print("\n=== Schema-similarity heuristic ===")
    schema_map = {}  # tuple(columns) -> list of (doc_id, filename, content_hash)
    for key, path in all_csvs.items():
        with path.open(newline="") as f:
            for line in f:
                if line.startswith("#"):
                    continue
                header = tuple(next(csv.reader([line])))
                break
            else:
                continue
        schema_map.setdefault(header, []).append((key[0], key[1], content_hash(path)))
    found_any = False
    for cols, entries in schema_map.items():
        if len(entries) < 2:
            continue
        docs = {e[0] for e in entries}
        if len(docs) < 2:
            continue   # multiple tables in same doc with same schema — OK
        hashes = {e[2] for e in entries}
        if len(hashes) == 1:
            continue   # already caught by duplicate-scan
        found_any = True
        print(f"  Schema {cols}:")
        for d, f, h in entries:
            print(f"    {d}/annex_tables/{f}  ({h[:16]}...)")
        print(f"    -> different content; verify these aren't supposed to match")
    if not found_any:
        print("  No schema-overlapping tables with divergent content detected.")

    print()
    if had_problem:
        if args.strict:
            sys.exit(1)
        print("Problems detected (non-strict mode: exiting 0).")
    else:
        print("All checks passed.")

if __name__ == "__main__":
    main()
