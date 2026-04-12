# TIA-102.BBAE-1 — Related Resources & Context

## Status

This document is **active** as of its March 2025 publication date. It is Addendum 1 to TIA-102.BBAE and will be incorporated into the next full revision of that base standard. No superseding document has been published. The addendum was approved for publication in January 2025 by the TIA TR-8.10 Air Interface Subcommittee.

A companion document, TIA-102.BBAD-A-1 (MAC Layer Messages Addendum 1 — Location on PTT and User Alias), was issued via a TIA call for interest in October 2024 and defines the actual MAC PDU formats that this document's procedures reference.

---

## Standards Family

This document sits within the P25 Phase 2 Two-Slot TDMA document suite, which itself is part of the broader TIA-102 series for APCO Project 25 digital Land Mobile Radio (LMR). The key related documents are:

### Direct Dependencies (Normative References)

| Document | Title / Scope |
|---|---|
| **TIA-102.BBAE** | Two-Slot TDMA MAC Layer Procedures (base document this addendum modifies) |
| **TIA-102.BBAD-A-1** | Two-Slot TDMA MAC Layer Messages Addendum 1 — Location on PTT (defines MAC PDU formats for LoPTT and User Alias) |
| **TIA-102.BBAD-A** | Two-Slot TDMA MAC Layer Messages (base MAC message definitions) |
| **TIA-102.AABC-E-1** | Trunking Control Channel Messages Addendum 1 — Location on PTT (defines control channel signaling, including MOT_STS_BCST) |
| **TIA-102.AABD-B** | Trunking Procedures (call setup, channel assignment procedures) |
| **TIA-102.BAEB-C** | IP Data Bearer Service Specification (context activation for trunked data) |
| **TIA-102.BAJC-B** | Tier 2 Location Services Specification (location registration procedures) |

### TDMA Suite Siblings

| Document | Title / Scope |
|---|---|
| **TSB-102.BBAA** | Two-Slot TDMA Overview (high-level architecture and scope) |
| **TIA-102.BBAB** | TDMA Physical Layer Protocol Specification (H-CPM/H-DQPSK modulation, 12 kb/s gross rate) |
| **TIA-102.BBAC / BBAC-A** | TDMA MAC Layer Description (architecture, logical channels, Um2 interface) |

### Conformance Testing

| Document | Title / Scope |
|---|---|
| **TIA-102.BCAD** | Two-Slot TDMA Trunked Voice Services CAI Conformance Specification |
| **TIA-102.BCAE** | Two-Slot TDMA Voice Services Message and Procedures Conformance Specification |
| **TIA-102.BCAF** | Trunked TDMA Voice Channel Conformance Profiles |

### Inter-System Interfaces (ISSI/CSSI)

| Document | Title / Scope |
|---|---|
| **TIA-102.BACA-B** | ISSI Messages and Procedures for Voice and Mobility Management |
| **TIA-102.BACD** | ISSI Messages and Procedures for Supplementary Data |

Note: This document explicitly states that Location on PTT is **not** conveyed over ISSI or CSSI in the current specification.

---

## Practical Context

### What Problem Does This Solve?

In operational public safety radio systems, dispatchers monitoring group calls can hear a field unit's voice but cannot determine the unit's physical location from the radio transmission alone. Location on PTT (LoPTT) closes this gap by piggybacking GPS coordinates onto TDMA voice channel MAC signaling, delivered to dispatch console mapping applications in near-real-time.

User Alias solves a similar usability problem: listening radios display a subscriber's unit ID (a numeric identifier), but without a pre-loaded alias table, the listener doesn't know who is speaking. User Alias embeds a 14-character human-readable name from the FNE into the outbound voice stream.

### Equipment and Systems

**Infrastructure:** This standard applies to P25 Phase 2 TDMA trunked systems. The two manufacturer-specific implementations correspond to the two dominant P25 infrastructure vendors:

- **MFID 0xA4 (L3Harris):** Used in L3Harris VIDA and predecessor Harris EDACS/OpenSky P25 infrastructure. The MFIDA4 variant supports roaming and works across any voice call type.
- **MFID 0x90 (Motorola Solutions):** Used in Motorola Solutions ASTRO 25 infrastructure. The MFID90 variant requires additional preconditions (data context activation, Tier-2 location registration) and assumes home-system operation.

**Subscriber Radios:** GPS-equipped P25 Phase 2 capable portable and mobile radios from both vendors. Radios must have LoPTT enabled in their codeplug (programming) and have an active GPS receiver.

**Dispatch Consoles:** Mapping applications integrated with dispatch consoles (e.g., Motorola CommandCentral, L3Harris Symphony) receive the location data via the network and plot subscriber positions.

### Backward Compatibility

The standard is designed so that legacy sites that cannot decode the new MAC messages will simply ignore them and continue conveying voice normally. This ensures a non-disruptive rollout on mixed-vintage infrastructure.

---

## Key Online Resources

