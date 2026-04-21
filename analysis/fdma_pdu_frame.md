# FDMA PDU Frame: Unified View Across BAAA-B, AABB-B, BAED-A, and BAEB-C

**Scope:** FDMA (Phase 1) Packet Data Unit — DUID `0xC`. Consolidates the wire-level
frame layout (BAAA-B), trunking-specific header variants (AABB-B), LLC header fields
and payload routing (BAED-A), and SNDCP IP encapsulation (BAEB-C) into a single
implementer's reference. This is the frame type that carries non-voice payloads
bigger than a single block: multi-block trunking (MBT), confirmed/unconfirmed
user data, Enhanced Addressing, and Response packets. Single-block TSBK traffic
uses its own DUID `0x7` (TSDU) and is out of scope here — see §3 for the split
between `0x7` and `0xC` and BAAA-B Implementation Spec §5.6 for TSDU framing.

**Why this note exists:** The PDU frame's definition is spread across four TIA
documents. A single 5-bit Format field in octet 0 of the header block selects the
subtype, but that field is only documented comprehensively if you read all four specs
together. One Format value (`0x15`) is shared between Unconfirmed Data and Standard
MBT, which is a common source of confusion. This note captures the dispatch logic
in one place with worked byte-level examples.

**Authoritative references:**
- TIA-102.BAAA-B §5 — PDU frame skeleton; Format field definitions for `0x03`, `0x15`,
  `0x16`; Header CRC-16, CRC-9, Packet CRC-32 polynomials (§6)
- TIA-102.AABB-B §5 — MBT Standard (`0x15` with trunking SAP) and AMBT (`0x17`)
- TIA-102.BAED-A §2 — LLC header field semantics, SAP routing, fragmentation
- TIA-102.BAEB-C §2 — SNDCP PDU types carried as data PDU payloads

---

## 1. One Frame, Six Payload Families

Every FDMA PDU on the air starts with the same 112-bit preamble (FS + NID, DUID=0xC)
followed by one or more 196-bit trellis-coded blocks:

```
┌─────────┬──────────┬──────────────┬──────────────┬─────┬──────────────┐
│ FS (48) │ NID (64) │ Header Block │ Data Block 1 │ ... │ Null + SS    │
│         │ DUID=0xC │   (196 bits) │  (196 bits)  │     │              │
└─────────┴──────────┴──────────────┴──────────────┴─────┴──────────────┘
```

Under DUID `0xC`, the first trellis-coded block is *always* a PDU header block
(never a TSBK — TSBKs are carried under their own DUID `0x7`; see §3). The
receiver decodes the header block and dispatches by the Format field and SAP
ID to one of five payload families:

| # | Family | First-block selector | Carries | Spec |
|---|--------|----------------------|---------|------|
| 1 | MBT Standard | Format `0x15` + SAP `0x3D`/`0x3F` | Multi-block trunking messages | AABB-B §5 |
| 2 | MBT Alternative (AMBT) | Format `0x17` + SAP `0x3D`/`0x3F` | Multi-block trunking messages with opcode in header | AABB-B §5 |
| 3 | Unconfirmed Data | Format `0x15` + SAP ≠ trunking | Packet data (SNDCP), OTAR, fire-and-forget user data | BAED-A, BAEB-C |
| 4 | Confirmed Data | Format `0x16` | ARQ'd packet data, registration, key mgmt | BAED-A |
| 5 | Response | Format `0x03` | ACK / NACK / SACK for confirmed packets | BAAA-B §5.5 |

Single-block TSDU/TSBK traffic is a sixth payload family conceptually but uses
DUID `0x7` on the wire, not DUID `0xC`. It therefore never enters this
dispatch path — see §3 and BAAA-B §5.6.

Enhanced Addressing (Direct/Repeated configurations) is not a seventh family — it's
a variant of families 4 and 5 where SAP = `0x1F` in the header and the first data
block carries a Second Header (source LLID + real SAP). See §5 of this note.

---

## 2. The Format 0x15 Dual-Use Problem

Format `0x15` (`%10101`) is the single most ambiguous value in P25 FDMA framing.
BAAA-B defines it as the Unconfirmed Data Packet header. AABB-B defines it as the
Standard MBT header. Both are correct — because MBT Standard is, structurally,
an Unconfirmed Data Packet whose SAP field happens to be set to Trunking Control.

The practical consequence: you **cannot** route a received `0x15` header by Format
alone. You must also examine the SAP:

