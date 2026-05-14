# Half-Rate Unvoiced Noise LCG: Chip vs. Spec

**Status:** Open divergence — empirically confirmed, formula stable, audibly distinguishable on fricative tails.
**Scope:** P25 Phase 2 / AMBE+2 half-rate decoder, BABA-A §13.4 / Chapter 11 unvoiced synthesis.
**Filed:** 2026-05-14 (from `gap_reports/0025_chip_uses_pn_lcg_for_halfrate.md`).

## Summary

BABA-A §13.4 inherits the full-rate Chapter 11 synthesis pipeline by
reference for half-rate decoding, including Eq. 117's linear-congruential
white-noise generator (LCG):

```
u(n+1) = (171 · u(n) + 11213) mod 53125
u(−105) = 3147                                   (Annex A §10 initial state)
```

The DVSI AMBE-3000R chip, when configured for the P25 half-rate FEC
profile (`PKT_RATET 33`), does **not** use Eq. 117. It uses a different
LCG — the one BABA-A §1.5 / Eq. 84–85 specifies for the full-rate
**PN-FEC bit-modulation masking** (an entirely separate function,
applied between FEC decode and bit-prioritisation, not in synthesis):

```
p_r(n+1) = (173 · p_r(n) + 13849) mod 65536      (the §1.5 PN-FEC LCG,
                                                   reused for unvoiced
                                                   synthesis on the chip)
seed     = 60584 at our codec's t=0 / sample 0   (frame-0 init scheme;
                                                   209-sample synth window)
```

The full-rate IMBE path on the same chip *does* use Eq. 117 — our codec
matches DVSI's `tv-rc/p25_fullrate/clean.pcm` reference at 1.001× RMS
without any LCG substitution. So the divergence is half-rate-specific.

## Evidence

Cross-correlation of an all-unvoiced probe stream (300 frames at
`b₀=60, b₁=16 (all-unvoiced V/UV codebook entry), b₂=16, b₃..b₈=0`)
against the chip:

| Generator | r vs chip at lag 0 | Interpretation |
|-----------|-------------------:|----------------|
| Eq. 117 (spec literal, 171, 11213, 53125, seed 3147) | **0.06** | Noise floor; uncorrelated sample sequence |
| §1.5 LCG repurposed (173, 13849, 65536, seed 60584) | **0.945** | Near-perfect; residual 5.5% from sub-frame init detail |

Reproduction: `crates/blip25-mbe/examples/lcg_probe_frame.rs` against
the AMBE-3000R on `pve` (192.168.1.6) via the `dvsi-driver` CLI's
`decode-halfrate` subcommand.

The 5.5% residual is consistent with a frame-0 buffer-prefill scheme
mismatch — our codec burns 209 LCG samples up front to fill the
synthesis window; the chip likely does something different at codec
init. Long-form streams stabilise once both generators leave their
transient regions.

## Why this matters

Per-sample noise sequence affects **audible character** of fricative
tails (/ʃ/, /ks/, /s/, /h/) at sub-frame timescales. Spectrally the
two generators are equivalent — both are 16-bit-ish uniform random
sequences shaped by the same per-band normalisation in Eq. 119–124 —
so long-term loudness, harmonic balance, and intelligibility are
indistinguishable. What changes is the texture of unvoiced segments:
the chip's noise is correlated with hardware decodes, JMBE's and
spec-literal blip25-mbe's noise is not.

For consumers comparing decoded PCM against chip-recorded PCM
sample-by-sample (e.g., conformance test against DVSI's published
`tv-rc/*` reference vectors), this divergence is the dominant residual
error after the rest of the pipeline matches.

## Why BABA-A is silent on this

BABA-A's published normative text covers the full-rate IMBE codec
(Chapters 1–12). Chapter 13 introduces the half-rate codec by
*reference* to Chapters 7 (FEC), 8 (enhancement), 9 (adaptive
smoothing), and **11 (synthesis)**, with half-rate-specific changes
called out in Chapters 13–16. The §13.4 closing paragraph reads as a
catch-all: "M̃_l are then used by the synthesis algorithm, as
described in Chapter 11."

