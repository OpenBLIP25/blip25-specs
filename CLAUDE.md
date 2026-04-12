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
