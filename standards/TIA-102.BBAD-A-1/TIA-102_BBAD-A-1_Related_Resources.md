# Related Resources & Context

## Status

This document is **active** as of its March 2025 publication date. It is an addendum to TIA-102.BBAD-A (published November 2019), meaning it does not replace the base standard but augments it with three new MAC messages. The base standard remains current and in force. TIA has indicated that the next full revision of TIA-102.BBAD-A will incorporate the changes from this addendum directly.

A companion addendum, TIA-102.BBAE-A-1 (Two-Slot TDMA MAC Layer Procedures Addendum 1 — Location on PTT and User Alias), was also issued via a parallel Call for Interest in October 2024. That document covers the procedural aspects (state machines, timing, sequencing) for these same features, while this document covers only the message formats.

---

## Standards Family

This document sits within the P25 Phase 2 Two-Slot TDMA suite of TIA-102 standards. The key companion documents are:

| Document | Title / Scope |
|---|---|
| TSB-102.BBAA | Two-Slot TDMA Overview — high-level description of the Phase 2 TDMA standard suite |
| TIA-102.BBAB | Two-Slot TDMA Physical Layer Protocol Specification — H-CPM/H-DQPSK modulation, symbol timing, slot structure |
| TIA-102.BBAC / BBAC-A / BBAC-1 | Two-Slot TDMA MAC Layer Description — architecture, logical channels, PDU structures, SACCH/FACCH framing |
| **TIA-102.BBAD-A** | **Two-Slot TDMA MAC Layer Messages — the base standard this document amends; defines all MAC PDU formats and opcodes** |
| TIA-102.BBAE | Two-Slot TDMA MAC Layer Procedures — state machines, call setup/teardown, registration, channel assignment procedures |
| TIA-102.BCAD | Phase 2 TDMA Trunked Voice Services CAI Conformance Specification — conformance test requirements |
| TIA-102.BCAE | Phase 2 TDMA Voice Services Message and Procedures Conformance Specification |
| TIA-102.BCAF | Trunked TDMA Voice Channel Conformance Profiles |

Related standards outside the TDMA suite that this document references or interacts with:

| Document | Relevance |
|---|---|
| TIA-102.BAJC / BAJC-A | Tier 2 Location Services Specification (LRRP) — defines the location encoding used by the MFID90 message's lat/lon fields |
| TIA-102.BAJB | Tier 1 Location Services (NMEA 0183) — simpler GPS sentence-based location |
| TIA-102.BAAA-A | Phase 1 FDMA CAI — the Phase 1 air interface that Phase 2 systems still use for control channels |
| TIA-102.AABC-B/C | Trunking Control Channel Messages — control channel message formats used alongside these voice channel messages |
| TIA-102.AABF-A | Link Control Word Formats and Messages — Phase 1 LCW equivalents of some of these features |
| TIA-102.AAAD-A | Block Encryption Protocol — referenced by the P bit (encryption state) in the MFID90 message |
| TIA-102.BABA / BABA-1 | IMBE/AMBE Vocoder Description — the voice codecs that run alongside these MAC messages |

---

## Practical Context

**Location on PTT** allows a P25 subscriber radio to automatically transmit its GPS coordinates as part of the MAC signaling whenever the user presses the push-to-talk button on a TDMA voice channel. This gives dispatchers and other units real-time position awareness without requiring a separate data channel location query. Two encoding variants exist: MFID $A4 (used by L3Harris systems) uses a degrees-minutes format with high-resolution time/speed/course, while MFID $90 (used by Motorola systems) uses the LRRP binary encoding with coarser resolution but includes group/source addressing inline. Both are inbound (radio-to-infrastructure) messages carried on the SACCH.

**User Alias** allows the system infrastructure to broadcast a 14-character ASCII name for the currently talking user on the SACCH, so receiving radios can display a human-readable identifier instead of (or alongside) a numeric radio ID. This is an outbound (infrastructure-to-radio) message.

These features are relevant to any P25 Phase II TDMA deployment — primarily large public safety trunked systems operated by state, county, and municipal agencies using infrastructure from Motorola Solutions (ASTRO 25 / APX), L3Harris (VIDA / XL series), or EF Johnson (Viking). The two different manufacturer IDs ($90 and $A4) reflect the multi-vendor reality of P25: both vendors defined proprietary extensions that are now being standardized for interoperability.

---

## Key Online Resources

- **TIA Call for Interest announcement (Oct 2024):**
  https://tiaonline.org/standardannouncement/tia-issues-call-for-interest-on-new-project-25-two-slot-tdma-mac-layer-messages-addendum-1-location-on-ptt-and-user-alias/

- **Project 25 approved standards list (P25 PTIG):**
  https://www.project25.org/images/stories/ptig/P25%20SC%2023-02-001-R1%20P25%20TIA%20Standards_Approved_16Feb2023.pdf