```
IF format == 0x15:
    SAP = header.octet1 & 0x3F
    IF SAP in {0x3D, 0x3F}:
        → dispatch to MBT Standard handler (AABB-B §5)
    ELSE:
        → dispatch to Unconfirmed Data handler (BAED-A §2.2, SAP routing §3)
```

All of the BAAA-B unconfirmed-data header fields (Blocks to Follow, Pad Octet Count,
Data Header Offset, Header CRC-CCITT-16) retain their positions and meanings in
MBT Standard. The only substantive difference is SAP routing: a `0x15` packet with
SAP = `0x3D` takes its payload to the trunking message dispatcher (AABC-E opcodes);
with any other SAP it takes its payload to the LLC user-data dispatcher.

AMBT (Format `0x17`) was added specifically to break this ambiguity for trunking
messages that benefit from having their opcode visible in the header (octet 7)
rather than buried in the first data block. AMBT is still an unconfirmed-delivery
packet with rate-½ data blocks; it just uses a distinct Format value so a receiver
can dispatch without examining SAP.

---

## 3. DUID `0x7` vs. `0xC`: Normative Split

BAAA-B §7.5.1 (Table 18) defines only six DUID values and leaves the remaining
ten "reserved for use in trunking or other systems." Two of those reserved
values are normatively assigned by **TIA-102.AABB-B §4.2**:

- DUID `$7` — **single-block format (TSBK).** Used for OSP/ISP TSBKs, 1–3 per
  TSDU, with no PDU header block. The 196-bit trellis-coded blocks following
  the NID are TSBKs directly.
- DUID `$C` — **multi-block format (PDU).** Used for every payload family in
  §1: MBT Standard, AMBT, Unconfirmed Data, Confirmed Data, and Response. The
  first block after the NID is always a PDU header block.

The two framings are fully disambiguated by DUID alone; no payload-shape
inspection is needed. A conformant decoder dispatches:

1. Decode NID, extract DUID.
2. If DUID = `0x0`, `0x3`, `0x5`, `0xA`, `0xF` → voice-frame dispatch.
3. If DUID = `0x7` → TSDU: decode the next 196-bit trellis-coded block as a
   TSBK (octet 0 = `LB/P/Opcode`), CRC-CCITT-16 verify, dispatch via AABC-E.
   Loop for up to 3 TSBKs until `LB = 1`.
4. If DUID = `0xC` → read the next 196-bit trellis-coded block as a PDU
   header block (octet 0 = `0/AN/IO/Format[4:0]`), Header-CRC-CCITT-16
   verify, then dispatch by Format/SAP as in §1.
5. Any other DUID → log and drop (reserved; AABB-B may assign more in the
   future, but no other assignments are known as of BAAA-B/AABB-B).

**Open-source cross-refs:** SDRTrunk (`P25P1DataUnitID.java`) and OP25 both
implement this two-DUID split. Neither treats DUID `0xC` as a TSBK carrier.

Gap report 0003 (`gap_reports/0003_duid_0x7_tsbk_reserved_but_universal.md`)
asked why `0x7` was "reserved" in BAAA-B Table 18 but universal on the air.
The answer: BAAA-B intentionally leaves trunking DUID assignments to the
trunking spec (AABB-B §4.2). BAAA-B's "reserved" language is the hook for
that delegation, not a prohibition.

---

## 4. Byte-Level Examples

### 4.1 TSBK inside a TSDU (Group Voice Channel Grant)

This is a control-channel channel-grant, the most common FDMA PDU in active
systems. Single-TSBK OSP at DUID `0x7`.

```
[FS: 48 bits]     0x5575F5FF77FF
[NID: 64 bits]    NAC=0x293, DUID=0x7, BCH parity...
[TSBK block: 12 octets pre-trellis, 196 bits post-trellis]

Octet:   0     1     2  3  4  5  6  7  8  9  10 11
Value:  0x80  0x00  TG_CH_T | TG_CH_R | GROUP_ADDRESS | TARGET_SUID | CRC-16

Octet 0 = 0x80 = 1000 0000  →  LB=1 (single TSBK OSP), P=0, Opcode=0x00 (GRP_V_CH_GRANT)
Octet 1 = 0x00  →  MFID = standard P25
Octets 2-9: opcode-specific arguments (per AABC-E)
Octets 10-11: CRC-CCITT-16 over octets 0-9
```

