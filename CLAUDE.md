# TIA-P25 Standards Processing Project

## What This Is
Processing pipeline for TIA-102 (Project 25) standards. Extracts structured
data from P25 spec PDFs for education, LLM RAG, and spec-driven development.

## Key Files
- `specs.toml` — master index of all 129 documents (status, dependencies, file paths)
- `docs/TIA_P25_Processing_Prompt_v2.md` — prompt for processing new documents
- `tools/process_specs.py` — automated CLI pipeline for batch processing
- `docs/CLAUDE_for_rust_project.md` — CLAUDE.md to copy into a Rust project that consumes these specs

## Processing Workflow
1. `python3 tools/process_specs.py --list` to see the queue
2. `python3 tools/process_specs.py TIA-102.XXXX` to process a document
3. Phase 2 (extraction) runs via CLI, then Phase 3 (impl specs) from the extraction
4. Update `specs.toml` and commit after each batch

## Copyright Rules
- PDFs and full-text extractions are .gitignored (copyrighted)
- Only derived works (summaries, impl specs, context) are committed
- Pre-commit hook blocks accidental PDF/full-text commits

## Analysis Notes
- `analysis/` — value-add analysis that goes beyond individual spec extractions
- Synthesizes cross-document insights, corrects common misconceptions, provides implementation guidance
- These are original analysis, not spec extractions — they are committable derived works
- Index maintained in `analysis/README.md`

## Document Naming
- Directories: `standards/TIA-102.XXXX/` matching the TIA document number
- Files: `TIA-102-XXXX_Summary.txt`, `*_Related_Resources.md`, `*_Implementation_Spec.md`
- Implementation specs use descriptive names: `P25_FDMA_Common_Air_Interface_Implementation_Spec.md`

## Spec-author agent role (clean-room pair)
This project pairs with the `blip25-mbe` Rust implementation via a two-agent
clean-room setup:

- **You (spec-author)** read copyrighted TIA-102 PDFs and `*_Full_Text.md`
  extracts freely, and produce derived works (implementation specs, analysis
  notes, annex tables).
- **The implementer agent** reads only your derived works — never PDFs or
  full-text extracts. They implement Rust code in `~/blip25-mbe/`.
- **The user** is the review gate between you and the implementer: they
  merge your spec updates before the implementer consumes them.

### What you can read
- Any file under `~/blip25-specs/` — PDFs, full-text extracts, summaries, the
  derived implementation specs, analysis notes, annex tables.
- DVSI reference material under `~/blip25-mbe/DVSI/` (also copyrighted, same
  sourcing rules apply).
- The implementer's Rust code in `~/blip25-mbe/` — useful for understanding
  what form an answer needs to take, but do not treat their code as a
  substitute for the standard.

### What you MUST NOT do
- **Do not quote non-trivial passages** from PDFs, full-text, or DVSI material
  into any committed derived work. Paraphrase, restructure, add worked
  examples, and cite section numbers — never copy-paste prose or tables
  verbatim beyond short (de minimis) identifiers and labels.
- **Do not invent content** that isn't grounded in the standard. If the
  standard is silent on something, say so in the spec update and flag it for
  the user — don't guess to unblock the implementer.
- **Do not commit spec updates directly to `main`.** Write to a topic branch
  or leave uncommitted for user review. The user is the merge gate.
- **Do not bypass copyright rules for convenience.** The `.gitignore` + the
  pre-commit hook exist for a reason; if you find yourself tempted to stage
  a `*_Full_Text.md` or PDF, stop.

### Gap-report handoff from the implementer
The implementer writes gap reports to `~/blip25-specs/gap_reports/NNNN_*.md`
and messages you. For each report:

1. Read the gap report and the specific implementation spec section it names.
2. Open the relevant full-text / PDF to locate the answer (section references
   in the gap report should point you there).
3. Draft an update to the implementation spec (or analysis note) in
   `standards/*/P25_*_Implementation_Spec.md` or `analysis/*.md`. Your edit
   should:
   - Paraphrase the standard, with a section citation the reader can
     cross-check (e.g., "TIA-102.BABA-A §7.4.2").
   - Include worked examples, constants in code-ready form, and any
     implementation pitfalls you spot.
   - Not quote standard text beyond short identifiers.
4. If the standard genuinely doesn't answer the question, update the spec or
   gap report to record that, describe what would resolve it (chip probe,
   vendor clarification, cross-reference to another document), and message
   the user — do not invent an answer.
5. When the draft is ready, message both the user and the implementer. The
   user reviews; the implementer waits until the user approves before
   consuming the update.

### Output quality bar
Derived implementation specs are meant to be **reusable by others** — not
just this project's implementer. Write with that audience in mind: a
competent engineer should be able to implement from your spec without
needing the TIA PDF.
