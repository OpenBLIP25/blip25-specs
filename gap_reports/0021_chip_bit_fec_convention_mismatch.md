# Gap 0021 — DVSI chip.bit FEC convention does not match spec-derived Golay/Hamming layout

**Status:** A/B probe executed 2026-04-25 (implementer). FEC layer
confirmed bit-equivalent to JMBE on chip.bit frame 0 — `u0..u7 = 0x015
0x5ca 0xaa3 0xcad 0x5a0 0x589 0x55e 0x019` with errors `[3,3,3,3,1,1,1]`
match across both pipelines exactly. Spec-author's prediction holds; the
gap is fully post-FEC.

**First post-FEC fix landed** (blip25-mbe commit `cd42c53`): mute now
emits comfort noise per BABA-A §1.11.2's primary recommendation rather
than the silence alternative. chip_enc_our_dec PESQ rose 1.319 → 1.393
(+0.07); canonical our_enc_our_dec unchanged at 3.202 (no regression).

**Reframed:** the residual chip_enc_our_dec gap to JMBE's 2.66 is a
post-FEC quality problem in implementation only — no further spec
disambiguation required. JMBE-style max-repeat reset to default amps=1
after 3 consecutive repeats (§1.11.1 / Eq. 105 region) is the next
candidate. Tracked in blip25-mbe; no spec-author action needed unless
new ambiguity surfaces.

Spec-audit doc preserved at
`analysis/imbe_spec_audit_vs_jmbe_vs_chip.md` for posterity.

**Filed:** 2026-04-24
**Filed by:** implementer (blip25-mbe)
**Spec area:** TIA-102.BABA-A (Vocoder), §1.5.1 [23,12] Golay, §1.5.2
[15,11] Hamming, §8.1 bit-layout, Annex H interleave table.
**Severity:** Medium — blocks chip↔ours interop end-to-end. Our encoder
self-consistency is fine; our decoder on chip-produced bits mutes or
produces degraded audio. The prior memory framing
`JMBE confirms chip.bit is valid IMBE 2026-04-24` / "bug in our decoder"
was too simple — see diagnostic evidence below.

## The question

When our spec-derived full-rate decoder runs on `chip.bit` (produced by
the AMBE-3000R on DVSI tv-std/tv/clean.pcm, 3700 frames), every frame
reports exactly **3 Golay errors on c̃₀..c̃₃** and **1 Hamming error on
c̃₄..c̃₆**, for a per-frame `ε_T ≈ 13–15`. The pattern is deterministic,
not channel-noise. JMBE decodes the same bytes at PESQ 3.83; our decoder
on the same bytes emits PESQ 1.32 (near-silence) via `FrameDisposition::
Mute` triggered when `ε_R` crosses 0.0875.

**Which FEC / bit-layout convention does the DVSI chip use, and where
does the implementation spec document it (if it does)?**

Specifically:
- (a) The chip IS spec-compliant per §1.5.1 / Annex H, and our
  implementation has a subtle interleave / dibit-packing error that
  passes self-roundtrip but diverges from chip.
- (b) The chip uses a **non-spec Golay generator polynomial or bit
  layout** (e.g., the reciprocal polynomial, or info in low 12 bits
  vs high 12 bits). JMBE accepts it because JMBE matches the chip's
  convention rather than the BABA-A spec.
- (c) The chip emits an **unspecified pre-transmission form** (e.g.,
  before some final scramble / bit-permutation that the P25 air
  interface would apply) and the chip-side tooling is providing
  effectively "raw IMBE parameters in FEC frames" that require a
  different reconstruction than the air-interface spec describes.
- (d) BABA-A §8.1 or Annex H has an ambiguity in bit-ordering
  (dibit-within-byte, bit-within-dibit, or codeword-to-dibit mapping)
  that the derived implementation spec resolves one way but the chip
  (and JMBE) resolves another.

## Why it matters

With this gap open, the `chip_enc_our_dec` cell of the PCM-quality A/B
matrix is effectively broken — preventing validation of our decoder
against chip-emitted wire bits and blocking the "beat the chip
end-to-end on chip bits too" quality target. JMBE is currently the only
working bridge; relying on a GPLv3 Java decoder as the authoritative
oracle is not acceptable long-term for a P25 tooling stack.

## Diagnostic evidence

### Reproducibility

- Input: `/tmp/ab_clean_full/chip.bit` (66 600 bytes = 3 700 frames ×
  18 B), produced by AMBE-3000R encoding `DVSI/Vectors/tv-std/tv/
  clean.pcm`.
- Our decoder: `blip25_mbe::p25_fullrate::frame::decode_frame` →
  `p25_fullrate::dequantize::dequantize` → `codecs::mbe_baseline::
  synthesize_frame`.
