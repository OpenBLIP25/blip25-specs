# halfrate_bit_allocations.csv — Sources and Status

## Purpose

Per-rate bit allocation (`b̂_k`) widths for the AMBE+2 half-rate quantizer
skeleton, keyed by chip rate index (0–61 per DVSI AMBE-3000F manual v4.0
Appendix 7.2 Table 119) plus project-extension rows 62 and 63 for P25
full-rate IMBE (which is not in the chip's rate-index table).

## Status

Partial. Normative rows are populated; others are `pending` or `n/a` by
generation.

| Rows    | Status                                                        |
|---------|---------------------------------------------------------------|
| 0–15    | Filled `n/a` — AMBE-1000 generation, not AMBE+2 half-rate skeleton |
| 16–32   | Filled `n/a` — AMBE-2000 generation, not AMBE+2 half-rate skeleton |
| 33      | **Normative** — P25 Phase 2 half-rate with FEC, BABA-A §12    |
| 34      | **Normative** — AMBE+2 half-rate no-FEC; same bit fields as 33 |
| 35–61   | `pending` — AMBE+2 variants; bit widths not in public sources |
| 62      | **Normative extension** — P25 full-rate IMBE with FEC, BABA-A §1 + §6 |
| 63      | **Normative extension** — P25 full-rate IMBE no-FEC           |

## Per-Field Sourcing

### Chip indices 33, 34 (AMBE+2 half-rate)

All `b_k` values from TIA-102.BABA-A Revision A, §12 (Half-Rate MBE
Quantizer), cross-referenced with US8595002 §3:

| Field        | Width | Codebook                        | Source                 |
|--------------|-------|---------------------------------|------------------------|
| `b0_pitch`   | 7     | Annex L pitch table (120 entries) | BABA-A §2.3, Annex L |
| `b1_vuv`     | 5     | Annex M V/UV codebook (32 entries, 8 bands) | BABA-A Annex M, US8595002 Table 2 |
| `b2_gain`    | 5     | Annex O differential gain (32 levels) | BABA-A Annex O |
| `b3_prba24`  | 9     | Annex P PRBA24 VQ (512 entries) | BABA-A Annex P         |
| `b4_prba58`  | 7     | Annex Q PRBA58 VQ (128 entries) | BABA-A Annex Q         |
| `b5_hoc1`    | 5     | Annex R HOC B5 (32 entries × 4-vec) | BABA-A Annex R, §12.14 |
| `b6_hoc2`    | 4     | Annex R HOC B6 (16 entries × 4-vec) | BABA-A Annex R       |
| `b7_hoc3`    | 4     | Annex R HOC B7 (16 entries × 4-vec) | BABA-A Annex R       |
| `b8_hoc4`    | 3     | Annex R HOC B8 (8 entries × 4-vec) | BABA-A Annex R        |
| **Total**    | **49**| — | Matches chip manual speech rate 2450 bps × 0.020 s |

FEC (chip 33 only): `[24,12]` extended Golay on û₀ (most sensitive 12 bits)
→ 24 coded bits; `[23,12]` Golay on û₁ (next 12 bits) → 23 coded bits,
with data-dependent scrambling seeded from û₀; remaining 25 info bits
unprotected. 4×18 row-column interleaver applied. Per BABA-A §2.4 and
US8595002 §3.

### Chip indices 62, 63 (project-extension rows — P25 full-rate IMBE)

These rows are NOT in the AMBE-3000F chip rate-index table (Appendix
7.2). P25 full-rate IMBE is invoked via direct RCW programming rather
than a rate index:

- With FEC (row 62): `PKT_RATEP 0x0558 0x086B 0x1030 0x0000 0x0000 0x0190`
- Without FEC (row 63): `PKT_RATEP 0x0558 0x086B 0x0000 0x0000 0x0000 0x0158`

Per `DVSI/AMBE-3000/AMBE-3000_Protocol_Spec.md` §RCW table and
`AMBE-3000_Operational_Notes.md` §2.

Bit allocation per TIA-102.BABA-A §1 (IMBE full-rate):

| Field     | Width      | Source                                          |
|-----------|------------|-------------------------------------------------|
| `b0_pitch`| 8          | BABA-A §1.5 (256-level pitch quantizer)        |
| `b1_vuv`  | L-dependent (K = ⌈L/3⌉ bits) | BABA-A §1.6 (K V/UV groups) |
| `b2_gain` | 6          | BABA-A §1.6 (gain quantizer)                    |
| `b3_prba24`..`b8_hoc4` | L-dependent | BABA-A Annex F (gain coefficient allocation by L) and Annex G (HOC allocation by L) — see `standards/TIA-102.BABA-A/annex_tables/annex_f_gain_allocation.csv` and `annex_g_hoc_allocation.csv` |
| **Total** | **88** for every valid L (9..56) | BABA-A §1.5 invariant |

FEC (row 62): 4× `[23,12]` Golay codes (u0–u3, 48 info bits → 92 coded),
3× `[15,11]` Hamming codes (u4–u6, 33 info bits → 45 coded), 7 uncoded
bits (u7), 1 parity bit = 144 total. Annex H interleaver. Per BABA-A §1.4
+ §6.

### Chip indices 35–61 (AMBE+2 variants) — `pending`

The DVSI AMBE-3000F manual (v4.0, Appendix 7.2) tabulates only
`total_rate / speech_rate / fec_rate` per rate index and does not
enumerate per-field bit widths. US8595002 §3 describes the half-rate
codebook structure and the "bit reduction trick" of using subset
codebooks for reduced bit rates, but does not specify the per-rate
subset sizes.

**Resolution path:** Empirical characterization against DVSI test
vectors. Decode a known-plaintext PCM through `tv-std/rNN/*` for each
non-P25 AMBE+2 rate, compare against known-full (chip 33/34) bit
allocations, and derive per-field widths from the bit-rate arithmetic +
codebook-size constraints. This work belongs in the `~/blip25-mbe`
implementation repo; results come back here as `analysis/` entries.

### Chip indices 0–32 (AMBE-1000 / AMBE-2000) — `n/a`

These generations use older quantizer families (AMBE-1000 or AMBE-2000)
that do not share the AMBE+2 half-rate skeleton. A CSV with the same
column schema cannot represent their bit allocations. The total frame
sizes and FEC overhead are captured in the `total_info_bits` column and
the notes; per-field widths are omitted as `n/a`.

## Project Alias Convention

The project uses `p25_fullrate` and `p25_halfrate` as **P25-scoped
aliases** for the two P25 vocoder algorithms. These are symbolic
identifiers in prose; they are distinct from the chip rate indices
used as CSV keys:

| Project alias    | Algorithm                | Chip rate-index mapping                    |
|------------------|--------------------------|--------------------------------------------|
| `p25_fullrate`   | P25 full-rate IMBE       | **NOT a chip index** — invoked via RCW; captured here as row 62 (with FEC) / 63 (no FEC) |
| `p25_halfrate`   | P25 half-rate AMBE+2     | **Chip index 33** (with FEC) or chip index 34 (no FEC) |

The aliases were previously written as `r33` / `r34`, which collided
with chip rate indices 33 and 34 (both AMBE+2 half-rate variants, not
P25 full-rate). See `analysis/ambe3000_rate_bit_allocation_gap.md`
Gap 2 for the historical discussion and the rationale for adopting
the current aliases. This CSV follows chip-index numbering
(consistent with `rate_index_table.csv` and `rate_control_words.csv`).

## CSV Column Definitions

| Column              | Type    | Meaning                                                |
|---------------------|---------|--------------------------------------------------------|
| `rate_index`        | int     | Chip rate index 0–61, or project extension 62/63       |
| `b0_pitch`          | int\|`n/a`\|`pending`\|`L-dependent` | Pitch quantizer bits |
| `b1_vuv`            | int\|`n/a`\|`pending`\|`L-dependent` | V/UV codebook bits   |
| `b2_gain`           | int\|`n/a`\|`pending`\|`L-dependent` | Gain quantizer bits  |
| `b3_prba24`         | int\|`n/a`\|`pending`\|`L-dependent` | PRBA24 VQ bits       |
| `b4_prba58`         | int\|`n/a`\|`pending`\|`L-dependent` | PRBA58 VQ bits       |
| `b5_hoc1`..`b8_hoc4`| int\|`n/a`\|`pending`\|`L-dependent` | HOC VQ bits          |
| `total_info_bits`   | int     | Sum of info-bit widths (speech_bps × 0.020)            |
| `fec_scheme`        | string  | Symbolic name for FEC structure, or `none` / `pending` |
| `interleaver`       | string  | Symbolic name for interleaver, or `none` / `pending`   |
| `notes`             | string  | Free-text per-row notes                                |
