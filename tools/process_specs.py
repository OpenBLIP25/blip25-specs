#!/usr/bin/env python3
"""
Process pending TIA-P25 documents through Claude Code CLI.

Reads specs.toml to find documents with status="pending" that have PDFs available,
then processes them through the Phase 2 extraction pipeline (summary, context,
classification). Phase 3 (implementation specs) is flagged for manual follow-up
on algorithm/message_format/protocol documents.

Phase 4 (verification & uplift) is an uplift pass on an existing impl spec —
see docs/TIA_P25_Processing_Prompt_v3_Phase4.md.

Usage:
    python3 tools/process_specs.py                      # interactive: pick from queue
    python3 tools/process_specs.py TIA-102.BAAC-D       # process a specific document
    python3 tools/process_specs.py --batch 5            # process next 5 in priority order
    python3 tools/process_specs.py --dry-run            # show what would be processed
    python3 tools/process_specs.py --phase4 TIA-102.BABA-A  # uplift existing Phase 3 output
    python3 tools/process_specs.py --vocoder-path --list    # list only vocoder-path docs
    python3 tools/process_specs.py --vocoder-path --batch 3 # process next 3 vocoder-path docs
"""
import argparse
import glob
import json
import os
import re
import subprocess
import sys
import tomllib
from datetime import date
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
STANDARDS_DIR = ROOT / "standards"
SPECS_TOML = ROOT / "specs.toml"
PROMPT_FILE = ROOT / "docs" / "TIA_P25_Processing_Prompt_v2.md"
PHASE4_PROMPT_FILE = ROOT / "docs" / "TIA_P25_Processing_Prompt_v3_Phase4.md"
LOG_DIR = ROOT / "tools" / "logs"


def load_specs():
    with open(SPECS_TOML, "rb") as f:
        return tomllib.load(f)


def find_pdf(doc_dir: Path) -> str | None:
    """Find the first PDF in a document directory."""
    pdfs = sorted(doc_dir.glob("*.pdf"), key=lambda p: p.stat().st_size, reverse=True)
    if not pdfs:
        return None
    # Prefer the largest PDF (likely the main document, not an addendum)
    return str(pdfs[0])


#: Documents on the vocoder data path — voice frames flow through these from
#: the air interface into BABA-A's MBE parameter boundary. Used to drive the
#: --vocoder-path filter and as the default priority order.
VOCODER_PATH = [
    "TIA-102.BABA-A",   # Vocoder itself (IMBE/AMBE frame formats, FEC, Annexes)
    "TIA-102.BAAA-B",   # FDMA CAI — LDU1/LDU2 carry 9 IMBE frames each
    "TIA-102.BBAC-A",   # TDMA MAC — 4V/2V bursts carry AMBE frames
    "TIA-102.BBAB",     # TDMA PHY — H-CPM, sync sequences, burst timing
    "TIA-102.BBAC-1",   # TDMA scrambling + Annex E (dual of BABA-A Annex S)
    "TIA-102.AABF-D",   # Link Control Word — call metadata on voice channel
]


def get_processable_docs(specs: dict, force: bool = False,
                         vocoder_only: bool = False) -> list[dict]:
    """Return pending docs that have PDFs available, sorted by priority.

    If vocoder_only=True, restrict to documents on the vocoder data path
    (VOCODER_PATH above). Useful for focused batching when voice decode
    is the downstream goal.
    """
    # Default priority: vocoder path first, then the other historical
    # high-priority items. Callers passing vocoder_only=True see only
    # the vocoder path.
    priority_order = VOCODER_PATH + [
        "TIA-102.AABC-E", "TIA-102.BAAC-D", "TIA-102.AAAD-B",
    ]

    processable_statuses = {"pending"} if not force else {"pending", "not_required", "needed"}

    docs = []
    for doc_id, info in specs.get("specs", {}).items():
        if info.get("status") not in processable_statuses:
            continue
        if vocoder_only and doc_id not in VOCODER_PATH:
            continue
        doc_dir = STANDARDS_DIR / doc_id
        pdf = find_pdf(doc_dir) if doc_dir.is_dir() else None
        if not pdf:
            continue
        # Calculate priority (lower = higher priority)
        try:
            pri = priority_order.index(doc_id)
        except ValueError:
            pri = 100  # everything else
        docs.append({
            "id": doc_id,
            "title": info.get("title", "Unknown"),
            "classification": info.get("classification", []),
            "pdf": pdf,
            "priority": pri,
            "needs_phase3": any(
                c in ("algorithm", "message_format", "protocol", "vocoder")
                for c in info.get("classification", [])
            ),
        })

    docs.sort(key=lambda d: d["priority"])
    return docs