- JMBE: `jmbe-1.0.9.jar` via `conformance/scripts/JMBEDecode.java`
  wrapper.

### PESQ baseline

| Path                                    | RMS    | PESQ (vs clean.pcm) |
|-----------------------------------------|-------:|--------------------:|
| chip.bit → OUR decoder (shipped)        |     19 | 1.319 |
| chip.bit → JMBE                          |    398 | 2.664 |
| chip.bit → OUR decoder, ε_t forced to 0 |   2324 | 1.090 |
| chip.bit → chip DECODER                  |   1123 | 2.394 |

The ε_t=0 variant confirms the MUTE disposition is *a* symptom but not
the root cause — forcing the synth to not mute produces louder but
*lower*-PESQ audio. The recovered params themselves are wrong.

### Per-frame error pattern is deterministic

Running `probe_chip_bit` (existing harness) across 3700 chip frames:

```
== chip.bit via MSB ==
  ok=3354 bad_pitch=346 other_err=0
  err_hist: 3->3054  2->554  1->92
```

Our Golay decoder reports **exactly 3 corrected bits on 82% of frames**.
Variants tried and rejected (all produced ≥1 error per frame):
- MSB-first dibit unpacking (baseline)
- LSB-first dibit unpacking
- MSB with intra-dibit bit swap
- Byte-reversed bits before MSB unpacking
- MSB + NO-PN (skip `modulation_masks` XOR on c̃₁..c̃₆)
- Sequential codeword packing (no Annex H interleave)

### Bit-level diff is systematic and info-determined

For frames with identical `info[0]` after our decode (e.g., frames 0, 3,
5, 16 all decode to `info[0]=0x015`), the XOR between chip's c[0] and
our re-encoded c[0] is **identical across frames**:

```
frame 0:  c[0] orig=0x010a881 rt=0x000a88d  xor=0b00100000000000000001100
frame 3:  c[0] orig=0x010a881 rt=0x000a88d  xor=0b00100000000000000001100
frame 5:  c[0] orig=0x010a881 rt=0x000a88d  xor=0b00100000000000000001100
frame 16: c[0] orig=0x010a881 rt=0x000a88d  xor=0b00100000000000000001100
```

Set bits in the XOR: positions 20, 3, 2 (23-bit word, bit 22=MSB).

For `info[1]=0x05ca`, the c[1] XOR is always `00011100000000000000000`
(bits 20, 19, 18) — similarly deterministic.

**This means chip's encoding of `info=X` is a fixed function of X**. The
"errors" our decoder reports are a systematic transform, not noise.

### Brute-force search: chip's codeword is not in our Golay codebook

For chip's `c[0] = 0x10a881` (info=0x015), enumerated our
`golay_23_12_encode(info)` across all 4096 info values and recorded
Hamming distances:

```
dist= 3  count=1    ← info=0x015 (what our decoder corrects to)
dist= 4  count=5
dist= 5  count=16
dist= 6  count=48
... (matches the Golay code's weight enumerator exactly)
```

So chip's codeword is **not** on our Golay code manifold. The nearest
codeword encodes info=0x015, so our decoder corrects 3 bits and returns
that — but it's a nearest-neighbor guess, not the chip's intended info.

### Tried variants that did NOT match

All yielded Hamming distance > 0 between chip's c[0]=0x10a881 and the
variant encoding of any info value:

- Bit-reversing the 23-bit codeword before decoding
- Swapping info-parity layout (info in low 12 bits, parity in high 11)
- Bit-reversing the generator matrix rows (reciprocal polynomial form)
- Encoding with info bit 0 at codeword bit 11 (LSB-first info layout)

### Our spec-derived implementation self-roundtrips cleanly

- Unit test `encode_decode_roundtrip_zero` in
  `crates/blip25-mbe/src/p25_fullrate/frame.rs`: encode all-zero info,
  decode, `errors == [0; 8]` ✓.
- `our.bit` (our encoder on same clean.pcm) → our decoder → 0 errors
  per frame, plausible params, PESQ 3.45.
- Generator matrix `GOLAY_23_12_GEN` in
  `crates/blip25-mbe/src/fec.rs:44-57` matches the impl spec's
  §1.5.1 table row-by-row (binary patterns identical).

## Options considered

### Option (a) — our implementation has a subtle interleave/layout bug

Possible, but:
- `encode→decode` roundtrips cleanly on random inputs
- Four dibit-packing / interleave variants tested (MSB, LSB, swap,
  byte-reversed, sequential) all yield similar non-zero error rates
- `spec_tables/annex_h_interleave.csv` has verified 144-bit coverage
  per the CSV header's OCR-fix note

