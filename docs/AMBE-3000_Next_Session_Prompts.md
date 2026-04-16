# AMBE-3000 Next-Session Prompts

Four self-contained prompts for future Claude Code sessions, in priority
order. Each prompt is pastable without modification and includes its own
project context, reading list, and deliverables — so any fresh session
can pick up where the previous work left off.

**Current state as of 2026-04-16** (needed by every prompt below):
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
- No validation against test vectors has been executed yet

---

## Prompt 1 — Decoder Validation Harness + First Test-Vector Runs

**Copy everything between the triple-dashes below into a fresh session.**

---

Project: `/home/chance/blip25-specs` — clean-room software implementation
of DVSI AMBE-3000 (P25 vocoder).

TASK: Build a validation harness that runs the
`AMBE-3000_Decoder_Implementation_Spec.md` through DVSI's pre-computed
test vectors and reports per-stage deviation from reference. Execute the
first four priority test cases from the decoder spec §11.3.

Target quality bar: working tool + empirical results table. The harness
is throwaway tooling (Python is fine, not normative). The results table
becomes input to a follow-up session that resolves empirical open
questions flagged in spec §12.

WHAT'S ALREADY IN PLACE:
- `DVSI/AMBE-3000/AMBE-3000_Decoder_Implementation_Spec.md` — 1290 lines,
  normative spec. §11 (validation plan) and §11.4 (debug order on
  mismatch) lay out the work plan.
- `DVSI/AMBE-3000/AMBE-3000_Test_Vector_Reference.md` — test vector
  catalog; tv-std has 62 rate configs + P25 variants, tv-rc has 64 rate-
  conversion configs. PCM and BIT files paired in each rate directory.
- `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` —
  BABA-A baseline the decoder spec cites heavily (§1.12 full-rate
  synthesis, §2.x half-rate).
- Test vector root: `/mnt/share/P25-IQ-Samples/DVSI Software/Docs/AMBE-3000_HDK_tv/`
  (97 input × 67 output configurations).
- Test recipes: `cmpstd.txt`, `cmpp25.txt`, `cmpp25a.txt`, `cmpp25x.txt`
  in the HDK_tv directory.

READ FIRST, in order:
1. `DVSI/AMBE-3000/AMBE-3000_Decoder_Implementation_Spec.md` §1 (scope),
   §11 (validation plan), §12 (open questions — harness should surface
   evidence for these)
2. `DVSI/AMBE-3000/AMBE-3000_Test_Vector_Reference.md`
3. Skim `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` §1.12
   (full-rate synthesis) and §2.x (half-rate path)
4. `analysis/vocoder_decode_disambiguations.md` — catches common
   implementation errors; §2 and §3 are load-bearing

EXPECTED HARNESS DELIVERABLES:
1. `tools/decoder_validation/` directory with:
   - `run_tv_std.py` — reads `.bit` from tv-std/rNN/, decodes via a
     software decoder implementation you'll shim (see below), writes
     `.pcm` to a working directory, compares to reference `.pcm`
   - `diff_mbe_params.py` — given a `.bit` frame, dumps
     `(ω̃₀, L̃, ṽ_l, M̃_l, M̄_l)` per the decoder spec §11.4 debug
     order: diff parameters FIRST, then phase, then synth output
   - `metrics.py` — SNR, MCD, per-frame bit-exact pass/fail for FEC
     stage, perceptual match for synthesis stages
   - `results.md` — table of test-vector ID × stage × pass/fail or
     quality metric, matching the format of decoder spec §11.2
2. A decoder shim. Do **not** implement the full decoder in this session —
   scope is too large. Options:
   - Use an existing OSS decoder (mbelib, JMBE, imbe_vocoder) as the
     "software decoder" being validated. Flag in results.md that the
     "software decoder" is currently an OSS implementation pending
     clean-room Rust replacement. This characterizes how close existing
     OSS implementations are to DVSI reference and pre-validates the
     approach.
   - Or: a minimal Python decoder that covers FEC + parameter recovery
     only (bit-exact stages per spec §11.2), with synthesis delegated
     to an OSS library for the perceptual comparison

