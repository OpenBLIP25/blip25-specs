# Motorola SNDCP N-PDU Preamble: Observations and Open Questions

**Scope:** Bytes that appear *between* the standard BAEB-C 2-byte SNDCP
header and the IPv4 header `0x45` on live Motorola P25 Unconfirmed Data
traffic. Consolidates what TIA-102.BAEB-C and BAED-A actually specify vs.
what's empirically on the air, and documents the heuristic a passive
decoder can use today.

**Not a TIA spec amendment.** The bytes documented here are either (a) a
non-standard interpretation of BAEB-C's compression fields or (b) a
vendor-proprietary wrapper. Either way the TIA-102 specs don't describe
them; this note records what's observed without inventing new wire-level
semantics.

**Related gap reports:**
- `gap_reports/0013_motorola_sndcp_npdu_header_variants.md` (source).

---

## 1. What BAEB-C Actually Says

The standard SN-Data / SN-UData PDU is a 2-octet header followed by the
network PDU (Figure 26 / Figure 27 / Figure 28 of TIA-102.BAEB-C):

```
Octet 0: [ PDU Type  (4b, MSB nibble) ][ NSAPI  (4b, LSB nibble) ]
Octet 1: [ PCOMP     (4b, MSB nibble) ][ DCOMP  (4b, LSB nibble) ]
Octet 2+: Network PDU (variable, typically IPv4 datagram starting 0x45)
```

Field semantics (from BAEB-C §6.1 and §6.4):

| Field | Legal values | Meaning |
|-------|--------------|---------|
| PDU Type | 4 = SN-Data (Confirmed), 5 = SN-UData (Unconfirmed), 0–3, 6 = context mgmt | Selects the SNDCP PDU variant. |
| NSAPI | 1–14 valid user contexts, 0 reserved for control, 15 reserved | Multiplexes up to 14 concurrent IP contexts per SU. |
| PCOMP | 0 = no header compression; 1–2 = RFC 1144 (VJ TCP/IP); 3–6 = RFC 2507 (V3 only) | Header compression scheme for the network PDU. **Variable-length compressed header precedes the decompressed IP datagram when PCOMP ≠ 0.** |
| DCOMP | 0 = no payload compression; 1–15 reserved for future use | Payload compression (no algorithm currently assigned by TIA). |

Two details matter for the preamble question:

1. **`Data Header Offset = %010`** per BAEB-C §6.4. This is set in the PDU
   *header* block (not the SNDCP header) and tells the receiver that user
   payload begins 2 octets into the first data block — i.e., after the
   2-byte SNDCP header. This is the normative indicator of where IP starts
   *when no header compression is active*.
2. **PCOMP = 6** = COMPRESSED_NON_TCP per RFC 2507, available only on V3
   SNDCP contexts. When active, the "network PDU" starts with an RFC 2507
   compressed-header frame of variable length, not with raw IPv4.

---

## 2. What's on the Wire (Motorola GMRS Capture)

From live Unconfirmed Data PDUs to Motorola dispatch LLIDs, the first 9
octets of the first data block look like:

```
Octet 0 1 | 2  3  4  5  6  7  8 | 9 ...
--------- | ------------------- | ------
40 06     | 40 00 17 xx xx xx xx | ...IPv4 datagram continues...
```

- Octet 0 = `0x40` → by Figure 26, PDU Type = `%0100` (4, SN-Data),
  NSAPI = `%0000` (0). **NSAPI 0 is reserved for control signaling per
  Table 12**, yet here it's on a data flow. First non-standard observation.
- Octet 1 = `0x06` → by Figure 26, PCOMP = `%0000` (0, no header compression),
  DCOMP = `%0110` (6, **Reserved per Table 15**). Second non-standard
  observation — DCOMP = 6 has no TIA-assigned meaning.
- Octets 2–8 = 7 bytes with no defined BAEB-C field at that offset. The
  IPv4 datagram (starting at `0x45`) is observed further into the block in
  some copies.

Three classes of explanation, ordered by standards-plausibility:

