# P25 Trellis Coding — MMDVMHost Cross-Reference Audit

**Audit subject:** `g4klx/MMDVMHost` (Jonathan Naylor, GPLv2) —
specifically `P25Trellis.cpp` / `P25Trellis.h`, the rate-½ and rate-¾
trellis encoder/decoder used in the MMDVM transceiver chain.

**Spec under audit:** `~/blip25-specs/standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md`,
§9.1 (interleave), §10 (trellis FSMs and constellation), against TIA-102.BAAA-B
§9 / §10 in the PDF.

**Why MMDVMHost specifically:** unlike OP25 (RX-focused) and SDRTrunk
(RX-only), MMDVMHost is part of a **transceiver** stack. Both encode and
decode paths exist in the same source file, so a bit-ordering mistake
that's symmetric inside the project would still round-trip against itself
— but it cannot agree with an independently-derived spec table unless the
table is right. That makes it a stronger oracle than RX-only projects for
the constellation/state-table layer.

## TL;DR

All four code-defining tables in the BAAA-B Implementation Spec match
MMDVMHost byte-for-byte:

| Spec table | Spec location | MMDVMHost symbol | Status |
|---|---|---|:---:|
| Rate-½ state transitions `TRELLIS_1_2[4][4]` | §10.2 | `ENCODE_TABLE_12[16]` (P25Trellis.cpp:38–42) | ✓ identical |
| Rate-¾ state transitions `TRELLIS_3_4[8][8]` | §10.3 | `ENCODE_TABLE_34[64]` (P25Trellis.cpp:28–36) | ✓ identical |
| Constellation point → dibit pair `CONSTELLATION[16]` | §10.4 | `pointsToDibits` switch (P25Trellis.cpp:270–340) | ✓ identical |
| Trellis interleave permutation `TRELLIS_INTERLEAVE[98]` | §9.1 | `INTERLEAVE_TABLE[98]` (P25Trellis.cpp:22–26) | ✓ identical |

Encoder structural choices (state update rule, MSB-first bit packing,
flush-symbol value) also match. The only meaningful divergence is the
**decoder algorithm**, which the standard does not specify — see
"Decoder choice differs" below.

This audit raises spec-author confidence in §9.1 and §10 of the BAAA-B
Implementation Spec from "extracted from PDF, structurally consistent,
not bit-validated against an independent encoder" to "structurally and
table-level matched against an independent open-source TX+RX
implementation."

## Provenance and license

- **Author:** Jonathan Naylor, G4KLX. © 2016, 2018, 2023, 2024, 2025.
- **License:** GPLv2 (per `P25Trellis.cpp` header lines 1–12).
- **Origin:** part of the MMDVM stack — written natively by Naylor for
  the host-side daemon that drives the MMDVM modem firmware. Not a fork
  of GNURadio, OP25, SDRTrunk, or balr0g/imbe_vocoder.

### Clean-room handling

This is a copyleft codebase. The implementer in `~/blip25-mbe/` MUST
NOT read it directly (combined GPL contamination + clean-room
separation). The spec-author side may read it freely as a
cross-reference, the same as SDRTrunk/OP25 and Pavel Yazev's IMBE.

This audit document is the derived work the implementer consumes. No
source paste beyond:

- Constant integer tables (de minimis — these are values dictated by
  TIA-102.BAAA-B; their numeric content is the standard's, not Naylor's
  creative expression).
- Filename and line-number citations.
- Algorithmic paraphrase.

## §10.2 Rate-½ state transition table — match

Spec §10.2 gives:

```
TRELLIS_1_2[state][input_dibit] → constellation_point
state 0: { 0, 15, 12,  3}
state 1: { 4, 11,  8,  7}
state 2: {13,  2,  1, 14}
state 3: { 9,  6,  5, 10}
```

MMDVMHost `P25Trellis.cpp` lines 38–42 declares `ENCODE_TABLE_12[16]`
with the same 16 values in the same row-major order, and indexes it as
`ENCODE_TABLE_12[state * 4U + bit]` (line 164). Equivalent layout.

→ **Identical.**

## §10.3 Rate-¾ state transition table — match

Spec §10.3 gives an 8×8 table indexed as
`TRELLIS_3_4[state][input_tribit] → constellation_point`.

MMDVMHost lines 28–36 declare `ENCODE_TABLE_34[64]` with the 64 values
in the same row-major order, indexed as `ENCODE_TABLE_34[state * 8U +
tribit]` (line 105).

Row-by-row spot check:

