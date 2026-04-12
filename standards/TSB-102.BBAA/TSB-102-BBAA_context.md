# TSB-102.BBAA — Related Resources & Context

## Status

This document is **active**. It appears in every published revision of the P25 Steering Committee's "Approved P25 TIA Standards" list, from the earliest available (2014) through the most recent (February 2023). It has not been superseded or revised — the March 2010 publication remains current. As a TSB (Technical Systems Bulletin), it is informative rather than normative; its role is to serve as the umbrella overview and roadmap for the Phase 2 TDMA standard suite, which means it is unlikely to be revised unless the suite itself undergoes major structural changes.

The companion normative documents it references have continued to evolve. Notably, the MAC layer spec has been consolidated into TIA-102.BBAC-A (November 2019), a TDMA MAC control channel document was published as TIA-102.BBAD-A (November 2019), and a TDMA MAC procedures document was added as TIA-102.BBAE (November 2019).

---

## Standards Family

This document sits within the TIA-102 series, which comprises the full P25 digital public safety radio standard. The TIA-102 numbering uses a four-character suffix that groups documents by functional area:

- **AAxx** — Common definitions, trunking, encryption, and link control
- **BAxx** — Phase 1 FDMA CAI, vocoder, ISSI
- **BBxx** — Phase 2 TDMA (this document's family)
- **BCxx/CBxx/CCxx** — Conformance, compliance, and measurement methods
- **CAxx** — C4FM/CQPSK transceiver measurement and performance

### Core Phase 2 TDMA Documents (BBxx family)

| Document | Title | Published |
|---|---|---|
| TSB-102.BBAA | Two-Slot TDMA Overview (this document) | Mar 2010 |
| TIA-102.BBAB | Phase 2 Two-Slot TDMA Physical Layer Protocol Specification | Jul 2009 |
| TIA-102.BBAC-A | Phase 2 Two-Slot TDMA MAC Layer Description (consolidated) | Nov 2019 |
| TIA-102.BBAD-A | Phase 2 Two-Slot TDMA MAC Layer Control Channel | Nov 2019 |
| TIA-102.BBAE | Phase 2 Two-Slot TDMA MAC Layer Procedures | Nov 2019 |

### Key Companion Documents Referenced by This Document

| Document | What It Covers |
|---|---|
| TSB-102-A | APCO Project 25 System and Standards Definition |
| TIA-102.BABA / BABA-1 | IMBE Vocoder Description / Half Rate Vocoder Annex |
| TIA-102.AABC (+ addenda) | Trunking Control Channel Messages (with TDMA addendum -B-4) |
| TIA-102.AABD | Trunking Procedures |
| TIA-102.AABF | Link Control Word Formats and Messages |
| TIA-102.AAAD | Block Encryption Protocol (with half-rate vocoder addendum) |
| TIA-102.AACA | Digital Over the Air Rekeying (OTAR) Protocol |
| TIA-102.BACA | ISSI Messages and Procedures for Voice Services |

### Testing and Compliance Documents

| Document | What It Covers |
|---|---|
| TIA-102.CCAA | Two-Slot TDMA Transceiver Measurement Methods |
| TIA-102.BCAD | TDMA Trunked Voice Services CAI Conformance |
| TIA-102.BCAE | TDMA Trunked Voice Services Message & Procedures Conformance |
| TIA-102.BCAF | Trunked TDMA Voice Channel Conformance Profiles |

---

## Practical Context

P25 Phase 2 TDMA is deployed extensively across U.S. public safety systems. Major statewide and metropolitan trunked radio systems (e.g., many state police networks, large county/city systems) have migrated or are migrating traffic channels to Phase 2 TDMA while retaining Phase 1 FDMA control channels — exactly the hybrid architecture this document describes.

The two primary infrastructure vendors implementing this standard are Motorola Solutions (ASTRO 25) and L3Harris (VIDA/EAGLE). Both support the Phase 1 control channel with Phase 2 TDMA traffic channels as the standard migration path. The subscriber radios from both vendors (e.g., Motorola APX series, L3Harris XL series) implement the dual-mode Phase 1 FDMA / Phase 2 TDMA operation described in Section 10 of this document.

The H-CPM (inbound) and H-DQPSK (outbound) modulation asymmetry described in Section 6 is a practical engineering tradeoff visible in the field — portable radios use constant-envelope H-CPM to maximize battery-powered RF amplifier efficiency, while base stations use linear H-DQPSK to improve simulcast delay spread tolerance in large-area systems.

The GTR 8000 base station (Motorola) is one of the FCC-certified infrastructure platforms implementing the TDMA physical layer described in the companion TIA-102.BBAB specification.

---

## Key Online Resources

### Standards Catalogs and Registries
- **GlobalSpec / Accuris Standards Store** — Official purchase portal for TIA-102 standards  
  https://standards.globalspec.com/std/1237514/tsb-102-bbaa
- **P25 Approved TIA Standards List** (Project 25 Steering Committee, most recent available)  
  https://www.project25.org/images/stories/ptig/P25%20SC%2023-02-001-R1%20P25%20TIA%20Standards_Approved_16Feb2023.pdf
- **P25 Document Suite Reference** (QSL.net mirror of Jan 2010 edition)  
  https://www.qsl.net/kb9mwr/projects/dv/apco25/P25_Standards.pdf

### Vendor and Industry White Papers
- **Motorola Solutions — P25 TDMA Trunking Suite White Paper (July 2013)**  
  https://www.motorolasolutions.com/content/dam/msi/Products/apx-p25/P25_TDMA_Trunking_Standard_White_Paper.pdf

### Community and Reference Sites
- **RadioReference.com** — Extensive P25 system databases with Phase 1/Phase 2 identification  
  https://www.radioreference.com
- **RTL-SDR.com — P25 Phase 2 tag** — Tutorials and news on decoding Phase 2 TDMA with SDR hardware  
  https://www.rtl-sdr.com/tag/p25-phase-2/
- **Internet Archive — TIA-102 Series Documents collection**  
  https://archive.org/details/TIA-102_Series_Documents

---

## Open-Source Implementations

Several open-source projects implement decoders and tools for the P25 Phase 2 TDMA air interface described by this document and its companion normative specifications:

### OP25 (boatbod fork)
Full P25 Phase 1 and Phase 2 TDMA decoder using GNU Radio and RTL-SDR hardware. Supports trunking, Phase 2 TDMA traffic channel following, Phase 2 tone synthesis, and AMBE half-rate vocoder decoding. Currently the most complete open-source Phase 2 implementation.  
https://github.com/boatbod/op25

### SDRTrunk
Cross-platform Java application for decoding and recording trunked radio. Supports P25 Phase 2 TDMA traffic channel decoding (two-timeslot), Phase 1 control channel parsing of TDMA-related messages (timing sync, channel identifiers), and AMBE vocoder via the JMBE library. Has ongoing work on Phase 2 TDMA control channel support.  
https://github.com/DSheirer/sdrtrunk

### Trunk-Recorder
C++/GNU Radio based P25 trunked system recorder with Phase 2 TDMA support. Parses TDMA frequency table identifiers (slots_per_carrier, phase2_tdma flags) from control channel messages and records TDMA traffic channels.  
https://github.com/TrunkRecorder/trunk-recorder

### Pocket25
Mobile P25 decoder using DSD-Neo as the backend. Has P25 Phase 2 UI support though audio quality is noted as experimental.  
https://github.com/SarahRoseLives/Pocket25

### DSD / DSD-FME
Digital Speech Decoder — the original open-source project for P25 and other digital voice modes. Various forks have added varying levels of Phase 2 support.  
https://github.com/lwvmobile/dsd-fme

---

## Standards Lineage

```
APCO Project 25 (1989–present)
│
├── TSB-102-A — System and Standards Definition (1995)
│   └── Statement of Requirements (2008)
│
├── Phase 1 — FDMA 12.5 kHz (Um interface)
│   ├── TIA-102.BAAA — FDMA CAI
│   ├── TIA-102.BABA — IMBE Vocoder (Full Rate, 7.2 kb/s)
│   ├── TIA-102.AABC — Trunking Control Channel Messages
│   ├── TIA-102.AABD — Trunking Procedures
│   ├── TIA-102.AABF — Link Control Word Formats
│   ├── TIA-102.AAAD — Block Encryption Protocol
│   ├── TIA-102.AACA — OTAR Protocol
│   ├── TIA-102.BACA — ISSI Messages and Procedures
│   └── TIA-102.CAAA — C4FM/CQPSK Measurement Methods
│
├── Phase 2 — Two-Slot TDMA 12.5 kHz (Um2 interface)
│   ├── TSB-102.BBAA — TDMA Overview ◄ THIS DOCUMENT
│   ├── TIA-102.BBAB — TDMA Physical Layer (PHY)
│   ├── TIA-102.BBAC-A — TDMA MAC Layer
│   ├── TIA-102.BBAD-A — TDMA MAC Control Channel
│   ├── TIA-102.BBAE — TDMA MAC Procedures
│   ├── TIA-102.BABA-1 — Half Rate Vocoder Annex (3.6 kb/s)
│   ├── TIA-102.AABC addenda — Trunking Messages for TDMA
│   ├── TIA-102.AAAD addenda — Encryption for Half Rate
│   ├── TIA-102.CCAA — TDMA Measurement Methods
│   ├── TIA-102.BCAD — TDMA CAI Conformance
│   ├── TIA-102.BCAE — TDMA Messages Conformance
│   └── TIA-102.BCAF — TDMA Conformance Profiles
│
└── ISSI / Wireline
    ├── TIA-102.BACA — ISSI Voice Services (with Phase 2 addenda)
    └── TIA-102.BACD — ISSI Supplementary Data
```