**Dispatch path:** FS → NID (DUID=0x7) → single TSBK → trellis-decode → CRC
validate → dispatch on opcode `0x00` to `GrpVChGrant` handler. No Format field
involved because TSBKs don't have one; the LB/P/Opcode layout of octet 0 is what
identifies the payload as a TSBK.

### 4.2 Standard MBT (Extended Channel Grant)

Multi-block trunking message with full target/channel info that doesn't fit in a
single TSBK. DUID `0xC`, Format `0x15`, SAP = Trunking Control.

```
[FS][NID DUID=0xC][Header block][Data block 1][Data block 2]

Header (pre-trellis, 12 octets):
Octet:  0     1     2     3  4  5  6     7      8    9    10 11
Value: 0x55  0xFD  0x00  LLID(24)  0x82  0x00  0x00 0x00  CRC-16

Octet 0 = 0x55 = 0101 0101  →  0, A/N=1, I/O=0, Format=%10101 (0x15)
Octet 1 = 0xFD = 1111 1101  →  top two bits '11', SAP = %111101 = 0x3D (Trunking Control)
                              → SAP=0x3D confirms MBT Standard, not Unconfirmed Data
Octet 6 = 0x82 = 1000 0010  →  high bit '1' (always), Blocks to Follow = 2
Octets 7-9: pad count, reserved, data header offset
Octets 10-11: Header CRC-CCITT-16

Data block 1 (12 octets): first chunk of message content (AABC-E extended-grant fields)
Data block 2 (12 octets): octets 0-7 = remaining message content,
                          octets 8-11 = Packet CRC-32 over all data block content
```

**Dispatch path:** FS → NID (DUID=0xC) → header block trellis-decode → header
CRC-CCITT → Format=0x15 + SAP=0x3D → MBT Standard handler → read 2 data blocks →
packet CRC-32 → route to AABC-E opcode dispatcher (opcode is encoded in the data
blocks for Standard MBT).

### 4.3 Unconfirmed Data PDU (SNDCP IP Datagram)

Same Format value (`0x15`), different SAP, different routing.

```
[FS][NID DUID=0xC][Header block][Data blocks × BTF]

Header:
Octet 0 = 0x55  →  Format=0x15 (same wire pattern as Standard MBT)
Octet 1 = 0x04 or 0xC0..0xC3 →  SAP = Packet Data (0x04) or Unencrypted User Data (0x00)
                                → SAP ≠ trunking → Unconfirmed Data Packet
Octet 6 = 0x85  →  high bit '1', BTF = 5 (5 data blocks follow)

Data blocks: rate-½ trellis, 12 octets each
Payload: SNDCP header (2 bytes: PDU Type + NSAPI + PCOMP + DCOMP) || IP datagram
Last 4 octets of final data block: Packet CRC-32 (over all user data + pad)
```

**Dispatch path:** FS → NID → header decode → Format=0x15 + SAP=0x04 → Unconfirmed
Data handler → read BTF data blocks → packet CRC-32 → deliver to SNDCP. BAEB-C
handles the 2-byte SNDCP header and upper-layer delivery.

### 4.4 Confirmed Data PDU (ARQ'd Data)

Format `0x16`, rate-¾ data blocks with per-block serial number + CRC-9.

```
[FS][NID DUID=0xC][Header block][Data blocks × BTF, 18 octets each]

Header:
Octet 0 = 0x56 = 0101 0110  →  A/N=1, I/O=0, Format=%10110 (0x16)
Octet 6: FMF + BTF
Octet 8: Syn(1) + N(S)(3) + FSNF(4)
Octet 9: Data Header Offset

Each data block (18 octets pre-trellis, 144 bits input to rate-¾ trellis → 196 bits):
  Octet 0 bits 7..1 = Serial Number (7 bits)
  Octet 0 bit 0 + Octet 1 = CRC-9 over (serial number || 16 octets of user data)
  Octets 2-17 = 16 octets of user data
  Last data block: last 4 user octets replaced by Packet CRC-32
```

**Dispatch path:** FS → NID → header decode → Format=0x16 → Confirmed Data handler →
read BTF rate-¾ blocks → validate per-block CRC-9 → validate Packet CRC-32 → if
any block CRC-9 fails, issue Response with SACK bitmap; else ACK.

### 4.5 Response Packet (SACK)

Single header block, no data blocks for ACK/NACK; 1-2 data blocks carrying selective
retry flags for SACK.