### 2.1 Explanation A — Nibble order swapped on this vendor/configuration

If the octet-1 high and low nibbles carry the opposite fields from
Figure 26 (i.e., DCOMP high, PCOMP low on this flow), then `0x06` decodes
to DCOMP=0 (no payload compression, standards-normative) and PCOMP=6
(COMPRESSED_NON_TCP per RFC 2507). The 7 mystery bytes then plausibly are
the RFC 2507 compressed-header frame for a non-TCP IP datagram, and the
IPv4 `0x45` that appears further downstream is the decompressed header
reconstituted after SNDCP strips the compression. BAEB-C V3 supports RFC
2507; an older Motorola V2 stack that back-ported the feature could have
landed on a different nibble convention.

This explanation is standards-adjacent (uses defined PCOMP codepoints) but
requires accepting that this vendor's nibble ordering on octet 1 disagrees
with Figure 26 of BAEB-C Rev C.

### 2.2 Explanation B — Motorola pre-standard SNDCP variant

Motorola P25 data pre-dates SNDCP V3 (published in BAEB-C). A legacy
Motorola data stack could have defined octet 1 as something unrelated to
PCOMP/DCOMP, later overlapping the TIA-specified field at the byte level
but not at the field level. In this reading, `0x06` is a Motorola-assigned
sub-type or wrapper indicator and the 7 trailing bytes are a proprietary
header (session ID, length, context reference, etc.). No TIA spec tells us
the layout; reverse engineering a canonical deployment would be required.

### 2.3 Explanation C — Standard SNDCP with undefined DCOMP

If we take Figure 26 at face value, PCOMP = 0 and DCOMP = 6 is emitted
despite DCOMP = 6 being reserved. A receiver strictly following BAEB-C
would flag DCOMP = 6 and discard the PDU. Motorola emitting this value and
receivers accepting it would imply a de-facto extension that TIA hasn't
caught up with. This is the least satisfying explanation because it
doesn't explain the 7 bytes between SNDCP and IPv4.

**None of A / B / C is definitively correct from BAEB-C full text alone.**
Either an MFID-$01 equipment spec, a Motorola proprietary document, or
packet captures with the preceding XID / SN-Context Activation exchange
would resolve the ambiguity.

---

## 3. Where the LLC Fields Actually Live (Not in the Preamble)

To head off one common mis-analysis: BAED-A LLC fields (N(S), FSNF, Data
Header Offset) are carried in the **PDU header block**, not in the first
data block's preamble. Per BAED-A §2.1 and the Confirmed/Unconfirmed PDU
header figures in BAAA-B (reproduced in `fdma_pdu_frame.md` §4.3 / §4.4):

- **N(S)** — Confirmed header octet 8 bits 6:4 (3 bits, mod 8)
- **FSNF** — Confirmed header octet 8 bits 3:0 (4 bits, LIC:1 + FSN:3)
- **Data Header Offset** — both headers, octet 9 bits 5:0 (6 bits)

The N(S), FSNF, and Data Header Offset are extracted from the Header
CRC-16-validated header block and do **not** consume payload bytes from
the first data block. The `Data Header Offset = 2` indication from BAEB-C
§6.4 reaches the receiver via that header field, so it's already known
when the decoder starts parsing the first data block's SNDCP header.

What this means for the preamble question: the mystery 7 bytes after
`40 06` are *not* misplaced LLC fields. They're at positions 2–8 of the
first data block, after the SNDCP header and before the network PDU, in a
region the standard leaves solely to the PCOMP-indicated compression
scheme (if any).

---

## 4. Current Heuristic in blip25

Without a definitive answer, a passive decoder can safely implement:

```
FUNCTION locate_network_pdu_start(data_block_0: bytes):
    # Standard fast path: no compression.
    IF data_block_0[1] & 0xF0 == 0x00:          # PCOMP = 0 under Figure 26 reading
        return 2                                # IP datagram starts at octet 2

    # Heuristic fallback: scan for plausible IPv4 start in the first 32 octets.
    FOR offset IN 2..32:
        IF data_block_0[offset] & 0xF0 == 0x40:  # IPv4 version nibble = 4
            ihl = data_block_0[offset] & 0x0F
            IF 5 <= ihl <= 15:
                total_len = u16_be(data_block_0[offset+2 : offset+4])
                IF total_len <= 510 AND total_len >= 20:   # Unconfirmed MTU = 510
                    return offset

    return NotFound
```

