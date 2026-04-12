# TIA-102.BAEB-C Related Resources & Context

**Document:** ANSI/TIA-102.BAEB-C-2019 — Project 25 IP Data Bearer Service Specification  
**Revision:** C (of TIA-102.BAEB-B)  
**Approved:** December 16, 2019  

---

## Status

**Active.** This is the current revision of the document. Revision C was adopted by the Project 25 Steering Committee on October 10, 2019 and approved by TIA on December 16, 2019. It cancels and replaces TIA-102.BAEB-B (2014).

Revision history:
- Interim Standard (1997): Original ballot draft
- Original (2000): First published standard (ANSI/TIA/EIA-102.BAEB)
- Revision A (2004): Incorporated SNDCP addendum (BAEB-1) and USB/PPP A-Interface addendum (BAEB-2)
- Revision B (2014): Major rewrite — added SNDCP Version 3, removed A/Ed Interface specs, scoped to IP Bearer Service only
- **Revision C (2019): Errata and editorial cleanup only**

No known successor document as of 2026. This document remains the authoritative specification for P25 IP data bearer protocols.

---

## Standards Family

This document sits within the **TIA-102 BAEX series** — the P25 Packet Data layer standards:

```
TIA-102 (Project 25 Standards Suite)
├── Series AAxx: Trunking Layer
│   ├── TIA-102.AABA-B  — Trunking Overview
│   ├── TIA-102.AABC-E  — Trunking Control Channel Messages
│   │     (SN-DATA_CHN_REQ, SN-DATA_CHN_GNT, SN-DATA_PAGE_REQ,
│   │      SN-DATA_PAGE_RES, SN-REC_REQ, DENY_RSP, QUE_RSP referenced here)
│   ├── TIA-102.AABD-B  — Trunking Procedures
│   └── TIA-102.AABF-D  — Link Control Word Formats and Messages
│         (LC_CALL_TRM_CAN used for TDS teardown)
│
├── Series BAxx: FDMA Physical/Data Layer
│   ├── TIA-102.BAAA-B  — FDMA Common Air Interface
│   ├── TIA-102.BAAC-D  — CAI Reserved Values (SAP values, broadcast LLID)
│   ├── TIA-102.BAAD-B  — Conventional Procedures
│   │
│   └── Series BAEx: Packet Data
│       ├── TIA-102.BAEA-C  — Data Overview and Specification ← PARENT DOC
│       ├── TIA-102.BAEB-C  — IP Data Bearer Service ← THIS DOCUMENT
│       ├── TIA-102.BAED-A  — Packet Data LLC Procedures (layer below)
│       └── TIA-102.BAEJ-A  — Conventional Management Service for Packet Data
│
└── TIA-902.BAEB-A  — WAI (Wideband/Phase 2) Packet Data Specification
      (informative reference; WAI shares SNDCP versioning space)
```

This document defines the convergence sublayer between the P25 LLC (BAED) and
IPv4 applications. It is mandatory for any implementation of P25 Trunked Packet Data,
and optionally applicable to Conventional Packet Data.

---

## Practical Context

**Who uses this:** Equipment manufacturers implementing P25 subscriber radios (SUs),
fixed network equipment (FNEs/infrastructure), and mobile data peripherals (MDPs —
laptops or MDTs connected to a radio via USB, serial, or PPP). Public safety agencies
operating P25 trunked systems rely on this for Mobile Data Terminal (MDT) connectivity,
AVL (Automatic Vehicle Location), and application data over P25.

**How it is used in equipment:**
- All certified P25 trunked data-capable radios must implement SNDCP. The APCO
  Project 25 Compliance Assessment Program (CAP) tests implementations against this.
- The SN-DATA_CHN_REQ / SN-DATA_CHN_GNT exchange over the trunked control
  channel is observable in P25 trunked system traffic and is decoded by most P25
  signal analyzers.
- SNDCP context activation is the negotiation phase before any IP data flows; it
  establishes the NSAPI, IP address, timer values, compression, and authentication.
- In practice, most deployments use SNDCP Version 1 or 2. Version 3 features
  (RFC 2507 compression, MSO, dynamic DAC) appear in more recent infrastructure.
- SDRTrunk and OP25 do not implement SNDCP/SCEP data plane decoding (they focus
  on voice), but the control channel messages (SN-DATA_CHN_REQ, etc.) that are
  referenced in TIA-102.AABC are decoded.

**Data throughput context:** P25 FDMA packet data operates at approximately 9.6 kbps
raw over a 12.5 kHz channel. The LLC fragmentation defined in BAED sets the practical
MTU for unconfirmed delivery at 510 bytes. Confirmed delivery with the 1500-byte MTU
setting requires LLC reassembly of multiple fragments. In practice, public safety data
applications (CAD queries, license plate lookups, AVL) are designed for these low
throughput constraints.

---

## Key Online Resources

- **TIA Standards Store** — Official source for the published document:
  https://www.tiaonline.org/standards/

- **APCO Project 25 Technology Interest Group (PTIG)**:
  https://www.project25.org/
  The PTIG manages the P25 Standard suite and Compliance Assessment Program.
  Implementation conformance testing references BAEB-C indirectly via the data
  conformance test suite.

- **P25 Standard Documents Archive** (many historical documents freely accessible):
  https://www.project25.org/resources/standards-docs.html