- **P25 Capabilities Guide (PTIG):**
  https://project25.org/images/stories/ptig/docs/PTIG_P25Capabilities_Guide_v1.7.pdf

- **Motorola P25 Phase 2 TDMA Standards white paper:**
  https://www.motorolasolutions.com/content/dam/msi/docs/business/_documents/white_paper/_static_files/p25_tdma_standard_white_paper_final.pdf

- **TIA-102 Series Documents (Internet Archive collection — older versions of many TIA-102 standards):**
  https://archive.org/details/TIA-102_Series_Documents

- **GlobalSpec standards index for TIA-102.BBAC (MAC Layer Description):**
  https://standards.globalspec.com/std/14216091/TIA-102.BBAC

- **GlobalSpec listing for TSB-102.BBAA (TDMA Overview):**
  https://standards.globalspec.com/std/1237514/tsb-102-bbaa

- **TIA standards purchase (Accuris/Techstreet):**
  https://store.accuristech.com/tia

- **RadioReference P25 forums (community discussion of TDMA decoding, talker alias, GPS):**
  https://forums.radioreference.com/

---

## Open-Source Implementations

Several open-source projects implement P25 Phase 2 TDMA MAC layer decoding, including handling of manufacturer-specific messages like those defined in this document:

- **SDRTrunk** — Java-based cross-platform SDR application with full P25 Phase 1 & 2 decoding, including talker alias/GPS support and MFID-specific message parsing. Version 0.6.1 added expanded P25 Phase 2 message decoding with talker alias and GPS location support.
  - GitHub: https://github.com/DSheirer/sdrtrunk
  - Releases: https://github.com/DSheirer/sdrtrunk/releases
  - P25 Phase 2 TDMA issues/development: https://github.com/DSheirer/sdrtrunk/issues/1304

- **OP25 (boatbod fork)** — GNURadio-based P25 decoder with Phase 2 TDMA support. The `p25p2_tdma.cc` module processes MAC PDUs on TDMA voice channels, including manufacturer-specific opcodes. The codebase handles SACCH MAC message extraction from the TDMA slot structure.
  - GitHub: https://github.com/boatbod/op25
  - TDMA decoder source: https://github.com/boatbod/op25/blob/master/op25/gr-op25_repeater/lib/p25p2_tdma.cc

- **DSD+ (closed-source but widely used)** — Digital Speech Decoder Plus; supports P25 Phase 2 TDMA and LRRP location decoding. Not open-source but frequently discussed alongside open-source tools.

---

## Standards Lineage

```
TIA-102 (P25 Standard Suite)
├── Phase 1 FDMA CAI
│   ├── TIA-102.BAAA-A ── FDMA Common Air Interface
│   ├── TIA-102.AABF-A ── Link Control Word Formats & Messages
│   ├── TIA-102.AABC-B/C ── Trunking Control Channel Messages
│   └── TIA-102.AAAD-A ── Block Encryption Protocol
│
├── Phase 2 Two-Slot TDMA CAI
│   ├── TSB-102.BBAA ── TDMA Overview (informational)
│   ├── TIA-102.BBAB ── Physical Layer Protocol Specification
│   │                    (H-CPM inbound, H-DQPSK outbound, slot structure)
│   ├── TIA-102.BBAC-A ── MAC Layer Description
│   │   └── TIA-102.BBAC-1 ── MAC Layer Description Addendum 1
│   ├── TIA-102.BBAD-A ── MAC Layer Messages  ◄── BASE STANDARD
│   │   └── TIA-102.BBAD-A-1 ── MAC Layer Messages Addendum 1  ◄── THIS DOCUMENT
│   │                            (Location on PTT + User Alias message formats)
│   ├── TIA-102.BBAE ── MAC Layer Procedures
│   │   └── TIA-102.BBAE-A-1 ── MAC Layer Procedures Addendum 1
│   │                            (Location on PTT + User Alias procedures)
│   └── Conformance Testing
│       ├── TIA-102.BCAD ── CAI Conformance Specification
│       ├── TIA-102.BCAE ── Message & Procedures Conformance
│       └── TIA-102.BCAF ── Voice Channel Conformance Profiles
│
├── Location Services
│   ├── TIA-102.BAJB ── Tier 1 Location (NMEA 0183)
│   └── TIA-102.BAJC-A ── Tier 2 Location (LRRP)  ◄── Referenced by MFID90 lat/lon encoding
│
├── Vocoder
│   ├── TIA-102.BABA ── IMBE Vocoder (Phase 1 full-rate)
│   └── TIA-102.BABA-1 ── Half-Rate Vocoder Addendum (Phase 2 AMBE)
│
└── Infrastructure Interfaces
    ├── TIA-102.BAHA ── Fixed Station Interface
    ├── TIA-102.BACA ── Inter-RFSS Interface (ISSI)
    └── TIA-102.BACD ── ISSI Supplementary Data
```
