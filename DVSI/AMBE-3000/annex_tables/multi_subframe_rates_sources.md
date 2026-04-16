# multi_subframe_rates.csv — Sources and Status

## Purpose

Per-rate subframe count (1 or 2) for AMBE-3000 chip rate indices,
required to dispatch decode / encode between the single-subframe
AMBE+2 path and the US6199037 multi-subframe joint-quantization path.

## Status

Partial. Only P25 rates are confirmed single-subframe. 60 of 64 rows
are `pending` empirical resolution.

| Rows    | Status                                                           |
|---------|------------------------------------------------------------------|
| 33, 34  | `subframes_per_frame = 1` — normative from BABA-A §2 / §12      |
| 62, 63  | `subframes_per_frame = 1` — normative from BABA-A §1 (project extension for P25 full-rate IMBE) |
| Others  | `pending` — no public source identifies subframe count per rate  |

## Why So Much Is Pending

The AMBE-3000F User Manual v4.0 does not contain subframe-count
information anywhere. Appendix 7.2 (rate-index table) gives only
total/speech/FEC rates per index. Appendix 7.3 gives Rate Control Word
values and configuration pin settings — the RCW bit layout may encode
subframe count internally, but the chip manual does not document which
RCW bits drive that choice.

Nothing elsewhere in the chip documentation (HDK manual, Designer
Notes, SD errata) references subframe counts by rate index.

**Key ambiguity:** two vocoders encoding the same nominal bit rate
(e.g., 4000 bps) can be either:
- Single-subframe at 20 ms frames (80 bits/frame × 50 Hz = 4000 bps)
- Two-subframe at 40 ms frames (160 bits/frame × 25 Hz = 4000 bps)

Both produce identical average bit rates, so the rate-index table
alone cannot disambiguate.

## US6199037 — Algorithm vs Rate Mapping

US6199037 describes the joint-subframe quantization algorithm
(4-bit scalar + 6-bit vector for pitch; 6-bit 16-element or
2×6-bit 8-element VQ for voicing). The patent:

- **Describes** the algorithm in full algorithmic detail
- **Does not identify** specific chip rate indices or AMBE
  product numbers (AMBE-1000 / AMBE-2000 / AMBE+2) as users

## Existing Impl-Spec Claims

The three AMBE-3000 impl specs currently cite "r17 and r18" as
two-subframe rates:

- `AMBE-3000_Encoder_Implementation_Spec.md` §10.2: "Rates r17, r18,
  and a few others encode 2 subframes per frame"
- `AMBE-3000_Rate_Converter_Implementation_Spec.md` §1.6 + §10.3:
  uses r17 as the example "two-subframe" rate

No source citation accompanies these claims. They predate this
annex table and the analysis entry; treat as unverified until the
empirical characterization lands.

Note also that chip rate indices 17 and 18 are labeled **AMBE-2000
generation** in the manual (`rate_index_table.csv`), not AMBE+2.
Whether US6199037's joint-subframe technique applies to AMBE-2000
rates is itself unresolved — the patent is family-agnostic about
AMBE generation.

## Resolution Path

Empirical characterization against DVSI test vectors (`tv-std/rNN/`):

1. For a candidate non-P25 chip rate, encode a known PCM through the
   DVSI USB-3000 chip at that rate.
2. Observe output channel-frame rate: 50 Hz (single-subframe) vs
   25 Hz (two-subframe).
3. Cross-reference with bit-count per channel frame: 160 PCM samples
   per frame → 1 subframe; 320 PCM samples per frame → 2 subframes.
4. Update this CSV row from `pending` to `1` or `2` with source
   citation to the test vector run.

This work belongs in `~/blip25-mbe` (the Rust implementation +
conformance harness repo). Results land back here as CSV updates +
analysis-entry appendices.

## Relation to Other CSVs

- `rate_index_table.csv` — total/speech/FEC rates; subframe-count
  independent.
- `rate_control_words.csv` — RCW values per rate; may internally
  encode subframe count but not externally documented.
- `halfrate_bit_allocations.csv` — per-field widths; pairs with
  subframe count to fully characterize per-rate bit layout.
- `rate_conversion_pairs.csv` — pair-level conversion config; its
  `supported = pending` classification already covers multi-subframe
  uncertainty. No pair-level duplication needed.

## CSV Column Definitions

| Column                  | Type                     | Meaning                                          |
|-------------------------|--------------------------|--------------------------------------------------|
| `rate_index`            | int                      | Chip rate index 0–61 or project extension 62/63  |
| `subframes_per_frame`   | int \| `pending`         | 1, 2, or `pending` until empirically resolved    |
| `source`                | string                   | Citation for the value (spec section, patent, `not in public sources`) |
| `notes`                 | string                   | Per-row free text                                |
