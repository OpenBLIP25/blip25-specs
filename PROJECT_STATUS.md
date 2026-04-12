# Project Status — 2026-04-13

## Documents Still Needed

**Pending TIA email response:**

| Document | What We're Asking | Priority |
|----------|-------------------|----------|
| TIA-102.BABB | Does it exist as a separate document from BBAB? | LOW — likely dead, BABG covers everything |
| TIA-102.BCAD MATLAB files | Where are the TDMA burst test vector M-files now that FTP server is down? | HIGH — bit-exact TDMA test vectors for your implementation |

## Batch Processing Retries (Have PDFs, Need Reprocessing)

| Document | Title | Issue |
|----------|-------|-------|
| TIA-102.BAAD-A | Conventional Procedures (rev A) | Timeout |
| TIA-102.BABA | Vocoder Description (original IMBE) | Timeout |
| TIA-102.BACF | ISSI Packet Data Services | Timeout |
| TIA-102.BAEB-B | IP Data Bearer (rev B) | Timeout |
| TSB-102-C | Documentation Suite Overview (rev C) | Timeout |
| J-STD-200 | Interworking Between LMR and 3GPP | Failed |

These just need another run — we have all the PDFs.

## Nothing Else Needed

Everything else is either processed, in the collection, or out of scope. To summarize where the collection stands:

| Category | Status |
|----------|--------|
| TIA-102 P25 specs | ~80 processed, 6 retries needed, ~44 pending specs.toml update |
| BABG + test vectors | Complete (PDF, WAVs, summary, context) |
| ITU-T P.862 PESQ | Complete (spec + C source, compiles and runs) |
| ITU-T P.863 POLQA | Complete (spec + algorithm descriptions + 450 test PCMs) |
| NTIA reports | 2 processed (fireground noise, tandem vocoding) |
| CAAA-F reference C code | Extracted and compiles |
| TSB-88 series | 6 PDFs organized, TSB-88.3-C processed |
| TSB-102-A | Processed (foundational P25 doc) |
| EDACS IS-69.5 | Processed (IMBE cross-reference) |
| Analysis notes | 2 written (wire vs codec, missing specs) |

The only thing worth actively chasing is the **BCAD MATLAB test vectors** — everything else is in hand or not needed.