The AMBE+2 codec itself — the proprietary DVSI variant deployed in
P25 Phase 2 systems — has **no separate TIA-102 specification**
(see `analysis/vocoder_missing_specs.md`). The BBA-C document that
would, in principle, specify AMBE+2 codec internals is not in the
TIA-102 publication tree. Consequently:

- §13.4's "Chapter 11" reference is the only normative statement
  about half-rate synthesis.
- DVSI's chip implementation differs from that reference for the
  unvoiced LCG (and at least one other behaviour — see
  `ambe3000_chip_oracle_caveats.md` §1.4 on spectral-discontinuity
  attenuation).
- This is **not** a BABA-A spec defect — BABA-A's claim is correct
  if read as "the IMBE full-rate synthesis algorithm." It is a gap
  in the AMBE+2 specification corpus, which BABA-A does not own.

## Likely cause (speculation, unverified)

The §1.5 PN-FEC LCG (173, 13849, 65536) is on-die for full-rate FEC
masking. DVSI's AMBE+2 firmware appears to reuse that hardware/code
path for the half-rate unvoiced synth rather than instantiating a
second LCG with the Eq. 117 parameters. The seed-value 60584
on a 209-sample frame-0 init looks consistent with a fixed power-on
state in the firmware that gets clocked forward as synth samples
are consumed. Both observations are speculative — only DVSI can
confirm the firmware mechanism.

## What implementers should do

1. **Chip-parity decoders** (e.g., scanner/recorder applications
   comparing against `tv-rc/*` reference PCM): use the chip LCG
   for half-rate unvoiced synthesis.
   - `u(n+1) = (173 · u(n) + 13849) mod 65536`
   - Init seed `60584` at the start of the codec's first synth frame;
     burn / skip whatever number of samples your synth window
     prefill scheme demands.
2. **Strict-spec decoders** (conformance test suites validating
   against the BABA-A text rather than the chip): keep Eq. 117 as
   written.
3. Both are valid implementation choices given BABA-A §13.4's
   inheritance ambiguity. Document the choice in your decoder so
   downstream consumers know which oracle you're calibrated against.

JMBE 1.0.9's `MBENoiseSequenceGenerator.java` implements Eq. 117
literally; correlation against the chip on the same probe is at the
noise floor.

## Open questions

- **Exact frame-0 init**: chip's seed value 60584 was estimated by
  brute-force search over the §1.5 LCG state space; the underlying
  firmware-level init (power-on seed, samples-burned-before-first-frame)
  is not yet pinned down. A follow-up probe with a known synth-window
  alignment could close the residual 5.5%.
- **Full-rate IMBE on the chip**: our codec already matches at 1.001×
  using Eq. 117, so we know full-rate uses the spec LCG. But we have
  not run the equivalent all-unvoiced-probe cross-correlation on
  full-rate to confirm the LCG identity there at lag 0.
- **DTX / silence frames**: not yet probed — DVSI's discontinuous
  transmit may or may not use the same LCG for comfort-noise
  generation.

## References

- `gap_reports/0025_chip_uses_pn_lcg_for_halfrate.md` — original gap
  filing with full probe methodology, ratio tables, and JMBE
  cross-reference.
- `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` §2.9.1
  — chip-divergence subsection in the implementation spec.
- `analysis/vocoder_missing_specs.md` — why AMBE+2 has no published
  TIA-102 specification of its own.
- `analysis/ambe3000_chip_oracle_caveats.md` — companion catalogue of
  proprietary chip behaviours not described in BABA-A.
- BABA-A §1.5 / Eq. 84–85 — the source of the chip's repurposed LCG.
- BABA-A §11 / Eq. 117 + Annex A §10 — the spec generator the chip
  bypasses for half-rate.
