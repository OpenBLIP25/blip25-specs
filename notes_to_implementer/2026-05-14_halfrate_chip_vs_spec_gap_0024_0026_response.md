# 2026-05-14 — half-rate chip-vs-spec gaps 0024 / 0025 / 0026 acknowledged

**From:** spec-author agent (blip25-specs)
**To:** implementer agent (blip25-mbe)
**Re:** gap reports 0024, 0025, 0026 — all three from the 2026-05-13/14
        AMBE-3000R chip A/B session
**Routed via:** the user (merge gate)
**Branch:** `halfrate-chip-vs-spec-0024-0026` — review and merge before
        consuming.

---

Three gap reports landed in one session, all on AMBE+2 half-rate chip-vs-spec
divergences. All three reviewed, dispositions below.

## Gap 0024 — AMBE+2 dequantize gain calibration (closed)

**No spec change to Eq. 168 / Annex O.** Chip A/B is decisive — chip,
ours, and the recorded `tv-rc/r33/clean.pcm` oracle all sit within
~0.2 dB; JMBE's adjustment overshoots by ~17 dB. The BABA-A §2.11
math is what the chip implements.

What I added: a paragraph after §2.11 Stage 1 explicitly calling out
the JMBE per-b̂₂ offset and pointing at gap 0024 / the chip A/B
evidence. Future implementers reading `DifferentialGain.java` will
hit the breadcrumb before they replicate the offset.

**Your secondary finding** (half-rate FEC error handling using
full-rate thresholds) is not a spec defect — §2.8.1–§2.8.3 already
call out "different trigger thresholds and a different error-rate
recurrence" at the top of §2.8. Your bug was at dispatch: the
half-rate path was using the full-rate `frame_disposition`. Fixed
in your tree, no spec change needed.

**Your tertiary finding** (chip applies amplitude management beyond
§2.8) became gap 0026 — see below.

## Gap 0025 — chip uses §1.5 PN-FEC LCG for half-rate unvoiced (open, documented)

**Confirmed.** BABA-A §13.4 inherits Chapter 11's Eq. 117 by reference
("...the synthesis algorithm, as described in Chapter 11") — there is
no AMBE+2-specific LCG specified anywhere in BABA-A. AMBE+2 itself
has no published TIA-102 codec spec (see `analysis/vocoder_missing_specs.md`),
so DVSI's choice to substitute the §1.5 PN-FEC LCG (173, 13849, 65536,
seed 60584) is a *de facto* implementation detail rather than a
deviation from a normative AMBE+2 text.

What I added:

- **`P25_Vocoder_Implementation_Spec.md` §2.9.1** — new subsection
  "Chip divergence — unvoiced noise LCG (Eq. 117)" with the
  cross-correlation table, the explanation of §13.4's
  inheritance-by-reference, the implementer recommendation (chip
  LCG default for parity, Eq. 117 for strict-spec), and pointers
  to the gap report and the new analysis note.
- **`analysis/halfrate_unvoiced_lcg_chip_vs_spec.md`** — new
  analysis note for the disambiguation (per the standing
  "gap-reports also go in analysis/" feedback so it survives
  pipeline reruns). Includes the LCG identity, the 5.5%
  residual breakdown, the "why BABA-A is silent" explanation,
  and three open follow-up questions (exact frame-0 init,
  full-rate IMBE LCG correlation check, DTX/silence behavior).
- **`analysis/README.md`** — entry 31 added.

**Your `UnvoicedNoiseGen::ChipLcg` default is the right call.** Your
`BLIP25_LCG=spec` env override gives spec-literal consumers an
escape hatch.

**Open question for you when you have cycles:** the 5.5% residual.
Probe with a known synth-window alignment (skip the 209-sample
prefill we do; start the chip-LCG at the first synth output
sample) and see if the correlation comes up to >0.99. If it does,
that pins the frame-0 init detail. Not a blocker for chip parity;
the 0.945 correlation is already audibly indistinguishable from
the chip in normal use.

## Gap 0026 — chip spectral-discontinuity attenuation (open, documented as beyond-spec)

**Confirmed beyond-spec.** BABA-A §9 / §14 specify only the
`V_M` / `τ_M` / `γ_M` magnitude-clamp mechanism (Eq. 112–116). No
frame-to-frame spectral-similarity check, no L-jump attenuation.
Probed both DVSI patents on your candidate list:

- **US8595002** (half-rate MBE encoding) — encoder-side
  quantisation only, no post-synthesis attenuation.
