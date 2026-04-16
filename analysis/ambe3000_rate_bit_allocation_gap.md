# AMBE-3000 Rate-Index Table: Bit-Allocation and Naming Gaps

**Status:** Partially open — two coupled gaps surfaced while producing
`DVSI/AMBE-3000/annex_tables/halfrate_bit_allocations.csv` and
`rate_conversion_pairs.csv`.

**Scope:** AMBE-3000 per-rate bit allocations for rates beyond P25, and
the project-vs-chip r33/r34 naming collision.

**Opened:** 2026-04-16

---

## Gap 1 — Per-Field Bit Allocations for AMBE+2 Rates 35–61

### Finding

The DVSI AMBE-3000F User Manual v4.0 (Appendix 7.2, Table 119) tabulates
only three values per chip rate index:

- `total_rate` (bps)
- `speech_rate` (bps)
- `fec_rate` (bps)

Appendix 7.3 adds the six-word Rate Control Word (RCW) values and the
six hardware configuration pins per rate. That is the complete public
characterization of each rate.

The manual does **not** enumerate the per-field bit allocation
(`b0_pitch`, `b1_vuv`, `b2_gain`, `b3_prba24`, `b4_prba58`,
`b5_hoc1`..`b8_hoc4`) for any rate. This breakdown is required to
decode, encode, or rate-convert any AMBE+2 half-rate-family rate
(chip indices 33–61).

The AMBE-3000 decoder impl spec §10.2 claims the manual has this
breakdown ("it comes from: DVSI AMBE-3000F Manual Appendix (rate table
summary)"). That claim is incorrect; the manual has only the three
rate totals.

### What Public Sources Do Provide

- **BABA-A §12** (TIA-102): full per-field breakdown for P25 half-rate
  (7/5/5/9/7/5/4/4/3 = 49 info bits). Maps to chip indices 33 (with
  FEC) and 34 (no FEC).
- **US8595002 §3**: half-rate codebook structure (9-bit PRBA24, 7-bit
  PRBA58, 5/4/4/3-bit HOCs, 5-bit V/UV, 5-bit gain) and the "bit
  reduction trick" of using subset codebook entries. This describes the
  fixed codebook widths but does **not** specify which rates use
  reduced widths or by how much.
- **`annex_tables/rate_index_table.csv`**: chip total/speech/FEC
  extracted from manual 7.2.
- **`annex_tables/rate_control_words.csv`**: RCW values extracted from
  manual 7.3. The internal bit layout of the RCW words encodes the rate
  configuration but is DVSI-internal; the manual does not document
  which RCW bits select which codebook subset.

### Implication

- `halfrate_bit_allocations.csv` can only be populated normatively for
  chip indices 33 and 34. Indices 35–61 must be marked `pending`.
- `rate_conversion_pairs.csv` inherits this: any pair involving a
  chip index in 35–61 is marked `supported = pending` regardless of
  generation-match.

### Resolution Path

Empirical characterization against DVSI test vectors. The AMBE-3000 HDK
vectors (`tv-std/rNN/`) contain paired PCM/BIT files per rate. Procedure:

1. Encode a known PCM through the DVSI USB-3000 chip at a target rate.
2. Use chip 33 or 34 as a reference to establish the common parameter
   baseline.
3. Compare the output bit stream against the hypothesized bit-field
   layout; adjust per-field widths until the decoded parameters match
   the PCM analysis on the reference rate.
4. Document the resolved per-field widths as an update to
   `halfrate_bit_allocations.csv` and upgrade affected rows in
   `rate_conversion_pairs.csv` from `pending` to `yes`.

This work belongs in `~/blip25-mbe` (the Rust implementation +
conformance harness repo), not here. Findings come back as updates to
the CSVs and this analysis entry gets marked "Resolved" per-rate.

---

## Gap 2 — r33 / r34 Naming Collision

### Finding

Across the impl specs, Objectives, Protocol spec, and Test Vector
Reference, the project uses `r33` and `r34` as shorthand for:

| Project label | Algorithm                   | Bit count (total / info) |
|---------------|-----------------------------|-------------------------|
| `r33`         | P25 Phase 1 full-rate IMBE  | 144 / 88                |
| `r34`         | P25 Phase 2 half-rate AMBE+2 | 72 / 49                 |

These labels are P25-scoped aliases. They are **not** chip rate indices.

The AMBE-3000F chip's own rate-index table (Appendix 7.2) assigns:

| Chip index | Algorithm                               | Bit count (total / info) |
|-----------:|-----------------------------------------|-------------------------|
| 33         | AMBE+2 half-rate with FEC (= P25 half-rate) | 72 / 49             |
| 34         | AMBE+2 half-rate without FEC             | 49 / 49                 |

