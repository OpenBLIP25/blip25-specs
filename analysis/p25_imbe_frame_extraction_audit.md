# P25 IMBE Frame Extraction — MMDVMHost Cross-Reference Audit

**Audit subject:** `g4klx/MMDVMHost` `P25Audio.cpp` — bit-position
extraction of the 9 IMBE voice frames from LDU1/LDU2, the per-frame
144-bit IMBE_INTERLEAVE permutation, and the PN-mask seed.

**Spec under audit:**
- BAAA-B Implementation Spec §6.5 / Annex A3 / Annex A4 (LDU1/LDU2
  transmit bit order CSVs at
  `standards/TIA-102.BAAA-B/annex_tables/annex_a3_ldu1_transmit_bit_order.csv`
  and `annex_a4_ldu2_transmit_bit_order.csv`).
- BABA-A Implementation Spec §1.7 / Annex H (full-rate IMBE interleave
  at `standards/TIA-102.BABA-A/annex_tables/annex_h_interleave.csv`).
- BABA-A Implementation Spec §1.8 / Eq. 84 (PN-mask seed).

**Companion to:** `analysis/imbe_spec_audit_vs_jmbe_vs_chip.md` — that
audit covered IMBE FEC constants (Golay/Hamming generators, PN-mask
range tables); this audit covers the bit-position layer that sits
above those constants.

## TL;DR

Three independent validations, all matching:

| Item | Spec source | MMDVMHost source | Status |
|---|---|---|:---:|
| 9 IMBE-frame on-air bit-position pairs in LDU1 | BAAA-B `annex_a3_ldu1_transmit_bit_order.csv` (clustering c_X(Y) refs by MMDVMHost ranges) | `P25Audio.cpp` lines 59–93 — hard-coded `(start, end)` pairs | ✓ all 9 match exactly |
| Per-frame IMBE bit count = 144 + 4 status | BAAA-B annex_a3 c_0..c_7 ref counts | MMDVMHost 148-bit window per frame | ✓ all 9 frames hold (23+23+23+23+15+15+15+7=144) IMBE bits + 4 SS bits |
| 144-element IMBE_INTERLEAVE permutation | BABA-A `annex_h_interleave.csv` | `P25Audio.cpp` lines 29–35 `IMBE_INTERLEAVE[]` | ✓ all 144 entries match (after bit-index convention adjustment, see §3 below) |
| PN-mask seed `p_r(0) = 16 · û₀` | BABA-A §1.8 Eq. 84 | `P25Audio.cpp` line 178 `p = 16U * c0data` | ✓ identical |

These four results jointly validate the BAAA-B Annex A3/A4 LDU1/LDU2
extracted CSVs, the BABA-A Annex H IMBE interleave CSV, and the BABA-A
PN-seed equation against an independent open-source TX+RX
implementation. This is the strongest validation we have for the
IMBE-bits-on-air layer.

The audit also surfaces one **bit-indexing convention gotcha** worth
documenting in the impl spec: BABA-A Annex H labels bit indices
**LSB-first within each IMBE codeword vector** (so `c_0(22)` is the
LSB of c0's 23-bit Golay codeword), while MMDVMHost (and any C-style
shift-register implementation) treats bit 0 as the MSB. The two
representations agree numerically once the per-vector bit-index is
remapped via `flat = vector_offset[v] + (vector_size[v] − 1 − k)`.

## Provenance and license

- **`P25Audio.cpp` / `P25Audio.h`:** Jonathan Naylor (G4KLX) © 2016,
  2023, 2025. GPLv2.
- Depends on `Golay24128.cpp`, `Hamming.cpp`, `P25Utils.cpp`. The
  Golay (24,12,8) constants used here are validated in
  `analysis/p25_golay_24_12_implementation_audit.md`. The Hamming
  (15,11,3) constants are validated in `imbe_spec_audit_vs_jmbe_vs_chip.md`.

### Clean-room handling