```
[FS][NID DUID=0xC][Header block][optional SACK data blocks]

Header:
Octet 0 = 0x03 or 0x23  →  I/O bit + Format=%00011
Octet 1 bits 7:6 = Class (2 bits)
Octet 1 bits 5:3 = Type (3 bits)
Octet 1 bits 2:0 = Status (3 bits; typically N(R) of the packet being acknowledged)
Octet 6 = X bit + Blocks to Follow

For SACK (Class=%10, Type=%000): BTF = 1 or 2 data blocks
Each SACK data block: 64 flag bits (f0..f63) + Packet CRC-32 in last 4 octets of last block
  fN=1 → block N received OK; fN=0 → retry block N; unused flags set to 1
```

**Dispatch path:** FS → NID → header decode → Format=0x03 → Response handler →
extract Class/Type/Status → if SACK, read BTF data blocks and apply selective
retry bitmap to the sender's packet buffer.

---

## 5. Enhanced Addressing (Direct / Repeated Data)

Symmetric addressing requires both source and destination LLIDs on every packet.
Since the PDU header block has only 24 bits of LLID space, Enhanced Addressing
uses a two-header scheme: the normal header carries the destination LLID, and the
first data block carries a Second Header with the source LLID and the real SAP.

**Trigger:** SAP field in the header block set to the EA SAP sentinel (`0x1F`).

```
HEADER BLOCK               DATA BLOCK 1                 DATA BLOCK 2..N
┌──────────────────┐      ┌──────────────────────────┐ ┌──────────────────┐
│ Format = 0x15    │      │ Second Header:           │ │ User Data        │
│ SAP     = 0x1F   │ ───► │   Source LLID (24b)      │ │ ...              │
│ Dest LLID (24b)  │      │   SAP = <real SAP>       │ │                  │
│ Header CRC-16    │      │   Header CRC 2           │ │                  │
└──────────────────┘      └──────────────────────────┘ └──────────────────┘
(rate-½ for Unconfirmed; rate-¾ for Confirmed)
```

For the Confirmed variant, the Second Header also gets a serial number + CRC-9
like any other confirmed data block, and the "real SAP" is in the second header's
body. See BAAA-B §5.7.6 and BAED-A §2.4.

**Dispatch:**

```
IF header.format in {0x15, 0x16} AND header.sap == 0x1F:
    → read first data block
    → parse Second Header: source LLID, real SAP
    → treat subsequent data blocks as payload under the real SAP
    → route using real SAP (not 0x1F)
```

Enhanced Addressing is mostly relevant for Direct Data (SU-to-SU without
infrastructure) and Repeated Data (SU-to-SU via a simple repeater). Conventional
and Trunked FNE configurations use asymmetric addressing (single LLID, direction
implicit in I/O bit) and therefore don't need EA.

---

## 6. Dispatcher Pseudocode

```
FUNCTION dispatch_fdma_pdu(nid: NID, first_block_bits: [u8; 12]):

    IF nid.duid == 0x07:
        // TSDU (single-block trunking) -- per AABB-B §4.2
        return parse_as_tsbk_sequence(first_block_bits)

    IF nid.duid != 0x0C:
        return Error(UnexpectedDuid)

    // DUID = 0xC: multi-block PDU -- MBT, data, or response.
    // First block is ALWAYS a PDU header block (never a TSBK).
    octet0 = first_block_bits[0]
    octet1 = first_block_bits[1]

    IF (octet0 & 0x80) != 0:
        return Error(ReservedHighBit)

    an     = (octet0 >> 6) & 1
    io     = (octet0 >> 5) & 1
    format = octet0 & 0x1F
    sap    = octet1 & 0x3F

    MATCH format:
        0x03 → parse_as_response(first_block_bits)                    // BAAA-B §5.5

        0x15 → IF sap in {0x3D, 0x3F}:
                   parse_as_mbt_standard(first_block_bits)             // AABB-B §5
               ELIF sap == 0x1F:
                   parse_as_enhanced_addressing_unconfirmed(...)       // BAAA-B §5.7.6
               ELSE:
                   parse_as_unconfirmed_data(first_block_bits)         // BAED-A §2.2

        0x16 → IF sap == 0x1F:
                   parse_as_enhanced_addressing_confirmed(...)         // BAAA-B §5.7.6
               ELSE:
                   parse_as_confirmed_data(first_block_bits)           // BAED-A §2.3

        0x17 → parse_as_mbt_alternative(first_block_bits)              // AABB-B §5

        _    → Error(ReservedFormat)
```

---

## 7. CRC Summary (All Three Coexist in One Frame)

A single confirmed data PDU carries up to three distinct CRCs, at three different
layers of integrity checking:

