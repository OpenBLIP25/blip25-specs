# Phase 4 Findings Log

Running log of **real spec correctness issues** and **material ambiguities**
caught during Phase 4 verification + uplift runs. This file is the persistent
home for findings that would otherwise live only in commit messages.

Two reasons for this file to exist:

1. **Resilience to pipeline reruns.** `standards/<DOC>/*_Implementation_Spec.md`
   is a regenerable artifact. If Phase 1-3 is re-run, ad-hoc inline fixes may
   be overwritten. `analysis/` is not touched by the pipeline, so findings
   here survive.

2. **Cross-document grep-ability.** "Did anyone find an OCR error in a
   generator matrix?" is a hard question to answer by walking commit
   messages. A single file with a consistent structure makes it trivial.

**Format per entry:** one header with the document ID, a dated bullet list
of findings, each citing (a) the PDF location, (b) the invariant or cross-
reference that caught it, (c) the commit hash that applied the fix, and
(d) the in-tree location where the fix now lives.

Entries are in-place corrections to the impl spec **and** the CSV
(where applicable). When an entry fixes a real spec-data bug (not just an
extraction artifact), it's flagged as **SPEC BUG** for emphasis.

---

## TIA-102.BABA-A (IMBE / AMBE Vocoder)

### 2026-04-13 / 2026-04-14 — Phase 4 uplift

- **SPEC BUG — [23,12] Golay generator matrix, 5 wrong rows** including 2
  rank-violating duplicates (rows 0 == 11, rows 6 == 7).
  *Location:* BABA-A §7.3, page 37.
  *Invariant that caught it:* matrix rank over GF(2) must equal 12; two
  equal rows makes that impossible.
  *Fix:* commit `9766491`. Rewritten matrix in impl spec §1.5.1; verified
  against canonical generator polynomial `g(x) = x^11 + x^9 + x^7 + x^6 +
  x^5 + x + 1` (octal 6265, 0xAE3).

- **SPEC BUG — Annex H (full-rate interleave) symbol 51 right-bit** printed
  as `c9(5)` in the PDF; c9 doesn't exist (only c0..c7).
  *Location:* BABA-A Annex H, page 102.
  *Invariant that caught it:* exhaustive coverage of (vector, bit) pairs —
  `c0(5)` was the only missing bit, so `c9(5)` must be `c0(5)`.
  *Fix:* commit `0dbb9c2`. CSV `annex_tables/annex_h_interleave.csv`,
  header notes the correction.

- **SPEC BUG — Annex S (half-rate interleave) symbol 34 bit1** printed as
  `c0(5)` in BABA-A PDF; cross-validated against BBAC-1 Annex E (burst
  symbol 44 = frame symbol 34), which prints `c0(6)`.
  *Location:* BABA-A Annex S, page 143.
  *Invariant that caught it:* exhaustive coverage check after OCR fix; then
  cross-validated against the BBAC-1 publication of the same table.
  *Fix:* commit `8e54534` + documentation in `3458dae`. CSV
  `annex_tables/annex_s_interleave.csv`, header cites the BBAC-1 cross-ref.

- **Annex B size discrepancy** — impl spec claimed 211 values n=−105..105,
  actual PDF has 301 values n=−150..150. The 211-value window is Annex I
  (synthesis), not Annex B (analysis / initial pitch estimation).
  *Location:* BABA-A Annex B, pages 79–80.
  *Fix:* commit `8e54534`. CSV `annex_tables/annex_b_analysis_window.csv`,
  symmetry check PASS at 301 values.

- **Missing decode algorithms** inlined from §6.4 / §6.5: inverse uniform
  quantizer (Eq. 68/71), per-block inverse DCT (Eq. 69/73), log-magnitude
  prediction (Eq. 75–79), frame-sync bit role (Eq. 80).
  *Fix:* commit `9143238`. Impl spec §1.8–§1.9 with all equations inlined.
  Not a spec bug — a scope gap from the original VOCODER branch of Phase 3,
  which explicitly deferred "full vocoder implementation" to JMBE/AMBE.