P25 full-rate IMBE is **not in the chip rate-index table**. It is
invoked via direct RCW programming:

- With FEC: `PKT_RATEP 0x0558 0x086B 0x1030 0x0000 0x0000 0x0190`
- Without FEC: `PKT_RATEP 0x0558 0x086B 0x0000 0x0000 0x0000 0x0158`

(Per `DVSI/AMBE-3000/AMBE-3000_Protocol_Spec.md` and
`AMBE-3000_Operational_Notes.md` §2.)

So:

- Project `r33` (P25 full-rate IMBE) ≠ chip index 33.
- Project `r34` (P25 half-rate AMBE+2) = chip index 33, **not** chip
  index 34. Chip index 34 is the no-FEC variant of the same algorithm.

### Where It Surfaces

- Decoder spec §10.1 table: labels "r33" as "P25 full-rate (IMBE), 88
  info / 144 total" — this matches the project alias but conflicts
  with the chip index 33 (which is actually 49 info / 72 total).
- Rate Converter spec §10.2: "r33 → r34 | P25 full-rate ingress,
  half-rate egress" — same project-alias usage.
- Encoder spec §10.1: "r33 | 7200 bps | full-rate dispatch" — also
  project-alias.
- `rate_index_table.csv`, `rate_control_words.csv`: use chip indexing.

The existing CSVs follow chip indexing; the impl specs use project
aliasing. Both are internally consistent but collide at the numeric
values 33 and 34.

### Resolution in the New CSVs (Option #1 Convention)

`halfrate_bit_allocations.csv` uses chip rate-index numbering (matching
existing CSVs). To accommodate P25 full-rate IMBE — which the chip
supports but does not index — two project-extension rows are added at
indices 62 and 63 (unused in the manual):

| CSV row | Algorithm                      | Project alias |
|---------|--------------------------------|---------------|
| 33      | P25 half-rate AMBE+2 with FEC  | r34 (with FEC) |
| 34      | AMBE+2 half-rate no-FEC        | r34-variant (no FEC) |
| 62      | P25 full-rate IMBE with FEC    | r33           |
| 63      | P25 full-rate IMBE no-FEC      | r33-nofec     |

`rate_conversion_pairs.csv` likewise uses chip indexing. The primary
spec target "r33 ↔ r34" becomes "{62, 63} ↔ {33, 34}" (eight pairs,
`category = p25_fullrate_to_halfrate`).

### Broader Resolution Path

The impl specs should be updated to either:

- **Option A**: Adopt chip indexing throughout and remove "r33"/"r34"
  as project aliases. Replace references with "chip r33" (AMBE+2
  half-rate) and "P25 full-rate IMBE" (explicit). Highest consistency
  cost but zero remaining ambiguity.
- **Option B**: Keep project aliases but add a disambiguation line in
  each spec's §1 explicitly stating that "r33/r34" here mean P25
  full-rate / half-rate and not the chip indices. Cheaper; some
  remaining collision risk.
- **Option C**: Define a distinct alias that doesn't collide — e.g.,
  `p25.fullrate` and `p25.halfrate` instead of `r33`/`r34`. Cleanest
  but a broad rename.

No resolution chosen yet — this is a separate, user-scoped decision
that doesn't block the CSVs themselves.

---

## Impact on Impl Specs

Decoder §10.2 currently states the manual has the bit-allocation
breakdown. That is incorrect and should be updated to cite this gap
entry. Fixed in the same commit that adds these CSVs.

Decoder §10.1, Encoder §10.1, Rate Converter §10.2 use the project
aliasing unchanged. Those are left for a follow-up pass once the
r33/r34 naming resolution option is chosen.

## Cross-References

- `DVSI/AMBE-3000/annex_tables/halfrate_bit_allocations.csv` and its
  `halfrate_bit_allocations_sources.md` sidecar
- `DVSI/AMBE-3000/annex_tables/rate_conversion_pairs.csv` and its
  `rate_conversion_pairs_sources.md` sidecar
- `standards/TIA-102.BABA-A/annex_tables/annex_f_gain_allocation.csv`
  (r33 L-dependent gain allocation, referenced from row 62/63)
- `standards/TIA-102.BABA-A/annex_tables/annex_g_hoc_allocation.csv`
  (r33 L-dependent HOC allocation, referenced from row 62/63)
- `docs/AMBE-3000_Next_Session_Prompts.md` — Prompt 1 is this CSV work;
  Prompt 2 (multi-subframe) is a separate dependency