| Row (state) | Spec | MMDVMHost row offset | Match |
|---:|---|---|:---:|
| 0 | 0,8,4,12,2,10,6,14 | indices 0–7 | ✓ |
| 1 | 4,12,2,10,6,14,0,8 | indices 8–15 | ✓ |
| 2 | 1,9,5,13,3,11,7,15 | indices 16–23 | ✓ |
| 3 | 5,13,3,11,7,15,1,9 | indices 24–31 | ✓ |
| 4 | 3,11,7,15,1,9,5,13 | indices 32–39 | ✓ |
| 5 | 7,15,1,9,5,13,3,11 | indices 40–47 | ✓ |
| 6 | 2,10,6,14,0,8,4,12 | indices 48–55 | ✓ |
| 7 | 6,14,0,8,4,12,2,10 | indices 56–63 | ✓ |

→ **Identical.**

## §10.4 Constellation point → dibit pair — match

Spec §10.4 maps constellation points 0–15 to ordered dibit pairs
in {−3, −1, +1, +3}. MMDVMHost's `pointsToDibits` (lines 270–340) is
a switch over the same 16 cases. Sample:

| Point | Spec (d0, d1) | MMDVMHost (d0, d1) |
|---:|:---:|:---:|
| 0 | (+1, −1) | (+1, −1) |
| 1 | (−1, −1) | (−1, −1) |
| 2 | (+3, −3) | (+3, −3) |
| 3 | (−3, −3) | (−3, −3) |
| 4 | (−3, −1) | (−3, −1) |
| 5 | (+3, −1) | (+3, −1) |
| 6 | (−1, −3) | (−1, −3) |
| 7 | (+1, −3) | (+1, −3) |
| 8 | (−3, +3) | (−3, +3) |
| 9 | (+3, +3) | (+3, +3) |
| 10 | (−1, +1) | (−1, +1) |
| 11 | (+1, +1) | (+1, +1) |
| 12 | (+1, +3) | (+1, +3) |
| 13 | (−1, +3) | (−1, +3) |
| 14 | (+3, +1) | (+3, +1) |
| 15 | (−3, +1) | (−3, +1) |

→ **Identical** for all 16 points. (`dibitsToPoints` at lines 232–268 is
the inverse; also matches.)

## §9.1 Trellis interleave permutation — match

Spec §9.1 declares `TRELLIS_INTERLEAVE[98]` as a 98-element permutation
written in 4 rows of 26/24/24/24 values. The semantic is `interleaved[out]
= encoded[TRELLIS_INTERLEAVE[out]]`.

