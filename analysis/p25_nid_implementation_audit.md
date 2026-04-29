# P25 NID — MMDVMHost Cross-Reference Audit

**Audit subject:** `g4klx/MMDVMHost` `P25NID.cpp` / `P25NID.h` plus its
BCH dependency `BCH.cpp` / `BCH.h`.

**Spec under audit:** `~/blip25-specs/standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md`,
§3.1 (NID structure), §3.2 (DUID values + parity bit), §3.3 (BCH(63,16,23)
generator), against TIA-102.BAAA-B §7.5.

**Companion to:** `analysis/p25_trellis_implementation_audit.md` — same
auditor pattern, same clean-room rules, different layer.

## TL;DR

Three independent confirmations of the NID layer impl spec:

| Item | Spec location | MMDVMHost source | Status |
|---|---|---|:---:|
| BCH(63,16,23) generator polynomial bits | §3.3 (octal `6331 1413 6723 5453` / hex `0x6649_0BDC_B52B`) | `BCH.cpp` `g[]` (lines 88–89) | ✓ identical |
| 64-bit NID layout (NAC/DUID/parity/overall-P) | §3.1 | `BCH::encode(unsigned char*)` + per-DUID parity sets in `P25NID` ctor | ✓ identical |
| DUID `0x7` (TSBK/TSDU) overall-parity P value | §3.2 (left as "*—*", deferred to AABB-B) | `m_tsdu[7U] &= 0xFEU;` → **P = 0** | new: third-party witness |

Plus one **architectural observation** worth pulling into the impl spec
as an informative §3.3.2 implementation pattern: MMDVMHost does **not**
run an algebraic BCH decoder at runtime. It pre-encodes the seven valid
NIDs for the configured NAC at construction time and decodes by Hamming
distance against that 7-element table. This is a legitimate decoder
choice that wasn't covered in §3.3.1's "Post-failure Recovery" section,
and it has direct implications for blip25-mbe receiver design (see
"Architectural divergence: 7-codeword Hamming search" below).

## Provenance and license

- **`P25NID.cpp` / `P25NID.h`:** Jonathan Naylor (G4KLX) © 2016, 2018,
  2023, 2025; Bryan Biedenkapp © 2018. GPLv2.
- **`BCH.cpp` / `BCH.h`:** based on Robert Morelos-Zaragoza's `bch3.c`
  (1994/1997), free-for-non-commercial-use academic code (header lines
  1–55). Used by many open-source projects as a canonical BCH reference.
  Naylor packaged it as a C++ class and added a P25-specific
  `encode(unsigned char*)` byte-buffer wrapper.

### Clean-room handling

