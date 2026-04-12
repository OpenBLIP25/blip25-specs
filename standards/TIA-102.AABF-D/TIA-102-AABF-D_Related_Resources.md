# TIA-102.AABF-D — Related Resources & Context

---

## Status

**Active at time of extraction (2026-04-12), but superseded by newer revisions.**

Two newer revisions exist in the same directory:
- `8.10_TIA-102.AABF-D-1_Final Published Document (Member Copy)_2023_12_13.pdf` — Revision D-1, December 2023
- `8.10-TIA-AABF-D-2_Final Published Document (Member Copy)_2023_07_26.pdf` — Revision D-2, July 2023

The parent standard TIA-102.AABF-D (Rev D, Feb 2015) has been amended at least twice since publication. Rev D-1 and D-2 likely address errata, additional LC message types, or field clarifications. **Implementations should be based on the latest revision available.**

ANSI approval: February 24, 2015. Standards body: TIA TR-8 Mobile and Personal Private Radio, Subcommittee TR-8.10 on Trunking and Conventional Control.

---

## Standards Family

This document is part of the **TIA-102 Project 25 CAI suite**. It defines content carried within air-interface frames specified by the Layer 1/2 documents.

```
TIA-102 Suite (relevant subset)
│
├── TSB-102.BBAA — TIA-102 Suite Overview (informative)
│
├── Layer 1 / Physical / Air Interface
│   ├── TIA-102.BAAA — FDMA CAI (defines LDU1/ETDU framing, Reed-Solomon+Hamming/Golay
│   │                             error coding of the LC word — LC word is 72 bits here)
│   ├── TIA-102.BAAC — CAI Reserved Values (MFID registry, Algorithm ID values)
│   └── TIA-102.BAAD — CAI Operational Description for Conventional Channels
│
├── Layer 2 / MAC / Trunking Control
│   ├── TIA-102.AABB — Trunking Control Channel Formats (TSBK framing)
│   └── TIA-102.AABC — Trunking Control Channel Messages (field value tables
│                       referenced throughout this document: Service Options,
│                       Channel Identifier, Group Address, Status, etc.)
│
├── THIS DOCUMENT — TIA-102.AABF-D
│   Link Control Word Formats and Messages
│   (LC words embedded in LDU1 and ETDU voice frames)
│
├── Protocol / Procedures
│   ├── TIA-102.AABD — Trunking Procedures (call setup, registration, roaming)
│   └── TIA-102.BAAD — Conventional channel operational descriptions
│
└── Security
    ├── TIA-102.AAAD — Block Encryption Protocol (Algorithm ID, Key ID usage)
    └── TIA-102.AACE — Link Layer Authentication
```

**Companion documents with direct field cross-references from AABF-D:**
- TIA-102.AABC: field value tables for Service Options, Channel fields, Priority Level, Status, Group Address, Subscriber Unit Address, System Services Available/Supported, System Service Class, T_wuid_validity
- TIA-102.BAAC: MFID registry, Algorithm ID enumeration
- TIA-102.AAAD: Algorithm ID operation, Key ID operation
- TIA-102.AABD: Location Registration Area field

---

## Practical Context

### How LC Words Are Used in Real Systems

Link Control Words are the "caller ID and context" of a P25 voice call. They appear in:
1. **LDU1** (Logical Link Data Unit 1) — the first of the two 9-packet voice frame groups, transmitted repeatedly throughout a call, once per ~180 ms superframe.
2. **ETDU** (Expanded Terminator Data Unit) — the call-end frame; LC word here typically carries LCO 15 (Call Termination/Cancellation).

A scanning receiver or dispatch console uses LC words to:
- Display talker ID and group/unit being called
- Route audio to the correct talkgroup decoder
- Detect call end (LCO 15 in ETDU) and release the channel
- Learn about active calls on adjacent channels (LCO 2 Group Voice Channel Update)
- Track network topology (LCOs 32–42 broadcast identity messages)

In **conventional systems**, LC words are transmitted by subscriber radios on every voice transmission. The repeater relays them. Receiving radios decode the LCO to identify caller, group, and service options.

In **trunked systems**, the FNE infrastructure generates or validates LC words. Broadcast messages (LCOs 32–42) are inserted by the site controller into the voice channel traffic. Subscriber units monitor these to maintain their network context even while on a traffic channel.

### Source ID Extension Protocol

A critical operational aspect not obvious from the LCO table: when a subscriber unit's identity spans systems (cross-patch, ISSI, console roaming), the abbreviated 24-bit Source/Target Address cannot uniquely identify the unit. The S-flag mechanism solves this: the LC word sets S=1, and the *next* LC word in the sequence (transmitted within the same LDU1) is a Source ID Extension (LCO 9) carrying the full WACN+System+Unit ID. Parsers must handle consecutive LC pairs, not just individual LCOs.

### Equipment That Uses This Standard

- **Subscriber radios**: Motorola APX series, Harris XG-100, Kenwood NX-5000 series, and all P25 Phase 1 FDMA certified radios
- **Dispatch consoles**: Motorola VESTA, Zetron MAX, Zetron ACOM, Catalyst Communications, Airbus Vesta
- **Site controllers / RFSSs**: Motorola ASTRO 25, Harris VIDA, EADS VESTA
- **Gateways and inter-RF subsystem links**: ISSI gateways that relay calls between sites include LC word forwarding as part of call context

---

## Key Online Resources

- **TIA standards catalog**: https://tiaonline.org/standards/catalog/
  Purchase page for AABF-D and newer revisions.

