# TIA-102.BBAD Related Resources and Context

## Status

**Active.** This document was published August 2017 and approved by the Project 25 Steering
Committee on June 22, 2017 as part of the Project 25 Standard. It is an original publication
with no known addendum or superseding revision as of April 2026. It defines the CCH MAC
layer for Phase 2 TDMA systems, a function not covered in any earlier TIA-102 document
(TIA-102.BBAC covers VCH MAC only).

The document can be purchased from TIA's standards store:
https://store.accuristech.com/tia

---

## Standards Family

This document is part of the **TIA-102 Phase 2 Two-Slot TDMA** sub-suite within the broader
Project 25 (P25) LMR standard family.

### Document Hierarchy

```
TIA-102 (Project 25 Standard)
└── Phase 2 Two-Slot TDMA Suite
    ├── TSB-102.BBAA       Overview of Two-Slot TDMA
    ├── TIA-102.BBAB       Physical Layer (modulation, timing, burst formats)
    ├── TIA-102.BBAC       VCH MAC Layer Description
    ├── TIA-102.BBAC-1     VCH MAC Layer Addendum 1
    ├── TIA-102.BBAD  ◄─── THIS DOCUMENT (CCH MAC Layer)
    └── (measurement/conformance docs for TDMA)

Referenced companion documents:
    ├── TIA-102.AABC-D     FDMA Trunking Control Channel Messages (source of MCOs)
    ├── TIA-102.AABC-D-1   FDMA Trunking Messages Addendum 1
    ├── TIA-102.AABD-B     FDMA Trunking Procedures
    ├── TIA-102.AABH       Dynamic Regrouping Messages and Procedures
    ├── TIA-102.AAAD-B     Block Encryption Protocol (ALGID/KID/MI)
    ├── TIA-102.BAAA-B     FDMA Common Air Interface
    ├── TSB-102-C          TIA-102 Documentation Suite Overview
```

### Companion Documents

| Document | Role |
|----------|------|
| TIA-102.BBAB | Physical layer — modulation (C4FM/CQPSK), burst timing, channel structure |
| TIA-102.BBAC | VCH MAC — defines MAC PDU types, superframe, SACCH/FACCH; BBAD reuses and extends |
| TIA-102.AABC-D | FDMA OSP/ISP messages that BBAD adapts into abbreviated/extended TDMA MAC messages |
| TIA-102.AABD-B | FDMA trunking procedures; BBAD's CCH acquisition and registration procedures align with these |
| TIA-102.AABH | MFID90 dynamic regrouping messages (sections 6.2.86–6.2.98 derive from here) |
| TIA-102.AAAD-B | Block encryption; MAC_PTT PDU carries MI, ALGID, KID from this spec |

---

## Practical Context

### How This Document Is Used

TIA-102.BBAD is the authoritative specification for any P25 Phase 2 equipment that
implements a TDMA trunked control channel. This includes:

**Fixed Network Equipment (FNE):** Trunked radio system infrastructure (base stations,
site controllers, RFSS) must implement the LCCH to transmit grants, broadcasts, and
system status. The FNE transmits OECI bursts and receives IECI bursts. The MAC_SIGNAL
PDU with its MCO-dispatched messages is the primary signaling vehicle.

**Subscriber Units (SU):** Portable and mobile radios that support Phase 2 trunking must
implement LCCH acquisition (S-ISCH sync detection, NAC verification), decode outbound
MAC messages, and transmit inbound requests. The SU's CCH state machine — scanning,
acquiring, registering, requesting calls — is driven by this specification.

**System Interoperability:** The Composite Control Channel feature allows systems to
simultaneously support FDMA (Phase 1) and TDMA (Phase 2) subscriber units on the same
trunked system, sharing channel resources. This is the primary migration path for agencies
moving from Phase 1 to Phase 2.

**Key Design Points for Implementers:**
- The MCO opcode space directly mirrors FDMA ISP/OSP opcodes for B2=1 messages,
  easing implementation by reusing AABC-D message tables with stripped CRC/MFID
- All post-initial-publication messages include a length octet, enabling forward-compatible
  parsing: unknown MCOs can be skipped without breaking PDU alignment
