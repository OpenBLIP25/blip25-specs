# Motorola SNDCP N-PDU Preamble (Trunked IV&D): LLC + SNDCP Layering

**Scope:** Bytes observed *between* the standard BAEB-C 2-byte SNDCP
header and the IPv4 header `0x45` on live Motorola P25 Unconfirmed Data
traffic. Explains the layering a passive decoder sees, what it means,
and the heuristic blip25 uses today.

**Mode:** Trunked IV&D only. For conventional IV&D, the preamble is
not SNDCP at all — it's SCEP (Motorola proprietary) riding on BAEB-A.
See `analysis/motorola_conventional_scep_vs_trunked_sndcp.md` for the
mode split.

**Not a TIA spec amendment.** The bytes surrounding the SNDCP header
belong to the TIA-102.BAED-A LLC user-plane layer, which is defined
but not drawn at byte-offset granularity in the existing derived
works.

**Related:**
- `analysis/motorola_conventional_scep_vs_trunked_sndcp.md` — the
  architectural companion to this note; explains why conventional
  captures look different.
- `gap_reports/0013_motorola_sndcp_npdu_header_variants.md` — resolved.
- `gap_reports/0018_trunked_ivd_llc_plus_sndcp_plus_rfc2507.md` —
  resolved; established the LLC+SNDCP layering.

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

## 2. The Layering on the Wire (Trunked IV&D)

Motorola's `MN005155A01-A` Trunked Data Services Feature Guide
§2.1.3 (p. 30) quantifies the full-uncompressed preamble as 30 bytes
across IPv4 + UDP + SNDCP. But that 30-byte figure does *not* include
the LLC user-plane layer, which sits between the BAEB-C PDU
reassembly boundary and the SNDCP header and has its own bytes on the
wire.

```
┌────────────────────────────────────────────────────────────┐
│  CAI PDU (TIA-102.BAEB-C blocks, reassembled payload)      │
│  └── LLC user-plane PDU (TIA-102.BAED-A)                   │
│      • sequence / window fields                            │
│      • ACK / SACK (message-level, distinct from BAAA-B     │
│        packet-level SACK)                                  │
│      └── SNDCP header (~2 bytes per BAEB-C §6.4)           │
│          • PDU Type + NSAPI + PCOMP + DCOMP                │
│          └── IPv4 (20 B) + UDP (8 B) + app payload         │
└────────────────────────────────────────────────────────────┘

Total pre-payload bytes (no compression) ≈ 30 B of IPv4+UDP+SNDCP
plus the LLC user-plane header (not separately quantified by
MN005155A01-A).
```

So the "extra bytes" historically observed between the BAEB-C 2-byte
SNDCP header and the IPv4 `0x45` — the mystery that motivated gap
0013 — are **LLC user-plane** bytes per TIA-102.BAED-A, not a
Motorola-proprietary extension and not a PCOMP/DCOMP
misinterpretation.

---

## 3. The `0x40 0x06 ...` bytes are LLC, not SNDCP

**Correction (2026-04-21, implementer feedback from `blip25-data`):**
earlier drafts of this note and the BAEB-B impl spec §9 tried to
decode the observed `0x40 0x06 ...` bytes as the start of the SNDCP
header (PDU Type / NSAPI in Octet 0, PCOMP/DCOMP or Reserved/DCOMP
in Octet 1). That was mis-scoping. Those bytes belong to the
**LLC user-plane** layer (TIA-102.BAED-A), which sits *before* SNDCP
in the wire layering per §2 of this note.

**Real SNDCP on trunked Motorola captures is at `ip_offset - 2`** —
i.e., two octets immediately preceding the IPv4 version nibble
`0x45`. At that location the SNDCP header decodes cleanly under
standard BAEB-B / BAEB-C Figure 26 / 27 / 28 semantics: Octet 0 =
`PDU Type | NSAPI` with plausible values (PDU Type 4 = SN-UData or
5 = SN-Data, NSAPI in the user-context range 1–14); Octet 1 =
`PCOMP | DCOMP` with PCOMP = 0 (no compression) and DCOMP = 0 (no
compression). No reserved-bit emission, no `DCOMP = 6` mystery —
those "reserved values" only appeared to exist because the wrong
bytes were being inspected.