If (a) is correct, the implementation spec or Annex H CSV has a subtle
bug. Needs a spec-author pass over BABA-A §8.1 + Annex H to confirm
the dibit-to-codeword-bit mapping in the derived CSV.

### Option (b) — chip uses non-spec Golay

DVSI AMBE-3000R firmware may implement an older or variant Golay
convention pre-dating the finalized TIA-102.BABA-A text. Prior memory
(`project_chip_oracle_investigation_2026-04-16`) already observed the
chip is *not* a spec-faithful BABA-A oracle on other dimensions
(frame-0 startup attenuation, stationary-signal suppression). An
FEC-level deviation would be consistent.

If (b), our spec-compliant decoder is correct and the chip's wire
bits need a chip-specific pre-transform to interoperate. Requires
reverse-engineering (or spec-author-driven documentation) of the chip
convention as a separate codec-variant, not a change to our
spec-compliant path.

### Option (c) — chip emits pre-air-interface raw form

Some reference codecs (ETSI TS 102 361 for DMR, for example) define
separate "bearer" and "air-interface" bit orders. If the AMBE-3000R
`dvsi encode` tool outputs bits at the bearer layer (pre-C4FM
air-interface scramble / permutation) rather than the air-interface
layer our decoder expects, a missing transform layer would explain the
systematic bit-diff.

### Option (d) — ambiguity in BABA-A §8.1 / Annex H

§8.1 describes the 88-bit info vector layout; Annex H describes the
144-bit interleaver. The dibit-to-bit direction — is symbol k's "high
bit" (transmitted first) assigned to `bit1_vector` / `bit1_index` in
the Annex H table? — is a specific convention choice.

## Chip probe plausibility

**Not directly resolvable by chip probe alone.** The chip produces the
bits; the spec (or JMBE's implementation details) would have to say what
those bits *represent*. A chip probe could:

- Observe whether a specific known-info dibit pattern round-trips through
  chip encode→chip decode identically (it does, per the existing
  `chip_enc_chip_dec` cell yielding PESQ 2.4).
- NOT tell us which Golay convention the chip uses without prior
  knowledge of at least one info-to-codeword mapping.

## Suggested resolution path

1. **Spec-author reads BABA-A §1.5.1 / §8.1 / Annex H** and verifies
   the derived `annex_h_interleave.csv` and `GOLAY_23_12_GEN` both
   match the authoritative text literally. If the derived works agree
   with the PDF, option (a) is ruled out.
2. **Spec-author reads JMBE's `jmbe.codec.imbe.IMBEInterleave` and
   `jmbe.edac.Golay*`** (allowed in the spec-author role; it's
   cross-project reference material) and reports whether JMBE's
   convention matches the BABA-A spec exactly, differs in a named
   way, or implements a superset.
3. If JMBE ≠ spec: spec-author documents the difference as a
   derived-works addendum, and the implementer adds a chip-variant
   decode path behind a feature flag.
4. If JMBE = spec but chip ≠ spec: option (b), document the chip's
   specific deviations separately; our shipped decoder stays
   spec-compliant; chip interop needs a pre-decode transform layer
   identified by probe.

## Artifacts

- Reproduction PCMs + cached chip.bit:
  `/tmp/ab_clean_full/{chip.bit, our.bit, *_*_dec.wav}`.
- Diagnostic probes (all in `conformance/speech-quality/examples/`):
  - `probe_chip_bit.rs` — existing; enumerates dibit-packing variants.
  - `probe_chip_dequant.rs` — dumps per-frame (ω₀, L, voicing, amp
    RMS, error counts) after our dequantize.
  - `chip_bit_zeroerr.rs` — re-decodes with ε_t=0 forced into synth
    to show mute isn't the full story.
  - `chip_bit_diff.rs` — per-codeword XOR diff between chip's raw
    frame and our encoder's roundtrip-encoding of our-decoded info[].
  - `golay_brute.rs` — Hamming-distance histogram of a target 23-bit
    word against our `golay_23_12_encode` over all 4096 info values.
- JMBE reference decoder wrapper:
  `conformance/scripts/JMBEDecode.{java,class}`.

## Relationship to prior memories

- `project_jmbe_validates_chip_bit_2026-04-24` — the "bug in OUR
  decoder" framing is **retracted in this report's scope.** Our
  decoder may well be spec-correct; the bug is at the FEC-convention
  boundary between chip (+ JMBE) and the spec's literal text.
- `project_chip_oracle_investigation_2026-04-16` — chip is already
  known not to be a spec-faithful oracle on §1.10/§11/analysis-side
  dimensions; this report extends the "chip ≠ spec" catalog to the
  §1.5 FEC layer.
