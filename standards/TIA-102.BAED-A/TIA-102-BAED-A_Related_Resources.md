# TIA-102.BAED-A — Related Resources & Context
# Project 25 Packet Data Logical Link Control Procedures

---

## Status

**Active.** ANSI/TIA-102.BAED-A was approved April 12, 2019 and is the current
edition. It cancels and replaces TIA-102.BAED (original edition, August 2013). Revision A
introduced no technical changes — it updated front matter and refreshed normative
references to align with current TR-8 document editions. No superseding document has
been issued as of the 2022–2024 approved P25 standards lists published by the Project 25
Steering Committee.

A TIA Call for Interest was issued at some point seeking input on potential future work for
this document, but no replacement revision had been published as of the September 2024
P25 approved standards list.

---

## Standards Family

This document belongs to the **TIA-102 BAE* packet data sub-family** within the broader
TIA-102 (Project 25) suite. The BAE series covers the complete P25 packet data stack:

```
TIA-102 (Project 25 Standards Suite)
└── BAE* — Packet Data
    ├── TIA-102.BAEA-C  — Data Overview and Specification (architecture, SAPs, configs)
    ├── TIA-102.BAEB    — Packet Data Convergence (SNDCP layer procedures)  [if exists]
    ├── TIA-102.BAEC    — (reserved/other data layer)
    ├── TIA-102.BAED-A  — Packet Data Logical Link Control Procedures  ← THIS DOCUMENT
    └── TIA-102.BAEJ-A  — Conventional Management Service Spec for Packet Data (CMS)
```

Normative dependencies (documents this document references):

```
TIA-102.BAED-A
├── TIA-102.BAAA-B  — FDMA Common Air Interface  (packet formats, CRC, FSNF encoding)
├── TIA-102.BAEA-C  — Data Overview and Specification  (architecture, configurations)
├── TIA-102.BAEJ-A  — Conventional Mgmt Service for Packet Data  (CMS / decryption fail)
├── TIA-102.AAAD-B  — Block Encryption Protocol  (encryption/Aux Header chaining)
├── TIA-102.BAAC-D  — CAI Reserved Values  (unit IDs, group addresses)
├── TIA-102.AABD-B  — Trunking Procedures  (NAC acceptance on trunked channels)
└── TIA-102.BAAD-B  — Conventional Procedures  (NAC acceptance on conventional channels;
                       FS_R repeater sender/receiver procedures)
```

Materials for the original edition were derived from TIA-102.BAAD-A (the predecessor
conventional procedures document). Development was managed by the TIA TR-8.5 Signaling
and Data Transmission Subcommittee, using source material from the TIA TR-8.15 Common
Air Interface Subcommittee.

---

## Practical Context

### Where LLC Fits in the P25 Packet Data Stack

P25 packet data uses a layered model. From bottom to top:

```
Application / IP layer
      ↕
SNDCP (Subnetwork Dependent Convergence Protocol)
      ↕
LLC — Logical Link Control  ← this document
      ↕
CAI MAC / Physical layer (TIA-102.BAAA-B)
      ↕
RF channel
```

The LLC layer's job is to take logical messages handed down from SNDCP (or directly from
CMS for management traffic) and break them into data packets that fit the CAI data packet
format, then convey them reliably (confirmed) or unreliably (unconfirmed) across the air
interface. On receipt, LLC reassembles fragments back into logical messages and passes
them up.

### Confirmed vs. Unconfirmed in Practice

In real P25 deployments, confirmed conveyance is used for IP data sessions requiring
reliability (e.g., Mobile Data Terminal file transfers, database queries). Unconfirmed
conveyance is used for low-latency or broadcast traffic where retransmission delay is
unacceptable, or for scan preamble messages that merely advertise capability.

The stop-and-wait nature of confirmed conveyance means throughput is fundamentally
limited by round-trip latency over the air interface — a known limitation acknowledged
by the P25 suite. Higher-layer protocols (TCP over SNDCP/IP) handle end-to-end
reliability above this layer.

### Configurations and Equipment

The four data configurations map to real equipment types:

- **Direct Data**: Portable-to-portable P25 data, no infrastructure (rare in practice).
- **Repeated Data**: Portable-to-portable via a fixed FS_R repeater; FS_R is transparent
  at the LLC level (it passes packets without sequence number changes).
- **Conventional FNE Data**: Portable communicates with a conventional dispatch console
  or backend system via a Fixed Network Equipment node.
- **Trunked FNE Data**: The dominant mode in large public safety systems (P25 Phase 1
  trunking). MDT/AVL data from subscriber units flows through a trunked channel to an
  FNE, then to CAD/dispatch servers.