def build_prompt(doc_id: str, pdf_path: str, classification: list[str]) -> str:
    """Build the processing prompt for a specific document."""
    prompt = Path(PROMPT_FILE).read_text()

    return f"""{prompt}

---

## Document to Process

**Document ID:** {doc_id}
**PDF path:** {pdf_path}
**Pre-classified as:** {', '.join(classification) if classification else 'Unknown — classify first'}

## Instructions for this run

1. Read the PDF at the path above.
2. Perform Phase 1 (classification) and Phase 2 (full text, summary, context/resources).
3. Write all output files into the directory: standards/{doc_id}/
   - {doc_id.replace('.', '-')}_Full_Text.md
   - {doc_id.replace('.', '-')}_Summary.txt
   - {doc_id.replace('.', '-')}_Related_Resources.md
4. Use the file naming convention from the prompt (hyphens not dots in filenames).
5. If this document needs Phase 3 implementation specs based on classification,
   note what specs should be produced but do NOT attempt them in this run —
   flag them for a follow-up pass.
"""


def build_phase4_prompt(doc_id: str, pdf_path: str, doc_dir: Path) -> str:
    """Build a Phase 4 (verification & uplift) prompt for an existing doc."""
    prompt = PHASE4_PROMPT_FILE.read_text()

    # Inventory the existing outputs so the agent knows exactly what to uplift.
    impl_specs = sorted(doc_dir.glob("*_Implementation_Spec.md"))
    existing_tables = sorted((doc_dir / "annex_tables").glob("*.csv")) \
        if (doc_dir / "annex_tables").is_dir() else []
    related = sorted(doc_dir.glob("*_Related_Resources.md"))

    impl_list = "\n".join(f"  - {p.relative_to(ROOT)}" for p in impl_specs) or "  (none found)"
    tables_list = "\n".join(f"  - {p.relative_to(ROOT)}" for p in existing_tables) or "  (none — annex_tables/ directory is empty or missing)"
    related_list = "\n".join(f"  - {p.relative_to(ROOT)}" for p in related) or "  (none)"

    return f"""{prompt}

---

## Document Under Uplift

**Document ID:** {doc_id}
**Source PDF:** {pdf_path}
**Document directory:** standards/{doc_id}/

### Existing Phase 3 outputs (to be uplifted in place)
{impl_list}

### Existing annex_tables/ CSV files
{tables_list}

### Related resources
{related_list}

## Instructions for this run

1. Follow the Phase 4 Workflow described above (Steps 1–7).
2. Work in place on the implementation spec(s) listed above — do not create
   a parallel "v2" file. Commits should show a clean before/after diff.
3. Use `pdftotext -layout "{pdf_path}"` as the extraction entry point.
4. Write new CSVs under `standards/{doc_id}/annex_tables/` with the mandatory
   header comment (title, source page, verification summary).
5. Commit each uplift separately with an invariant-citing message, per the
   "Typical Phase 4 Commit Pattern" example. Do not batch unrelated fixes
   into a single commit.
6. When finished, run the Quality Checklist at the end of the prompt and
   report which items passed and which were deferred (with reasons).
"""


