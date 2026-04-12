# TIA-102.BBAC-A — Related Resources & Context

## Status

This document is **active** as of the latest P25 Steering Committee approved standards list (February 2023). It was published in November 2019, approved by ANSI in June 2019, and adopted by the P25 Steering Committee on October 10, 2019. It supersedes and cancels the original TIA-102.BBAC (December 2010). The addendum TIA-102.BBAC-1 (February 2013) was absorbed into this revision.

No successor revision has been published. The document remains current P25 Phase 2 normative material.

## Standards Family

This document is part of the TIA-102 "BB" series, which covers the P25 Phase 2 Two-Slot TDMA air interface. It sits at the MAC layer between the physical layer below and the trunking/messaging layers above.

### Direct Companion Documents

| Document | Title / Scope |
|----------|--------------|
| TIA-102.BBAB | **Physical Layer Protocol Specification** — Defines H-DQPSK modulation, symbol timing, ramp/guard, and other PHY-layer parameters for the Um2 interface. This document's burst structures reference BBAB extensively. |
| TIA-102.BBAD-A | **Two-Slot TDMA MAC Layer Messages** — Defines all MAC PDU message formats (VCU, MAC_PTT, MAC_END_PTT, MAC_IDLE, etc.) carried in the burst fields this document specifies. |
| TIA-102.BBAE | **Two-Slot TDMA MAC Layer Procedures** — Defines call setup/teardown state machines, SACCH access priority, FACCH usage rules, and other procedural behavior. |
| TIA/TSB-102.BBAA | **Two-Slot TDMA Overview** — Informative overview of the entire Phase 2 TDMA suite. |

### Key Cross-Referenced Documents

| Document | Title / Scope |
|----------|--------------|
| TIA-102.BAAA-B | **FDMA Common Air Interface** — Phase 1 FDMA CAI; the VCH and control channel architectures in this document are based on FDMA precedents. |
| TIA-102.AABC-E | **Trunking Control Channel Messages** — Defines SYNC_BCST TSBK used for FDMA-to-TDMA time alignment. |
| TIA-102.AABB-B | **Trunking Control Channel Formats** — Defines TSBK OSP formats referenced for SYNC_BCST conveyance. |
| TIA-102.AABD-B | **Trunking Procedures** — Provides FDMA trunking procedures that TDMA LCCH procedures extend. |
| TIA-102.AAAD-B | **Block Encryption Protocol** — Defines ALGID, KID, MI used in the ESS fields this document specifies. |
| TIA-102.BABA-A | **Vocoder Description** — Defines the half-rate AMBE vocoder whose 20ms frames are packed into the 4V/2V bursts. |
| TIA/TSB-102-C | **Documentation Suite Overview** — Master index of the entire TIA-102 standard family. |

### Conformance & Test Documents

| Document | Scope |
|----------|-------|
| TIA-102.BCAD | Two-Slot TDMA CAI Conformance Specification |
| TIA-102.BCAE | Two-Slot TDMA Messages and Procedures Conformance Specification |
| TIA-102.BCAF | Trunked TDMA Voice Channel Conformance Profiles |
| TIA-102.CCAA | Two-Slot TDMA Transceiver Measurement Methods |

## Practical Context

P25 Phase 2 TDMA is deployed primarily by large public safety agencies (state police, major metro areas) to double voice channel capacity on existing 12.5 kHz allocations. The MAC layer defined in this document governs how every Phase 2 subscriber radio and base station repeater structures its over-the-air transmissions.

Motorola Solutions (APX series), L3Harris (XL series), and Kenwood/JVCKENWOOD (VP/NX series) all implement this specification in their P25 Phase 2 capable radios. Base station infrastructure from Motorola (GTR 8000), L3Harris (VIDA), and other vendors implements the FNE side.

Key real-world behaviors governed by this document include: how quickly a radio can begin transmitting after being granted a TDMA traffic channel (ISCH sync acquisition), how encryption keys are delivered within the voice stream (ESS distribution across 4V/2V bursts), and how SACCH signaling coexists with voice without interrupting audio (inverted timeslot design).

## Key Online Resources

- **P25 Steering Committee Standards List** — Official list of all approved P25/TIA-102 standards with revision status:
  https://www.project25.org/images/stories/ptig/P25%20SC%2023-02-001-R1%20P25%20TIA%20Standards_Approved_16Feb2023.pdf

- **TIA Standards Store (Accuris/Techstreet)** — Purchase point for official TIA-102 standards:
  https://store.accuristech.com/tia

