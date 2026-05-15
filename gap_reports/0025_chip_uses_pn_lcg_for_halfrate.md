# Gap 0025 — DVSI AMBE-3000R chip uses the §1.5 PN-FEC LCG for half-rate unvoiced noise, not the §1.12.1 spec LCG

**Status:** Open — empirically confirmed; spec text describes different generator than the de-facto industry reference.
**Filed:** 2026-05-14.
**Affects:** BABA-A §1.12.1 Eq. 117 (when applied to half-rate per §2.9).
**Impact:** Subjectively audible on fricative tails (/ʃ/, /ks/, /s/). The
per-sample noise sequence used by the chip is uncorrelated with the
spec generator.

## What the spec says

BABA-A §1.12.1 Eq. 117 (page reference held by spec-author):
```
u(n+1) = (171 · u(n) + 11213) mod 53125
u(−105) = 3147        (Annex A initial state)
```

§2.9 says the half-rate path uses §1.12.1 synthesis verbatim, including
Eq. 117 for the unvoiced noise sequence.

JMBE 1.0.9 implements this literally (`MBENoiseSequenceGenerator.java`).
blip25-mbe implemented this literally until 2026-05-14. The two were
verified to share the same LCG up to a startup-offset of 233 samples
(cross-correlation 0.932 in our earlier audit).

## What the DVSI chip actually does on half-rate (P25 Phase 2 AMBE+2)

Empirical: the AMBE-3000R chip configured for `PKT_RATET 33` (half-rate
with FEC) uses a **different** LCG:

```
u(n+1) = (173 · u(n) + 13849) mod 65536
```

This is the same recurrence the spec uses for **FEC masking** (§1.5
Eq. 84-85, `p_r(n+1) = (173·p_r(n) + 13849) mod 65536`) — repurposed by
DVSI for the half-rate unvoiced synth. The chip-internal seed maps to
state value `60584` at our codec's `t=0 sample 0` on a 209-sample
frame-0 init.

Cross-correlation of our codec output (using this LCG with seed 60584)
against the chip on an all-unvoiced probe: **0.9447 at lag 0** (Pearson,
properly normalized). The residual 5.5% corresponds to a sub-frame
implementation detail (probably frame-0 buffer-init scheme); it doesn't
affect the LCG identity.

The IMBE full-rate path (`PKT_RATEP 0x0558 0x086B 0x1030 0x0000 0x0000
0x0190`, rate index 44) appears to use the spec LCG — our codec
matches DVSI's `tv-rc/r33/clean.pcm` (full-rate) at 1.001× ratio without
this change.

## Reproduction

Synthetic all-unvoiced probe frame (so output is pure shaped LCG noise):

```rust
// crates/blip25-mbe/examples/lcg_probe_frame.rs
let mut b = [0u16; 9];
b[0] = 60;  // pitch index — mid range
b[1] = 16;  // V/UV codebook → all-unvoiced (AMBE_VUV_CODEBOOK[16])
b[2] = 16;  // gain index — mid range
// b[3..=8] = 0 — flat M̃ via lowest-index PRBA/HOC choices
```

Stream 300 copies through chip and ours; cross-correlate. With the
spec LCG (171, 11213, 53125, seed 3147) the correlation against the
chip is **0.06** (= noise floor). With the chip LCG (173, 13849, 65536,
seed 60584) the correlation is **0.945** (= near-perfect, residual
explained by sub-frame init details).

`crates/blip25-mbe/src/codecs/mbe_baseline.rs:UnvoicedNoiseGen` exposes
both generators; default is now `ChipLcg`. Override via env
`BLIP25_LCG=spec` for spec-literal output.

## Hypothesised cause

Speculation, not verified: the BABA-A spec text describes IMBE
full-rate behaviour. The half-rate "AMBE+2" annex (§2 of the
implementation spec, BBA-C in the published TIA hierarchy) is the DVSI
proprietary codec layered onto BABA-A's overall framework. §2.9's
"half-rate uses §1.12 verbatim" appears to be the spec-author's
inference rather than a normative statement — the actual chip
implementation reuses BABA-A's FEC-masking LCG (already on-die for §1.5)
rather than instantiating a second LCG with different parameters for
the unvoiced synth.

## Spec-author action items

1. Verify with DVSI: does the AMBE+2 spec proper (BBA-C or whatever the
   half-rate-specific document is) describe Eq. 117 explicitly, or does
   it inherit §1.12.1 by reference only?
2. If it inherits by reference: §2.9 of the implementation spec should
   be amended to note the actual chip-implementation generator differs
   from the inherited Eq. 117. JMBE and other open-source half-rate
   decoders that follow the spec literally produce non-chip-equivalent
   per-sample output (though spectrally equivalent).
3. If BBA-C specifies a different generator: §2.9 should cite that
   explicitly rather than inheriting §1.12.1.

## Implementation-side action items (already taken)

- Default `UnvoicedSynthState` now uses `UnvoicedNoiseGen::ChipLcg` —
  matches the de-facto industry reference (the DVSI hardware).
- `BLIP25_LCG=spec` env override falls back to spec-literal LCG. Used
  by conformance tests that validate against the published spec rather
  than the chip.
- `UnvoicedNoiseGen` is `serde`-serializable for downstream tools.

## Residual notes

The chip-LCG match is 0.945 (not 1.0). The remaining 5.5% comes from a
sub-frame implementation detail not yet identified. The probe-1 analysis
narrowed it to the frame-0 buffer-init scheme (chip likely doesn't
burn 209 LCG samples up front the way we do); a follow-up probe should
pin down the exact init.
