# Related Resources & Context

## Status

This document is **active and current**. It was published in November 2019 and approved by the Project 25 Steering Committee on October 10, 2019 as part of the P25 Standard. It appears on the most recent P25 Steering Committee approved standards lists (confirmed through at least the February 2023 edition of the approved standards roster).

This document is Revision A of TIA-102.BBAD (originally published August 2017). It cancels and replaces TIA-102.BBAD entirely. The revision restructured the Two-Slot TDMA document suite by separating MAC messages (this document) from MAC bursts/coding (moved to TIA-102.BBAC-A) and MAC procedures (moved to TIA-102.BBAE).

An addendum is under development: **TIA-102.BBAD-A-1**, titled "Project 25 Two-Slot TDMA MAC Layer Messages Addendum 1 — Location on PTT and User Alias." TIA issued a call for interest in October 2024, with development led by the TR-8.10 Engineering Committee on Air Interface.

---

## Standards Family

This document is part of the **TIA-102 Two-Slot TDMA document suite** within the broader P25 standard. The companion documents are:

| Document | Title | Scope |
|----------|-------|-------|
| TSB-102.BBAA | Two-Slot TDMA Overview | High-level overview of the P25 Phase 2 TDMA standard suite |
| TIA-102.BBAB | Two-Slot TDMA Physical Layer Protocol Specification | PHY layer: H-CPM (inbound) and H-DQPSK (outbound) modulation at 12 kb/s in 12.5 kHz |
| TIA-102.BBAC-A | Two-Slot TDMA MAC Layer Specification | MAC layer bursts, coding, and channel structure |
| **TIA-102.BBAD-A** | **Two-Slot TDMA MAC Layer Messages** | **MAC PDU formats and message definitions (this document)** |
| TIA-102.BBAE | Two-Slot TDMA MAC Layer Procedures | MAC layer procedures (call setup, teardown, registration, etc.) |

Key cross-referenced documents from outside the TDMA suite:

| Document | Title | Relationship |
|----------|-------|-------------|
| ANSI/TIA-102.AABC-E | Trunking Control Channel Messages | Source of FDMA ISP/OSP opcodes reused by TDMA MAC messages |
| ANSI/TIA-102.BAAA-B | FDMA Common Air Interface | FDMA CAI counterpart; HDU/ETDU equivalencies for MAC_PTT/MAC_END_PTT |
| ANSI/TIA-102.AAAD-B | Block Encryption Protocol | Defines ALGID, Key ID, and MI fields used in encrypted call PDUs |
| TIA-102.AABH | Dynamic Regrouping Messages and Procedures | Source of MFID90/MFIDA4 dynamic regrouping message definitions |
| TSB-102-C | Documentation Suite Overview | Master index describing the Um2 interface and all P25 document relationships |

Conformance testing documents:

| Document | Title |
|----------|-------|
| TIA-102.BCAD | Two-Slot TDMA Trunked Voice Services CAI Conformance Specification |
| TIA-102.BCAE | Two-Slot TDMA Trunked Voice Services Message and Procedures Conformance Specification |
| TIA-102.BCAF | Trunked TDMA Voice Channel Conformance Profiles |
| TIA-102.CCAA | Two-Slot TDMA Transceiver Measurement Methods |

---

## Practical Context

This document defines the message-level protocol that P25 Phase 2 TDMA radios and infrastructure must implement to interoperate. In practice:

- **Infrastructure equipment** (e.g., Motorola Solutions GTR 8000 / GCM 8000 repeaters, L3Harris VIDA base stations) implements the outbound MAC messages defined here — channel grants, status broadcasts, authentication demands, synchronization broadcasts, power control — on both the LCCH and VCH.

- **Subscriber units** (e.g., Motorola APX series, L3Harris XL series, EF Johnson VP series portable and mobile radios) implement the inbound MAC messages — voice service requests, registration requests, authentication responses, status updates — and must correctly parse all outbound messages they receive.

- **P25 Phase 2 systems** use FDMA Phase 1 control channels alongside TDMA Phase 2 voice/traffic channels. The control channel broadcasts TDMA channel identifiers and timing synchronization, and when a call is granted on a TDMA channel, the MAC messages defined in this document govern all signaling on that channel.

- **Dynamic Dual Mode (DDM)** systems dynamically assign calls as Phase 1 FDMA or Phase 2 TDMA depending on subscriber capabilities. The MFID90 manufacturer-specific messages in this document support Motorola's dynamic regrouping over the TDMA air interface.

- The message formats here are what SDR-based monitoring tools (OP25, SDRTrunk, DSD+) must decode to interpret P25 Phase 2 TDMA signaling traffic.

---

## Key Online Resources

