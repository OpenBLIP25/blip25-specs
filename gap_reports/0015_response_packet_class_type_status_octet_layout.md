# 0015 — Response Packet class/type/status field byte positions

**Status:** drafted (2026-04-21) — opcode table delivered as `annex_tables/response_packet_opcodes.csv` with bit-field decomposition AND whole-byte values; `analysis/fdma_pdu_frame.md` §4.5 updated with cross-reference. Transcription error in the full-text extraction and implementation spec (Illegal Format was listed as Class=%00; correct per BAAA-B Revision B Table 10 is Class=%01 Type=%000 = octet 1 pattern `0x40 | status`) was corrected in the same pass.
**Filed:** 2026-04-21
**Filer:** implementer (p25-decoder / blip25-data)
**For:** spec-author
**Related:**
- `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md` §5.7.5 (Response Packet Header)
- `standards/TIA-102.BAED-A/P25_Packet_Data_LLC_Implementation_Spec.md` §2.5 (Response Packet Structure)
- `analysis/fdma_pdu_frame.md` §4.5 (Response / SACK example)

---

## 1. What the spec says

BAAA-B §5.7.5 defines the Response Packet header with three fields:

- **Class** (2 bits) — ACK / NACK / SACK / reserved
- **Type** (3 bits) — subtype within the class
- **Status** (3 bits) — in Class=ACK, carries N(R); in other classes,
  error reason

`fdma_pdu_frame.md` §4.5 draws this as:

```
Octet 1 bits 7:6 = Class (2 bits)
Octet 1 bits 5:3 = Type (3 bits)
Octet 1 bits 2:0 = Status (3 bits; typically N(R))
```

— all three fields packed into octet 1.

BAED-A §2.5 is a bit less explicit on the exact bit positions.

## 2. What's actually on the wire and in SDRTrunk's parser

SDRTrunk's `ResponseHeader.java` (`module/decode/p25/phase1/message/pdu/
response/...`) reads an 8-bit "response status byte" from bits 8..15 (octet 1
in BAAA-B terms). It treats well-known values like:

- `0x08` — ALL BLOCKS RECEIVED (ACK)
- `0x40` — ILLEGAL FORMAT
- `0x48` — PACKET CRC FAIL
- `0x50` — MEMORY FULL
- `0x80` — SELECTIVE RETRY (SACK)

as whole-byte opcodes, rather than decomposing into Class/Type/Status bit
fields.

If you decompose `0x08` per `fdma_pdu_frame.md` §4.5:
- Class = bits 7:6 = `00` (ACK)
- Type  = bits 5:3 = `001` (subtype 1)
- Status = bits 2:0 = `000` (N(R) = 0)

That's self-consistent with ACK semantics. But `0x80 = 10000000`:
- Class = `10` (SACK)
- Type  = `000` (subtype 0)
- Status = `000` (no N(R) needed for SACK-request)

Also consistent. And `0x48 = 01001000`:
- Class = `01` (NACK)
- Type  = `001` (subtype 1 — "packet CRC fail")
- Status = `000`

Consistent. So the bit decomposition in `fdma_pdu_frame.md` §4.5 matches
SDRTrunk's byte-values — they're just two ways of naming the same thing.

## 3. The actual gap

The gap is lower-stakes than the title suggests: the bit layout is
consistent between spec and implementations. What *is* missing:

1. **A canonical table of (Class, Type, Status) triples** and what they
   mean. §5.7.5 describes the fields exist; it doesn't enumerate which
   triples are valid and what each means.
2. **An opcode reference** like SDRTrunk has (`0x08` = "ALL BLOCKS
   RECEIVED", `0x50` = "MEMORY FULL"). Whether to present it as a whole-byte
   table or a (Class, Type, Status) cross-reference is a style choice, but
   right now an implementer has to grep SDRTrunk source to find out what
   `response_status = 0x50` actually means.
3. **Extended-byte layout for Class=SACK.** `fdma_pdu_frame.md` §4.5 says
   SACK is `Class=%10, Type=%000` but doesn't say what Status is used for
   (I've been treating it as unused / reserved). If SACK needs a block-count
   or flag-continuation field, it might live in Status.

## 4. Question

**Can the derived work provide a Response Packet opcode table listing every
defined `(Class, Type, Status)` triple with its meaning?** Ideally as a CSV
in `annex_tables/response_packet_opcodes.csv` so implementers can codegen
dispatch tables rather than hand-copying from SDRTrunk.

Source material is in BAAA-B §5.5 + full-text tables, plus SDRTrunk
`ResponseHeader.ResponseType`, `ResponseHeader.PduClass`, and related enums.

## 5. What we do today

My decoder (`crates/blip25-core/src/pdu.rs::parse_pdu_header` Response
case) stores the whole 8-bit response byte as `response_status`, plus
decomposed `response_class` (bits 7:6) and `response_type` (bits 5:3).
Downstream code that wants to interpret these reaches back to SDRTrunk's
enums ad-hoc. No enum in blip25-core yet — filed as pending because the
opcode table question was open.

## 6. Chip probe plausibility

Not applicable — derived-work table extraction.

## 7. Priority

Low-to-medium. The bit decomposition already works; the enumeration of
meanings is an ergonomics improvement. More important once the
`blip25-data` crate starts reasoning about Response Packets for
retransmit-aware analytics (e.g., "how often does this site NAK / MEMORY
FULL?" as a link-quality indicator).