| CRC | Polynomial | Init | Scope | Purpose |
|-----|-----------|------|-------|---------|
| Header CRC-16 | `G(x) = x^16 + x^12 + x^5 + 1` (CRC-CCITT) | 0xFFFF | Header block octets 0-9 | Validate header-block integrity before trusting BTF, LLID, SAP |
| Per-block CRC-9 | `G(x) = x^9 + x^6 + x^4 + x^3 + 1` | 0x1FF | 7-bit serial number + 16 data octets (confirmed only) | Identify *which* data blocks need retransmission for SACK |
| Packet CRC-32 | `G(x) = x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1` | 0xFFFFFFFF | All user-data octets across all data blocks + pad | End-to-end packet integrity |

All three use the "invert with I(x) = x^(n-1) + ... + 1" convention of BAAA-B §5.2,
§5.3.3, and §5.4.2. The bit-order convention is **MSB first**: for the header CRC,
MSB of octet 0 is the highest-degree coefficient of M(x); for the packet CRC, MSB
of the first data octet maps to x^(k−1). Numerical values and encoding examples
are in BAAA-B Implementation Spec §13.

Unconfirmed Data Packets and Response Packets omit the per-block CRC-9 entirely
(there's no serial number, no retry); they rely only on the Header CRC-16 plus the
Packet CRC-32.

---

## 8. Common Implementation Pitfalls

1. **Dispatching on Format alone.** Format `0x15` is ambiguous (Unconfirmed vs.
   Standard MBT). Always combine with SAP to disambiguate.

2. **Treating DUID `0xC` as a polymorphic TSBK/header container.** AABB-B §4.2
   is normative: DUID `0x7` always carries 1–3 TSBKs (TSDU); DUID `0xC` always
   carries a PDU header block followed by data blocks. A receiver should
   dispatch on DUID *before* inspecting block contents. Conversely, don't
   drop DUID `0x7` as "reserved" — BAAA-B Table 18 delegates trunking DUID
   assignments to the trunking spec (see §3).

3. **Computing CRCs in LSB-first order.** BAAA-B is MSB-first for all three PDU
   CRCs. The CCITT convention in consumer modem code (e.g., Kermit, XMODEM) is
   often LSB-first with reflected polynomials; that code will produce wrong values
   for P25 PDUs. Verify against a known-good test vector before shipping.

4. **Including header octets 10-11 in the header CRC computation.** The CRC covers
   octets 0-9 only. Octets 10-11 carry the CRC itself.

5. **Confusing the Packet CRC-32 polynomial with Ethernet/ZIP CRC-32.** P25 uses
   the same polynomial (0x04C11DB7), but the initial value and the bit-order
   convention differ. The CRC bytes appear in the last 4 octets of the last data
   block (Confirmed) or within the final 4 octets of the user-data area (Unconfirmed),
   laid out MSB-first relative to x^31..x^0.

6. **Treating rate-¾ blocks as rate-½ blocks.** Both are 196 bits on the wire, but
   the trellis decoder must be configured for the right rate (48 tribits in, 49
   dibits out for rate ¾; 48 dibits in, 49 dibits out for rate ½). Confirmed blocks
   are 18 octets of input; Unconfirmed are 12. Using the wrong rate produces garbage
   with a plausible-looking bit pattern.

7. **Applying the header's FMF bit to non-confirmed packets.** FMF (octet 6 bit 7)
   is specific to Confirmed Data. In Unconfirmed and MBT headers, bit 7 of octet 6
   is fixed to `1` and carries no selective-retry semantics.

8. **Losing encrypted-packet structural fields.** When a PDU is protected, the
   LLID and user-data octets are encrypted but the LB/P bits, MFID, SAP, BTF,
   pad count, and CRC-16 remain in clear so non-crypto SUs can still maintain
   frame sync and CRC integrity. See AABB-B §5.7 for the trunking-protected
   field map.

---

## 9. Relationship to TDMA

This note is FDMA-only. The Phase 2 TDMA equivalent (packet data on DCH, signaling
on LCCH via MAC PDUs) has a completely different framing layer — different burst
structure, different FEC stack (Reed-Solomon + Golay instead of trellis), different
block sizes. The upper layers (LLC per BAED-A, SNDCP per BAEB-C, AABC-E trunking
opcodes) are shared between FDMA and TDMA and sit above both framing layers.

See TIA-102.BBAC-A Implementation Spec for the TDMA MAC equivalent.