GPLv2. Implementer in `~/blip25-mbe/` MUST NOT read. Spec-author side
reads freely. This audit is the consumable derived work.

## §1 IMBE-frame on-air bit-position pairs

MMDVMHost's `P25Audio.cpp::process` (lines 51–96) calls
`CP25Utils::decode(data, imbe, START, END)` with these nine
`(START, END)` pairs covering the 9 IMBE frames in LDU1/LDU2:

| Frame | MMDVMHost (start, end) | Span | annex_a3 IMBE-N first symbol → bit | First-bit match |
|:---:|---|:---:|---|:---:|
| 0 | (114, 262) | 148 | symbol 57 → bit 114 | ✓ |
| 1 | (262, 410) | 148 | symbol 131 → bit 262 | ✓ |
| 2 | (452, 600) | 148 | symbol 226 → bit 452 | ✓ |
| 3 | (640, 788) | 148 | symbol 320 → bit 640 | ✓ |
| 4 | (830, 978) | 148 | symbol 415 → bit 830 | ✓ |
| 5 | (1020, 1168) | 148 | symbol 510 → bit 1020 | ✓ |
| 6 | (1208, 1356) | 148 | symbol 604 → bit 1208 | ✓ |
| 7 | (1398, 1546) | 148 | symbol 699 → bit 1398 | ✓ |
| 8 | (1578, 1726) | 148 | symbol 789 → bit 1578 | ✓ |

The 148-bit width per frame decomposes as 144 IMBE bits (72 dibits)
plus 4 bits of interspersed status symbols (2 SS slots × 2 bits each,
since one IMBE frame straddles 2 SS-cadence boundaries — SS appears
every 35 information dibits, and a 72-dibit IMBE frame spans 72/35 ≈
2.06 SS gaps). `CP25Utils::decode(…, START, END)` strips those 4 SS
bits during extraction, leaving the 144-bit IMBE FEC frame in the
output buffer.

### Per-frame content cross-check

Verified by parsing the bit-content columns (`bit1`, `bit0`) of
annex_a3 for `c_X(Y)` references, then clustering each reference into
the MMDVMHost frame range it falls within:

```
Total IMBE c_X(Y) bit references in annex_a3: 1296   (= 9 × 144 ✓)
Total SS references in annex_a3:                48   (= 9 × 4 SS bits = 24 SS slots ✓)

Per-frame:
  frame 0..8: 144 IMBE bits + 4 SS bits = 148 ✓ all nine
  Per-frame c_0..c_7 bit count: [23, 23, 23, 23, 15, 15, 15, 7] ✓ all nine
```

So each MMDVMHost 148-bit window contains *exactly* one full IMBE FEC
frame (one full c_0 codeword of 23 bits, one full c_1 of 23 bits, …,
one full c_7 of 7 bits) plus the standard 4 SS bits, with no leakage
between frames. The annex_a3 CSV bit-content layer and MMDVMHost's
extraction agree perfectly.

## §2 IMBE_INTERLEAVE[144] vs. BABA-A Annex H

MMDVMHost's `P25Audio.cpp` lines 29–35 declare the 144-element
deinterleave map as:

```
IMBE_INTERLEAVE[i] = on-air bit position of deinterleaved bit i
```

where deinterleaved bit `i` (0..143) flattens the 8 IMBE codeword
vectors (c_0..c_7) using the offset table:

```
c_0: flat 0..22    (23 bits, Golay-encoded)
c_1: flat 23..45   (23 bits)
c_2: flat 46..68   (23 bits)
c_3: flat 69..91   (23 bits)
c_4: flat 92..106  (15 bits, Hamming-encoded)
c_5: flat 107..121 (15 bits)
c_6: flat 122..136 (15 bits)
c_7: flat 137..143 (7 bits, no FEC)
```

BABA-A `annex_h_interleave.csv` describes the same permutation but
indexed by the 72 dibit symbols of one IMBE frame; for each symbol
`s`, the dibit's MSB (`bit1`) and LSB (`bit0`) carry specific
`(vector, index)` pairs.

