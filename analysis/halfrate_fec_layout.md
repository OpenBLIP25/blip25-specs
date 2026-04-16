# Half-Rate AMBE+2 FEC Layout: Only Two Codes, Not Three

**Scope:** the FEC structure of the 72-bit half-rate AMBE+2 voice frame
(BABA-A §14.2, Figure 27, Eq. 189–192). This note exists because the
in-tree impl spec and Full_Text extraction both originally claimed that
`c̃₁..c̃₃` were all [23,12] Golay-coded, which is wrong and cannot
possibly close the bit-width budget.

## The canonical layout (PDF-authoritative)

Per TIA-102.BABA-A §14.2 Eq. 189–192 (page 68) and Figure 27 (page 66):

| Code vector | Bits | FEC                                | Source |
|-------------|-----:|------------------------------------|--------|
| ĉ₀          |   24 | [24,12] extended Golay of û₀       | Eq. 189 |
| ĉ₁          |   23 | [23,12] Golay of û₁, XOR with m̂₁ PN | Eq. 190 |
| ĉ₂          |   11 | **uncoded** pass-through of û₂     | Eq. 191 |
| ĉ₃          |   14 | **uncoded** pass-through of û₃     | Eq. 192 |
| **Total**   |   72 |                                    |        |

Information-bit totals (from §14.1 Tables 15–18): û₀=12, û₁=12, û₂=11,
û₃=14 → **49 bits**, matching Table 12 "Half-Rate Voice/Silence Frame
Bit Allocation". The 23 FEC bits come entirely from the two Golay codes
(12 parity from [24,12] + 11 parity from [23,12] = 23).

## The bug being disambiguated

Pre-fix, the impl spec §2.4.2 read "Applied to u1 and u2" and the §2.6
C-comment described `c_out[2]: 23 bits ([23,12] Golay)`. The §16.5 prose
in the Full_Text extraction echoed the same claim (`c̃₁..c̃₃: [23,12]
Golay`). All three are wrong.

**Why the math rules it out directly:**

If ĉ₂ were Golay-coded at [23,12], the four code vectors would be
24 + 23 + 23 + ĉ₃ = 72, forcing ĉ₃ = 2. With û₃ then at 2 bits,
the information total would be 12 + 12 + 12 + 2 = **38**, not 49. So
the "three Golay codes" story cannot simultaneously produce 72 channel
bits and 49 information bits. It can only produce one or the other.

**Independent confirmation (Annex S bit interleave):**

The half-rate bit interleave table (BABA-A Annex S, p. 143) lists bit
positions for four code vectors with widths c₀=24, c₁=23, c₂=11, c₃=14.
That is the layout that actually round-trips through the channel codec
and also matches the cross-reference in TIA-102.BBAC-1 Annex E (burst
symbol 44 = frame symbol 34 prints `c0(6) | c1(11)`, which is only
consistent with c₂ ≤ 11 bits wide).

**PDF Figure 27 (page 66)** makes it visual: only the `û₀` and `û₁`
lines pass through Golay boxes. The `û₂, û₃` branch is labeled
**"No FEC"**.

## Implementation checklist

When reviewing a half-rate implementation, the three load-bearing
assertions to verify are:

1. **Two code vectors are FEC-protected, not three.** Decoders that
   attempt to Golay-decode `c_out[2]` will produce garbage
   spectral-shape payloads on every frame.
2. **ĉ₂ is 11 bits wide, ĉ₃ is 14 bits wide.** Not 23 + something-else.
   A 23-bit ĉ₂ cannot exist; the bits simply aren't on the wire.
3. **PN modulation is applied only to ĉ₁.** Eq. 190 is the only equation
   that adds `m̂₁`; ĉ₀, ĉ₂, ĉ₃ are PN-free. The PN seed `p_r(0) = 16·û₀`
   makes ĉ₁ frame-to-frame-unique even when û₁ repeats.

## Error counting for frame-repeat logic (§14.5)

Because only two codewords carry FEC, the error-estimation equations
(§14.5 Eq. 196–197) use only two terms:

```
ε_T = ε₀ + ε₁
ε_R(0) = 0.95·ε_R(−1) + 0.001064·ε_T                (Eq. 197)
```

`ε₀` = bits corrected by [24,12] extended Golay on ĉ₀.
`ε₁` = bits corrected by [23,12] Golay on ĉ₁.

There is no `ε₂` or `ε₃` — the uncoded vectors have no correction
metric. This is consistent with the FEC layout and is a secondary sanity
check on the two-Golay-codes reading.

## Related impl-spec fixes

- `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` §2.3,
  §2.4, §2.5, §2.6 — all brought into line with Eq. 189–192.
- `standards/TIA-102.BABA-A/TIA-102-BABA-A_Full_Text.md` §16.5 —
  paraphrase corrected.
- `standards/TIA-102.BABA-A/annex_tables/annex_s_interleave.csv` — was
  already correct (it's the source of truth that disagreed with the
  prose in the first place).
