# TIA-102.BAEB-B Related Resources and Context
## Packet Data Specifications for P25 Data Bearer Services

---

## Document Status

| Field | Value |
|---|---|
| Document Number | TIA-102.BAEB-B |
| Full Title | Packet Data Specifications for Project 25 Data Bearer Services |
| Published | September 2, 2014 |
| Supersedes | TIA-102.BAEB-A |
| Status (as of pipeline processing, 2026) | Active (no known superseding revision) |
| Classification | Protocol; Data Services |
| Derivation Status | Phase 2 complete; Phase 3 (implementation spec) not yet attempted |

Revision B adds relative to Revision A:
- SNDCP Version 3 support (RFC 2507 header compression, MSO, Version Supported negotiation, expanded reject codes)
- WAI SNDCP interoperability fields
- An expanded reject code table

---

## TIA-102 Standards Family Position

TIA-102.BAEB-B sits in the data services layer of the P25 standards stack. It depends on the physical/MAC layers for actual packet delivery and on the trunking/conventional layer for channel grants and state management.

```
┌─────────────────────────────────────────────────────────────────┐
│           Application Layer (CAD, AVL, messaging, etc.)         │
├─────────────────────────────────────────────────────────────────┤
│     IP / IPv4 (RFC 791)                                         │
├─────────────────────────────────────────────────────────────────┤
│  SCEP (Direct/Repeated/Conv)  │  SNDCP (Conv FNE / Trunked FNE) │
│                    TIA-102.BAEB-B  ◄── THIS DOCUMENT            │
├─────────────────────────────────────────────────────────────────┤
│           Packet Data LLC / Data Headers                        │
│      TIA-102.BAED-A              TIA-102.AABC / AABF            │
├─────────────────────────────────────────────────────────────────┤
│  FDMA CAI/MAC           │  TDMA CAI/MAC                         │
│  TIA-102.BAAA-B         │  TIA-102.BBAC-A                       │
│  TIA-102.AABB-B         │  TIA-102.BBAB                         │
├─────────────────────────────────────────────────────────────────┤
│  Trunking Procedures    │  Conventional Procedures              │
│  TIA-102.AABD-B         │  TIA-102.BAAD-B                       │
│  TIA-102.BACA/BACE (TMS)│                                       │
└─────────────────────────────────────────────────────────────────┘
```

### Upstream Dependencies (what BAEB-B requires from other docs)

| Document | Role in BAEB-B context |
|---|---|
| TIA-102.BAAA / BAAB | Trunking system MAC procedures; channel grants (SN-DATA_CHN_GNT) |
| TIA-102.BBAC | TDMA MAC layer data packet delivery (referenced as [8]) |
| TIA-102.AABC / AABF | SAP values (Packet Data Control, Unencrypted User Data); LLID designations |
| TIA-102.BACA / BACE | TMS registration procedures (drive Closed↔Idle in TDS state machine) |
| TIA-102.AACA | Over-the-air key management; provides encryption layer for SNDCP data PDUs |
| TIA-102.BAED-A | Packet data LLC — data headers, Data Header Offset, CAI confirmed/unconfirmed delivery |
| RFC 1144 | Van Jacobsen TCP/IP header compression (IPHC/HCOMP bit 1) |
| RFC 1661 | PPP framing (PCO tunneled PPP information) |
| RFC 1994 | CHAP (PCO Protocol Identity 0xC223) |
| RFC 2507 | IP header compression (HCOMP bit 2, V3+) |

### Downstream (what depends on BAEB-B)

Documents that reference this spec's protocols or PDU formats:
- Any P25 implementation spec addressing IP data bearer (e.g., `P25_IP_Data_Bearer_Implementation_Spec.md` in this pipeline)
- Trunked data service implementations that need SNDCP context management
- Conventional data implementations using SNDCP optional mode

---

## Phase 3 Implementation Spec Assessment

This document would warrant the following Phase 3 artifacts:

| Artifact | Rationale |
|---|---|
| **SNDCP Context Management State Machine Spec** | Two independent state machine families (TDS + CDS) with ~6 states each, multiple timer interactions, and V1/V2/V3 version branches. Complex enough to warrant a dedicated state machine spec. |
| **SNDCP PDU Parser Spec** | 28 distinct PDU formats across 3 versions. V3 has variable-length PDUs with conditional fields (IPv4 Address conditioned on NAT). Parser dispatch on PDU Type + direction + version. |
| **SCEP Protocol Spec** | Simpler but distinct: ARP adaptation (22-byte format, ARP-TYPE_CAI=33), IP datagram conveyance for Direct/Repeated/Conventional FNE scenarios. |
| **PCO/CHAP Encoding Spec** | PPP-tunneled CHAP authentication inside SNDCP PCO field; V2 vs. V3 PCO format differences (deprecated protocols, Reserved nibble). |
| **APN Encoding Spec** | APN length encoding (index vs. string), DNS-format label rules, Network ID vs. Operator ID. |

