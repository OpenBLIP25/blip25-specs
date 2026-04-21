# 0007 — Pad octet distribution when pad count exceeds one data block's capacity

**Status:** drafted (2026-04-21)
**Filed:** 2026-04-21
**Filer:** spec-author agent
**For:** spec-author agent (self-filed during PDU framing consolidation)
**Resolution:** BAED-A §5.4 caps pad at `P_MAXC = 15` (Confirmed) and
`P_MAXU = 11` (Unconfirmed), keeping pad within the last two data blocks.
Strategy 1 (greedy-fill from the end) documented in BAAA-B §5.7.3 and §5.7.4
with worked examples. Pad octet value is conventionally `0x00`; not
explicitly specified by the standard.
**Related:**
- `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md` §5.7.3 (Unconfirmed Data), §5.7.4 (Confirmed Data)
- BAAA-B Full Text §5.4.3 "Confirmed Data Packet Last Data Block Format"
- `standards/TIA-102.BAED-A/P25_Packet_Data_LLC_Implementation_Spec.md` §4 (Fragmentation)

---

## 1. What the spec says

BAAA-B §5.4.3 (full text) states:

> Up to 12 pad octets may be included in the last data block. If more than 12
> pad octets are needed, additional pad octets are included in the second-to-last
> data block. The last four octets of the last data block consist of the packet
> CRC.

The Pad Octet Count field in the header is 5 bits wide (octet 7 bits 4:0),
giving a range of 0..31 pad octets. Unconfirmed data blocks are 12 octets of
user data each; Confirmed data blocks are 16 octets of user data each.

## 2. The ambiguity

"Additional pad octets are included in the second-to-last data block" leaves
three things implicit:

1. **Distribution rule.** If pad count is 28, does the last block get 12 pads
   (max it can hold) and the second-to-last get 16 pads? Or is it split more
   evenly? Or does the second-to-last get (pad_count − 12) regardless of its
   own user-data capacity?

2. **Pad octet values.** The spec doesn't specify the byte value used for
   padding. Real-world traffic uses `0x00`, but some implementations use `0xFF`
   or random octets. Is this normative or implementation-defined?

3. **Maximum reach.** If pad count > 12 + (block capacity), does the pad
   cascade into the third-to-last block? The 5-bit Pad Octet Count field
   allows up to 31; for Unconfirmed (12-octet blocks) that means pad can span
   at most `⌈31/12⌉ = 3` blocks. For Confirmed (16-octet blocks), at most
   `⌈31/16⌉ = 2` blocks. The spec text ("second-to-last") implies at most 2
   blocks are affected, which is consistent with Confirmed but under-specifies
   Unconfirmed.

## 3. Question

**For a Data Packet with Pad Octet Count > 12 (Unconfirmed) or > 4
(Confirmed — since the Packet CRC-32 consumes 4 octets of the last block),
what is the exact layout of pad octets across the final N data blocks?**

Candidate answers:

1. **Greedy fill:** Last block is filled with as many pad octets as it can
   hold (12 for Unconfirmed, since CRC-32 takes 4 of 16; up to 12 for
   Confirmed because the last block ends with 4 octets of CRC-32). Remaining
   pad cascades into second-to-last block (and earlier if needed). User data
   occupies the remaining positions in affected blocks, starting from the end
   working backward. Block Serial Number field in Confirmed mode still
   increments normally.

2. **Spread evenly:** Pad is distributed across all data blocks evenly.
   Inconsistent with the "second-to-last" language in §5.4.3.

3. **Last two blocks only:** Up to 12 pad in the last block, up to
   (pad_count − 12) in the second-to-last block, none elsewhere. For
   Unconfirmed with pad count > 24, this is impossible; in which case the
   sender must emit an extra data block of all-pad (pushing BTF up by 1).

## 4. Diagnostic evidence

SDRTrunk's Confirmed Data decoder in
`module/decode/p25/phase1/message/pdu/pdu/PDUMessageFactory.java` appears to
use Strategy 1 (greedy fill from the end), but the code is not well-commented
and this interpretation may be wrong. OP25's PDU decoder hasn't been
cross-checked for this question.

Real-world impact: most P25 packet data traffic is small enough that pad
count stays ≤ 12, so this edge case rarely manifests on-air.

## 5. What the implementer needs

A worked example in BAAA-B §5.7.3 or §5.7.4 showing pad octet placement for
a packet with pad_count = 20 (or similar > 12 case). Ideally a figure or a
short pseudocode showing which octets of which blocks carry pad vs. user data
vs. CRC.

If the standard is genuinely ambiguous, the resolution should be to document
Strategy 1 (greedy fill from the end) as a convention and add test vectors.

## 6. Chip probe plausibility

Not applicable — wire-layer question. Answerable by re-reading BAAA-B §5 or
(more likely) checking BAED-A's full text for an LLC transmit procedure that
specifies pad placement. If both are silent, `wont-fix` with a documented
convention is acceptable.