**Standards Bodies & Official Sources**
- TIA Standards Store (purchase): https://global.ihs.com or https://standards.globalspec.com (search TIA-102.BBAD)
- Project 25 PTIG approved standards list: https://www.project25.org
- TIA call for interest on BBAD-A-1: https://tiaonline.org/standardannouncement/tia-issues-call-for-interest-on-new-project-25-two-slot-tdma-mac-layer-messages-addendum-1-location-on-ptt-and-user-alias/

**Technical References**
- GlobalSpec entry for TIA-102.BBAC (MAC Layer Spec, companion document): https://standards.globalspec.com/std/14216091/TIA-102.BBAC
- GlobalSpec entry for TSB-102.BBAA (TDMA Overview): https://standards.globalspec.com/std/1237514/tsb-102-bbaa
- Motorola Solutions P25 TDMA Standards White Paper: https://www.motorolasolutions.com/content/dam/msi/docs/business/_documents/white_paper/_static_files/p25_tdma_standard_white_paper_final.pdf

**Community & Monitoring**
- RadioReference P25 Wiki: https://wiki.radioreference.com
- RadioReference Trunked Radio Decoders page: https://wiki.radioreference.com/index.php/Trunked_Radio_Decoders
- RTL-SDR P25 Phase 2 decoding tutorial: https://www.rtl-sdr.com/tutorial-on-setting-up-op25-for-p25-phase-2-digital-voice-decoding/

**Archive**
- Internet Archive TIA-102 Series (older pre-Revision-A documents): https://archive.org/details/TIA-102_Series_Documents

---

## Open-Source Implementations

The following open-source projects implement or directly relate to decoding the MAC messages and PDUs defined in this document:

**OP25** — GNU Radio-based open-source P25 receiver supporting Phase 1 and Phase 2 TDMA decoding. The `p25p2_tdma.cc` module handles Phase 2 TDMA demodulation and MAC PDU extraction. The "boatbod" fork is the most actively maintained version.
- Osmocom repository: https://gitea.osmocom.org/op25/op25
- Boatbod fork: https://github.com/boatbod/op25

**SDRTrunk** — Cross-platform Java application for trunked radio decoding via SDR. Supports P25 Phase 1 control channels with automatic Phase 2 TDMA traffic channel following. Active development includes TDMA control channel (LCCH) decoding support (issue #1304). The P25 Phase 2 decoder parses the MAC PDU formats and message opcodes defined in this document.
- GitHub: https://github.com/DSheirer/sdrtrunk

**DSD+ (Digital Speech Decoder Plus)** — Closed-source but freely available decoder that handles P25 Phase 1 and Phase 2 TDMA audio. Not open source but widely used alongside the open-source tools above.

**Trunk-Recorder** — Open-source tool for recording trunked radio calls using SDR, with P25 Phase 2 TDMA support built on OP25 components.
- GitHub: https://github.com/robotastic/trunk-recorder

---

## Standards Lineage

```
P25 Standard (APCO Project 25)
├── TSB-102-C — Documentation Suite Overview
│
├── FDMA Common Air Interface (Phase 1)
│   ├── TIA-102.BAAA-B — FDMA CAI
│   ├── TIA-102.AABC-E — Trunking Control Channel Messages ←──┐
│   ├── TIA-102.AABD — Trunking Procedures                    │
│   ├── TIA-102.AABH — Dynamic Regrouping Messages ←────────┐ │
│   └── TIA-102.AAAD-B — Block Encryption Protocol ←──────┐ │ │
│                                                          │ │ │
├── Two-Slot TDMA (Phase 2)                                │ │ │
│   ├── TSB-102.BBAA — TDMA Overview                       │ │ │
│   ├── TIA-102.BBAB — Physical Layer Protocol Spec        │ │ │
│   ├── TIA-102.BBAC-A — MAC Layer Specification           │ │ │
│   ├── TIA-102.BBAD-A — MAC Layer Messages ◄══ THIS DOC ──┘─┘─┘
│   │   └── TIA-102.BBAD-A-1 — Addendum 1 (in development)
│   └── TIA-102.BBAE — MAC Layer Procedures
│
├── Vocoder
│   ├── TIA-102.BABA — IMBE Vocoder (Phase 1 full rate)
│   └── TIA-102.BABA-1 — Half Rate Vocoder Addendum (Phase 2)
│
├── Conformance Testing (TDMA)
│   ├── TIA-102.BCAD — CAI Conformance Specification
│   ├── TIA-102.BCAE — Message & Procedures Conformance
│   └── TIA-102.BCAF — Voice Channel Conformance Profiles
│
├── Transceiver Measurement
│   └── TIA-102.CCAA — Two-Slot TDMA Measurement Methods
│
└── Inter-RF Subsystem Interface
    ├── TIA-102.BACA — ISSI Voice/Mobility Messages
    └── TIA-102.BACD — ISSI Supplementary Data
```

Arrows (←) indicate normative references from this document to FDMA-suite companion documents. The TDMA MAC messages reuse and adapt FDMA ISP/OSP opcodes, encryption fields, and dynamic regrouping message structures from those referenced documents.