- **AMBIGUITY — Asymmetric forward/inverse DCT convention (Eq. 60–61 vs
  Eq. 69/73).** The encoder uses uniform `1/N` on every coefficient with
  no α-weighting; the decoder uses α(k) = {1, 2, 2, …} with no `1/N`.
  This is *not* an orthonormal DCT-II / DCT-III pair. Downstream
  implementers writing a forward/inverse round-trip test may default to
  textbook orthonormal scaling (`√(1/N)`, `√(2/N)`) and see their test
  drift — the fix is to match the spec convention exactly.
  *Location:* BABA-A §6.3.1 Eq. 60–61 (encoder) and §6.4.1 Eq. 69 +
  §6.4.2 Eq. 73 (decoder), pages 25, 29, 30.
  *Invariant that caught it:* downstream implementer's round-trip test
  (`block_dct_forward_then_inverse_is_identity` for n ∈ {1, 2, 3, 5, 7, 10})
  failed until the forward was rewritten to match Eq. 61's uniform `1/N`.
  *Fix:* analysis/vocoder_decode_disambiguations.md §9 (reference
  implementations). Impl spec §1.8.3 now cross-references the analysis
  entry with a short asymmetry note.

- **Missing V/UV band-to-harmonic mapping** for both rates.
  *Location:* full-rate in BABA-A §5.2 Eq. 32–33; half-rate in §13.2
  Eq. 147–149.
  *Fix:* commit `8fcd05d`. Impl spec §1.3.2 (full-rate) and §2.3.6
  (half-rate), with a note about the pdftotext rendering dropping the `l`
  multiplicand in Eq. 147.

- **Full_Text extraction error — Eq. 34 K formula** paraphrased as
  `floor(2ω̂₀/π × 56)` when the PDF says `floor((L̂+2)/3) if L̂ ≤ 36 else 12`.
  *Location:* `TIA-102-BABA-A_Full_Text.md` §4.1 (gitignored file; locally
  corrected, will need fix applied if regenerated).
  *Fix:* noted in impl spec §1.3.2 and full clarification in
  `vocoder_decode_disambiguations.md` §6.

- **Bit prioritization scan algorithm** (Figure 22 in PDF) never inlined.
  Three plausible interpretations (bit-plane, parameter-sequential,
  significance-weighted) would produce incompatible 88-bit frame layouts.
  *Location:* BABA-A §7.1, pages 33–35.
  *Fix:* commit `425dc15`. Impl spec §1.4 with the full bit-plane scan
  algorithm + pre-computed CSVs:
  `annex_tables/imbe_bit_prioritization.csv` (4224 rows, 48 L × 88 bits)
  and `annex_tables/ambe_bit_prioritization.csv` (49 rows).

---

## TIA-102.BAAA-B (FDMA Common Air Interface)

### 2026-04-13 — Phase 4 uplift

- **SPEC BUG — Frame sync value** had internal contradiction. Initial
  value `0x5575F5FF77FF` was correct, but a later "reconciliation"
  paragraph introduced `0x5575F5FF7FFF` (0x7F where 0x77 should be).
  *Location:* BAAA-B §6.5 and Table 15, page 43.
  *Invariant that caught it:* cross-referencing the two in-spec statements.
  Expanded the bit-table (0111 0111 1111 1111 = 0x77FF) against the summary
  hex value.
  *Fix:* commit `8b732e4`. Any FDMA decoder built against the wrong value
  would have failed to sync entire voice streams.

- **SPEC BUG — GF(2^6) EXP/LOG tables, 8+ OCR errors.**
  Wrong values at e=10, 11, 12, 13, 42, 43, 60, 61 in GF64_EXP, plus
  multiple errors in GF64_LOG.
  *Location:* BAAA-B Annex D Table 6, pages 60–61.
  *Invariant that caught it:* Galois field round-trip check
  `EXP[LOG[b]] == b` for b=1..63, plus α^6 = α+1 from the primitive
  polynomial x^6 + x + 1 (0x43).
  *Fix:* commit `8519688`. Values regenerated programmatically; CSV
  `annex_tables/gf64_lookup_tables.csv` with invariant citation in header.
  Silent breakage for any Reed-Solomon implementation built against the
  OCR'd tables.

- **SPEC BUG — BCH(63,16,23) NID generator matrix was an all-zero
  placeholder** in the impl spec. Network ID coding was literally
  unimplementable from the spec as shipped.
  *Location:* BAAA-B Table 19, page 45.
  *Invariant that caught it:* rows of a generator matrix cannot be all
  zero. Trivial check, caught by "is the matrix rank = 16?" → no, it's 0.
  *Fix:* commit `eac9c97`. CSV `annex_tables/bch_nid_generator_matrix.csv`
  with identity-portion verified and generator polynomial monic.