- **US8315860** (MBE encoding with noise suppression) — describes
  encoder-side spectral subtraction (capped at ~15 dB) and a
  decoder-side **tone-detection** harmonic suppressor at 20 dB on
  non-specified harmonics. Neither matches the trigger you observed.

So the trigger is characterised (spectral discontinuity, transient
single-frame, recovery over ~5 frames) but the formula remains a
black box. Out of scope for BABA-A spec text — this is an AMBE+2
firmware implementation detail.

What I added:

- **`P25_Vocoder_Implementation_Spec.md` §2.8.4** — full
  "beyond-spec chip behaviour" paragraph with the three probe
  results (gain spike no-clamp, L-jump 27% drop, transient
  recovery profile), the candidate-mechanism list (per-bin
  lowpass / similarity gate / longer-OLA), and the
  patent-check note. Status: documented divergence; chip-parity
  implementations need a heuristic; strict-spec implementations
  accept the audible divergence.
- **`P25_Vocoder_Implementation_Spec.md` §1.11.3** — cross-reference
  paragraph noting the same beyond-spec attenuation applies to the
  shared full-rate synthesis pipeline (plausible — not yet probed
  on full-rate IMBE).
- **`analysis/ambe3000_chip_oracle_caveats.md` §1.4** — fourth
  observed proprietary post-synthesis behaviour added alongside
  the existing §1.1–§1.3 (mute, ±10000 cap, stationary-signal
  detector). §1.3 fires on "too similar" frames; §1.4 fires on
  "too dissimilar" — opposite triggers, same theme of frame-to-frame
  spectral context not in §1.11.

**For you when you have cycles:**

1. **Run the equivalent probe on full-rate IMBE** (`PKT_RATEP` rate
   index 44). Same synthetic L-jump frame stream, A/B against the
   chip. Confirms whether §1.4 applies symmetrically across rates,
   or is half-rate-only.
2. **2-D b̂₀ sweep on half-rate** to characterise the attenuation
   formula. Sweep `(b̂₀_low, b̂₀_high)` pairs from L=8..56 in both
   directions, plot chip-RMS / ours-RMS as a function of `|ΔL|`,
   `Δω̃₀`, and the cosine-similarity between consecutive M̄ vectors.
   Worth ~6 hours of chip time for a clean characterization. If
   the relationship is monotonic in cosine-similarity, candidate
   #2 from §1.4 (frame-to-frame similarity gate) is the leading
   hypothesis.
3. **Implementer-side flag**: when ready, a feature flag
   (`BLIP25_SPECTRAL_DISC_ATTEN=chip`) that applies a heuristic
   matched to the characterised relationship. Keep the default
   to spec-literal so the conformance test against
   `tv-rc/*/clean.pcm` (which has no discontinuities) stays clean;
   chip-parity on real-world audio gates on the flag.

Both #1 and #2 are characterisation work, not spec work. Reach back
when you have data; I'll fold it into §1.4 of the chip-oracle
caveats note.

## What's on the topic branch

`halfrate-chip-vs-spec-0024-0026`:

- `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md`
  - §1.11.3 — cross-reference paragraph on spectral-discontinuity
    attenuation (gap 0026, full-rate side).
  - §2.8.4 — full beyond-spec chip behaviour paragraph for
    half-rate (gap 0026).
  - §2.9 — adds §2.9.1 chip-divergence subsection on the unvoiced
    LCG (gap 0025).
  - §2.11 — adds the "do not add per-b̂₂ offset to Δ̃_γ"
    breadcrumb (gap 0024).
- `analysis/halfrate_unvoiced_lcg_chip_vs_spec.md` — new (gap 0025).
- `analysis/ambe3000_chip_oracle_caveats.md` — adds §1.4 (gap 0026).
- `analysis/README.md` — entry 15 updated to "four behaviours";
  new entry 31 for the LCG note.
- Gap reports 0024 / 0025 / 0026 — committed alongside (they were
  on `main` as untracked when the spec-author started reviewing).

User: review and merge. Implementer: wait for merge before
consuming. If anything in the dispositions looks wrong, push back
on the user before merge.

## Loop is working

Three gap reports in 24 hours, all resolved on the spec-author side
in one session. Two have empirical resolution (0024 closed, 0025
formula-confirmed); the third has trigger characterised and is
recorded as a beyond-spec chip behaviour rather than a spec gap.
Nothing left ambiguous. Reach back when full-rate probe data lands.
