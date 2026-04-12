# specs/ — P25 TIA-102 Standards Reference

This directory contains extracted and derived reference materials from the TIA-102
(Project 25) standards suite, organized for use during Rust implementation.

## Directory Structure

```
specs/
├── specs.toml                  # Cross-reference index (tracks processing status)
├── .gitignore                  # Keeps copyrighted material out of public repo
│
├── raw/                        # Direct extractions (PRIVATE — .gitignored full texts)
│   ├── TIA-102-BBAC-1_Full_Text.md      ← .gitignored (reproduction)
│   ├── TIA-102-BBAC-1_Summary.txt       ← committed (original analysis)
│   └── ...
│
├── impl/                       # Implementation specs (PUBLIC — derived works)
│   ├── TIA-102-BBAC-1_Context.md
│   ├── P25_TDMA_Scrambling_Implementation_Spec.md
│   ├── P25_TDMA_MAC_Message_Parsing_Spec.md
│   ├── P25_TDMA_Annex_E_Burst_Bit_Tables.md
│   └── ...
│
├── pdfs/                       # Original TIA PDFs (PRIVATE — .gitignored)
│   ├── TIA-102.BBAC-1.pdf
│   └── ...
│
└── prompt/                     # Processing prompt template
    └── TIA_P25_Processing_Prompt_v2.md
```

## What Gets Committed to Public Repo

✅ `specs.toml` — Index file (no copyrighted content, just metadata)  
✅ `impl/*` — Implementation specs, opcode tables, matrix data, test vectors  
✅ `raw/*_Summary.txt` — Original analytical summaries  
✅ `raw/*_Context.md` — Standards status and cross-references  
✅ `prompt/` — Processing prompt template  

## What Stays Private

🚫 `pdfs/*` — Original TIA standard PDFs  
🚫 `raw/*_Full_Text.md` — Near-verbatim text extractions  

## Processing Workflow

1. Obtain TIA PDF (purchase from TIA store or via institutional access)
2. Start new Claude conversation, upload PDF + processing prompt
3. Claude classifies the document and produces Phase 2 + Phase 3 outputs
4. Move files into `raw/` and `impl/` directories
5. Update `specs.toml` with processing metadata
6. For large docs (>50 pages), Claude will produce split PDFs for chunked processing

## Spec Processing Priority

See the priority queue at the bottom of `specs.toml` for recommended processing order.
The dependency chain starts with the TDMA MAC layer and works outward through
control channel messages, link control words, and encryption.