def process_document(doc: dict, dry_run: bool = False, model: str = "sonnet",
                     phase4: bool = False) -> bool:
    """Process a single document through Claude Code CLI.

    If phase4=True, run the Phase 4 verification & uplift pass over the
    existing impl spec rather than re-running Phase 1/2/3.
    """
    doc_id = doc["id"]
    pdf_path = doc["pdf"]
    classification = doc["classification"]

    mode = "Phase 4 uplift" if phase4 else "Phase 1–3"
    print(f"\n{'=' * 60}")
    print(f"Processing: {doc_id}  [{mode}]")
    print(f"  Title: {doc['title']}")
    print(f"  PDF: {os.path.basename(pdf_path)}")
    print(f"  Classification: {', '.join(classification) or 'TBD'}")
    if not phase4:
        print(f"  Phase 3 needed: {'Yes' if doc['needs_phase3'] else 'No'}")
    print(f"{'=' * 60}")

    if dry_run:
        print("  [DRY RUN] Would process this document.")
        return True

    if phase4:
        doc_dir = STANDARDS_DIR / doc_id
        impl_specs = sorted(doc_dir.glob("*_Implementation_Spec.md"))
        if not impl_specs:
            print(f"  ERROR: no *_Implementation_Spec.md found in {doc_dir}.")
            print(f"  Phase 4 uplifts an existing Phase 3 output; run Phase 1–3 first.")
            return False
        prompt = build_phase4_prompt(doc_id, pdf_path, doc_dir)
    else:
        prompt = build_prompt(doc_id, pdf_path, classification)

    LOG_DIR.mkdir(exist_ok=True)
    suffix = "_phase4" if phase4 else ""
    log_file = LOG_DIR / f"{doc_id.replace('.', '-')}{suffix}_{date.today()}.log"

    # Phase 4 vocoder-sized uplifts (multi-annex extractions + per-annex commits
    # + spec prose updates) consistently hit the old 30-turn cap mid-stream. 60
    # gives enough headroom for ~15 annex extractions with per-annex commits
    # without leaving the agent stranded with uncommitted working-tree edits.
    # Phase 1-3 runs are smaller and don't benefit from the extra headroom,
    # but there's no downside to the higher ceiling for them either.
    max_turns = "60" if phase4 else "30"
    cmd = [
        "claude",
        "-p", prompt,
        "--model", model,
        "--allowedTools", "Read,Write,Edit,WebSearch,WebFetch,Bash,Grep,Glob",
        "--max-turns", max_turns,
        "--output-format", "json",
    ]

    print(f"  Running claude CLI...")
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=1800,  # 30 minute timeout per document
            cwd=str(ROOT),
        )

        # Log the full output
        with open(log_file, "w") as f:
            f.write(f"Document: {doc_id}\n")
            f.write(f"Date: {date.today()}\n")
            f.write(f"Command: {' '.join(cmd[:6])}...\n")
            f.write(f"Return code: {result.returncode}\n")
            f.write(f"\n{'=' * 40} STDOUT {'=' * 40}\n")
            f.write(result.stdout)
            f.write(f"\n{'=' * 40} STDERR {'=' * 40}\n")
            f.write(result.stderr)

        # For Phase 4, a nonzero exit most often means the agent hit max_turns
        # while still making real progress (commits + uncommitted edits). Don't
        # treat that as a hard failure — instead, report it as "incomplete" and
        # let the user decide whether to resume or patch up manually.
        terminal_reason = None
        if phase4 and result.stdout:
            m = re.search(r'"terminal_reason"\s*:\s*"([^"]+)"', result.stdout)
            if m:
                terminal_reason = m.group(1)

        if result.returncode != 0 and not phase4:
            print(f"  FAILED (exit code {result.returncode})")
            print(f"  See log: {log_file}")
            return False

        # Check if output files were created
        doc_dir = STANDARDS_DIR / doc_id
        if phase4:
            # Success signal for Phase 4: new or modified impl spec, and/or
            # new CSVs in annex_tables/. We can't distinguish a no-op run
            # from a legitimate success without a git check, so just report
            # what's on disk and let the user review the log + diff.
            impl_spec_count = len(list(doc_dir.glob("*_Implementation_Spec.md")))
            csv_count = len(list((doc_dir / "annex_tables").glob("*.csv"))) \
                if (doc_dir / "annex_tables").is_dir() else 0
            if result.returncode != 0:
                status = f"INCOMPLETE (exit {result.returncode}"
                if terminal_reason:
                    status += f", terminal_reason={terminal_reason}"
                status += ")"
                print(f"  Phase 4 run {status}.")
                print(f"    The agent may have made progress without finishing.")
            else:
                print(f"  Phase 4 run complete (exit 0).")
            print(f"    Impl specs in {doc_id}/: {impl_spec_count}")
            print(f"    CSV tables in {doc_id}/annex_tables/: {csv_count}")
            print(f"    Review: git log --oneline -10 && git diff standards/{doc_id}/")
            print(f"    Log: {log_file}")
            # Return True on max_turns (real progress) and False on genuine errors.
            return terminal_reason != "error" if terminal_reason else result.returncode == 0
        output_files = list(doc_dir.glob("*_Summary.txt")) + list(doc_dir.glob("*_summary.txt"))
        if output_files:
            print(f"  SUCCESS — files written to {doc_id}/")
            print(f"  Log: {log_file}")
            return True
        else:
            print(f"  WARNING — claude completed but no summary file found in {doc_id}/")
            print(f"  Check log: {log_file}")
            return False

    except subprocess.TimeoutExpired:
        print(f"  TIMEOUT after 30 minutes")
        return False
    except FileNotFoundError:
        print("  ERROR: 'claude' CLI not found. Is Claude Code installed?")
        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(description="Process TIA-P25 specs through Claude Code")
    parser.add_argument("doc_id", nargs="?", help="Specific document ID to process")
    parser.add_argument("--batch", type=int, metavar="N", help="Process next N documents in priority order")
    parser.add_argument("--dry-run", action="store_true", help="Show what would be processed without doing it")
    parser.add_argument("--model", default="sonnet", help="Model to use (default: sonnet)")
    parser.add_argument("--list", action="store_true", help="List all processable documents")
    parser.add_argument("--force", action="store_true", help="Include not_required docs (measurement, conformance)")
    parser.add_argument("--phase4", action="store_true",
                        help="Run Phase 4 (verification & uplift) on an existing impl spec "
                             "instead of Phase 1-3. Requires a doc_id.")
    parser.add_argument("--vocoder-path", action="store_true",
                        help="Restrict the queue to documents on the vocoder data path "
                             f"({', '.join(VOCODER_PATH)}).")
    args = parser.parse_args()

    specs = load_specs()
    # Phase 4 runs on documents that already have Phase 3 outputs, so we need
    # to accept already-processed statuses too.
    docs = get_processable_docs(specs,
                                force=args.force or args.phase4,
                                vocoder_only=args.vocoder_path)

    if args.list:
        print(f"Processable documents ({len(docs)} with PDFs available):\n")
        for i, doc in enumerate(docs, 1):
            phase3 = " [NEEDS PHASE 3]" if doc["needs_phase3"] else ""
            print(f"  {i:3}. {doc['id']:<25} {doc['title'][:50]}{phase3}")
        return

    if args.phase4:
        # Phase 4 requires a specific doc_id and operates on any status
        # (the target is already past Phase 3, not in the pending queue).
        if not args.doc_id:
            print("ERROR: --phase4 requires a doc_id (e.g. TIA-102.BABA-A).")
            sys.exit(2)
        info = specs.get("specs", {}).get(args.doc_id)
        if not info:
            print(f"ERROR: {args.doc_id} not found in specs.toml.")
            sys.exit(1)
        doc_dir = STANDARDS_DIR / args.doc_id
        pdf = find_pdf(doc_dir) if doc_dir.is_dir() else None
        if not pdf:
            print(f"ERROR: no PDF found in standards/{args.doc_id}/.")
            sys.exit(1)
        doc = {
            "id": args.doc_id,
            "title": info.get("title", "Unknown"),
            "classification": info.get("classification", []),
            "pdf": pdf,
            "priority": 0,
            "needs_phase3": False,
        }
        success = process_document(doc, dry_run=args.dry_run, model=args.model, phase4=True)
        sys.exit(0 if success else 1)

    if args.doc_id:
        # Process a specific document
        doc = next((d for d in docs if d["id"] == args.doc_id), None)
        if not doc:
            # Maybe it's already processed or doesn't have a PDF
            print(f"Document {args.doc_id} not found in processable queue.")
            print("It may already be processed, or the PDF directory may not exist.")
            print("(If it's been through Phase 3 already and you want to uplift it, "
                  "use --phase4.)")
            sys.exit(1)
        success = process_document(doc, dry_run=args.dry_run, model=args.model)
        sys.exit(0 if success else 1)

    elif args.batch:
        # Process N documents
        to_process = docs[:args.batch]
        print(f"Processing {len(to_process)} documents (of {len(docs)} available):")
        succeeded = 0
        failed = 0
        for doc in to_process:
            if process_document(doc, dry_run=args.dry_run, model=args.model):
                succeeded += 1
            else:
                failed += 1
        print(f"\nDone: {succeeded} succeeded, {failed} failed")

    else:
        # Interactive: show the queue and let user pick
        if not docs:
            print("No pending documents with PDFs available.")
            return

        print(f"Processing queue ({len(docs)} documents ready):\n")
        for i, doc in enumerate(docs[:20], 1):
            phase3 = "*" if doc["needs_phase3"] else " "
            print(f"  {i:3}. [{phase3}] {doc['id']:<25} {doc['title'][:45]}")

        if len(docs) > 20:
            print(f"  ... and {len(docs) - 20} more (use --list to see all)")

        print(f"\n  * = needs Phase 3 implementation specs")
        print(f"\nUsage:")
        print(f"  python3 tools/process_specs.py TIA-102.BAAC-D        # process one (Phase 1-3)")
        print(f"  python3 tools/process_specs.py --batch 5              # process next 5")
        print(f"  python3 tools/process_specs.py --dry-run --batch 3")
        print(f"  python3 tools/process_specs.py --phase4 TIA-102.BABA-A  # uplift Phase 3 output")


if __name__ == "__main__":
    main()