GPLv2 (Naylor's code) plus the academic license on the
Morelos-Zaragoza core. The implementer in `~/blip25-mbe/` MUST NOT
read either. Spec-author side reads freely as a third-party reference.
This audit document is the derived work.

## §3.3 BCH(63,16,23) generator polynomial — bit-for-bit match

Impl Spec §3.3 line 275 specifies the generator polynomial in octal:

```
g(x) = octal 6331 1413 6723 5453
```

Expanding to binary (high-degree term first), 48 coefficients:

```
1 1 0 0 1 1 0 1 1 0 0 1 0 0 1 1
0 0 0 0 1 0 1 1 1 1 0 1 1 1 0 1
0 0 1 1 1 0 1 1 0 0 1 0 1 0 1 1
```

MMDVMHost `BCH.cpp` lines 88–89 declares the same 48 coefficients in the
same high-to-low order:

```
g[] = {1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1,
       0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1,
       0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1};
```

→ **Identical for all 48 coefficients.** The `BCH::encode(int* data, int*
bb)` function (BCH.cpp lines 99–124) is a textbook systematic LFSR
encoder computing `b(x) = x^(n-k) · data(x) mod g(x)` (Lin & Costello,
"Error Control Coding," ch. 4).

This validates §3.3 of the impl spec at the polynomial-coefficient
level. Combined with the existing JMBE/SDRTrunk constants noted in
`imbe_spec_audit_vs_jmbe_vs_chip.md`, the BCH polynomial is now
witnessed by three independent open-source codebases plus the PDF.

## §3.1 NID layout — match

Impl Spec §3.1:

```
[63..52] = NAC (12 bits)
[51..48] = DUID (4 bits)
[47..1]  = BCH parity (47 bits)
[0]      = Overall parity P
```

MMDVMHost layout (decoded from the constructor and `BCH::encode`):

| Byte | Bit positions (MSB first) | Field |
|:---:|---|---|
| 0 | bits 7..0 | NAC[11..4] |
| 1 | bits 7..4 | NAC[3..0] |
| 1 | bits 3..0 | DUID[3..0] |
| 2–6 | all 40 bits | BCH parity bits 0..39 |
| 7 | bits 7..1 | BCH parity bits 40..46 |
| 7 | bit 0 | Overall parity P |

Verification:
- Constructor lines 45–48 (HDU example):
  - `m_hdr[0] = (nac >> 4) & 0xFF` → NAC[11..4] in byte 0.
  - `m_hdr[1] = (nac << 4) & 0xF0` → NAC[3..0] in byte 1 high nibble.
  - `m_hdr[1] |= P25_DUID_HEADER` (= `0x00`) → DUID in byte 1 low nibble.
  - `bch.encode(m_hdr)` reads bits 0..15 (which is bytes 0–1) MSB-first
    via `READ_BIT(nid, i)` → matches the (NAC, DUID) info word.
  - `BCH::encode(unsigned char*)` lines 137–140 writes the 47 parity bits
    into positions `i + 16` for `i = 0..46` → bytes 2–7 high bits.
  - Final `m_hdr[7U] &= 0xFEU` clears the last bit (byte 7 LSB) → bit
    [0] is the overall parity P.

→ **Layout matches** the impl spec bit-for-bit.

## §3.2 DUID + parity-bit table — match, with disambiguation for TSBK

Impl Spec §3.2 table:

| DUID (hex) | Parity (P) | Data Unit |
|:---:|:---:|---|
| `0x0` | 0 | HDU |
| `0x3` | 0 | TDU |
| `0x5` | 1 | LDU1 |
| `0x7` | *—* | TSBK (per AABB-B) |
| `0xA` | 1 | LDU2 |
| `0xC` | 0 | PDU |
| `0xF` | 0 | TDULC |

MMDVMHost constructor sets the parity bit per DUID:

| DUID | MMDVMHost line | Operation | Resulting P |
|:---:|:---:|---|:---:|
| `0x0` HDU | 49 | `m_hdr[7U] &= 0xFEU` | **0** |
| `0x3` TDU | 77 | `m_term[7U] &= 0xFEU` | **0** |
| `0x5` LDU1 | 56 | `m_ldu1[7U] |= 0x01U` | **1** |
| `0x7` TSBK | 84 | `m_tsdu[7U] &= 0xFEU` | **0** |
| `0xA` LDU2 | 63 | `m_ldu2[7U] |= 0x01U` | **1** |
| `0xC` PDU | 91 | `m_pdu[7U] &= 0xFEU` | **0** |
| `0xF` TDULC | 70 | `m_termlc[7U] &= 0xFEU` | **0** |

→ **All seven match** the impl spec — and the impl spec's open
"*—*" entry for DUID `0x7` is now witnessed: **P = 0**. Recommend
updating the §3.2 table from "*—*" to "0 (per AABB-B; confirmed by
MMDVMHost)" with a brief note that MMDVMHost's transmit path
unconditionally sets the TSBK NID's overall parity to 0.

## Architectural divergence: 7-codeword Hamming search

MMDVMHost does not run an algebraic BCH decoder at all. Its `decode()`
method (P25NID.cpp lines 105–155) compares the received NID against a
pre-computed table of all seven valid `(NAC, DUID)` codewords and
accepts whichever is within `MAX_NID_ERRS = 5` bits.

The constructor (lines 32–92) pre-encodes one full 64-bit NID per DUID
using the configured NAC, storing them in `m_hdr`, `m_ldu1`, `m_ldu2`,
`m_term`, `m_termlc`, `m_tsdu`, `m_pdu`. At runtime, `decode()` runs
`CP25Utils::compare()` (Hamming distance) against each in turn and
returns the first match within 5 bits.

### Why this is correct

The BCH(63,16,23) code has minimum distance 23, so a fully-general
algebraic decoder corrects up to `t = ⌊(23-1)/2⌋ = 11` errors anywhere
in the 16K-codeword space.

But MMDVMHost is solving a much smaller problem: the receiver knows its
NAC ahead of time, so it only needs to distinguish among **7 valid
codewords sharing one NAC**, not all 65,536 possible codewords. By
linearity of BCH:

```
codeword(NAC, DUIDᵢ) ⊕ codeword(NAC, DUIDⱼ) = codeword(0, DUIDᵢ ⊕ DUIDⱼ)
```

The right-hand side is a non-zero codeword, so it has weight ≥ 23.
Therefore the **pairwise minimum distance among the seven NID codewords
sharing a fixed NAC is also ≥ 23**, and Hamming-nearest-codeword
correction can recover up to 11 errors with no miscorrection — the same
floor as the algebraic decoder.