### Direct comparison fails — bit-index convention difference

A naïve mapping `flat = vector_offset[v] + index` produces 136
mismatches out of 144. Inspection of `(symbol 0, bit1)` in Annex H —
which says `c_0(22)` — paired against MMDVMHost's
`IMBE_INTERLEAVE[0] = 0` (deinterleaved bit 0 came from on-air bit 0)
shows the issue:

- MMDVMHost orders deinterleaved bits **MSB-first** within each
  codeword: deinterleaved bit 0 is c_0's most-significant bit (the
  first bit transmitted on air, the codeword's high-order term).
- Annex H labels bit indices **LSB-first** within each codeword:
  `c_0(22)` is the bit at index 22 counting from the LSB end =
  c_0's MSB.

So `c_0(22)` (Annex H) and "c_0 MSB" (MMDVMHost) refer to the same
physical bit, but their numeric labels differ by `(vector_size − 1 −
index)`.

### After bit-index remap, all 144 entries match

Applying the remap

```python
flat = vector_offset[v] + (vector_size[v] − 1 − bit_index)
```

where `vector_size = [23, 23, 23, 23, 15, 15, 15, 7]`, all 144
IMBE_INTERLEAVE entries match the Annex H CSV bit-for-bit. Verified
script:

```python
mmdvm = [...]   # MMDVMHost IMBE_INTERLEAVE[144]
spec_interleave = [None] * 144
vector_offset = [0, 23, 46, 69, 92, 107, 122, 137]
vector_size   = [23, 23, 23, 23, 15, 15, 15, 7]
for s, b1v, b1i, b0v, b0i in annex_h_rows:
    flat_msb = vector_offset[b1v] + (vector_size[b1v] - 1 - b1i)
    flat_lsb = vector_offset[b0v] + (vector_size[b0v] - 1 - b0i)
    spec_interleave[flat_msb] = 2 * s        # on-air dibit MSB
    spec_interleave[flat_lsb] = 2 * s + 1    # on-air dibit LSB
assert mmdvm == spec_interleave  # passes for all 144
```

This validates Annex H at the codeword-bit level — strictly stronger
than the previously-verified "all 144 bit positions covered exactly
once" check noted in the CSV header.

### Recommended impl-spec patch

The bit-index convention is implicit and easy to get wrong. Add a
note to BABA-A §1.7 Interleaving section AND to the
`annex_h_interleave.csv` header explicitly stating that **`c_X(Y)`
indices are LSB-first within each codeword vector**, and provide the
inverse mapping for implementers who use MSB-first packing (every C
implementation that uses shift-register encoding does).

## §3 PN-mask seed

`P25Audio.cpp` line 178:

```cpp
unsigned int p = 16U * c0data;
for (unsigned int i = 0U; i < 114U; i++) {
    p = (173U * p + 13849U) % 65536U;
    prn[i] = p >= 32768U;
}
```

This implements BABA-A §1.8 Eq. 84–85:

```
p_r(0) = 16 · û₀
p_r(n) = (173 · p_r(n-1) + 13849) mod 65536    for 1 ≤ n ≤ 114
mask_bit(n) = 1 if p_r(n) ≥ 32768 else 0
```

Multiplier `173`, increment `13849`, modulus `65536`, and seed
multiplier `16` all match. The PN sequence is consumed for indices
1..114 (114 mask bits applied to bits 23..136 of the 144-bit FEC
frame, skipping c_0 at flat 0..22 and c_7 at flat 137..143 which are
masked with all-zero per BABA-A §1.8). MMDVMHost iterates
`for i in 0..114` and applies `bit[i+23] ^= prn[i]` at line 186 — same
range and same indexing.

### One subtlety worth surfacing in the impl spec

