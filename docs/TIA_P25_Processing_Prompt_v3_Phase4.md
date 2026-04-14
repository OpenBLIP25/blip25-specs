# TIA-P25 Processing — Phase 4: Verification & Uplift

This prompt defines **Phase 4**, a verification and completion pass over the
output of Phase 3 (implementation specs). It exists because Phase 3's
classification-dependent prompts permit deferral of large numerical tables and
do not require invariant checks on the data that *is* extracted. The result
has been that matrix OCR errors, incomplete annexes, and unresolved
"see Equation N" cross-references slip through as committed output.

Phase 4 is an **uplift pass on an existing implementation spec**. It does not
re-run Phase 1 or Phase 2. It reads what Phase 3 produced, identifies every
place where data is deferred, placeholder, or unverified, and closes those
gaps — programmatically where possible, with verification invariants in every
case.

## When to Run Phase 4

Run Phase 4 on any document whose implementation spec contains one or more of:

- Placeholder tokens: `todo!()`, `TBD`, `placeholder`, `needs extraction`,
  `structure described, full table not extracted`, `needs raster verification`
- Cross-references that were never inlined: `see Equation N–M`, `see Annex X`,
  `refer to Figure N` with no inline follow-up
- Numerical structures that were not independently verified (FEC generator
  matrices, interleave tables, bit-allocation tables, quantizer levels,
  block-length partitions)
- Implementation code in a specific language (Rust, C++, etc.) rather than
  language-neutral C with CSV data files

## Phase 4 Workflow

### Step 1 — Inventory and Escalation Decision
Read the existing implementation spec(s) in `standards/<DOC_ID>/` and the
source PDF. Produce an internal checklist of every item needing uplift:

1. **Deferred tables** (Annex E, F, G, H, J, etc. flagged as not extracted)
2. **Unresolved cross-references** (prose says "see Eq. N", but Eq. N isn't
   inlined)
3. **Unverified extracted data** (tables present but with no invariant check
   run against them)
4. **Language-specific code blocks** (Rust, C++, Python) that should be C
5. **Placeholder function stubs** (`todo!()`, `unimplemented`, etc.)

**Escalation check — before proceeding to Step 2, assess whether an uplift is
the right response at all, or whether a ground-up Phase 1-3 rerun is needed.**

Escalate to ground-up rerun (STOP Phase 4, produce an escalation report
instead of editing the spec) if any of the following hold:

- **Structural misalignment:** the impl spec's section outline does not
  correspond to the PDF's section outline in an obvious way — e.g., major
  sections are missing entirely, or the impl spec describes a different
  document than the PDF.
- **Wrong classification:** the PDF content doesn't match the Phase 1
  classification recorded in `specs.toml` (e.g., spec was classified as
  PROTOCOL but the PDF is mostly ALGORITHM material, or vice versa).
- **Extensive prose drift:** the impl spec's prose contains statements that
  contradict the PDF directly (not just OCR artifacts in tables, but
  substantive factual errors in the English text).
- **More than ~30% of the invariant checks in Step 3 fail** on the extracted
  data, or the failures span multiple independent artifact types suggesting
  a broken extraction pipeline rather than isolated OCR hiccups.
- **Missing whole annexes:** the PDF contains normative annexes that are
  entirely absent from the impl spec (not just "structure described, data
  not extracted" — truly missing, including from the table of contents in
  §7 or equivalent).

When escalating, produce `standards/<DOC_ID>/PHASE4_ESCALATION.md` with:

```
# Phase 4 Escalation: <DOC_ID>

## Decision
Escalate to Phase 1-3 rerun (ground-up).

## Reasons
- <bulleted list citing specific findings, with line numbers in the impl spec
  and page numbers in the PDF>

## Recommended scope for rerun
<which parts of the impl spec need to be regenerated vs. which can be
preserved; if classification is wrong, propose the corrected classification>

## Phase 4 findings preserved for the rerun
<any verified CSV extractions, cross-reference resolutions, or other work
that should NOT be thrown away and should carry forward into the rerun>
```

Do not make destructive changes to the existing impl spec when escalating.
The point of the escalation report is to hand off cleanly to a new Phase 1-3
run, not to half-fix a structurally broken spec.

If none of the escalation conditions hold, proceed to Step 2 normally.

### Step 2 — Programmatic Extraction
For each deferred table, extract it from the PDF using a verified pipeline:

```
pdftotext -layout <pdf> /tmp/doc_text.txt
# then a Python parser that:
#   - uses re.findall with row-shape regexes, not hand-counted columns
#   - applies an invariant check before returning (see Step 3)
#   - emits a CSV with a header comment citing the PDF page(s)
```

Store each table as a CSV under
`standards/<DOC_ID>/annex_tables/annex_<x>_<description>.csv`.
The CSV header MUST include:

```
# TIA-102.<DOC_ID> annex_<x>_<description>.csv — <Title>
# Source: <DOC_ID> PDF, page(s) <N>
# Extracted from pdftotext -layout output with verification: <invariant summary>
<column,names>
<data rows>
```

Never hand-transcribe numerical data. If the PDF layout defeats pdftotext
(e.g., rotated pages, bit-field diagrams, vector-drawn figures), use PyMuPDF
for structured extraction or fall back to visual reading of the rasterized
page — and mark those rows "visually extracted" in the CSV header.

### Step 3 — Invariant Verification (MANDATORY)

Every extracted numerical artifact MUST pass at least one invariant check
before being committed. Minimum invariants by artifact type:

