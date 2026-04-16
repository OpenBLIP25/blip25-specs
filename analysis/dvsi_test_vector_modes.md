# DVSI Test Vector Modes: Rate 33 vs 34 vs 39

**Context.** The DVSI AMBE-3000 reference chip (the canonical source of
P25 half-rate test vectors) operates in multiple rate modes selected by a
configuration register. Only one of them produces bits that conform to
TIA-102.BABA-A's over-the-air half-rate frame format. Confusing the modes
produces extremely misleading diagnostic output that looks like a spec bug
when it's actually a test-vector-selection bug.

This entry exists so no future session repeats the multi-day debug arc
that surfaced it.

## The three rate modes that matter for P25 half-rate work

| DVSI rate | Bit rate | Voice bits | FEC bits | On-wire shape | Use |
|-----------|---------:|-----------:|---------:|---------------|-----|
| **33**    | 3600 bps | 2450       | 1150     | 72-bit frame with Golay [24,12] + Golay [23,12] + uncoded per BABA-A §14.2 Eq. 189–192 | **P25 half-rate conformance** — interoperable with Phase 2 TDMA |
| **34**    | 2450 bps | 2450       | 0        | 49-bit payload only, no FEC | P25 voice parameters without FEC (post-Golay-decode equivalent) |
| **39**    | 3600 bps | 3600       | 0        | 72 raw voice bits, no Golay | DVSI-internal high-capacity voice; **not P25-interoperable** |

Source: DVSI AMBE-3000 Vocoder Chip User's Manual, rate mode table.
(Not reproduced here — DVSI documentation is proprietary. Check the
manual section titled "Rate Selection" or the AT command reference for
`ATRATE=nn`.)

## Which rate to use for a given purpose

- **Decoder conformance against BABA-A over-the-air format** → rate 33.
  These `.bit` files carry full Golay [24,12] ĉ₀ and Golay [23,12]+PN ĉ₁
  codewords and match BABA-A Eq. 189–192 bit-for-bit.
- **Decoder dequantization conformance without FEC complexity** → rate 34.
  Skip the Golay layer entirely; feed the 49-bit payload directly to
  the prioritization inverse (Tables 15–18) and the parameter
  reconstruction (§2.11–§2.13). Useful for bisecting "is the bug in
  FEC or in dequantize?"
- **PCM-to-PCM end-to-end conformance** → either rate 33 or rate 34,
  matched against DVSI's corresponding reference PCM output.
- **Rate 39 is not useful for P25 work.** It's a different bit
  allocation and has no Golay. Do not point P25 conformance harnesses
  at rate 39 files.

## The diagnostic fingerprint for "wrong rate mode"

If your harness decodes a DVSI `.bit` file and Golay-24-12 reports errors
on every frame with this distribution:

| errors | frequency |
|-------:|----------:|
|      0 |     ~0%   |
|      1 |  ~0.5–1%  |
|      2 |  ~6–12%   |
|      3 | ~43–87% (of correctable frames) |
|     ≥4 |  ~40–50%  |

— specifically, **if the errors={0, 1, 2, 3} distribution closely matches
`C(24, k) / 2325`** and about **57% of frames are correctable vs 43%
uncorrectable** — you are decoding **non-FEC bits as though they were
FEC-encoded**. This is the statistical signature of uniformly random
24-bit vectors being tested for Golay codeword membership, which is
mathematically what happens when you feed raw payload to an FEC decoder.

Computation: the [24,12] Golay has 2¹² codewords × (C(24,0) + C(24,1) +
C(24,2) + C(24,3)) = 4096 × 2325 = 9,523,200 vectors within correction
radius, out of 2²⁴ = 16,777,216 total. Ratio = 56.8% correctable. Within
the correctable set, distance-k frequency = C(24, k) / 2325.

If your histogram doesn't match this shape (e.g., it has a sharp peak at
errors=0 or at a specific low error count), the bug is elsewhere — a
real deinterleave convention issue, bit layout mismatch, or similar.
The fingerprint is specific to "no code in the bits at all."

## Recovery when the fingerprint hits

1. Confirm which DVSI rate the `.bit` file was captured under. Look for
   the rate code in the capture's metadata / filename / accompanying
   `.txt` log if any.
2. If rate 39 (or any no-FEC mode): either re-capture at rate 33 for
   full-stack testing, or point the harness at rate 34 files and skip
   the Golay layer entirely.
3. Confirm that rate-33 `.bit` files, when tested, produce **errors=0
   on every frame** (clean reference; FEC is error correction, not
   error introduction, so synthetic reference vectors never carry
   errors unless they're channel-impaired). Non-zero errors on a clean
   rate-33 file would indicate a real convention bug.

## Related impl-spec and analysis entries

- `analysis/halfrate_fec_layout.md` — the Golay [24,12] + [23,12] +
  uncoded frame structure that rate 33 implements.
- `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` §14.2 —
  Eq. 189–192 and the G_{24,12} / G_{23,12} matrices.
- `analysis/vocoder_decode_disambiguations.md` §11 — the γ_w and
  voiced-synthesis investigation whose phase-flip finding was
  potentially confounded by running against rate 39 files; re-verify
  under rate 33 before trusting the earlier candidate list.

## Broader lesson

A spec-repo-side investigation that keeps finding "the PDF matches your
code, but DVSI disagrees" across multiple independent layers (Annex O,
Table 15, G_{23,12}, G_{24,12}, Annex S) is a strong signal that the
test-vector source, not the spec, is mis-configured. Two or three
independent layers all clean-in-isolation but collectively diverging
from a reference implementation is the pattern to watch for.