Failure modes:

- **Non-IP payloads** (pure PPP, IPX, compressed headers that don't
  decompress inline) break the `0x45` scan silently. For the GMRS CAD flows
  currently in scope this doesn't happen, but a decoder that graduates to
  Trunked PDCH carrying arbitrary SNDCP V3 traffic will need an actual RFC
  2507 decompressor.
- **Coincidental `0x4X`** bytes inside whatever precedes IPv4 could
  trip the scan. The `IHL ∈ [5, 15]` and `total_len ∈ [20, 510]` sanity
  checks bound the false-positive rate; in a full month of Sachse capture
  no false matches have been observed, but the check is statistical not
  deterministic.

This heuristic sidesteps the PCOMP / DCOMP ambiguity at the cost of not
extracting the compression-context state. For passive monitoring of
plain-IP payloads it's sufficient; for deployments where compression is
actually active, the heuristic would silently mis-align payload bytes and
the decoder would need to observe the preceding SN-Context Activation
exchange to learn what PCOMP scheme is in effect per NSAPI.

---

## 5. SDRTrunk Reference

SDRTrunk's `module.decode.p25.phase1.message.pdu.UnconfirmedDataPacket` →
`IPPacket` dispatch is the open-source de-facto reference for parsing
these frames; it reproduces the preamble bytes without interpreting them
and then finds the IPv4 start. Cross-referencing SDRTrunk's specific
offsets and checks would be a reasonable next step for a decoder that
wants to tighten the heuristic, but SDRTrunk has the same underlying
uncertainty as blip25 — it can decode what's on the wire without knowing
whether the bytes mean what BAEB-C §6.4.1 says or something else.

Per `analysis/oss_implementations_lessons_learned.md` the rule for
SDRTrunk is: trust it for things the spec makes testable (CRCs, trellis
decoding, Viterbi); treat it as secondary on things the spec is silent
about (like this preamble). The preamble belongs in the second category.

---

## 6. Open Questions

Recorded here so a future investigator doesn't start from zero:

1. **Which of Explanations A / B / C is correct?** Requires vendor
   documentation (unlikely to be available), or observation of a complete
   SN-Context Activation exchange to learn the negotiated PCOMP/DCOMP
   setting for the NSAPI carrying the observed data PDU.
2. **Does NSAPI = 0 carry data on this flow despite Table 12 reserving it?**
   Worth confirming whether SDRTrunk actually parses NSAPI = 0 as control
   or treats it as an overridable case.
3. **Are the 7 trailing bytes fixed-length or variable?** Count copies with
   different payload sizes and measure the offset of the IPv4 `0x45`
   relative to the SNDCP header. A fixed preamble length argues for
   Explanation B (proprietary wrapper). A variable length correlated with
   header-compression state argues for Explanation A.
4. **Do other vendors (L3Harris, Kenwood, BK) emit the same preamble?**
   If yes, it's a de-facto cross-vendor extension worth documenting
   formally. If no, it's Motorola-specific and should stay filed under
   MFID = $01.

---

## 7. Cross-References

- `standards/TIA-102.BAEB-C/P25_IP_Data_Bearer_Implementation_Spec.md` §2,
  §6 — the SNDCP header layout and PCOMP / DCOMP dispatch table.
- `standards/TIA-102.BAED-A/P25_Packet_Data_LLC_Implementation_Spec.md` §2
  — LLC header fields that are *not* in the data-block preamble.
- `analysis/fdma_pdu_frame.md` §4.3 — Unconfirmed Data PDU wire layout,
  including where Data Header Offset lives.
- `analysis/oss_implementations_lessons_learned.md` — policy for when to
  trust SDRTrunk as a secondary reference.