- **5 missing CSVs** extracted and verified: HDU (396 rows), LDU1 (864),
  LDU2 (864), GF64 lookups, BCH matrix.
  *Fix:* commit `f7e9aca`. All with invariant citations in CSV headers.

---

## TIA-102.BBAC-A (TDMA MAC Layer)

### 2026-04-13 — Phase 4 uplift

- **SPEC BUG — I-ISCH (40,9,16) generator matrix, 2 row transcription
  errors** from the Phase 3 draft.
  Row 0: bits 36 and 35 were swapped (PDF: `1000 1...`, draft: `1001 0...`).
  Row 4: bit 26 set instead of bit 27 (`0x021007F7FF` → `0x020807F7FF`).
  *Location:* BBAC-A Table 15, page 28.
  *Invariant that caught it:* minimum Hamming distance = 16 check, plus
  GF(2) rank = 9 and "column 1 all zero" note from the PDF.
  *Fix:* commit `43f9577`.

- **SPEC BUG — DUID (8,4,4) pre-computed codeword table** was corrupted.
  *Location:* BBAC-A Table 12, page 24.
  *Invariant that caught it:* systematic-form check on the generator
  matrix, then reverifying the derived codewords by encoding each 4-bit
  info value through the matrix.
  *Fix:* commit `43f9577`.

- **Extensive verification pass** (no bugs, all PASS): GF(64) EXP/LOG
  tables, RS(63,35,29) generator polynomial (28 roots α^1..α^28), RS
  generator matrix (Horner evaluation of all 35 rows), RS derived codes
  for 6 applications, sync sequences.

- **Note — mega-commit hygiene:** this document was uplifted in a single
  commit (`43f9577`) that bundled 6 separate logical fixes (2 bugs + 4
  verifications + language neutralization). Future Phase 4 runs should
  split these per the "one commit per logical uplift" guidance in the
  Phase 4 prompt. Not reverted because the work itself is correct.

---

## TIA-102.BBAB (TDMA Physical Layer)

### 2026-04-14 — Phase 4 uplift

- **No spec bugs found.** Useful null result: Phase 4's invariant-driven
  approach isn't producing false positives when everything is actually
  correct. BBAB is a DSP-heavy PHY spec; most of its content is modulation
  math (H-DQPSK, H-CPM, π/8-DQPSK) that's already in algebraic form.

- **2 placeholders resolved** without creating bugs:
  - §7.3 ramp profile: the `MISSING FROM` tag replaced with "implementation
    defined" citation to BBAB §4.3 pp. 14–16 (the PDF explicitly leaves ramp
    shape normatively open).
  - §10.1 I-ISCH coset code: replaced `MISSING FROM` with cross-reference
    to BBAC-A (the authoritative source for the I-ISCH generator matrix;
    an architectural split between physical layer and MAC layer that Phase
    3 missed).

- **H-D8PSK informative annex (§13) added** with `annex_A_hd8psk_bit_symbol_mapping.csv`.
  Gray-code ordering + odd-integer symbol values {−7, −5, …, +7}
  verified.

- *Commit:* `c1f25b6..2a9c23c` (two per-topic commits — correct hygiene).

---

## TIA-102.AABF-D (Link Control Word Formats and Messages)

### 2026-04-14 — Phase 4 uplift

- **SPEC BUG — LCO $07 (Telephone Interconnect Answer Request), octet 5 digit label**
  PDF page 12 section 7.3.7 prints octet 5 as "Digit 8 | Digit 10" instead of
  "Digit 9 | Digit 10". The nibble-pair sequential pattern (1-2, 3-4, 5-6, 7-8, 9-10)
  unambiguously identifies "Digit 8" as a typo for "Digit 9".
  *Location:* TIA-102.AABF-D §7.3.7, page 12.
  *Invariant that caught it:* sequential nibble-pair pattern coverage — digit 9
  was missing from the sequence.
  *Fix:* commit `2e50f98`. Impl spec §4.5.

- **COMPLETENESS GAP — LCO $11 (Unit Registration Command) field layout**
  Prior extraction placed Reserved at octet 5 and had ambiguous Target ID
  boundaries. PDF section 7.3.10 clearly shows: octets 5-7 = Target ID (24 bits),
  octet 8 = Reserved.
  *Location:* TIA-102.AABF-D §7.3.10, page 14.
  *Invariant that caught it:* total field bit-count: 8+20+12+24+8 = 72 bits (PASS
  with corrected layout; FAIL with original which had a phantom extra Reserved).
  *Fix:* commit `2e50f98`. Impl spec §4.10.