- **APCO Project 25 Technology Interest Group (P25 TTIG)**:
  https://www.apcointl.org/tech/standards/p25
  APCO is the principal user organization for P25 standards. The TTIG participates in TIA TR-8.10 ballot proceedings.

- **RadioReference P25 wiki and forums**: https://www.radioreference.com/apps/db/
  Real-world trunked system databases. Talkgroup monitoring discussions reference LC word fields extensively (e.g., decoding talker IDs from LDU1 streams).

- **Accurate Technologies (TIA standards reseller)**:
  https://store.accuristech.com/tia
  Source for member and non-member copies of TIA standards.

- **National Interoperability Field Operations Guide (NIFOG)**:
  https://www.cisa.gov/safecom/nifog
  CISA guide for interoperability; references P25 CAI including LC word-level interoperability.

---

## Open-Source Implementations

LC word decoding is implemented in several open-source P25 monitoring projects. All parse the 9-octet structure by extracting LCF (octet 0), dispatching on LCO, and extracting fields per the layouts in this document.

### SDRTrunk
- **Repository**: https://github.com/DSheirer/sdrtrunk
- **Language**: Java
- **LC implementation**: `src/main/java/io/github/dsheirer/module/decode/p25/phase1/message/lc/`
  - `LinkControlOpcode.java` — LCO enum (all 43 named values)
  - Individual message classes: `GroupVoiceChannelUser.java`, `UnitToUnitVoiceChannelUser.java`, `CallTermination.java`, `SystemServiceBroadcast.java`, `RFSSStatusBroadcast.java`, etc.
  - `SourceIDExtension.java` — handles the LCO 9 paired message
- SDRTrunk implements all standard AABF-D message types and handles the S-flag / Source ID Extension pairing

### OP25
- **Repository**: https://github.com/boatbod/op25
- **Language**: C++ / Python
- **LC implementation**: `op25/gr-op25-repeater/lib/p25p1_voice_decode.cc`, `op25/gr-op25-repeater/lib/p25_lc.py`
- OP25 decodes LC words as part of its IMBE voice channel decoder pipeline; LC word fields are extracted and logged alongside audio.

### DSDPlus / DSD (Digital Speech Decoder)
- https://github.com/szechyjs/dsd (community maintained fork)
- LC word decoding in the P25 phase 1 decoder path; less comprehensive than SDRTrunk but covers the common voice channel messages.

### P25Dump / Unitrunker (historical reference)
- https://github.com/furrtek/DMRDemodulator (tangential; DMR focus)
- Unitrunker (closed source but widely used) implements full LC decoding for trunked system following

### Rust / P25 Library (context-relevant)
- No known production-quality Rust P25 library as of 2026. The SDRTrunk Java implementation is the most complete open-source reference for all LC messages. For Rust implementation, the SDRTrunk source and this document together constitute a complete specification.
- Related Rust radio DSP crates: `liquid-dsp` bindings, `gnuradio-rs` (nascent)

---

## Standards Lineage

```
TIA-102.AABF (original, ~2003)
  ├── AABF-A (Dec 2004)
  │   ├── AABF-A-1 (deleted LCO 18 Unit Auth Command)
  │   ├── AABF-A-2
  │   └── AABF-A-3
  └── AABF-B (Jul 2009 — merged A, A-1, A-2, A-3)
      └── AABF-C (Jan 2011)
          ├── AABF-C-1
          └── AABF-D (Feb 2015) ← THIS DOCUMENT
              ├── AABF-D-1 (Dec 2023) ← newer revision, same directory
              └── AABF-D-2 (Jul 2023) ← newer revision, same directory
```

**Note on D-1 / D-2 ordering**: The directory contains both D-1 (Dec 2023) and D-2 (Jul 2023). The earlier publication date of D-2 vs D-1 is unusual and may indicate D-2 was an addendum published concurrently with or before D-1 was finalized, or the dates reflect different approval stages. Both should be examined for incremental changes relative to AABF-D.

---

## Notes for Implementation

1. **LCO dispatch table**: Parsers should use LCO (bits 5–0 of octet 0) as the primary dispatch key. SF (bit 6) distinguishes implicit vs explicit MFID formats; for standard P25 messages SF=1 in almost all cases. The Conventional Fallback (LCO 42) is the only standard message with SF=0 (and explicit MFID=$00 in octet 1).

2. **S-flag chaining**: Any LCO that includes an S-flag field (LCOs 0, 5, 12, 19, 20, 22) must be handled as a potential two-LC-word pair. If S=1, the parser should consume the next LC in the LDU1 sequence as LCO 9 (Source ID Extension) to get the full SUID.

3. **LCO 10 special case**: Unit-to-Unit Voice Channel User – Extended (LCO 10) has S=1 hardcoded (S shall always be 1 per the spec), so it always requires a following Source ID Extension.

4. **Obsolete LCOs**: LCO 18 (Unit Authentication Command) and LCO 37 (Protection Parameter Broadcast) are defined but struck-through as OBSOLETE. Implementations should ignore received messages with these LCOs or log and skip them.

5. **Explicit-channel messages (LCOs 38–41)**: These carry both Channel(T) and Channel(R) fields for systems where transmit and receive frequencies differ by more than a standard offset. The implicit-channel variants (LCOs 33–36) carry a single Channel field plus a Channel Identifier. Systems must support both variants.

6. **Field value references**: This document defines field layouts only. Actual encoded values for Service Options, System Service Class, Priority Level, and all channel-related fields are in TIA-102.AABC. Algorithm ID values are in TIA-102.BAAC and TIA-102.AAAD.