| Resource | URL |
|---|---|
| TIA Standards Store (purchase) | https://store.accuristech.com/tia |
| TIA Online — Standards Home | https://tiaonline.org |
| Project 25 Technology Interest Group (PTIG) | https://www.project25.org |
| P25 Approved TIA Standards List | https://www.project25.org/images/stories/ptig/ |
| DIN Media — This Document Listing | https://www.dinmedia.de/en/standard/eia-tia-102-bbae-1/391027750 |
| GlobalSpec — TIA-102.BBAC (TDMA MAC Description) | https://standards.globalspec.com/std/1291609/TIA-102.BBAC |
| GlobalSpec — TSB-102.BBAA (TDMA Overview) | https://standards.globalspec.com/std/1237514/tsb-102-bbaa |
| Motorola P25 TDMA Standards White Paper | https://www.motorolasolutions.com/content/dam/msi/docs/business/_documents/white_paper/_static_files/p25_tdma_standard_white_paper_final.pdf |
| Internet Archive — TIA-102 Series Documents | https://archive.org/details/TIA-102_Series_Documents |
| RadioReference Wiki — P25 | https://wiki.radioreference.com/index.php/OP25 |
| TIA Call for Interest — TIA-102.BBAD-A-1 | https://tiaonline.org/standardannouncement/tia-issues-call-for-interest-on-new-project-25-two-slot-tdma-mac-layer-messages-addendum-1-location-on-ptt-and-user-alias/ |

---

## Open-Source Implementations

No open-source project currently implements the specific LoPTT or User Alias MAC messages defined by this addendum, as the standard was only published in March 2025. However, several projects implement P25 Phase 2 TDMA MAC layer decoding at the level where these messages would appear:

| Project | Description | URL |
|---|---|---|
| **SDRTrunk** | Java-based P25 Phase 1 & Phase 2 decoder with TDMA CC support, traffic channel following, and GPS position decoding. Most likely candidate to add LoPTT message parsing. | https://github.com/DSheirer/sdrtrunk |
| **OP25 (boatbod fork)** | GNU Radio-based P25 Phase 1 & Phase 2 decoder with TDMA control channel support. | https://github.com/boatbod/op25 |
| **OP25 (osmocom)** | Original osmocom OP25 implementation. | https://github.com/osmocom/op25 |
| **DVMHost** | MMDVM-based Digital Voice Modem host software supporting P25 TDMA. Implements TIA/V.24 DFSI interface. Explicitly disclaims public safety use. | https://github.com/DVMProject/dvmhost |
| **Digital Speech Decoder (DSD)** | C-based decoder for P25, DMR, NXDN, and other digital voice protocols including Phase 2 TDMA. | https://github.com/szechyjs/dsd (and forks) |

SDRTrunk is particularly relevant — its v0.6.1 release already includes GPS position handling for P25 messages and L3Harris TDMA control channel parsing, making it the closest open-source codebase to eventual LoPTT message support.

---

## Standards Lineage

```
TIA-102 Series (APCO Project 25)
│
├── TSB-102-B ─── System and Standards Definition (Overview)
│
├── Phase 1 FDMA
│   ├── TIA-102.BAAA ─── FDMA CAI
│   ├── TIA-102.AABF ─── Link Control Word Formats
│   ├── TIA-102.AABC ─── Trunking Control Channel Messages
│   │   └── TIA-102.AABC-E-1 ─── CCH Messages Addendum: Location on PTT  ◄── referenced
│   ├── TIA-102.AABD ─── Trunking Procedures                              ◄── referenced
│   └── TIA-102.AAAD ─── Block Encryption Protocol
│
├── Phase 2 Two-Slot TDMA
│   ├── TSB-102.BBAA ─── TDMA Overview
│   ├── TIA-102.BBAB ─── TDMA Physical Layer Protocol
│   ├── TIA-102.BBAC ─── TDMA MAC Layer Description
│   │   └── TIA-102.BBAC-1 ─── MAC Description Addendum 1
│   ├── TIA-102.BBAD ─── TDMA MAC Layer Messages
│   │   └── TIA-102.BBAD-A-1 ─── MAC Messages Addendum: LoPTT & Alias     ◄── referenced
│   ├── TIA-102.BBAE ─── TDMA MAC Layer Procedures
│   │   └── ★ TIA-102.BBAE-1 ─── THIS DOCUMENT (LoPTT & User Alias)
│   └── Conformance
│       ├── TIA-102.BCAD ─── TDMA CAI Conformance Spec
│       ├── TIA-102.BCAE ─── TDMA Message/Procedures Conformance
│       └── TIA-102.BCAF ─── TDMA Voice Channel Conformance Profiles
│
├── Data Services
│   ├── TIA-102.BAEB ─── IP Data Bearer Service                            ◄── referenced
│   └── TIA-102.BAJC ─── Tier 2 Location Services                         ◄── referenced
│
├── Inter-System Interfaces
│   ├── TIA-102.BACA ─── ISSI Voice & Mobility Management
│   └── TIA-102.BACD ─── ISSI Supplementary Data
│
├── Vocoder
│   ├── TIA-102.BABA ─── IMBE Vocoder (Phase 1 full rate)
│   └── TIA-102.BABA-1 ─── Half Rate Vocoder Addendum (Phase 2)
│
└── Test & Measurement
    ├── TIA-102.CAAA ─── C4FM/CQPSK Transceiver Measurement Methods
    └── TIA-102.CAAB ─── C4FM/CQPSK Modulation Performance
```

**Legend:**
- ★ = This document
- ◄── referenced = Normatively referenced by this document
