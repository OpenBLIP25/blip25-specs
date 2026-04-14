# Open-Source P25 Vocoder Implementations: Lessons Learned

## Purpose

This document captures what's **trustworthy** and what's **known-flawed**
in the major open-source P25 vocoder implementations. It exists because
the project's spec-extraction directive excludes OSS as a source, but
OSS is still useful as a **secondary reference for implementation
verification** — provided you know where each project diverges from
the 2014 BABA-A spec and real DVSI behavior.

The rule this supports:
- **Do not** derive spec values from OSS code.
- **Do** compare your implementation's output against OSS on deterministic
  stages (deinterleave, FEC, dequantization, DCT, MBE parameter recovery)
  as one of several independent sanity checks. Log divergences to
  `phase4_findings_log.md`.

Entries here are intended to grow over time as the project team and
downstream implementers discover specific strengths and weaknesses in
each codebase. Treat this as a living document, not a definitive audit.

---

## mbelib (https://github.com/szechyjs/mbelib)

**Scope:** Full-rate IMBE (P25 Phase 1) + half-rate AMBE (P25 Phase 2).
C code. Used by OP25, DSDPlus, and many hobbyist P25 decoders.

**Wire-format processing (trustworthy):**
- Deinterleave + Golay/Hamming FEC decode match the BABA-A wire format.
- Bit prioritization unpack produces the correct b̂_l quantizer values
  for any valid L.

**MBE parameter recovery (mostly trustworthy, with caveats):**
- Inverse uniform quantizer uses the `+0.5` midtread offset correctly.
- Per-block inverse DCT uses the J̃_i denominator (not a fixed 6-point).
- Log-magnitude prediction implements Eq. 77 with ρ = 0.65.

**Known divergences from BABA-A 2014 / DVSI AMBE-3000 (full-rate):**
- Uses the **1993 IMBE algorithm** for synthesis, not the 2014 BABA-A
  revision. This affects final PCM output quality — specifically the
  enhancement filter, adaptive smoothing coefficients, and phase
  regeneration. MBE parameters (ω₀, L, K, v_l, M_l) should still agree;
  PCM output will not be bit-identical to DVSI.
- Frame concealment heuristics (repeat/mute thresholds) are 1993-vintage.

**Known bugs (verify before trusting):**
- Error-count arithmetic at high BER can overflow in some code paths
  (reported downstream — verify against your own impl).
- Some precision issues in the inverse-DCT accumulator at L > 40 due
  to f32 instead of f64 intermediates.

**Use it to verify:**
- Deinterleave + FEC decode outputs
- Dequantized b̂_l values
- Recovered G̃_m, R̃_i, C̃_{i,k}
- Per-frame M̃_l values (pre-synthesis)

**Don't use it to verify:**
- Final PCM synthesis quality
- Frame concealment behavior under sustained errors
- Any bit-exact comparison to DVSI output

---

## JMBE (https://github.com/DSheirer/jmbe)

**Scope:** Half-rate AMBE+2 (P25 Phase 2) only. Java code. Bundled with
SDRTrunk.

**Wire-format processing (trustworthy):**
- Deinterleave + extended Golay + standard Golay FEC match BABA-A half-rate
  wire format.
- Bit prioritization unpack matches BABA-A §14.1 Tables 15–18 (verified
  during the Phase 4 BABA-A pass).

**MBE parameter recovery (trustworthy):**
- Annex L pitch lookup + Annex M V/UV codebook expansion match BABA-A
  §13.1–§13.2.
- PRBA24 / PRBA58 / HOC VQ lookups match Annexes P / Q / R.

**Known limitations:**
- **Half-rate only.** No full-rate IMBE decode — you cannot use JMBE to
  validate full-rate (Phase 1) impls.
- Synthesis quality is reasonable but not bit-identical to DVSI
  AMBE-3000. Expect audible-but-not-spec differences in the enhancement
  filter output.