Manufacturers implementing P25 packet data (Motorola APX/ASTRO, Harris Unity, Kenwood
NX-5000 series, etc.) implement LLC procedures internally in firmware. Interoperability
testing for P25 packet data compliance is managed through the DHS PSCR P25 CAP program.

---

## Key Online Resources

- **TIA standards catalog** (purchase/access point for official document):
  https://www.tiaonline.org/standards/catalog/

- **Project 25 Technology Interest Group (PTIG)** — official P25 standards list:
  https://www.project25.org/

- **PTIG Approved P25 TIA Standards (September 2024)**:
  https://project25.org/images/stories/ptig/P25_SC_24-09-002%20P25%20TIA%20Standards_Approved%20Sep2024.pdf

- **PTIG Approved P25 TIA Standards (June 2019)** (confirms BAED-A listing):
  https://www.project25.org/images/stories/ptig/P25_SC_19-06-002-R1_Approved_P25_TIA_Standards_-_June_2019.pdf

- **TIA-102 Wikipedia overview**:
  https://en.wikipedia.org/wiki/TIA-102

- **TIA-102 Series Documents archive** (Internet Archive — historical versions):
  https://archive.org/details/TIA-102_Series_Documents

- **TIA Call for Interest — Packet Data LLC (future revision notice)**:
  https://tiaonline.org/standardannouncement/tia-issues-call-for-interest-on-new-project-for-packet-data-logical-link-control-procedures/

- **P25 document suite reference (QSL.net, 2010 snapshot)**:
  https://www.qsl.net/kb9mwr/projects/dv/apco25/P25_Standards.pdf

- **GNURadio P25 overview (NI/National Instruments)**:
  https://wiki.gnuradio.org/images/f/f2/A_Look_at_Project_25_(P25)_Digital_Radio.pdf

---

## Open-Source Implementations

No open-source project is known to implement the LLC layer procedures from this document
as a standalone, named module. However, the following projects implement P25 packet data
reception that necessarily involves LLC-level processing:

### OP25 (GNU Radio P25)
- Repository: https://github.com/balint256/op25 (original); https://github.com/boatbod/op25 (active fork)
- OP25 implements P25 Phase 1 FDMA decoding including packet data unit detection. The
  `p25craft.py` tool constructs P25 PDUs including data packets. LLC-level reassembly of
  confirmed data packet fragments is partially implemented in the data path.
- Relevant file: `op25/gr-op25_repeater/apps/tx/p25craft.py`

### SDRTrunk
- Repository: https://github.com/DSheirer/sdrtrunk
- Wiki: https://github.com/DSheirer/sdrtrunk/wiki/APCO25_V0.3.0
- SDRTrunk decodes P25 packet data headers including LLID extraction, BTF, and data
  block parsing. It handles confirmed data packet header and block decoding. Full LLC
  confirmed conveyance state machine (ACK/NACK/SACK response generation) is not
  implemented since SDRTrunk is a passive monitor rather than a transceiver.

### p25decode / gr-p25
- Various GNU Radio OOT modules implement P25 data unit parsing at the physical/MAC
  layer; LLC reassembly above that layer is generally not implemented in receiver-only tools.

**Note for implementers:** A full LLC implementation requires both sender and receiver
state machines. The confirmed sender requires:
- Stop-and-wait sequence number management (V(S) modulo 8)
- Full/selective retry logic with T_RETRY timer and TX_MAX counter
- CRC_PACKET (32-bit), CRC_HEADER (16-bit), CRC_BLOCK (9-bit) computation
  (all defined in TIA-102.BAAA-B, not this document)
- FSNF fragmentation/reassembly

The unconfirmed sender is simpler: construct packet, compute CRCs, transmit once.

---

## Standards Lineage

```
Project 25 Packet Data Standards Lineage
─────────────────────────────────────────

TIA-102.BAAD-A  (original Conventional Procedures, contained LLC material)
       │
       │  materials extracted and published separately
       ↓
TIA-102.BAED    (original — August 2013)
  "Project 25 Packet Data Logical Link Control Procedures"
  [TR-8.5 / TR-8.15 development]
       │
       │  Revision A: errata fixes, reference updates, no technical change
       ↓
TIA-102.BAED-A  (current — ANSI/TIA-102.BAED-A-2019, approved April 12, 2019)
  ← THIS DOCUMENT

Peer documents in the BAE packet data sub-family:
  TIA-102.BAEA   → TIA-102.BAEA-B → TIA-102.BAEA-C  (Data Overview)
  TIA-102.BAEJ   → TIA-102.BAEJ-A                    (CMS for Packet Data)

Physical layer foundation:
  TIA-102.BAAA   → TIA-102.BAAA-A → TIA-102.BAAA-B  (FDMA Common Air Interface)
    [defines data packet formats, CRCs, FSNF — essential companion to BAED-A]
```