PRIORITY TEST VECTORS (decoder spec §11.3):
1. `tv-std/r34/` silence inputs — validates §3 FEC decode + §9.4 frame
   dispatch (should be 100% bit-exact; if not, FEC layer has a bug)
2. `tv-std/r34/` DTMF / single-harmonic tones — validates §4 parameter
   recovery, §5 phase regen on trivial spectra, §6 low-L synthesis
3. `tv-std/r34/alert*` or equivalent voiced speech — end-to-end
4. `tv-std/r33/` full-rate equivalent

KEY OBSERVATIONS TO CAPTURE IN `results.md`:
- FEC stage bit-exactness (expect 100%)
- Parameter-recovery bit-exactness (expect 100% on valid frames)
- Phase regeneration: perceptual match (SNR measured)
- Unvoiced synthesis: expected NOT bit-exact; γ_w calibration value
  that best matches (candidate input to future analysis entry)
- Voiced synthesis: sample-near-exact if phase matches
- Overall PCM: target SNR ≥ 15 dB, MCD < 1.5 dB on tv-std/r34

CONVENTIONS:
- Harness tooling can be Python — not normative, just a workbench
- Do NOT commit PCM / BIT test-vector files (copyrighted, .gitignored)
- DO commit the harness code and results.md
- Spec docs are language-neutral C for any pseudocode — but tools/ is
  free to use Python
- Reference PDF sections/pages (BABA-A §X, page N) when citing the
  standard, not the impl spec's synthesized section numbers

Start by reading the three files above (Decoder spec §1/§11/§12,
Test_Vector_Reference, disambiguations §§2–3), then sketch the harness
architecture — which OSS decoder you'd shim, how the per-stage dumps
work, what failure modes you expect to find. Run it past me before
writing code.

---

## Prompt 2 — Resolve Empirical Open Questions via Test-Vector Runs

**Copy everything between the triple-dashes below into a fresh session.**
**Prerequisite:** Prompt 1's validation harness must be in place.

---

Project: `/home/chance/blip25-specs` — clean-room software implementation
of DVSI AMBE-3000 (P25 vocoder).

TASK: Use the existing decoder validation harness
(`tools/decoder_validation/`) to resolve the empirical open questions
flagged in the AMBE-3000 decoder / encoder / rate-converter specs §12.
Each resolved question becomes a new `analysis/` entry.

OPEN QUESTIONS TO RESOLVE (from decoder spec §12):
1. **Phase regen scaling γ empirical value.** Spec §5.2 uses
   γ = 0.44 per US5701390 col. 7 line 55. Sweep γ ∈ [0.3, 0.6] in
   0.02 steps against tv-std/r34 voiced speech. Find the γ that
   minimizes PCM-domain MCD against DVSI reference. If the patent
   value is within the best-match band, confirm; if not, document
   empirical optimum. Output: `analysis/ambe3000_phase_gamma.md`.
2. **γ_w unvoiced synthesis calibration.** Inherited from BABA-A (see
   `analysis/vocoder_decode_disambiguations.md` §11 for the state
   of the investigation — spec value 146.6 produces ~150× overshoot
   vs DVSI reference). Sweep γ_w ∈ [0.5, 200] logarithmically against
   unvoiced-heavy test vectors (fricatives). Output: extend the
   existing disambiguation note with AMBE+2-specific findings, OR
   create `analysis/ambe3000_gamma_w_resolved.md` if the resolution
   differs from BABA-A.
3. **Low-L̃ quarter-boundary of phase regen.** Spec §5.3 / decoder
   spec §12 item 3: test whether `l ≤ L̃/4` vs `l < L̃/4` vs
   `l ≤ floor(L̃/4)+1` matches DVSI reference. Run on low-L̃
   test vectors (high-pitch female speech or synthetic sinusoids at
   high f₀). Output: `analysis/ambe_phase_quarter_boundary.md`.
4. **Log-magnitude floor behavior.** Spec §5.4 item 6: `log2(M̄_l = 0)`
   → floor at log2(1e-10), treat-as-zero, or skip. Test the three
   behaviors against unvoiced-band-dense test vectors. Output:
   `analysis/ambe_log_mag_floor.md`.