- **GlobalSpec Standards Listing for TIA-102.BBAC** — Free metadata, scope, and cross-references:
  https://standards.globalspec.com/std/14216091/TIA-102.BBAC

- **Internet Archive TIA-102 Series Collection** — Historical/reference copies of various TIA-102 documents:
  https://archive.org/details/TIA-102_Series_Documents

- **RadioReference P25 Wiki** — Community-maintained technical reference for P25 systems, frequencies, and decoding:
  https://wiki.radioreference.com/index.php/Project_25

- **OP25 Wiki on RadioReference** — Guide to the OP25 open-source P25 decoder including Phase 2 TDMA:
  https://wiki.radioreference.com/index.php/OP25

## Open-Source Implementations

These projects implement decoders or encoders that directly relate to the burst structures, DUID values, ISCH encoding, Reed-Solomon FEC, sync sequences, and voice frame packing defined in this document.

| Project | Description | URL |
|---------|-------------|-----|
| **SDRTrunk** | Java-based trunked radio decoder with full P25 Phase 1 & Phase 2 TDMA traffic channel decoding. Implements ISCH decoding, DUID parsing, TDMA burst demodulation, and ESS extraction. | https://github.com/DSheirer/sdrtrunk |
| **OP25 (Boatbod fork)** | GNURadio-based P25 decoder supporting Phase 1 and Phase 2 TDMA. DQPSK demodulation chain, ISCH/sync detection, and voice decoding with AMBE. | https://github.com/boatbod/op25 |
| **OP25 (Osmocom)** | Original OP25 codebase from Osmocom. Foundation for most open-source P25 Phase 2 work. | https://gitea.osmocom.org/op25/op25 |
| **DSD (Digital Speech Decoder)** | C-based decoder for multiple digital voice protocols including P25 Phase 1 and Phase 2 (X2-TDMA). | https://github.com/szechyjs/dsd |
| **JMBE** | Java library for IMBE/AMBE vocoder decoding, used by SDRTrunk for P25 Phase 1 (IMBE) and Phase 2 (AMBE) audio. | https://github.com/DSheirer/jmbe |
| **dvmhost** | MMDVM-based Digital Voice Modem firmware/host supporting P25, DMR, and NXDN trunking — implements P25 Phase 2 TDMA at the infrastructure level. | https://github.com/DVMProject/dvmhost |
| **Trunk Recorder** | GNURadio-based P25 trunking recorder that follows control channels and records traffic channels, including Phase 2 TDMA. | https://github.com/robotastic/trunk-recorder |

Notable implementation detail: SDRTrunk issue #1304 documents real-world differences between Motorola and L3Harris implementations of the I-ISCH LCH Type field (bits 8:7) — Motorola traffic channels leave these as %00 (legacy VCH), while L3Harris TDMA control channels use %11 (LCCH) as defined in Revision A of this document.

## Standards Lineage

```
TIA/TSB-102-C (Documentation Suite Overview)
├── TIA-102.BAAA-B (FDMA CAI — Phase 1)
│   ├── TIA-102.AABB-B (Trunking Control Channel Formats)
│   ├── TIA-102.AABC-E (Trunking Control Channel Messages)
│   ├── TIA-102.AABD-B (Trunking Procedures)
│   └── TIA-102.AAAD-B (Block Encryption Protocol)
│
├── TIA/TSB-102.BBAA (Two-Slot TDMA Overview — Phase 2)
│   │
│   ├── TIA-102.BBAB (PHY Layer Protocol)
│   │
│   ├── TIA-102.BBAC-A ◄── THIS DOCUMENT (MAC Layer Spec)
│   │   ├── supersedes TIA-102.BBAC (2010)
│   │   └── absorbs TIA-102.BBAC-1 (2013 Addendum)
│   │
│   ├── TIA-102.BBAD-A (MAC Layer Messages)
│   │
│   └── TIA-102.BBAE (MAC Layer Procedures)
│
├── TIA-102.BABA-A (Vocoder — Half-Rate AMBE)
│
├── Conformance & Test
│   ├── TIA-102.BCAD (TDMA CAI Conformance)
│   ├── TIA-102.BCAE (TDMA Messages/Procedures Conformance)
│   ├── TIA-102.BCAF (TDMA Voice Channel Conformance Profiles)
│   └── TIA-102.CCAA (TDMA Transceiver Measurement Methods)
│
└── Inter-RF Subsystem / ISSI
    ├── TIA-102.BACA (ISSI Messages & Procedures)
    └── TIA-102.BACD (ISSI Supplementary Data)
```
