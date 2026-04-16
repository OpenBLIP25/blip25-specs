# AMBE-3000 Multi-Subframe Rate Mapping — Unresolved

**Status:** Open.

**Scope:** Which AMBE-3000 chip rate indices use US6199037 multi-subframe
joint quantization, and whether the impl specs' existing `r17`/`r18`
examples are correctly identified.

**Opened:** 2026-04-16

---

## Gap Summary

The three AMBE-3000 impl specs (decoder, encoder, rate converter) now
describe the US6199037 multi-subframe algorithmic path in their
respective §10 sections. The algorithm itself is well-defined by the
patent. **What is not resolved:** which chip rate indices actually
invoke this algorithm.

## Evidence

### The AMBE-3000F Manual Is Silent on Subframe Counts

DVSI AMBE-3000F User Manual v4.0:

- **Appendix 7.2** (Vocoder Rate by Index Number): tabulates
  `rate_index, total_rate, speech_rate, fec_rate` per index 0..61. No
  subframe-count column.
- **Appendix 7.3** (Rate Control Words / Configuration Pins): six-word
  RCW values plus six configuration-pin settings per rate. RCW bit
  layout internals are not documented; may or may not encode subframe
  count.
- No other section of the manual references subframes per rate.

The bit rate alone cannot distinguish 1-subframe vs 2-subframe
operation:

- 4000 bps × 20 ms frame = 80 bits/frame × 50 Hz = 4000 bps  *(single-subframe)*
- 4000 bps × 40 ms frame = 160 bits/frame × 25 Hz = 4000 bps  *(two-subframe)*

Both are 4000 bps on paper.

### Impl-Spec Claims Have No Citation

Three locations in the impl specs identify specific rate indices as
multi-subframe:

1. `AMBE-3000_Encoder_Implementation_Spec.md` §10.2:
   > "Rates r17, r18, and a few others encode 2 subframes per frame"

2. `AMBE-3000_Rate_Converter_Implementation_Spec.md` §1.6:
   > "single-subframe p25_halfrate → two-subframe r17"
   > "2 → 1 (e.g., r17 → p25_halfrate) | emit two target frames per source frame"

3. `AMBE-3000_Rate_Converter_Implementation_Spec.md` §10.3:
   > "| Multi-subframe ↔ single-subframe | r17 ↔ p25_halfrate | TODO §10; out of P25 scope |"

None of these claims cite a source. They appear to be worked examples
rather than verified facts. The identification of r17/r18 as
multi-subframe should be treated as unverified until empirical
characterization confirms or refutes it.

### AMBE Generation Concern

Chip indices 17 and 18 are labeled **AMBE-2000** in the manual's
Appendix 7.2, not AMBE+2:

| Chip index | Generation    | Total bps | Speech bps |
|------------|---------------|-----------|------------|
| 17         | AMBE-2000     | 4000      | 4000       |
| 18         | AMBE-2000     | 4800      | 4800       |

Meanwhile, US6199037 was filed 1997-12-04 and has no AMBE-generation
restriction in its claims. The multi-subframe technique *could* apply
to AMBE-2000 rates; it is not exclusive to AMBE+2.

However, if r17/r18 are AMBE-2000 generation, they do **not** use the
AMBE+2 half-rate codebook skeleton (the PRBA24/PRBA58/HOC structure
from BABA-A §12 and US8595002). So "r17/r18 as 2-subframe AMBE+2"
collapses two separate claims:

1. r17/r18 are 2-subframe (unverified, no citation).
2. r17/r18 use AMBE+2's codebooks (contradicted by the manual's
   generation label).

Even if (1) turns out to be correct via empirical characterization,
(2) is unlikely given the manual's labeling.

### US6199037 Does Not Identify Rates by Index

US6199037's claims and specification describe:

- The smoothing + log-constrain + VQ voicing pipeline (§6-col. 9)
- Joint pitch Method 1 (4-bit scalar + 6-bit vector) and Method 2
  (scalar + 2-bit interp)
- Codebook sizes (6-bit 64-entry for voicing and pitch-vector,
  4-bit 16-level for pitch-scalar, 8-bit 256-entry for spectral mean)

None of these reference AMBE product models or chip rate indices.

## Resolution Path

Empirical characterization against DVSI test vectors, identical in
structure to the `halfrate_bit_allocations.csv` resolution path:

1. For each candidate chip rate, encode a known PCM sequence through
   the DVSI USB-3000 chip at that rate.
2. Observe the output channel-frame rate at the USB interface:
   - 50 Hz output → 1 subframe per frame (20 ms frames)
   - 25 Hz output → 2 subframes per frame (40 ms frames)
3. Cross-reference with PCM-samples-per-frame: 160 → 1 subframe, 320
   → 2 subframes.
4. Update `DVSI/AMBE-3000/annex_tables/multi_subframe_rates.csv` to
   replace `pending` with the measured value, source-cited to the
   test-vector run.
5. Update this analysis entry with the resolved per-rate mapping and
   the correctness check of the existing r17/r18 impl-spec claim.

This work belongs in `~/blip25-mbe` — same argument as the
halfrate_bit_allocations gap. Results land back here as CSV updates +
analysis appendices + impl-spec §10.2 / §10.5 / §12 cleanups.

## What the Impl Spec §10 Sections Do Now

With this analysis entry published, the three §10 multi-subframe
sections:

1. Describe the US6199037 algorithm independent of rate-index mapping
   (data structures, decode/encode/convert pipelines).
2. Use `r17`/`r18` only as worked examples, explicitly marked
   unverified with a cross-reference to this analysis entry.
3. Note the empirical resolution path in §12 of each spec.

The actual impl code consuming these specs should dispatch on the
`multi_subframe_rates.csv` value (1 or 2) rather than hardcoding
rate-index lists, so that when the CSV updates, no spec-consumer
code needs to change.

## Related Gaps

- `analysis/ambe3000_rate_bit_allocation_gap.md` — parallel gap for
  per-field bit widths on rates 35–61. Same resolution repo
  (`~/blip25-mbe`), same test-vector infrastructure.

## Cross-References

- `DVSI/AMBE-3000/annex_tables/multi_subframe_rates.csv` and sidecar
  `multi_subframe_rates_sources.md`
- `DVSI/AMBE-3000/AMBE-3000_Patent_Reference.md` §3 (US6199037
  algorithmic summary)
- `DVSI/AMBE-3000/AMBE-3000_{Decoder,Encoder,Rate_Converter}_Implementation_Spec.md`
  §10 multi-subframe sections + §12 open questions
- `DVSI/AMBE-3000/annex_tables/rate_index_table.csv` (chip-indexed
  rate metadata, including generation label)
- `docs/AMBE-3000_Next_Session_Prompts.md` — Prompt 2 scope