5. **VQ tie-breaking convention.** Encoder spec §12 item 4: when two
   codebook entries score equally, does DVSI pick lowest index,
   highest index, most-recent? Test via synthetic PCM input designed
   to produce tie conditions (smooth spectra on codebook decision
   boundaries). Output: `analysis/ambe3000_vq_tiebreak.md`.

READ FIRST:
1. The three AMBE-3000 impl specs' §12 sections for context on each
   open question
2. `analysis/vocoder_decode_disambiguations.md` §11 for the existing
   γ_w investigation
3. `tools/decoder_validation/results.md` from Prompt 1's output
4. `analysis/ambe_predictor_state_separation.md`,
   `analysis/ambe_encoder_closed_loop_predictor.md`,
   `analysis/ambe_phase_regen_hilbert.md` — recent structural-invariant
   entries; new analysis/ entries should match their format (status,
   scope, date, test methodology, related entries)

OUTPUT FORMAT (each analysis/ entry):
- Frontmatter: Scope / Status / Date / Date Resolved
- Investigation methodology (which test vectors, what sweep parameters)
- Empirical results table
- Conclusion + implementation recommendation
- Updates to the impl spec §12 to mark the question resolved (follow
  the pattern used in the decoder spec §12 "Previously listed here
  and now resolved" block for the Eq. 200 → Eq. 185 resolution)

CONVENTIONS:
- Commit each resolved open question as a separate commit with a
  clear title matching recent analysis-entry commits (e.g., "phase
  regen γ empirical resolution: γ = 0.44 confirmed / 0.50 optimum /
  ..."). Don't bundle multiple resolutions into one commit.
- Update `analysis/README.md` index as each new entry lands
- Update the relevant impl spec §12 open-question section to cite
  the new analysis entry and mark it resolved

PRIORITY ORDERING (do in this order unless harness data shows otherwise):
1. γ_w calibration — longest-standing open question, biggest quality
   impact on unvoiced bands
2. Phase γ — most load-bearing for overall perceptual quality
3. Low-L̃ boundary — affects high-pitch speech
4. Log-magnitude floor — numerical detail, small impact
5. VQ tie-breaking — encoder-side, only matters for bit-exact match

Start by reading the §12 sections and sketching an investigation plan
for item 1 (γ_w) — what test vectors, what sweep, how to measure.
Run plan past me before executing. Each open question resolution
should be a single-session commit, so keep scope tight.

---

## Prompt 3 — Per-Rate Config CSVs (Non-P25 Rates)

**Copy everything between the triple-dashes below into a fresh session.**
This prompt expands AMBE-3000 coverage beyond P25 (r33, r34) to the full
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

## Prompt 4 — Multi-Subframe Joint Quantization (US6199037)

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
- **`annex_tables/rate_conversion_pairs.csv`** — if Prompt 3 has
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
   fits the Prompt 3 decision
3. Commit as one logical unit: "DVSI AMBE-3000: multi-subframe
   (US6199037) joint quantization for r17/r18"

CONVENTIONS:
- C snippets not Rust
- Cite US6199037 by column/line when pulling equations
- Follow the existing spec structure: section intros, bit-tables,
  code fragments, cross-frame state additions to the §4.4 tables
- Flag any empirical open questions in §12 of each spec (DVSI's
  exact codebook values for r17/r18 are not in the patent — they
  need black-box characterization against test vectors)

Start by reading the three §10 sections and the patent reference,
then sketch the structure of the multi-subframe additions (which
specs get which sections, how the data structures grow). Run past
me before drafting.

---

## Meta: When to Use Which Prompt

- **Prompt 1** unlocks everything else. It produces the first empirical
  measurements and establishes the harness pattern.
- **Prompt 2** depends on Prompt 1's harness. Runs in tight loops per
  open question; each open question is one session.
- **Prompt 3** is independent of Prompts 1–2 (different scope: CSVs,
  not validation). Can run in parallel. Dependency on DVSI manual
  access.
- **Prompt 4** depends on Prompts 1 and 3 conceptually (harness to
  validate + CSVs to parameterize) but can be drafted against the
  patent alone and validated later.

Reasonable next session: Prompt 1. If harness is already in place from
a prior session, go to Prompt 2. If non-P25 coverage is urgent (e.g.,
DMR/D-STAR bridging comes up), Prompt 3.