| Artifact | Invariant |
|----------|-----------|
| FEC generator matrix | Full row rank over GF(2); equivalently, no two rows equal. If cyclic, every row = x · (previous row) mod g(x) where g(x) is the generator polynomial. |
| FEC parity-check matrix | Orthogonality: G · H^T = 0 over GF(2). |
| Interleave / bit-frame table | Exhaustive coverage: every (vector, bit) position in the source frame appears exactly once across the table. |
| Partition table (e.g., block lengths) | Each row sums to the partition's total (e.g., L̂ for Annex J). |
| Bit-allocation table | Sum of bit counts per L equals the frame's total information bits (e.g., Annex F + Annex G must sum to 88 − 8 − 6 − K̃ per L̂). |
| Quantizer levels | Row count matches the bit-width (64 for 6-bit, 32 for 5-bit, etc.); values monotonic where the spec says so. |
| Cyclic generator polynomial | Constant term = 1; degree = n − k; (x^n − 1) is divisible by g(x). |

When an invariant fails, **do not silently fix or skip** — investigate. The
failure is either (a) a real OCR error (like the `c9(5)` artifact in Annex H
or the rank-deficient Golay matrix from the first extraction) that must be
corrected and documented, or (b) a legitimate feature of the data that the
invariant needs to be relaxed for (e.g., Annex G allowing B_m = 0 for
untransmitted HOCs). In either case, the CSV header must record what was
checked and any deviations from the obvious invariant.

### Step 4 — Cross-Reference Resolution

For every "see Equation N–M", "see Annex X", "refer to Figure N" in the
spec's prose:

- **Inline the referenced content** in the spec, with page-number citation
- OR **explicitly mark the reference as out-of-scope** with a one-sentence
  rationale (e.g., "The full PRBA24 codebook is 512 × 3 = 1536 values and
  is not inlined here; see annex_tables/annex_p_prba24.csv")

The goal is that a reader never has to open the PDF to understand what the
spec says. They may open the PDF to cross-check numbers, but the spec text
should be complete.

### Step 5 — Language Neutralization

Per project convention (see memory: "Spec documents are language-neutral"):

- Bulk numerical data → CSV files under `annex_tables/`
- Inline code snippets → C (stdint.h, stdbool.h, typedef struct, static
  const arrays, function prototypes). NOT Rust, C++, Python, etc.
- Binary literals → hex constants for C99 compatibility
- Algorithm sketches that are too complex for a C prototype → prose
  pseudocode, NOT language-specific code

### Step 6 — Placeholder Sweep

Grep the updated impl spec for every instance of:
- `todo!`, `unimplemented!`, `TBD`, `XXX`, `FIXME`
- "placeholder", "not extracted", "needs verification", "needs raster"

Each must be resolved (content added, or explicit out-of-scope marker with a
rationale), not left in the committed output. The spec is a reference
document, not a scratchpad.

### Step 7 — Commit

One commit per logical uplift (e.g., "Fill Annex X tables", "Verify Y
matrix against generator polynomial", "Convert Rust snippets to C"). Each
commit message must:

- Cite the invariant that was verified, with the result
- Note any OCR fixes applied and how they were detected
- Reference the source PDF page(s)

## Quality Checklist

Before declaring Phase 4 complete on a document:

- [ ] Step 1 escalation check was performed; either no escalation conditions
      were met, or `PHASE4_ESCALATION.md` was written and the run stopped
      cleanly without partial edits to the impl spec
- [ ] Every table listed in Phase 3 §7.2 "Tables NOT Extracted" has been
      either extracted into a CSV with invariant verification, or its
      non-extraction has been justified in writing
- [ ] Every `todo!()` / placeholder has been removed or replaced
- [ ] Every "see Eq. N" / "see Annex X" / "see Figure N" in prose has been
      inlined or explicitly deferred
- [ ] At least one invariant per numerical artifact has been checked and
      documented in the CSV header or spec text
- [ ] All code in the impl spec is C (or pseudocode); no Rust / C++ /
      Python blocks survive
- [ ] `git log --oneline` for the uplift shows specific, reviewable commits
      with cited invariants, not a single "Fix vocoder spec" mega-commit

## Out of Scope for Phase 4

- **New Phase 2 extractions.** Phase 4 runs on an existing impl spec; it
  does not re-generate full-text summaries.
- **Re-classifying the document.** Phase 1 classifications are assumed
  correct.
- **External cross-document synthesis.** That belongs in `analysis/`, not
  in the per-document impl spec.

## Typical Phase 4 Commit Pattern (worked example)

```
commit 1: Fix <matrix> generator matrix in <DOC> spec
  - OCR introduced 5 row errors including 2 rank-violating duplicates
  - Corrected against PDF Section X.Y, page N
  - Verified against canonical generator polynomial g(x) = ...

commit 2: Fill <DOC> gap: <topic>
  - Section X.Y was referenced but not inlined
  - Extracted equations N–M directly from PDF
  - Bit budget verified (144 bits total, 114 PN indices consumed)

commit 3: Fill <DOC> gaps: Annex E, F, G, H, J tables
  - Programmatic extraction via pdftotext -layout + parser
  - Verification: Annex J rows sum to L; Annex H covers 144 bits
  - OCR fix applied and documented (symbol 51: c9(5) → c0(5))

commit 4: Move annex tables to CSV; keep spec language-neutral
  - Data moved from inline Rust arrays to annex_tables/*.csv
  - Schema documented in spec; C consumer signatures shown

commit 5: Convert remaining code blocks in <DOC> spec to C
```

The BABA-A uplift (commits `1833066` → `24cb0ce`) is a working reference.
