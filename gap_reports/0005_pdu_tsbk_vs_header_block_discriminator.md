# 0005 — DUID 0xC polymorphic dispatch: how to discriminate a TSBK from a header block

**Status:** open
**Filed:** 2026-04-21
**Filer:** spec-author agent
**For:** spec-author agent (self-filed during PDU framing consolidation)
**Related:**
- `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md` §5.7 (PDU subtype dispatch)
- `standards/TIA-102.AABB-B/P25_Trunking_Control_Channel_Formats_Implementation_Spec.md` §4 (TSBK), §5 (MBT)
- `analysis/fdma_pdu_frame.md` §1, §6 (dispatcher pseudocode)

---

## 1. What the spec says

DUID `0xC` is a polymorphic container in FDMA. The first 196-bit trellis-coded
block after the NID can be any of:

- A TSBK (12-octet block with `octet 0 = LB | P | Opcode[5:0]`)
- A PDU header block (12-octet block with `octet 0 = 0 | A/N | I/O | Format[4:0]`)

Both BAAA-B §5 (PDU header formats) and AABB-B §4 (TSBK framing) describe these
layouts independently. Neither spec provides an explicit rule for how a receiver
holding an arbitrary decoded 12-octet block under DUID `0xC` decides which layout
applies.

In practice implementers use one of two strategies:

- **Strategy A: dispatch on DUID first.** DUID `0x7` (AABB-B convention) →
  TSBK; DUID `0xC` → header block. This works for systems that use the
  AABB-B DUID assignment but fails on systems that send everything under
  `0xC`.
- **Strategy B: payload-shape heuristic under DUID `0xC`.** Inspect octet 0 bit 7
  (`LB` for TSBK, forced `0` for header blocks) and/or octets 10-11 (TSBK has
  CRC-16 there; header block has CRC-16 there too, so this doesn't disambiguate).

## 2. The ambiguity

Under DUID `0xC` with Strategy B, the only reliable structural cue is that
**BAAA-B header blocks require octet 0 bit 7 = `0`**, while TSBKs have LB as a
free bit in that position. If LB = 1 (which is the common case for single-TSBK
OSPs and all ISPs), the block is unambiguously a TSBK because a header block
couldn't have that bit set. If LB = 0 (multi-TSBK OSPs, blocks other than the
last), the high-bit-zero pattern matches both a middle-of-OSP TSBK *and* a valid
header block.

**Deeper disambiguator: octet 1.** In TSBKs, octet 1 is the 8-bit MFID. In header
blocks, octet 1 is `1 | 1 | SAP[5:0]` — the top two bits are fixed to `11`.

- MFID = `0x00` or `0x01` → top two bits `00` → NOT a valid header octet 1
- MFID with top two bits `11` → `0xC0`..`0xFF` → ambiguous with header

Looking at real-world MFID assignments in TIA-102.BAAC-D: all standard MFIDs
(`0x00`, `0x01`, `0x10`, `0x58`, `0x68`, `0x90`, `0xA4`, `0xB5`, `0xE5`) have
top two bits ≠ `11`. So in practice, examining octet 1's top two bits gives a
clean discriminator for most blocks. But the spec doesn't actually guarantee
that no future MFID will land in `0xC0..0xFF` range — that range is simply not
yet assigned.

## 3. Question

**Does the P25 standard require a receiver under DUID `0xC` to use one of the
following rules, and if so which one?**

1. Always prefer DUID-based dispatch: if DUID = `0x7`, TSBK; if DUID = `0xC`,
   header block only (never TSBK). Systems that violate this by sending TSBKs
   under DUID `0xC` are non-conformant.

2. Payload-shape rule: under DUID `0xC`, examine octet 1 top two bits. If `11`,
   it's a header block (Format dispatch applies); otherwise it's a TSBK
   (LB/P/Opcode dispatch applies). MFID values with top two bits = `11` are
   implicitly forbidden.

3. Context rule: control-channel receivers know from channel assignment whether
   to expect TSBKs vs. data; traffic-channel receivers know from the grant
   message whether the channel carries data or signaling. Structural
   discrimination is unnecessary.

4. None of the above — the standard is silent and this is an unwritten
   convention.

## 4. Diagnostic evidence

Both SDRTrunk (`P25P1MessageFramer.java`) and OP25 (`rx_tsbk.cc` / `rx_pdu.cc`)
use Strategy A: they dispatch strictly on DUID. Neither reference implementation
encounters the ambiguity in practice because the systems they decode obey
Strategy 1 above. This suggests that in real deployments, Strategy 1 is the
operating assumption, but it's not written down anywhere normative.

Our consolidated analysis note `analysis/fdma_pdu_frame.md` §6 punts on this
question with a `looks_like_tsbk(block)` helper whose definition is left
informal.

## 5. What the implementer needs

Either:
- Cite the normative source (likely AABB-B §4 or §3) that restricts DUID `0xC`
  to header-block dispatch and DUID `0x7` to TSBK dispatch, and we update
  BAAA-B §5.7 to state this explicitly, OR
- Confirm this is unwritten convention, in which case the implementer carries a
  permanent "assume DUID = `0x7` for TSBK" comment and we document Strategy 1
  as operational practice in `analysis/fdma_pdu_frame.md`.

## 6. Chip probe plausibility

Not applicable — wire-layer question, answerable from TIA specs.