- **APCO P25 Compliance Assessment Program (CAP)**:
  https://www.project25.org/compliance/
  Testing for P25 data compliance includes SNDCP interoperability testing between
  subscriber units and infrastructure.

- **Motorola Solutions P25 Technical Resources** (vendor reference):
  https://www.motorolasolutions.com/en_us/products/land-mobile-radio-communications/p25.html

- **Harris/L3 (now L3Harris) SNDCP references** appear in their P25 technical notes,
  though most are behind NDAs or support portals.

- **IETF RFC archive** (referenced protocols):
  - RFC 791 (IPv4): https://www.rfc-editor.org/rfc/rfc791
  - RFC 826 (ARP): https://www.rfc-editor.org/rfc/rfc826
  - RFC 1144 (Van Jacobsen TCP compression): https://www.rfc-editor.org/rfc/rfc1144
  - RFC 1994 (PPP CHAP): https://www.rfc-editor.org/rfc/rfc1994
  - RFC 2507 (IP header compression): https://www.rfc-editor.org/rfc/rfc2507

---

## Open-Source Implementations

### SDRTrunk
- **Repository:** https://github.com/DSheirer/sdrtrunk
- **Relevance:** Decodes P25 trunked control channel messages including
  SN-DATA_CHN_REQ, SN-DATA_CHN_GNT, SN-DATA_PAGE_REQ defined in AABC-E
  and referenced by this document. Does not implement SNDCP data plane.
- **Language:** Java

### OP25
- **Repository:** https://github.com/boatbod/op25
- **Relevance:** P25 protocol suite implementation including control channel
  decoding. SN-DATA channel messages visible. No SNDCP data plane.
- **Language:** Python / C++

### gr-p25
- **Repository:** https://github.com/osmocom/gr-p25 (and related forks)
- **Relevance:** GNU Radio P25 decoder. Control channel messages decoded
  including data channel grant messages.

### DSDPlus / UniTrunker (closed-source)
- Decode P25 control messages including SNDCP data channel grants.
  Referenced for context; not open source.

### No known complete open-source SNDCP implementation exists as of 2026.
The context activation state machine and PDU parsing defined in this document
would be the foundation for implementing SNDCP in an open-source P25 data stack.
The most likely home for such work would be within OP25 or SDRTrunk extensions.

---

## Standards Lineage

```
Project 25 (P25) Standard
├── Governed by: APCO Project 25 Steering Committee (P25SC)
├── Published by: TIA TR-8 Committee, TR-8.5 Subcommittee
│
└── TIA-102 Suite
    ├── TIA-102.BAEA-C — Data Overview and Specification (parent)
    │   └── TIA-102.BAEB (IP Bearer Service)
    │       ├── Interim Standard (1997) — Original draft
    │       ├── ANSI/TIA/EIA-102.BAEB (2000) — First published
    │       ├── ANSI/TIA/EIA-102.BAEB-1 (2001) — SNDCP addendum
    │       ├── ANSI/TIA/EIA-102.BAEB-2 (2002) — USB/PPP A-Interface addendum
    │       ├── ANSI/TIA-102.BAEB-A (2004, Revision A) — Consolidated
    │       ├── ANSI/TIA-102.BAEB-B (2014, Revision B) — Added SNDCP V3
    │       └── ANSI/TIA-102.BAEB-C (2019, Revision C) — Errata ← CURRENT
    │
    └── Related WAI variant:
        └── TIA-902.BAEB-A — WAI Packet Data Specification
              (P25 Phase 2 wideband; shares SNDCP protocol with different
               version number space; WAI versions occupy bits 8-15 of the
               Version Supported field)
```

---

## Phase 3 Implementation Spec Flag

**Recommended Phase 3 specs for a follow-up pass:**

1. **`TIA-102-BAEB-C_SNDCP_Protocol_Implementation_Spec.md`** — PROTOCOL type:
   - TDS and CDS state machines with complete state tables
   - Context activation/deactivation procedure sequences (all 4 cases: SU/FNE × TDS/CDS)
   - Timer behavior (start/stop/restart semantics in each state transition)
   - PDCH access procedures (RA vs AA, DAC bit checking)
   - Ready Roaming reconnect procedure
   - Version negotiation procedure (reject code 23 + retry)
   - Pseudocode for SU and FNE state machines in Rust-friendly form

2. **`TIA-102-BAEB-C_SNDCP_Message_Parsing_Spec.md`** — MESSAGE_FORMAT type:
   - PDU dispatch table (4-bit PDU Type field)
   - Byte-level format diagrams for all 7 PDU types × applicable versions
   - Conditional field logic (IPv4 Address presence rules in V3)
   - Variable-length field parsing (APN, PCO, MSO)
   - Parser pseudocode showing dispatch + extraction pattern
   - Complete field encoding tables (all 43 tables from Section 6)

3. **`TIA-102-BAEB-C_SCEP_Implementation_Spec.md`** — PROTOCOL + MESSAGE_FORMAT:
   - ARP request/reply format (22-byte wire format)
   - ARP procedure state (request → retry → fail)
   - IP datagram conveyance rules (SAP values, addressing modes)
   - Configuration table: valid SCEP configurations per data type

These three specs are independent and can be produced in any order in a follow-up conversation.