- The LCCH inbound uses a stronger RS(46,23) code vs. VCH's RS(17,8) — parsing
  code must handle both when processing mixed channel traffic
- Multi-fragment messages (only on LCCH) require tracking consecutive-burst reassembly
- The Color Code field in outbound PDUs and ISCH provides de-facto site identification
  for receivers that have acquired sync but not yet read the NAC

---

## Key Online Resources

### Official / Standards Bodies
- **TIA Standards Store (purchase):** https://store.accuristech.com/tia
- **P25 Technology Interest Group (PTIG):** https://www.project25.org
- **PTIG Standards Documents list:** https://www.project25.org/index.php/standards/documents
- **DHS SAFECOM P25 Overview:** https://www.cisa.gov/safecom/p25

### Technical References
- **PTIG Conformance Testing:** https://www.project25.org/index.php/testing/p25-compliance-assessment-program
- **DHS PSAC P25 CAP Testing results:** https://www.dhs.gov/science-and-technology/p25-cap
- **TIA TR-8 Committee (source working group):** https://tiaonline.org/engineering-committees/tr-8/

### P25 Documentation Archives
- **OpenMHz P25 documentation collection:** https://openmhz.com
- **RadioReference P25 Wiki:** https://wiki.radioreference.com/index.php/APCO_Project_25

---

## Open-Source Implementations

The following open-source projects implement portions of what this document specifies.
None are known to fully implement the TDMA CCH MAC layer independently — most
implement Phase 1 (FDMA) as their primary path and treat Phase 2 as partial/experimental.

### SDRTrunk
**Repository:** https://github.com/DSheirer/sdrtrunk

The most complete open-source P25 Phase 2 decoder. Implements TDMA channel
decoding including LCCH/VCH burst detection. The MAC message parsing for TDMA
mirrors the FDMA TSBK structure documented here.

Relevant source paths:
- `src/main/java/io/github/dsheirer/module/decode/p25/phase2/` — Phase 2 decoder
- `src/main/java/io/github/dsheirer/module/decode/p25/phase2/message/mac/` — MAC message decoding
- MAC message classes correspond directly to sections 6.2.x of this document

### OP25
**Repository:** https://github.com/boatbod/op25

GNURadio-based P25 decoder with Phase 2 support. Python implementation of
MAC layer parsing.

Relevant source paths:
- `op25/gr-op25-r2/lib/p25p2_*` — Phase 2 burst processing
- `op25/gr-op25-r2/lib/p25p2_tdma.cc` — TDMA MAC handling

### p25lib / imbe_vocoder
- https://github.com/szechyjs/dsd — Digital Speech Decoder, P25 support
- https://github.com/g4klx/MMDVM — Multi-Mode Digital Voice Modem, P25 mode

### Rust Ecosystem
No known Rust-native P25 Phase 2 LCCH implementation exists as of April 2026.
The SDRTrunk Java implementation is the most complete reference for a from-scratch
TDMA CCH parser.

---

## Standards Lineage

```
Project 25 (APCO/TIA Joint Program, 1989–present)
│
├── Phase 1: FDMA 12.5 kHz (1990s–present)
│   ├── TIA-102.BAAA-B  FDMA CAI
│   └── TIA-102.AABC-D  Trunking Messages (ISP/OSP opcodes reused by BBAD)
│
└── Phase 2: TDMA 6.25 kHz-equivalent (2009–present)
    ├── TIA-102.BBAB    Physical Layer (July 2009)
    ├── TIA-102.BBAC    VCH MAC Layer (December 2010)
    ├── TIA-102.BBAC-1  VCH MAC Addendum (February 2013)
    └── TIA-102.BBAD    CCH MAC Layer (August 2017)  ◄── this document
                        First standardized TDMA CCH specification;
                        prior to this, TDMA systems relied on FDMA CCH
                        via Composite Control Channel configurations.
```

**Historical note:** The gap between BBAC (2010) and BBAD (2017) reflects the industry's
phased deployment approach — Phase 2 systems initially used a Composite Control Channel
(TDMA voice + FDMA CCH) before the TDMA-native CCH was standardized. BBAD's
publication enables fully TDMA-native trunked systems.