MMDVMHost `INTERLEAVE_TABLE[98]` (lines 22–26) holds the same 98 values
in the same order. Naylor's `interleave` function (lines 199–230)
implements `data[i] ← dibits[INTERLEAVE_TABLE[i]]` in dibit form (i.e.
`data` is the interleaved output, `dibits[INTERLEAVE_TABLE[i]]` is the
encoder's output before interleaving). Direction matches the spec
comment exactly.

`deinterleave` (lines 175–197) inverts it via `dibits[INTERLEAVE_TABLE[i]]
← data[i]` — uses the same table forward.

→ **Identical**, both as a table and as a direction-of-application.

## Encoder structural choices — match

**State update rule.** The spec encoder (§10.5) sets `state = input_dibit`
after each output (rate-½) — i.e. the next state is the current input
symbol. Same for rate-¾ where `state = input_tribit`. MMDVMHost line 167
(rate-½) and line 107 (rate-¾) implement exactly this. → match.

**Bit packing.** The spec encoder pseudo-code processes input dibits MSB-
first within each byte (`(data[b] >> shift) & 0x03` with shift starting
at 6, line 1763 of the impl spec). MMDVMHost `bitsToDibits` (lines 364–
381) reads bits 0,1,2,… of the payload buffer with `READ_BIT(payload,
i*2 + 0/1)` and packs them MSB-first into a dibit (`dibit |= b1 ? 2U :
0U; dibit |= b2 ? 1U : 0U;`). `bitsToTribits` (lines 342–362) does the
analogous MSB-first packing of three bits per tribit. → same convention.

**Flush symbol.** Spec §10.5 line 1772 specifies a final flush dibit of
`0x0` (00). MMDVMHost line 380: `dibits[48U] = 0U` (rate-½) and line 361:
`tribits[48U] = 0U` (rate-¾). → match.

These structural matches mean a Rust implementation built solely from
the BAAA-B Implementation Spec §9.1/§10 and round-tripped against
MMDVMHost should produce **bit-identical encoded output** for identical
input, on every block.

## Decoder choice differs — both legitimate

The standard specifies the encoder, not the decoder. The BAAA-B
Implementation Spec §10.5 notes: *"Trellis decoding is typically done
with Viterbi algorithm (not specified in the standard). SDRTrunk
implements this in TrellisCodec.java."* This is correct as far as it
goes, but oversells Viterbi as the canonical choice. MMDVMHost shows a
different valid approach.

**SDRTrunk:** maximum-likelihood Viterbi over the full block.

**MMDVMHost:** sequential trellis search with bounded backtracking
(`fixCode34`/`fixCode12`, lines 418–527). The decoder walks the trellis
state-by-state. If the received constellation point is a valid edge from
the current state, accept and advance. If not, try all 16 substitutions
at that position and keep the one that lets the decoder progress
furthest before failing again. Iterate up to 20 times. Backtrack one
position once if needed.

Why MMDVMHost picks this: it's a Pi/STM32 codebase. The sequential
search is `O(n)` with a small constant factor, no soft metrics, no
traceback memory. Viterbi is `O(n · |states|²)` with full path metrics.
For low-error-rate channels (decoded data blocks see relatively clean
4-FSK after frame-sync acquisition), the sequential search recovers the
same bits Viterbi would, much faster.

**Implications for blip25-mbe:**

- The spec is silent on decoder; either implementation is conformant.
- For first-cut implementation, the sequential approach is simpler and
  matches MMDVMHost. For RF performance under burst errors, Viterbi
  (matching SDRTrunk) is stronger.
- Round-trip oracle testing (encode → decode) does not distinguish them
  on clean input — both must recover the original bits exactly. Use
  noisy input to compare error-correction floors.

## Bit packing convention — caveat for the implementer

When transcribing tribits and dibits, MMDVMHost confirms **MSB-first**
within each input symbol:

- Rate-½: input bit at offset `n*2 + 0` is the dibit MSB; `n*2 + 1` is
  the LSB.
- Rate-¾: input bits at offsets `n*3 + 0,1,2` map to tribit bits 2,1,0.

This matters because it determines how the 12-byte payload (rate-½) or
18-byte payload (rate-¾) is decomposed into the 48 input symbols. A
project that packs LSB-first will produce trellis output that decodes
back to scrambled bytes with no errors flagged — round-trip clean
against itself but bit-incompatible with every other P25 implementation.

The BAAA-B Implementation Spec §10.5 line 1754 calls this out
("MSB-first within each byte"); MMDVMHost is the third-party
confirmation.

## What this audit does NOT cover

- **Trellis FSM convolutional polynomial.** The spec gives the state
  transition table directly (which is sufficient and what implementers
  should code from). MMDVMHost uses the same table approach, so neither
  exposes the underlying generator polynomials. Not a gap — table form
  is canonical.
- **Soft-decision input.** Both spec §10 and MMDVMHost are
  hard-decision. Soft-decision Viterbi (which would use symbol metrics)
  is out of scope here.
- **PDU header CRC, packet CRC-32, per-block CRC-9.** These ride on top
  of the trellis-coded blocks but are independent of the trellis layer.
  Audited elsewhere if needed (see `analysis/fdma_pdu_frame.md` §6 for
  the unified CRC summary).
- **Status-symbol stripping before trellis decode.** Status symbols are
  inserted every 36 raw dibits during transmission and stripped before
  trellis processing. This is a layer above trellis coding; MMDVMHost
  handles it at the modem-firmware boundary and `P25Trellis.cpp`
  receives already-stripped data. Not in scope for this audit.

## Recommended use as test oracle

A blip25-mbe trellis implementation can be validated against MMDVMHost
without the implementer ever reading MMDVMHost source:

1. Spec-author writes a small CLI wrapper around `CP25Trellis::encode12`
   / `encode34` / `decode12` / `decode34` (this side; not in
   blip25-mbe). The wrapper takes hex on stdin and emits hex on stdout.
2. Implementer's Rust trellis code emits the same hex format.
3. Both sides ingest the same fuzzed inputs (12-byte payloads for
   rate-½, 18-byte for rate-¾). Compare 196-bit outputs byte-for-byte.
4. For decode oracle: feed deliberately-corrupted blocks; both should
   recover the original payload up to the code's correction limit (free
   distance 4 for rate-½, 3 for rate-¾).

A diff at any block is either a Rust bug or a spec extraction bug. If
multiple inputs diverge, the spec is suspect; if isolated to specific
state transitions, the Rust implementation is.

The wrapper is a derived work of MMDVMHost (links against GPLv2 code),
so it stays on the spec-author side and is not committed to
`~/blip25-mbe/` — the implementer only sees the byte-level test
vectors it produces.

## References

- TIA-102.BAAA-B (2017) §9, §10 — the standard text.
- `~/blip25-specs/standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md`
  §9.1, §10 — derived implementation spec.
- `g4klx/MMDVMHost` `P25Trellis.cpp`, `P25Trellis.h` — third-party
  open-source TX+RX implementation, GPLv2.
- `g4klx/MMDVM` `P25TX.cpp`, `P25RX.cpp` — companion firmware-side RF
  framing (out of scope for this audit; covers the C4FM modem layer
  feeding into / out of the trellis layer).
