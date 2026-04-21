# 0006 — SACK selective-retry bitmap for packets with more than 64 blocks

**Status:** drafted (2026-04-21)
**Filed:** 2026-04-21
**Filer:** spec-author agent
**For:** spec-author agent (self-filed during PDU framing consolidation)
**Resolution:** Two-block SACK layout documented in BAAA-B §5.7.5. Block 1
carries `f0..f63` in octets 0–7 (no CRC). Block 2 carries `f64..f127` in octets
0–7, with Packet CRC-32 in octets 8–11. `f127` is always 1 because BTF field
(7-bit) caps packets at 127 blocks. Noted that capping SACK at 64 blocks per
Response (requiring multiple Response packets for long Confirmed packets) is
an acceptable simpler alternative used in some implementations.
**Related:**
- `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md` §5.7.5 (Response Packet, SACK)
- BAAA-B Full Text §5.5, Figure 22
- `standards/TIA-102.BAED-A/P25_Packet_Data_LLC_Implementation_Spec.md` §2.5 (Response Packet Structure)

---

## 1. What the spec says

BAAA-B §5.5 (full text) states:

> When blocks are to be selectively retried (Class=%10), subsequent blocks of
> additional information are appended to the header block. The format for
> selective retry data block flags allows up to 64 blocks per block (two blocks
> allow up to 127 blocks, which is the maximum in a packet).

Figure 22 (Response Packet Data Block) shows octets 0-7 carrying flags `f0..f63`
(8 octets × 8 bits = 64 flags), with octets 8-11 containing the Packet CRC-32.

The 7-bit Blocks-to-Follow (BTF) field on a Confirmed Data Packet allows up to
127 data blocks per packet (2^7 − 1 = 127; some implementations cap at 126 for
off-by-one reasons).

Implication: a Response Packet carrying SACK for a packet with more than 64
blocks must emit **two** SACK data blocks (BTF = 2 in the Response header).

## 2. The ambiguity

The spec does not explicitly show the second SACK data block layout. Open
questions:

1. **Flag placement in the second block.** Does it carry `f64..f127` in
   octets 0-7 (same layout as the first block, shifted by 64)? Or does it
   continue with a different packing? Figure 22 shows only one block template.

2. **Packet CRC-32 placement when two SACK blocks are sent.** Figure 22's
   Packet CRC-32 sits in octets 8-11 of "the" SACK data block — but if there
   are two such blocks, which block carries the CRC? By analogy to confirmed
   data, the CRC would go in the last (second) SACK data block only, and the
   first block would use all 12 octets for flags (`f0..f95`). That would give
   a total flag capacity of 96 + 64 = 160 flags, more than the 127 needed —
   but contradicts the spec's own "64 per block, two blocks → 127" statement.

3. **Alternative interpretation.** "Two blocks allow up to 127 blocks" may
   mean: first block uses octets 0-7 for `f0..f63` + octets 8-11 for padding;
   second block uses octets 0-7 for `f64..f126` + octets 8-11 for Packet CRC-32.
   This matches the "64 per block" claim literally but wastes the first
   block's octets 8-11.

None of BAAA-B, AABB-B, or BAED-A impl specs gives a Figure-22-equivalent for
the second SACK data block.

## 3. Question

**When a Response Packet issues SACK for a Confirmed Data Packet with more
than 64 data blocks, what is the exact layout of the second SACK data block?**

Specifically: where do flags `f64..fN` go, where does the Packet CRC-32 go,
and are flags `fN+1..f127` (beyond the covered packet's BTF) all set to 1 as
per Figure 22's "unused flag bits" rule?

## 4. Diagnostic evidence

Neither SDRTrunk (`module/decode/p25/phase1/message/pdu/response/...`) nor
OP25 (`rx_pdu.cc`) appears to implement a two-block SACK decoder; both treat
a single SACK data block as the common case. Real-world Confirmed Data
Packets with more than 64 blocks would be rare (at 16 octets/block × 64 blocks
= 1024 octets of user data, it exceeds most SNDCP MTU settings), so this
ambiguity may not have been tested on-air.

## 5. What the implementer needs

A concrete layout description for the second SACK data block, or confirmation
that P25 implementations are expected to cap selective-retry to 64 blocks per
response and issue multiple Response Packets for larger SACK requests. Either
answer is fine — we need to pick one and document it in BAAA-B §5.7.5 and
BAED-A §2.5.

## 6. Chip probe plausibility

Not applicable — wire-layer question. Answerable by re-reading BAAA-B §5.5
carefully (possibly in a newer revision if available) or by checking whether
BAED-A §5 ARQ receiver procedures constrain SACK packet size. If the standard
is silent, `wont-fix` with a recommendation (cap at 64 blocks, issue multiple
Response Packets) is acceptable.