**Consequence for a passive decoder:**

- Locate IPv4 `0x45` first (the scan-for-version-nibble heuristic in
  §6 below is the right way to do this under current tooling).
- SNDCP header = `payload[ip_offset - 2 : ip_offset]`. Decode under
  BAEB-C Figure 26 semantics.
- Everything between the BAEB-C reassembled-payload boundary and
  `ip_offset - 2` is the LLC user-plane frame (sequence number,
  window/flow control, ACK/SACK book-keeping per BAED-A).

**What this retires:**

- The three "candidate explanations" for `0x40 0x06` that earlier
  drafts speculated about (swapped nibble order / pre-standard
  Motorola variant / reserved-DCOMP-with-proprietary-wrapper) were
  all variations on the same mis-scoping. None of them apply; the
  real SNDCP bytes decode normally.
- The "DCOMP = 6 is outside the standard" observation was looking
  at LLC bytes and calling them DCOMP. With the correction, DCOMP
  on Motorola trunked captures is 0, as expected.
- The "NSAPI = 0 on a data flow" anomaly similarly relaxes — the
  `0x40` nibble was an LLC field, not SNDCP. Actual NSAPI on the
  real SNDCP header sits at a user-context value.

**What remains:**

- The exact LLC user-plane byte layout is still an open question.
  BAED-A §2 describes the fields semantically but the byte offsets
  of the sequence / window / ACK fields in the first data block
  need SDRTrunk-source confirmation or a clean context-activation
  capture to pin down.

---

## 4. Where the LLC Header and the PDU-Header LLC Fields Differ

Two distinct sets of LLC-related fields exist:

- **PDU-header LLC fields** (N(S), FSNF, Data Header Offset) — per
  BAED-A §2.1 and the Confirmed/Unconfirmed PDU header figures in
  BAAA-B (see `analysis/fdma_pdu_frame.md` §4.3 / §4.4). These live in
  the Header block and are Header-CRC-16 protected; they do not
  consume payload bytes in the first data block.
- **LLC user-plane bytes** — the sliding-window sequence, window, and
  ACK/SACK mechanics per BAED-A. These *do* live on the user-payload
  side of the PDU boundary, sitting between BAEB-C reassembled
  payload and the SNDCP header.

What this means for the preamble question: **the SNDCP header sits
at `ip_offset - 2`**, directly in front of the IPv4 `0x45` version
nibble. Everything between the BAEB-C reassembled-payload boundary
and `ip_offset - 2` is LLC user-plane overhead. `MN005155A01-A`
§3.1.8 (p. 58) enumerates the RNG-side user-plane LLC config knobs
(`LLC Number of Attempts`, `LLC Timer (sec)`, `LLC User Plane
Response Timer`, `LLC User Plane Window Size`), which confirms the
layer is real and config-tunable on a live system.

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

1. **LLC user-plane byte layout.** BAED-A describes the layer
   semantically; the exact byte positions of sequence / window /
   ACK fields in the first data block still need confirmation
   against either SDRTrunk source or a live SN-Context Activation
   capture. This is the *new* top priority now that SNDCP itself
   is no longer mysterious — the remaining unknowns all live in
   the LLC layer.
2. **SNDCPv3 wire shape on Motorola.** BAEB-B Figures 18/21 show
   V3's variable-length Request/Accept (IPv4 Address conditional
   on NAT). Motorola trunked APX radios emit both V1 and V3; we
   don't yet have a Motorola capture that clearly exercises the
   V3 variable-length PDU shape. SN-Data / SN-UData at
   `ip_offset - 2` decode identically in V1/V2/V3 so this only
   matters at context activation.

**Retired by the `ip_offset - 2` correction:**
- ~~NSAPI = 0 on a data flow~~ — we were looking at LLC bytes,
  not the SNDCP header. Real SNDCP NSAPI on Motorola trunked
  captures sits at a user-context value.
- ~~DCOMP = 6 emission~~ — same misattribution. Real DCOMP
  decodes to 0 at `ip_offset - 2`.

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