MMDVMHost's threshold of 5 is therefore conservative by design. Its
purpose is a different problem: rejecting NIDs from foreign NACs that
might happen to land closer than the t-error sphere of the local NAC.
Setting the threshold at 5 (well below 11) gives a wide margin against
cross-NAC false positives.

### Why the implementer should care

This pattern works **only when the receiver knows the NAC up front**.
It's perfect for an MMDVM transceiver — you've configured your network's
NAC, you don't decode anyone else's traffic.

It is **NOT correct** for a passive scanner that decodes whatever signal
is on the air (`blip25-mbe`'s likely use case). A scanner doesn't know
the NAC; it has to *learn* it from the BCH-decoded NID. The 7-codeword
table approach reduces to "is this NID exactly the configured NAC + one
of 7 DUIDs?" — which always returns false on first sight of a new
network.

For a passive scanner, the right pattern is:
1. Algebraic BCH decode (Berlekamp-Massey or syndrome-table lookup) to
   recover `(NAC, DUID)` from any received NID, regardless of NAC value.
2. Optionally, once the active NAC(s) are known, layer the 7-codeword
   table on top as a **fast path** for already-tracked NACs. Falling
   back to the algebraic decoder for first-sight NIDs.

This two-tier approach is what makes the most sense for `blip25-mbe`:
algebraic BCH for discovery, codeword-table lookup as a hot-path
optimization once steady-state tracking begins. MMDVMHost shows that
the codeword-table layer corrects to the full t = 11 floor with zero
implementation complexity (no syndrome math, no Galois-field tables) —
it's purely a 64-bit XOR + popcount per candidate, 7 times per NID.

This pattern also suggests a **diagnostic invariant**: in steady state,
a scanner should observe the algebraic BCH decoder and the 7-codeword
lookup agreeing on every NID for a tracked NAC. A divergence indicates
either a software bug or a NAC change in the source, both worth
reporting.

### Recommendation: spec impl spec §3.3.1 expansion

The current §3.3.1 ("Post-failure Recovery") describes one widely-used
pattern (NAC-forced retry) for handling algebraic-decode failures. It
should be expanded with the **MMDVMHost pattern as Pattern B**:

- **Pattern A (existing):** algebraic BCH decoder + NAC-forced retry on
  failure.
- **Pattern B (new):** pre-computed 7-codeword Hamming-distance lookup,
  optionally as the *only* decoder (transceiver / known-NAC use case)
  or as a fast-path layer (scanner / mixed use case).

Both patterns are interop-safe (same `(NAC, DUID)` output when they
succeed; standard is silent on the algorithm).

A draft of the new §3.3.2 informative subsection is appended below.

## Decoder algorithm difference vs. SDRTrunk and OP25

| Project | Decoder | NAC handling | Effective t |
|---|---|---|:---:|
| SDRTrunk (`NID.java`) | algebraic BCH | discovers from decode | 11 |
| OP25 (`p25p1_fdma.cc::process_NID`) | algebraic BCH | discovers from decode | 11 |
| MMDVMHost (`P25NID.cpp`) | 7-codeword Hamming search | pre-configured | 11 (capped at 5 by `MAX_NID_ERRS`) |

All three are conformant to TIA-102.BAAA-B §7.5. The standard specifies
the encoder; each decoder choice reflects a different operational
context (multi-NAC passive monitoring vs. single-NAC transceiver). For
blip25-mbe, the right choice is *both*: algebraic decoder for
discovery + codeword-table lookup as a fast path once a NAC is being
tracked.

## What this audit does NOT cover

- **NAC value semantics.** The 12-bit NAC space has reserved values
  (`0x000`, `0xF7E`, `0xF7F`, etc.) defined in TIA-102.BAAC-D
  (Reserved Values). MMDVMHost treats NAC as opaque; semantics audited
  separately.
- **Frame Sync detection.** The 48-bit FS pattern (`0x5575F5FF7FFF`)
  precedes the NID. MMDVMHost handles this in modem firmware
  (`P25RX.cpp`, not in scope here).
- **Status-symbol stripping inside the NID.** Status symbols are
  transmitted every 36 raw dibits and stripped before NID processing.
  MMDVMHost's `CP25Utils::decode(data, nid, 48U, 114U)` (P25NID.cpp
  line 110) extracts NID bits 48..114 from the post-stripped data
  buffer; this is consistent with the impl spec's §11.2 Status Symbol
  Stripping but the SS-stripping logic itself wasn't re-audited here.

## Recommended impl spec patch

