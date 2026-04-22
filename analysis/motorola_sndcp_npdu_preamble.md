# Motorola SNDCP N-PDU Preamble (Trunked IV&D): SNDCP Layering and Unexplained Pre-IP Bytes

**Scope:** Bytes observed *between* the BAEB-C reassembled-payload
boundary and the IPv4 header `0x45` on live Motorola P25 Unconfirmed
Data traffic. Explains what TIA-102 *does* specify, what it *does not*
specify, the heuristic blip25 uses today, and why the bytes remain
unexplained under the standard.

**Mode:** Trunked IV&D only. For conventional IV&D, the preamble is
not SNDCP at all — it's SCEP (Motorola proprietary) riding on BAEB-A.
See `analysis/motorola_conventional_scep_vs_trunked_sndcp.md` for the
mode split.

**Correction (2026-04-22).** An earlier version of this note (and
gap 0018's first resolution) asserted that the bytes surrounding the
SNDCP header are "LLC user-plane bytes per TIA-102.BAED-A." A full
re-read of BAED-A does **not** support that claim: BAED-A defines a
stop-and-wait ARQ, carries all LLC state variables (V(S), V(R),
N(S), N(R), FSNF, SYN) in the Header-CRC-16-protected PDU header
block (BAAA-B format), and chains **only** Auxiliary Header(s)
(AAAD-B, encryption only) or a Second Header (Enhanced Addressing,
Direct/Repeated Data only) into the logical message fragment. No
"LLC user-plane" wire-format bytes are defined. The observed bytes
are therefore **unexplained under TIA standards**; see §§3–4 and §8
for candidate explanations and what would resolve them.

**Related:**
- `analysis/motorola_conventional_scep_vs_trunked_sndcp.md` — the
  architectural companion to this note; explains why conventional
  captures look different.
- `gap_reports/0013_motorola_sndcp_npdu_header_variants.md` — resolved.
- `gap_reports/0018_trunked_ivd_llc_plus_sndcp_plus_rfc2507.md` —
  re-opened 2026-04-22; earlier "LLC user-plane" resolution
  withdrawn.
- `standards/TIA-102.BAED-A/P25_Packet_Data_LLC_Implementation_Spec.md`
  §13 — explicit statement of what BAED-A does and does not specify
  about the first data block.

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

1. **`Data Header Offset = %010`** per BAEB-C §6.4. Set in the PDU
   *header* block (not the SNDCP header), it tells the receiver that
   user payload begins 2 octets into the first data block — i.e.,
   after the 2-byte SNDCP header, when no header compression is active.
2. **PCOMP = 6** = COMPRESSED_NON_TCP per RFC 2507, available only on
   V3 SNDCP contexts. When active, the "network PDU" starts with an
   RFC 2507 compressed-header frame of variable length, not with raw
   IPv4.

---

## 2. The Layering on the Wire (Trunked IV&D, TIA-defined portion)

Motorola's `MN005155A01-A` Trunked Data Services Feature Guide
§2.1.3 (p. 30) quantifies the full-uncompressed preamble as 30 bytes
across IPv4 + UDP + SNDCP. That matches what TIA-102 specifies:

```
┌────────────────────────────────────────────────────────────┐
│  CAI PDU (TIA-102.BAEB-C blocks, reassembled payload)      │
│  • Logical message fragment after LLC block reassembly.    │
│  • Per BAEB-C §1.3 / §2.3 for SN-Data / SN-UData:          │
│    Data Header Offset = 2, so the fragment layout is       │
│    [SNDCP 2B][network PDU...] with no other TIA-defined    │
│    bytes in between.                                       │
│                                                            │
│  └── SNDCP header (2 bytes per BAEB-C §6.4)                │
│      • PDU Type + NSAPI + PCOMP + DCOMP                    │
│      └── Network PDU:                                      │
│          • If PCOMP = 0: raw IPv4 (20 B) + UDP (8 B) + app │
│          • If PCOMP ≠ 0 (V3 only): RFC 2507 compressed     │
│            header of variable length, then raw / compressed │
│            network-layer payload                            │
└────────────────────────────────────────────────────────────┘

Total pre-payload bytes, no compression: 30 B (IPv4 20 + UDP 8 +
SNDCP 2). TIA-102 does not define any additional bytes on the
payload side of the BAEB-C block-reassembly boundary for trunked
IV&D data PDUs.
```

**What the implementer observation actually is.** The `0x40 0x06 ...`
bytes seen at the start of some trunked IV&D reassembled payloads,
before the first `0x45` IPv4 version nibble, are **not** accounted
for by BAED-A, by BAEB-C, or by any other TIA document in this
working set. See §§3–4 and §8 for candidate explanations.

---

## 3. The `0x40 0x06 ...` bytes — status as of 2026-04-22

Two earlier framings have been withdrawn:

1. **Framing A (withdrawn 2026-04-21 after `blip25-data` feedback).**
   The bytes were originally decoded as the start of the SNDCP
   header (PDU Type / NSAPI in Octet 0, PCOMP/DCOMP or
   Reserved/DCOMP in Octet 1). This produced implausible values
   (NSAPI = 0, DCOMP = 6) and does not match clean SNDCP decoding at
   the expected offset.
2. **Framing B (withdrawn 2026-04-22).** The bytes were then
   reframed as "LLC user-plane bytes per TIA-102.BAED-A." A full
   re-read of BAED-A does not support that framing: BAED-A is a
   stop-and-wait ARQ whose LLC state (V(S), V(R), N(S), N(R), FSNF,
   SYN) lives in the Header-CRC-protected PDU header block, not on
   the payload side. The only payload-side items BAED-A §3.2.5 step
   2.2 chains in are Auxiliary Header(s) (AAAD-B, encryption only)
   and a Second Header (Enhanced Addressing, Direct/Repeated Data
   only — not trunked IV&D). See BAED-A impl spec §13 for the
   load-bearing statement.

**Heuristic still in use.** The scan-for-`0x45` in §6 below is
unchanged — it remains the fastest way to get passive-decoder
coverage on broadcast Motorola CAD traffic, where compression is
disallowed and a clean [SNDCP 2B][IPv4] fragment is emitted. At
`ip_offset - 2` the two bytes decode cleanly as a BAEB-C Figure 26
SNDCP header (PDU Type = 4/5, NSAPI in the user-context range,
PCOMP = 0, DCOMP = 0). What this heuristic does **not** do is
explain what the bytes ahead of `ip_offset - 2` are.

**Candidate explanations for the pre-`ip_offset-2` bytes** (none
concluded; each requires implementer-side experimentation):

| # | Hypothesis | How to test |
|---|------------|-------------|
| a | RFC 2507 compressed header on SNDCPv3 (PCOMP ≠ 0). The SNDCP header is at the BEGINNING of the fragment, not at `ip_offset - 2`; the "network PDU" slot holds a variable-length RFC 2507 compressed frame; the scan-for-`0x45` is landing on coincidental bytes inside the compressed or decompressed payload. | Decode the first two bytes as SNDCP. If PDU Type ∈ {4, 5}, NSAPI ∈ 1–14, PCOMP ∈ 3–6, this is V3 RFC 2507. Implement an RFC 2507 decompressor (SDRTrunk has one) or feed these bytes through a standalone RFC 2507 library. |
| b | Encryption Auxiliary Header (AAAD-B) with SAP = Encrypted User Data. Pre-IP bytes are ALGID + KID + MI. | Check the PDU header SAP field. If SAP = Encrypted User Data ($01), the payload is ciphertext and no IPv4 `0x45` should appear inside it — so if `0x45` is found, this isn't it. |
| c | Motorola-proprietary pre-IP wrapper not defined by any TIA document in this working set. | Cross-check against SDRTrunk's `UnconfirmedDataPacket`→`IPPacket` dispatch and Motorola source reference material (`~/blip25-motospec/`). |
| d | Block-reassembly boundary off-by-one: the "extra" bytes are actually the tail of the preceding CRC-protected structure or a stray pad octet, not payload at all. | Re-derive the first-data-block boundary from BTF / Pad Octet Count / Data Header Offset in the PDU header and confirm it matches the position the parser is treating as "start of logical message fragment." |

**What this retires** (still valid from the 2026-04-21 pass):

- The "DCOMP = 6 is outside the standard" observation — the bytes
  being inspected were not necessarily the SNDCP header.
- The "NSAPI = 0 on a data flow" anomaly — same reason.

---

## 4. What BAED-A Actually Specifies (and What It Doesn't)

The "LLC user-plane" framing relied on a reading of BAED-A that does
not hold up on a full read of the document. Concretely:

- **BAED-A is stop-and-wait**, not sliding-window (§3.2 opening). One
  data packet is in flight at a time. There is no "window" to carry
  on the wire.
- **All LLC state variables — V(S), V(R), N(S), N(R), FSNF
  (LIC + FSN), SYN** — are carried in the **PDU header block** per
  BAAA-B, not on the payload side. The header block is
  Header-CRC-16 protected; none of these fields consume bytes in
  the first data block.
- **Only two things ever chain onto the payload side** per BAED-A
  §3.2.5 step 2.2: (a) Auxiliary Header(s) per AAAD-B, present only
  when the logical message fragment is encrypted; (b) a Second
  Header, present only for Enhanced Addressing (Direct Data /
  Repeated Data — both symmetric-addressing configurations).
  **Neither applies to trunked IV&D data PDUs from an unencrypted
  user-data SAP.**
- **BAED-A §3.2.4 Table 4 SACK** is the response-packet-level SACK
  (Class = %10, Type = %000, Status = N(R), selective-retry flags),
  i.e., the same SACK as BAAA-B §5.7.5. There is not a second,
  message-level SACK layer riding in data-block bytes.

**What about the Motorola `LLC User Plane Window Size` tunable?**
`MN005155A01-A` §3.1.8 enumerates RNG-side LLC config knobs
(`LLC Number of Attempts`, `LLC Timer (sec)`, `LLC User Plane
Response Timer`, `LLC User Plane Window Size`). These are
**sender-behavior** parameters: retry counts, inter-retry timers,
and the number of *messages* an infrastructure endpoint will hold
pending at once for a given SU. They tune how the LLC state machine
runs; they do not imply additional bytes on the wire beyond what
BAED-A specifies.

**Net:** for a trunked IV&D SN-Data / SN-UData PDU carrying an
unencrypted IP datagram, the reassembled logical message fragment
per BAEB-C §1.3 / §2.3 is exactly `[SNDCP 2B][network PDU]`, with
Data Header Offset = 2. Any bytes the implementer sees between the
BAEB-C block-reassembly boundary and the start of a decodable SNDCP
header are **outside what TIA-102 specifies for that configuration**
and must be explained by one of the candidates in §3 above.

---

## 5. RFC 2507 Header Compression Changes the Parse Problem

When a session negotiates RFC 2507 compression during SN-Context
Activation, only the first datagram of the session carries a full
30-byte IPv4+UDP+SNDCP stack. Subsequent datagrams carry a
compressed header (context ID + deltas). Two RNG-side knobs
(`Max Number of Compressed Headers Between Full Headers`,
`Max Time Between Full Headers`) control how often a full header
re-appears. Exceptions: broadcast traffic is never compressed (no
per-receiver context), and encrypted traffic cannot be compressed
(per `MN005155A01-A` §2.1.3 p. 31).

**Consequence for a passive decoder:** a scan-for-IPv4 heuristic
silently mis-aligns on every compressed datagram after the first in a
session. The blip25 heuristic currently works because the Sachse
capture is broadcast CAD (compression-exempt). A decoder that
graduates to unicast trunked traffic must either decompress RFC 2507
frames or restrict itself to broadcast / encrypted captures where
full headers are guaranteed.

---

## 6. Current Heuristic in blip25

Without LLC-layer parsing, a passive decoder scans the reassembled
payload for the IPv4 version nibble and then back-aligns the SNDCP
header at `ip_offset - 2`:

```
FUNCTION locate_ip_and_sndcp(payload: bytes):
    /* Scan first 64 octets for plausible IPv4 start. */
    FOR offset IN 2..64:
        IF payload[offset] & 0xF0 == 0x40:            /* version nibble 4 */
            ihl = payload[offset] & 0x0F
            IF 5 <= ihl <= 15:
                total_len = u16_be(payload[offset+2 : offset+4])
                IF 20 <= total_len <= 510:             /* unconfirmed MTU */
                    ip_offset    = offset
                    sndcp_offset = offset - 2         /* SNDCP is at ip_offset - 2 */
                    return (sndcp_offset, ip_offset)
    return NotFound
```

At `sndcp_offset`, decode under BAEB-C Figure 26:

- `payload[sndcp_offset]` — upper nibble = PDU Type (4 = SN-UData,
  5 = SN-Data); lower nibble = NSAPI (user-context value).
- `payload[sndcp_offset + 1]` — upper nibble = PCOMP (0 = no header
  compression under BAEB-C; reserved under BAEB-B); lower nibble =
  DCOMP (0 = no payload compression).

Failure modes:

- Non-IP payloads (pure PPP, IPX, compressed headers) break the
  `0x45` scan silently. Blip25 doesn't see these on the current GMRS
  CAD flows, but a decoder targeting general trunked SNDCP V3 traffic
  needs an RFC 2507 decompressor.
- Coincidental `0x4X` bytes inside the LLC header could trip the
  scan. The IHL-range and total-len bounds shrink the false-positive
  rate; no false matches have been observed in a month of Sachse
  capture.

---

## 7. SDRTrunk Reference

SDRTrunk's `module.decode.p25.phase1.message.pdu.UnconfirmedDataPacket`
→ `IPPacket` dispatch is the open-source de-facto reference for these
frames. SDRTrunk extracts the IP datagram cleanly, so its parser
encodes the LLC+SNDCP byte layout it uses; cross-referencing its
offsets is the fastest way to tighten blip25's heuristic into a real
parser.

Per `analysis/oss_implementations_lessons_learned.md`: trust SDRTrunk
for things BAED-A and BAEB-C make testable (CRCs, PDU field
locations, dispatch ladders); treat it as an implementation witness
on byte offsets the spec leaves room to interpret. The LLC
user-plane byte layout falls in the latter category.

---

## 8. Remaining Open Questions

Recorded so a future investigator doesn't start from zero:

1. **Identity of the pre-`ip_offset-2` bytes.** Per §3, this is
   currently unexplained under TIA standards in this working set.
   Candidate hypotheses and their tests are enumerated in §3's
   table. This is the top priority for a conformant trunked-IV&D
   parser; it requires implementer-side experimentation (RFC 2507
   decompressor, SDRTrunk source read, Motorola-proprietary
   investigation, or re-derivation of the first-data-block
   boundary), not further spec-author work. Gap 0018 is re-opened
   on this basis.
2. **SNDCPv3 wire shape on Motorola.** BAEB-B Figures 18/21 show
   V3's variable-length Request/Accept (IPv4 Address conditional
   on NAT). Motorola trunked APX radios emit both V1 and V3; we
   don't yet have a Motorola capture that clearly exercises the
   V3 variable-length PDU shape. Independent of the §3 question.
