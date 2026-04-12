#!/usr/bin/env python3
"""
Process pending TIA-P25 documents through Claude Code CLI.

Reads specs.toml to find documents with status="pending" that have PDFs available,
then processes them through the Phase 2 extraction pipeline (summary, context,
classification). Phase 3 (implementation specs) is flagged for manual follow-up
on algorithm/message_format/protocol documents.

Usage:
    python3 tools/process_specs.py                  # interactive: pick from queue
    python3 tools/process_specs.py TIA-102.BAAC-D   # process a specific document
    python3 tools/process_specs.py --batch 5         # process next 5 in priority order
    python3 tools/process_specs.py --dry-run         # show what would be processed
"""
import argparse
import glob
import json
import os
import subprocess
import sys
import tomllib
from datetime import date
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
STANDARDS_DIR = ROOT / "standards"
SPECS_TOML = ROOT / "specs.toml"
PROMPT_FILE = ROOT / "docs" / "TIA_P25_Processing_Prompt_v2.md"
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


def get_processable_docs(specs: dict, force: bool = False) -> list[dict]:
    """Return pending docs that have PDFs available, sorted by priority."""
    # Priority order from specs.toml comments — manually defined
    priority_order = [
        "TIA-102.AABC-E", "TIA-102.AABF-D", "TIA-102.BAAC-D",
        "TIA-102.AAAD-B", "TIA-102.BAAA-B", "TIA-102.BABA-A",
    ]

    processable_statuses = {"pending"} if not force else {"pending", "not_required", "needed"}

    docs = []
    for doc_id, info in specs.get("specs", {}).items():
        if info.get("status") not in processable_statuses:
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


def process_document(doc: dict, dry_run: bool = False, model: str = "sonnet") -> bool:
    """Process a single document through Claude Code CLI."""
    doc_id = doc["id"]
    pdf_path = doc["pdf"]
    classification = doc["classification"]

    print(f"\n{'=' * 60}")
    print(f"Processing: {doc_id}")
    print(f"  Title: {doc['title']}")
    print(f"  PDF: {os.path.basename(pdf_path)}")
    print(f"  Classification: {', '.join(classification) or 'TBD'}")
    print(f"  Phase 3 needed: {'Yes' if doc['needs_phase3'] else 'No'}")
    print(f"{'=' * 60}")

    if dry_run:
        print("  [DRY RUN] Would process this document.")
        return True

    prompt = build_prompt(doc_id, pdf_path, classification)

    LOG_DIR.mkdir(exist_ok=True)
    log_file = LOG_DIR / f"{doc_id.replace('.', '-')}_{date.today()}.log"

    cmd = [
        "claude",
        "-p", prompt,
        "--model", model,
        "--allowedTools", "Read,Write,Edit,WebSearch,WebFetch,Bash,Grep,Glob",
        "--max-turns", "30",
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

        if result.returncode != 0:
            print(f"  FAILED (exit code {result.returncode})")
            print(f"  See log: {log_file}")
            return False

        # Check if output files were created
        doc_dir = STANDARDS_DIR / doc_id
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
    args = parser.parse_args()

    specs = load_specs()
    docs = get_processable_docs(specs, force=args.force)

    if args.list:
        print(f"Processable documents ({len(docs)} with PDFs available):\n")
        for i, doc in enumerate(docs, 1):
            phase3 = " [NEEDS PHASE 3]" if doc["needs_phase3"] else ""
            print(f"  {i:3}. {doc['id']:<25} {doc['title'][:50]}{phase3}")
        return

    if args.doc_id:
        # Process a specific document
        doc = next((d for d in docs if d["id"] == args.doc_id), None)
        if not doc:
            # Maybe it's already processed or doesn't have a PDF
            print(f"Document {args.doc_id} not found in processable queue.")
            print("It may already be processed, or the PDF directory may not exist.")
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
        print(f"  python3 tools/process_specs.py TIA-102.BAAC-D   # process one")
        print(f"  python3 tools/process_specs.py --batch 5         # process next 5")
        print(f"  python3 tools/process_specs.py --dry-run --batch 3")


if __name__ == "__main__":
    main()