Two textual updates to `P25_FDMA_Common_Air_Interface_Implementation_Spec.md`:

1. **§3.2 table** — change DUID `0x7` (TSBK) row from "*—*" to "0 (per
   AABB-B; MMDVMHost confirms)" with a footnote citing this audit.

2. **§3.3.1 → split into §3.3.1 and §3.3.2** — keep the existing
   NAC-forced retry as §3.3.1 ("Pattern A"), add a new §3.3.2 ("Pattern
   B") describing the 7-codeword Hamming-distance lookup. Both
   informative, both interop-safe.

A draft of §3.3.2 follows.

---

### Draft text for new §3.3.2 (informative)

> #### 3.3.2 Pre-computed 7-Codeword Lookup (informative)
>
> A receiver that knows its NAC ahead of time (transceiver, fixed-base
> repeater, or a scanner that has already locked onto a NAC) can replace
> the algebraic BCH decoder with a Hamming-distance lookup against a
> pre-encoded table of seven NIDs.
>
> At configuration time, encode all seven valid `(NAC, DUIDᵢ)` 64-bit
> NIDs using the BCH(63,16,23) generator from §3.3 plus the per-DUID
> overall-parity bit from §3.2:
>
> ```c
> typedef struct {
>     uint64_t nid_hdu;     /* DUID 0x0, P = 0 */
>     uint64_t nid_tdu;     /* DUID 0x3, P = 0 */
>     uint64_t nid_ldu1;    /* DUID 0x5, P = 1 */
>     uint64_t nid_tsbk;    /* DUID 0x7, P = 0 (per AABB-B) */
>     uint64_t nid_ldu2;    /* DUID 0xA, P = 1 */
>     uint64_t nid_pdu;     /* DUID 0xC, P = 0 */
>     uint64_t nid_tdulc;   /* DUID 0xF, P = 0 */
> } NidCodewordTable;
>
> /* Build the 7-codeword table for a given 12-bit NAC. */
> void nid_codeword_table_init(NidCodewordTable *t, uint16_t nac);
> ```
>
> At decode time, for each received 64-bit NID, return the closest
> entry in Hamming distance if its distance is within an
> implementation-chosen threshold; otherwise return decode-failure.
> MMDVMHost uses a threshold of 5 bits (`MAX_NID_ERRS = 5`); a
> threshold up to 11 is theoretically safe (since the pairwise minimum
> distance among the seven entries is ≥ `dmin = 23`, by linearity of
> BCH), but lower thresholds reduce the cross-NAC false-positive rate
> in environments with multiple co-channel networks.
>
> ```c
> int nid_decode_lookup(const NidCodewordTable *t, uint64_t received,
>                       unsigned int max_errs,
>                       uint8_t *out_duid) {
>     uint64_t entries[7] = {t->nid_hdu, t->nid_tdu, t->nid_ldu1,
>                            t->nid_tsbk, t->nid_ldu2, t->nid_pdu,
>                            t->nid_tdulc};
>     uint8_t  duids[7]   = {0x0, 0x3, 0x5, 0x7, 0xA, 0xC, 0xF};
>     for (int i = 0; i < 7; i++) {
>         unsigned int errs = (unsigned int)__builtin_popcountll(received ^ entries[i]);
>         if (errs <= max_errs) { *out_duid = duids[i]; return 0; }
>     }
>     return -1;
> }
> ```
>
> This decoder is bit-compatible with the algebraic decoder up to its
> `max_errs` threshold and produces identical `(NAC, DUID)` output. It
> is appropriate as the sole decoder in a single-NAC system, or as a
> fast path layered above the algebraic decoder in a multi-NAC scanner
> (algebraic decoder for discovery, lookup for tracked-NAC steady
> state).

## References

- TIA-102.BAAA-B §7.5 — Network Identifier and BCH code definition.
- `~/blip25-specs/standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md`
  §3 — derived implementation spec for NID.
- TIA-102.AABB-B §4.2 — DUID `0x7` (TSBK) normative assignment.
- `g4klx/MMDVMHost` `P25NID.cpp`, `P25NID.h`, `BCH.cpp`, `BCH.h`,
  `P25Defines.h` — third-party TX+RX implementation, GPLv2.
- Robert Morelos-Zaragoza, *bch3.c* (1994/1997) — academic-license BCH
  reference; basis for `BCH.cpp`.
- `analysis/p25_trellis_implementation_audit.md` — companion audit for
  the trellis layer.
- `analysis/imbe_spec_audit_vs_jmbe_vs_chip.md` — earlier-pattern
  spec/JMBE cross-check for IMBE FEC.
