# AMBE-3000 Next-Session Prompts

Two self-contained prompts for future Claude Code sessions in
`~/blip25-specs`, in priority order. Each prompt is pastable without
modification and includes its own project context, reading list, and
deliverables.

**Empirical work happens elsewhere.** The decoder validation harness
and the test-vector sweeps that resolve §12 open questions (γ_w, phase
γ, low-L̃ boundary, log₂(0) floor, VQ tie-breaking) run in
`~/blip25-mbe` (the Rust implementation). Findings come back here as
`analysis/` entries and impl-spec §12 updates. See
`~/blip25-mbe/CLAUDE.md` for that repo's workflow; the gap-report
process it describes is how empirical results land in this repo.

**Current state as of 2026-04-16** (needed by both prompts below):
- Three AMBE-3000 impl specs drafted and pushed:
  `DVSI/AMBE-3000/AMBE-3000_{Decoder,Encoder,Rate_Converter}_Implementation_Spec.md`
- BABA-A PDF spec bug fixed (Eq. 69 gain IDCT denominator: use 6, not J̃_i —
  editorial error in the printed standard, verified against PDF 2026-04-16)
- ρ predictor citations corrected: half-rate decoder predictor is BABA-A
  Eq. 185 (page 65), not Eq. 200 (which is the first of the Eq. 200–205
  frame-repeat block on page 71)
- Three new `analysis/` entries on predictor-state separation, closed-loop
  encoder feedback, and the Hilbert-transform interpretation of the
  phase-regen kernel

---

## Prompt 1 — Per-Rate Config CSVs (Non-P25 Rates)

**Copy everything between the triple-dashes below into a fresh session.**
This prompt expands AMBE-3000 coverage beyond P25 (p25_fullrate, p25_halfrate) to the full
r0–r63 rate table.

---

Project: `/home/chance/blip25-specs` — clean-room software implementation
of DVSI AMBE-3000 (P25 vocoder).

TASK: Produce two new CSVs in `DVSI/AMBE-3000/annex_tables/` that
parameterize the decoder and rate converter specs for non-P25 rates.
Both CSVs are currently flagged as TODO in the impl specs.

CSVs TO CREATE:
1. **`halfrate_bit_allocations.csv`** — for each chip rate index
   (r0–r63), the bit allocation `b̂_k` widths when the rate uses the
   AMBE+2 half-rate skeleton. Columns:
   `rate_index, b0_pitch, b1_vuv, b2_gain, b3_prba24, b4_prba58,
   b5_hoc1, b6_hoc2, b7_hoc3, b8_hoc4, total_info_bits, fec_scheme,
   interleaver, notes`. Rows where the rate does not use AMBE+2
   half-rate skeleton (e.g., full-rate IMBE variants, AMBE-1000 /
   AMBE+ rates) should have `fec_scheme = "N/A"` or a pointer to
   which alternate pipeline they use.
2. **`rate_conversion_pairs.csv`** — per (source, target) rate pair,
   the conversion configuration. Columns: `source_rate, target_rate,
   supported, voicing_xform, magnitude_xform, unvoiced_compensation,
   predictor_rho, special_notes`. Pairs where both rates share the
   same AMBE+2 half-rate skeleton should use `predictor_rho = 0.65`
   and all transforms enabled. Identity pairs (source = target) get
   all transforms as no-ops. Cross-family pairs (AMBE → AMBE+2) need
   TODO notes.

SOURCES:
- **DVSI AMBE-3000F Manual v4.0** at
  `/mnt/share/P25-IQ-Samples/DVSI Software/Docs/AMBE-3000F_manual.pdf` —
  rate table is the authoritative source. Look for chapter/section
  tabulating bits/frame, FEC, and interleaver per rate.
- **US8595002 col. 22 lines 10–25** — describes the "bit reduction
  trick" where non-P25 half-rate variants use subset codebooks. Not
  sufficient to derive the CSVs on its own, but confirms the general
  structure.
- **`annex_tables/rate_index_table.csv`** — already committed;
  contains basic rate metadata (total bps, speech bps, FEC bps,
  AMBE generation). Cross-reference when populating the new CSVs.
- **`annex_tables/rate_control_words.csv`** — RCW packing per rate;
  some bit-allocation detail may be inferrable from RCW bits.

READ FIRST:
1. `DVSI/AMBE-3000/AMBE-3000_Decoder_Implementation_Spec.md` §10
   (rate dispatch) — the CSVs feed this section
2. `DVSI/AMBE-3000/AMBE-3000_Rate_Converter_Implementation_Spec.md`
   §10 — `rate_conversion_pairs.csv` consumers
3. Existing `annex_tables/rate_index_table.csv` and
   `annex_tables/rate_control_words.csv` for CSV-format conventions
4. DVSI AMBE-3000F manual rate table chapter — primary source

CONVENTIONS:
- CSVs use comma-separated, one header row, LF line endings, UTF-8
- Rate index column first, always
- String fields quoted only when they contain commas
- TODO entries use `pending` or similar sentinel, not empty fields
- Add a `.txt` or `.md` sidecar file in `annex_tables/` describing
  the source of each CSV and where each column value came from

SCOPE DECISION BEFORE STARTING:
Some of the 64 rates in the AMBE-3000 rate table are for earlier
AMBE generations (AMBE-1000, AMBE+) that don't use AMBE+2's codebook-
based quantization. For those, the bit allocation CSV can't be
populated in the same format. Two options:
(A) Include all 64 rates with placeholder `"non-AMBE+2"` for the
    codebook-based fields — transparent but many rows mostly-empty
