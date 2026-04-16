# AMBE-3000 Rate-Index Table: Bit-Allocation and Naming Gaps

**Status:** Gap 1 open (per-field bit allocations for rates 35–61
deferred to blip25-mbe empirical work); Gap 2 resolved 2026-04-16 via
rename to `p25_fullrate` / `p25_halfrate` aliases.

**Scope:** AMBE-3000 per-rate bit allocations for rates beyond P25, and
the project-vs-chip r33/r34 naming collision that surfaced during the
bit-allocation CSV work.

**Opened:** 2026-04-16
**Gap 2 resolved:** 2026-04-16

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

### Resolution in the New CSVs (CSV keying)

`halfrate_bit_allocations.csv` uses chip rate-index numbering (matching
existing CSVs). To accommodate P25 full-rate IMBE — which the chip
supports but does not index — two project-extension rows are added at
indices 62 and 63 (unused in the manual). See the row map under
"Resolution Adopted — Option C" below.

`rate_conversion_pairs.csv` likewise uses chip indexing. The primary
spec target is the eight-pair set `{62, 63} ↔ {33, 34}` with
`category = p25_fullrate_to_halfrate`.

### Broader Resolution Path — Options Considered

The impl specs could be updated any of three ways:

- **Option A**: Adopt chip indexing throughout and remove "r33"/"r34"
  as project aliases. Replace references with "chip r33" (AMBE+2
  half-rate) and "P25 full-rate IMBE" (explicit). Highest consistency
  cost but zero remaining ambiguity.
- **Option B**: Keep project aliases but add a disambiguation line in
  each spec's §1 explicitly stating that "r33/r34" here mean P25
  full-rate / half-rate and not the chip indices. Cheaper; some
  remaining collision risk.
- **Option C**: Define distinct aliases that don't collide — e.g.,
  `p25_fullrate` and `p25_halfrate` instead of `r33`/`r34`. Cleanest
  but a broad rename.

### Resolution Adopted — Option C

**Decision (2026-04-16):** Option C. The project's existing
`~/blip25-mbe` implementation already uses `p25_fullrate` and
`p25_halfrate` as its peer crate/module identifiers (per that repo's
README), so adopting the same convention in the spec repo aligns the
two vocabularies.

Rename performed across the AMBE-3000 impl specs, annex table
sidecars, analysis cross-references, and the next-session prompts
file:

- `r33` → `p25_fullrate` (P25 Phase 1 full-rate IMBE, 144/88 bits)
- `r34` → `p25_halfrate` (P25 Phase 2 half-rate AMBE+2, 72/49 bits)

Rename scope was **project aliases in prose only**. Filesystem paths
referencing DVSI test-vector directories (`tv-std/r33/`,
`tv-std/r34/`, `tv-rc/r33→r34/` and similar) use chip rate-index
numbering directly and were left intact. Any correctness issues with
those paths — e.g., the impl spec's `tv-std/r33/*` citations may be
mis-indexed since chip index 33 is AMBE+2 half-rate, not P25
full-rate IMBE — are a separate test-vector-reference pass and do
not block this resolution.

### Resolution in the New CSVs

`halfrate_bit_allocations.csv` and `rate_conversion_pairs.csv` use
chip rate-index numbering as keys. P25 full-rate IMBE (which has no
chip rate index in the manual's Appendix 7.2) occupies
project-extension rows 62 and 63. Historical alias → CSV row map:

| CSV row | Algorithm                      | Current alias      | Old alias    |
|---------|--------------------------------|--------------------|--------------|
| 33      | AMBE+2 half-rate with FEC      | `p25_halfrate` w/FEC  | `r34` (w/FEC) |
| 34      | AMBE+2 half-rate no-FEC        | `p25_halfrate` no-FEC | `r34` (no-FEC) |
| 62      | P25 full-rate IMBE with FEC    | `p25_fullrate` w/FEC  | `r33` (w/FEC) |
| 63      | P25 full-rate IMBE no-FEC      | `p25_fullrate` no-FEC | `r33` (no-FEC) |

The primary spec target previously written "r33 ↔ r34" is now
"`p25_fullrate` ↔ `p25_halfrate`" in prose, backed by the
`{62, 63} ↔ {33, 34}` pair set (eight CSV rows,
`category = p25_fullrate_to_halfrate`).

---

## Impact on Impl Specs

Decoder §10.2 (resolved in the earlier commit): no longer claims the
manual contains per-field bit allocations; points at this gap entry.

Decoder §10.1, Encoder §10.1, Rate Converter §10.2 (resolved by the
Option C rename): now use `p25_fullrate` / `p25_halfrate` aliases
consistent with this entry and with blip25-mbe's module naming.

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