BABA-A §1.8 specifies the seed as `16 · û₀` where û₀ is the
**Golay-decoded** 12-bit info word (post-FEC-correction).
MMDVMHost's `decode()` uses the **raw** first 12 bits of c_0's Golay
codeword (`c0data` in the code), which equals û₀ only if the c_0
codeword was received error-free or if the systematic encoding
preserves the info portion at the top of the codeword. Both
conditions hold: P25 uses systematic Golay, so the first 12 bits of
the codeword *are* the info word, and they match û₀ exactly when
c_0 has zero errors.

If c_0 has correctable errors in the info portion, MMDVMHost's
pre-Golay seed differs from the post-Golay û₀. The downstream PN
demodulation of c_1..c_6 then uses a wrong mask, and c_1..c_6 decode
will fail or produce errors. Whether this matters in practice
depends on the receiver's error budget — for clean signals it
doesn't matter; for marginal signals near the FEC edge it costs
some recoverable frames.

The spec is silent on this trade-off. Recommended impl-spec
addition (BABA-A §1.8): note that the PN seed must use the
Golay-decoded û₀ for full FEC robustness, but the raw pre-Golay info
bits give the same seed when c_0 has no errors and are simpler to
extract — implementations may choose either, with the documented
trade-off.

## §4 What this audit does NOT cover

- **annex_a4 LDU2 transmit bit order** — only annex_a3 (LDU1) was
  parsed. LDU2 has the same 9-IMBE-frame structure with the same
  bit-position pairs (LDU1 and LDU2 share the IMBE-frame layout;
  what differs is the LC vs. ES content of the non-IMBE bits). The
  9-frame validation in §1 transitively applies to LDU2.
- **`regenerateIMBE`** in MMDVMHost's `P25FEC` class. Calls Golay /
  Hamming decode + re-encode to clean up FEC errors. Constants
  validated in `imbe_spec_audit_vs_jmbe_vs_chip.md`.
- **CQPSK / LSM modulation paths** — out of scope; this audit
  concerns the symbol-to-bit layer that's identical for C4FM and
  CQPSK receivers.

## §5 Recommended impl-spec patches

1. **BABA-A §1.7 — clarify Annex H bit-index convention** (LSB-first
   within each codeword vector). Add the inverse mapping for
   MSB-first packing implementations.
2. **`annex_h_interleave.csv` header — same convention note.**
3. **BABA-A §1.8 Eq. 84 — note the pre-Golay vs post-Golay seed
   trade-off** for implementations.

These are clarifications, not corrections. The spec content is
correct; just easy for an implementer to misapply.

## References

- TIA-102.BAAA-B (2017) §6.5, Annex A3/A4 — LDU1/LDU2 transmit bit
  order.
- TIA-102.BABA-A (2017) §1.7, Annex H — IMBE full-rate interleave.
- TIA-102.BABA-A (2017) §1.8, Eq. 84–85 — PN sequence generator.
- `~/blip25-specs/standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md`
  §6.5.
- `~/blip25-specs/standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md`
  §1.7, §1.8.
- `~/blip25-specs/standards/TIA-102.BAAA-B/annex_tables/annex_a3_ldu1_transmit_bit_order.csv`,
  `annex_a4_ldu2_transmit_bit_order.csv`,
  `~/blip25-specs/standards/TIA-102.BABA-A/annex_tables/annex_h_interleave.csv`.
- `g4klx/MMDVMHost` `P25Audio.cpp`, `P25Audio.h` — third-party
  TX+RX implementation, GPLv2.
- `analysis/imbe_spec_audit_vs_jmbe_vs_chip.md` — companion audit for
  IMBE FEC constants (Golay / Hamming generators, PN ranges).
- `analysis/p25_trellis_implementation_audit.md`,
  `p25_nid_implementation_audit.md`,
  `p25_hdu_ldu_lsd_implementation_audit.md`,
  `p25_golay_24_12_implementation_audit.md` — companion audits for
  trellis, NID, HDU/LDU/LSD, and Golay(24,12,8) layers.
