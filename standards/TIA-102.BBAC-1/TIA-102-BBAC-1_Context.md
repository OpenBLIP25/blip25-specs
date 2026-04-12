# TIA-102.BBAC-1 — Related Resources & Context

## Status

This document was published in February 2013. According to the P25 Steering Committee's approved standards list (February 2023), this addendum has been **absorbed into TIA-102.BBAC-A**, the consolidated "A" revision of the base MAC layer specification. The addendum is therefore no longer a standalone active document — its content has been folded into the current revision. Implementers should reference TIA-102.BBAC-A (available from the TIA standards store) for the authoritative current text.

The base document TIA-102.BBAC was first published in December 2010 and received ANSI approval in November 2012. This addendum followed in February 2013 (ANSI approved January 2013).

## Standards Family

This document is part of the P25 Phase 2 Two-Slot TDMA standards suite within the broader TIA-102 series. The TDMA suite defines the air interface for doubling voice channel capacity relative to Phase 1 FDMA within 12.5 kHz channels.

### Core Phase 2 TDMA Documents

| Document | Title | Role |
|---|---|---|
| TSB-102.BBAA | P25 Two-Slot TDMA Overview | Overview and architecture of the TDMA suite |
| TIA-102.BBAB | Phase 2 Two-Slot TDMA Physical Layer Protocol Specification | PHY layer: H-DQPSK modulation, symbol rate, burst structure |
| TIA-102.BBAC / BBAC-A | Phase 2 Two-Slot TDMA MAC Layer Description | **This document's parent.** MAC layer: PDU formats, call procedures, scrambling, messages |
| TIA-102.BBAD | Phase 2 Two-Slot TDMA MAC Layer Control Channel | TDMA control channel operation (published August 2017) |

### Key Referenced/Companion Documents

| Document | Title | Relationship |
|---|---|---|
| TIA-102.AABC (series) | Trunking Control Channel Messages | Control channel grant/assignment messages that initiate TDMA VCH calls |
| TIA-102.AAAD-A | Block Encryption Protocol | Encryption applied to voice on TDMA channels |
| TIA-102.BABA / BABA-1 | IMBE/Half-Rate Vocoder Description | AMBE vocoder used in Phase 2 TDMA voice |
| TIA-102.AABF-A | Link Control Word Formats and Messages | LCW definitions referenced by MAC messages |
| TIA-102.BAAA-A | FDMA Common Air Interface | Phase 1 FDMA CAI (coexists with TDMA in hybrid systems) |

### Conformance and Test Documents

| Document | Title |
|---|---|
| TIA-102.BCAD | Phase 2 TDMA CAI Conformance Specification |
| TIA-102.BCAE | Phase 2 TDMA Messages and Procedures Conformance Specification |
| TIA-102.BCAF | Trunked TDMA Voice Channel Conformance Profiles |
| TIA-102.CCAA | Two-Slot TDMA Transceiver Measurement Methods |

## Practical Context

P25 Phase 2 TDMA is deployed on public safety radio systems across the United States and internationally. The MAC layer defined by this document and its parent governs how subscriber radios and Fixed Network Equipment (FNE) coordinate voice calls on TDMA traffic channels — including call setup (MAC_PTT exchanges), voice traffic mode (SACCH-based talker identification), call teardown (MAC_END_PTT), hangtime/message trunking, and audio preemption for emergency overrides.

In practice, this standard is implemented by infrastructure vendors (Motorola Solutions, L3Harris, EF Johnson/Kenwood) in their base station repeaters (BRs) and site controllers, and by subscriber radio manufacturers in portable and mobile units. The scrambling mechanism defined in Section 7.2.5 — seeded from WACN ID, System ID, and Color Code — is critical for co-channel interference rejection between adjacent P25 systems sharing frequencies.

The MAC message table (Table 8-2) is directly used by open-source decoders to parse TDMA traffic channel signaling and identify talkgroups, source/target unit IDs, and encryption parameters from over-the-air captures.

## Key Online Resources

- **TIA Standards Store** — Purchase official copies: https://store.accuristech.com/tia
- **Project 25 PTIG** — Approved standards lists and compliance documents: https://www.project25.org
- **GlobalSpec** — Document metadata and cross-references: https://standards.globalspec.com/std/1291609/TIA-102.BBAC
- **Internet Archive** — Historical TIA-102 series collection: https://archive.org/details/TIA-102_Series_Documents
- **RadioReference Wiki** — Community documentation on P25 systems and monitoring: https://wiki.radioreference.com/
- **Motorola P25 TDMA Standards Whitepaper** — https://www.motorolasolutions.com/content/dam/msi/docs/business/_documents/white_paper/_static_files/p25_tdma_standard_white_paper_final.pdf

## Open-Source Implementations

Several open-source projects implement P25 Phase 2 TDMA decoding, directly using the MAC message definitions, scrambling algorithms, and burst structures specified in this document:

- **OP25 (Osmocom)** — GNU Radio-based P25 decoder with Phase 2 TDMA support. The `p25p2_tdma.cc` module implements the TDMA slot processing, scrambling, and MAC message parsing defined by this standard.
  - https://gitea.osmocom.org/op25/op25

- **OP25 (Boatbod fork)** — Actively maintained fork with enhanced TDMA decoding, DQPSK demodulation, and trunking logic.
  - https://github.com/boatbod/op25

- **SDRTrunk** — Java-based cross-platform P25 decoder. Implements Phase 2 TDMA traffic channel decoding including MAC message parsing, ESS extraction, and AMBE audio decoding. Recent work (Issue #1304) added TDMA control channel support.
  - https://github.com/DSheirer/sdrtrunk

- **Trunk-Recorder** — Automated multi-channel P25 recorder built on GNU Radio, supporting Phase 2 TDMA traffic channel recording.
  - https://github.com/robotastic/trunk-recorder

## Standards Lineage

```
TIA-102 (Project 25)
├── Phase 1 FDMA
│   ├── TIA-102.BAAA-A     FDMA Common Air Interface
│   ├── TIA-102.AABC (series)  Trunking Control Channel Messages
│   ├── TIA-102.AABF-A     Link Control Word Formats
│   ├── TIA-102.AAAD-A     Block Encryption Protocol
│   └── TIA-102.BABA / BABA-1  IMBE Vocoder / Half-Rate Vocoder
│
├── Phase 2 TDMA
│   ├── TSB-102.BBAA        TDMA Overview
│   ├── TIA-102.BBAB        TDMA Physical Layer (PHY)
│   ├── TIA-102.BBAC        TDMA MAC Layer ◄── base document
│   │   ├── TIA-102.BBAC-1  TDMA MAC Layer Addendum 1 ◄── THIS DOCUMENT
│   │   └── TIA-102.BBAC-A  TDMA MAC Layer (consolidated, absorbs BBAC-1)
│   ├── TIA-102.BBAD        TDMA MAC Layer Control Channel
│   └── Conformance
│       ├── TIA-102.BCAD    TDMA CAI Conformance
│       ├── TIA-102.BCAE    TDMA Messages/Procedures Conformance
│       ├── TIA-102.BCAF    TDMA Voice Channel Conformance Profiles
│       └── TIA-102.CCAA    TDMA Transceiver Measurement Methods
│
└── Inter-System
    ├── TIA-102.BACA (series)  ISSI Messages and Procedures
    └── TIA-102.BAHA        Fixed Station Interface
```