- **COMPLETENESS GAP — LCO $20 (System Service Broadcast) field split**
  Prior extraction had Reserved at octet 3, Services Available at octets 4-6 (only
  partial), and Services Supported at octets 7-8 (only 16 bits). The PDF shows
  a clean 3+3 octet split: Available = octets 3-5 (24 bits), Supported = octets 6-8
  (24 bits). The "Reserved" octet 3 was fabricated in the extraction.
  *Location:* TIA-102.AABF-D §7.3.13, page 16.
  *Invariant that caught it:* total bit-count: 8+8+24+24 = 64 information bits
  in octets 1-8 (PASS with corrected layout).
  *Fix:* commit `2e50f98`. Impl spec §4.21.

- **COMPLETENESS GAP — Subscriber Unit Address table constant name mismatch**
  Prior spec Section 6 labeled $FFFFFD as `ADDR_FNE_DISPATCH` and $FFFFFE as
  `ADDR_SYSTEM_DEFAULT`. PDF page 36 defines: $FFFFFC = "Reserved for FNE Use",
  $FFFFFD = "System Default", $FFFFFE = "Registration Default". The prior constant
  names were mapped one entry off and the two middle entries were conflated.
  *Location:* TIA-102.AABF-D §7.4 "Subscriber Unit Address" field, page 36.
  *Invariant that caught it:* address range boundary non-overlap check — 4 distinct
  special values above $FFFFFB (PASS with correction).
  *Fix:* commit `2e50f98`. Impl spec §6 and
  `annex_tables/subscriber_unit_address_table.csv`.

- **AMBIGUITY — LCO usage matrix for LCO 9 and 10 (conventional vs trunked only)**
  Prior extraction Section 2.1 marked LCO 9 (Source ID Extension) and LCO 10
  (Unit-to-Unit VCU - Extended) with 'x' in the Conventional Outbound/Inbound
  columns. PDF Table 3 (page 39) does NOT mark these for conventional — they are
  trunked only. The error is plausible because LCO 0 (Group Voice Channel User),
  which can trigger Source ID Extension, does support conventional — but the
  extension message itself is restricted to trunked per the table.
  *Location:* TIA-102.AABF-D Table 3, page 39.
  *Invariant that caught it:* cross-reference between impl spec Section 2.1 and
  PDF Table 3.
  *Fix:* commit `2e50f98`. Impl spec §2.1 usage matrix corrected.

---

## Cumulative Score (2026-04-14)

- **8 real spec correctness bugs caught and fixed**: 3 in BABA-A (Golay
  matrix, Annex H c9(5), Annex S c0(5)), 3 in BAAA-B (frame sync, GF64,
  BCH matrix), 2 in BBAC-A (I-ISCH, DUID), 1 in AABF-D (LCO $07 digit label
  OCR artifact). One of the BABA-A items is also an inter-spec cross-validation
  (Annex S confirmed against BBAC-1).
- **Dozens of completeness gaps filled** (missing algorithms, missing
  tables, missing cross-references, missing disambiguations).
- **Zero false-positive invariant failures** — every invariant that fired
  was either a real bug, a legitimate zero-allocation edge case (Annex G
  `B_m = 0` for unallocated HOC coefficients), or a data feature worth
  documenting (Annex B actually being 301 values not 211).

---

## How to Extend This File

When running Phase 4 on a new document (or following up on a user-reported
spec gap), append a new `## TIA-102.<DOC>` section (or add a dated entry
under an existing one) in the same format:

```
### YYYY-MM-DD — <reason for entry>

- **[SPEC BUG | AMBIGUITY | COMPLETENESS GAP] — <one-line summary>**
  *Location:* <PDF section, page>.
  *Invariant that caught it:* <rank / sum / coverage / cross-ref / etc.>
  *Fix:* commit `<short hash>`. <In-tree location of fix.>
```

Use "SPEC BUG" only when the data in the TIA PDF itself is wrong (OCR
artifacts count; wrong-in-the-PDF counts). Use "AMBIGUITY" for cases where
the PDF is consistent but leaves multiple plausible interpretations.
Use "COMPLETENESS GAP" for missing-content cases.

**Automatic updates:** per the Phase 4 prompt (v3, Step 8), agents should
append findings here as part of the uplift run, not just in commit
messages. Users investigating the history should start here, not with
`git log --grep`.