**Use it to verify:**
- Half-rate wire-format decode + MBE parameter recovery
- Pitch (b̂₀) → ω₀ lookup via Annex L
- V/UV (b̂₁) + per-harmonic expansion via Eq. 147/149

**Don't use it to verify:**
- Full-rate (use mbelib)
- Final PCM synthesis (use DVSI)

---

## OP25 (https://osmocom.org/projects/op25)

**Scope:** Full P25 receiver stack. Python + C++. Uses mbelib for
vocoder. Not a vocoder itself but a consumer of one.

**Useful for:**
- Cross-validating **frame-level** processing (sync, deinterleave, FEC,
  NID) for FDMA voice paths.
- Reference for the **air-interface timing** (LDU1/LDU2 cadence,
  interslot placement) — implements BAAA-B correctly.

**Limitations:**
- Vocoder quality limited by mbelib (see above).
- Annex A bit-order tables are partially hard-coded; may lag spec
  updates.

---

## SDRTrunk (https://github.com/DSheirer/sdrtrunk)

**Scope:** Full P25 receiver stack. Java. Uses JMBE for half-rate.
Large, mature, widely-deployed.

**Useful for:**
- Cross-validating **trunking control-channel** processing (TSBK parsing,
  grant handling) — implements AABC/AABD correctly.
- Reference for **TDMA MAC** processing (BBAC-A) — burst reassembly and
  superframe tracking.
- Annex E (BBAC-1 burst bit tables) — SDRTrunk's implementation is one of
  the reasons we cross-validated our Annex S extraction against BBAC-1
  Annex E during Phase 4.

**Limitations:**
- Half-rate vocoder is JMBE (same caveats as JMBE).
- No full-rate vocoder — relies on JMBE for Phase 2 only.

---

## codec2 AMBE support (various forks)

**Status:** Various forks of David Rowe's codec2 include AMBE-compatible
decoders of uncertain lineage. Quality and accuracy vary significantly;
treat as tertiary reference only unless you can audit the specific fork.

**Use it to verify:** nothing specific without audit.

---

## Cross-Project Consistency

If two of mbelib / JMBE / OP25 / SDRTrunk disagree with your
implementation on a deterministic stage (wire format, FEC, dequantization,
DCT, MBE parameters), the strong signal is that **your** impl or **one
project's** impl has a bug. If all of them agree *and* you disagree, take
a hard look at your impl before assuming they're all wrong.

If they all **agree with each other** but disagree with **DVSI** or a
**BABA-A invariant check**, you may have discovered:
- A place where the OSS implementations all inherited the same old bug
  (likely — they share lineage).
- A place where the BABA-A 2014 spec genuinely diverges from 1993 IMBE
  (common at the synthesis stage).
- A DVSI-specific quirk that the spec doesn't mandate.

Document the verdict in `phase4_findings_log.md` under the relevant
spec's section, flagged "AMBIGUITY — OSS divergence" or "SPEC BUG" as
appropriate.

---

## What This Project Does NOT Trust OSS For

Per the project's extraction rules:

- **Generator matrices, polynomials, step sizes.** Derive from the PDF
  directly. OSS values may be correct, but may also carry forward
  decade-old OCR errors that nobody cross-checked against the 2014 spec.
- **Annex table values.** Same reason.
- **Algorithm implementations.** OSS implementations are functional, but
  they're not the spec; using them as the spec is how 1993-IMBE
  artifacts end up in a 2014-IMBE implementation.

---

## How To Add To This File

When you (or a Phase 4 run) discover something new about an OSS
implementation — a bug, a trustworthy stage, a divergence from DVSI,
a useful heuristic — add it to the relevant section above with a dated
note. Example:

> **2026-04-14 / testing note:** JMBE's error-rate smoothing uses a
> different α coefficient than BABA-A §9.4 specifies; produces audible
> artifacts on sustained-error streams. Discovered when comparing
> mute-threshold behavior between JMBE and our impl.

Cross-link to `phase4_findings_log.md` entries where applicable.