The existing `P25_IP_Data_Bearer_Implementation_Spec.md` in this pipeline is a stub. A full Phase 3 run should expand it to cover all of the above.

---

## Open Source Implementation References

The following open-source P25 projects implement portions of this specification. These can be consulted for implementer perspective but are not authoritative.

### OP25 (GNURadio-based P25 receiver)
- Data channel decoding is present in the OP25 codebase
- SNDCP parsing: look for `p25_decoder` components or data-channel-related source files
- SCEP/SNDCP state machines may be partially implemented

### SDRTrunk
- Java-based P25 decoder with more complete data channel support
- Search for `SNDCP`, `SNDCPPacket`, or `DataChannel` classes
- Context management state machines are likely implemented in the trunked data service module

### OP25 (Osmocom variant)
- Fork of OP25 with additional data channel work; may have more complete SNDCP coverage

> **Clean-room note:** These references are cited for orientation only. The implementer (blip25-mbe) should implement from derived specs in this pipeline, not from OP25 or SDRTrunk source.

---

## Document Lineage

```
TIA-102.BAEB     (original, not surveyed)
    │
    ▼
TIA-102.BAEB-A   (adds SNDCP V2: APN, PCO/CHAP, DHCP, Activation Command)
    │
    ▼
TIA-102.BAEB-B   (this document, 2014)
                 (adds SNDCP V3: RFC 2507 compression, MSO, Version Supported,
                  expanded reject codes, WAI SNDCP interoperability)
    │
    ▼
TIA-102.BAEB-C?  (not confirmed; the derived implementation spec in this
                  pipeline references BAEB-C — needs verification)
```

> **Note:** The pipeline's `CLAUDE.md` references `TIA-102.BAEB-C` in the data services section. If a Revision C exists, it postdates this document and may add further SNDCP version support or corrections. Verify against `specs.toml`.

---

## Scope Boundaries (What This Document Does NOT Specify)

| Out of Scope | Notes |
|---|---|
| IPv6 | Document is IPv4-only throughout; IPv6 not mentioned |
| Payload compression algorithms | DCOMP field reserved for future use; no algorithm defined |
| Specific Ready/Standby timer values | Tables 23/25 define encoding; actual values are local policy |
| FNE routing procedures | FNE internal routing to DHN is beyond scope |
| UDP/IP header compression (V1/V2) | Anticipated but never defined; superseded by RFC 2507 in V3 |
| NSAPI 2–14 guarantee | Only NSAPI 1 is required to be supported by all FNEs |
| APN index-to-string mapping | Local FNE policy; not standardized |
| MDP-to-SU APN communication | How MDP informs SU of desired APN is beyond scope |

---

## Key Implementation Pitfalls

1. **PDU Type 0 is used for both Activation Request and Accept.** Direction (SU→FNE vs FNE→SU) determines interpretation of Octet 0 lower nibble. A bidirectional parser must track direction context.

2. **V3 PDUs have conditional IPv4 Address fields.** The NAT field (Octet 1 bits 3:0) must be read before interpreting offsets of all subsequent fields. V3 parsers cannot use fixed field offsets.

3. **IPHC (V1/V2) vs HCOMP (V3) is a rename, not a new field.** The 8-bit compression bitmap occupies the same position; V3 adds a second meaningful bit (RFC 2507). Implementers treating them as distinct fields will break cross-version negotiation.

4. **TCPSS (4-bit, V1/V2) vs VJ_TCP_CSS (8-bit, V3) are different widths.** The V3 rename also expands the field. A V3 SU negotiating with a V1/V2 FNE must use the TCPSS width.

5. **NON_TCP_CSS is 16 bits (2 octets) in V3.** Only RFC 2507 non-TCP compression state slots. Don't confuse with the UDPSS field (4 bits, V1/V2, reserved/zero).

6. **DAC field is TDS-only and SNDCP-version-sensitive for unsolicited updates.** CDS implementations should not parse or act on DAC. V1/V2 implementations must ignore DAC in unsolicited Accepts; V3+ may update it.

7. **Reject Code 23 is the only V3 reject code that appends an extra field.** The Version Supported field (2 octets) appears only when Reject Code = 23; parsing the remainder of the PDU as variable-length for any other code is incorrect.