3. **Priority ordering of §3 candidates.** RFC 2507 (§3 row a) is
   the most likely single explanation because it predicts exactly
   the observed symptom (compressed header of variable length
   ahead of coincidental bytes that happen to start with `0x45`)
   and because Motorola's own documentation (`MN005155A01-A`
   §2.1.3 pp. 30–31) confirms RFC 2507 is active on trunked IV&D
   after the first datagram of a session. A short experiment —
   pointing an RFC 2507 library at the bytes from the start of
   the reassembled fragment — would either confirm or rule it
   out quickly.

**Still retired** (from the 2026-04-21 pass, independent of the
2026-04-22 withdrawal): concerns that earlier drafts's raw
interpretation of `0x40 0x06` as the SNDCP header produced
implausible NSAPI / DCOMP values.

---

## 9. Cross-References

- `standards/TIA-102.BAEB-B/P25_IP_Data_Bearer_BAEB_B_Implementation_Spec.md`
  §5–§7 — full PDU-type table, field definitions, and the V1/V2/V3
  Figures 16–28 that pin down the Octet 0 / Octet 1 semantics used
  throughout this note.
- `standards/TIA-102.BAEB-C/P25_IP_Data_Bearer_Implementation_Spec.md`
  §2, §6 — BAEB-C-era SNDCP header and PCOMP compression table.
- `standards/TIA-102.BAED-A/P25_Packet_Data_LLC_Implementation_Spec.md`
  §2 — LLC layer: PDU-header fields and user-plane mechanics.
- `annex_tables/sndcp_field_definitions.csv` — code-ready field
  encoding (PDU type, NSAPI, NAT, DSUT, MTU, IPHC/HCOMP, etc.).
- `analysis/fdma_pdu_frame.md` §4.3 — Unconfirmed Data PDU wire
  layout (trunked IV&D), including Data Header Offset location.
- `analysis/motorola_conventional_scep_vs_trunked_sndcp.md` —
  architectural sibling; explains why conventional captures use
  zero-wrapper SCEP and don't show this layering at all.
- `analysis/oss_implementations_lessons_learned.md` — policy for
  trusting SDRTrunk as a secondary reference.