(B) Only include the AMBE+2 / half-rate-skeleton rates; separate
    CSVs or sidecar notes for the others

Recommend (A) for completeness. Run past me if you pick (B).

Start by opening the AMBE-3000F manual rate table chapter and mapping
out what's there (how many rates, how the bit allocation is specified
— is it per-rate table or derived from RCW fields?). Sketch the CSV
schema and propose a row or two before filling out all 64.

---

## Prompt 2 — Multi-Subframe Joint Quantization (US6199037)

**Copy everything between the triple-dashes below into a fresh session.**
This prompt extends the three DVSI specs to cover multi-subframe rates.

---

Project: `/home/chance/blip25-specs` — clean-room software implementation
of DVSI AMBE-3000 (P25 vocoder).

TASK: Extend the three DVSI AMBE-3000 impl specs to cover multi-subframe
rates — AMBE-3000 chip rates r17/r18 and similar where each channel
frame encodes 2 consecutive 20 ms subframes with jointly-quantized
pitch and voicing metrics per US6199037.

CURRENT STATE:
- All three impl specs (decoder, encoder, rate converter) currently
  cover **single-subframe rates only**. §10 in each spec flags
  multi-subframe as "out of scope for first cut" / TODO.
- Encoder spec §10.2 describes the general US6199037 structure at a
  high level:
  - Joint pitch: 4-bit scalar on average log-pitch + 6-bit vector on
    inter-subframe difference
  - Joint voicing: 6-bit codebook on 16-element (8 bands × 2
    subframes) vector, or split as two 8-element 6-bit codebooks
- The data structures `b̂_k[2]` (for two-subframe rates) are mentioned
  but not detailed

SOURCES:
- **US6199037** (expired 2017-12-04) — primary patent for joint
  subframe pitch and voicing quantization. The existing
  `DVSI/AMBE-3000/AMBE-3000_Patent_Reference.md` §3 has high-level
  notes; implementing rates r17/r18 needs the full algorithmic detail
  (codebook indexing, interpolation rules for the scalar+vector
  hybrid, cold-start behavior for the 2-subframe state machine).
  Flag if you need the patent text I don't have summarized.
- **DVSI AMBE-3000F Manual** — rate table will identify which rates
  are 2-subframe (look for `subframes = 2` or equivalent).
- **`annex_tables/rate_conversion_pairs.csv`** — if Prompt 1 has
  landed, use it to identify which cross-rate conversions involve
  subframe-count changes (1↔2 needs time alignment logic per rate
  converter spec §1.6).

READ FIRST:
1. `DVSI/AMBE-3000/AMBE-3000_Patent_Reference.md` §3 (US6199037)
2. `DVSI/AMBE-3000/AMBE-3000_{Decoder,Encoder,Rate_Converter}_Implementation_Spec.md`
   §10 in each (current single-subframe dispatch)
3. Rate Converter spec §1.6 (multi-subframe considerations, currently
   scoped out)

DELIVERABLES:
1. New subsection in each of the three specs detailing multi-subframe
   handling:
   - **Decoder**: new §10.x "Multi-Subframe Rates" — how to recover
     2× parameter sets from one channel frame, pitch dequant via the
     joint 4-bit scalar + 6-bit vector, voicing dequant via the 6-bit
     codebook. Output: 2× `(ω̃₀, L̃, ṽ_l, M̃_l)` tuples per channel
     frame → 320 PCM samples instead of 160.
   - **Encoder**: new §10.x — joint quantization of 2 consecutive
     analyzed subframes, forward direction of the decoder's recovery.
     VQ search over the joint voicing codebook.
   - **Rate Converter**: new §10.x — time alignment logic for
     1↔2 subframe conversions (accumulate 2 source frames per target
     frame for 1→2; emit 2 target frames per source frame for 2→1).
2. Either extend existing `rate_conversion_pairs.csv` with 2-subframe
   columns OR create `multi_subframe_pairs.csv` sidecar, whichever
   fits the Prompt 1 decision
3. Commit as one logical unit: "DVSI AMBE-3000: multi-subframe
   (US6199037) joint quantization for r17/r18"

CONVENTIONS:
- C snippets not Rust
- Cite US6199037 by column/line when pulling equations
- Follow the existing spec structure: section intros, bit-tables,
  code fragments, cross-frame state additions to the §4.4 tables
- Flag any empirical open questions in §12 of each spec (DVSI's
  exact codebook values for r17/r18 are not in the patent — they
  need black-box characterization against test vectors, which
  happens in `~/blip25-mbe` — do not try to resolve those here)

Start by reading the three §10 sections and the patent reference,
then sketch the structure of the multi-subframe additions (which
specs get which sections, how the data structures grow). Run past
me before drafting.

---

## Meta: When to Use Which Prompt

- **Prompt 1** is independent and unblocks non-P25 rate coverage.
  Dependency: DVSI AMBE-3000F manual access.
- **Prompt 2** extends the three specs to 2-subframe rates. Can be
  drafted against US6199037 alone; empirical codebook values for
  r17/r18 get resolved later via `~/blip25-mbe` validation.

Reasonable next session in this repo: Prompt 1 (CSVs). For anything
related to the decoder validation harness or resolving §12 open
questions empirically (γ_w, phase γ, low-L̃ boundary, log₂(0) floor,
VQ tie-breaking), switch to `~/blip25-mbe`; the resulting
`analysis/` entries land back here.
