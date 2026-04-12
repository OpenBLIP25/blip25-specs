# TIA-102.AABB-B — Trunking Control Channel Formats
## Related Resources & Context

---

## Status

**Active, with a newer revision available.** TIA-102.AABB-B (approved April 2011) is Revision B of the document originally published in July 2005 as TIA-102.AABB-A. According to the Project 25 Technology Interest Group's approved standards listing (February 2023), **Revision C (TIA-102.AABB-C) has been developed and approved**, superseding this -B revision. Systems and implementations built to the -B revision remain interoperable since the core TSBK structure and multi-block framing defined here are stable and backward-compatible. The -C revision is expected to refine rather than fundamentally restructure these packet formats.

**Patent encumbrance:** Ericsson Inc. held Patent No. US 5,574,788 identified as potentially relevant at time of publication. Willingness-to-license statements were filed with APCO/NASTD/FED.

---

## Standards Family

This document sits in the **TIA-102 AA (Trunking/Control Channel Application)** layer of the Project 25 FDMA suite. The following companion documents are directly relevant:

### Normative References (required to implement)

| Document | Title | Relationship |
|---|---|---|
| **TIA-102.BAAA** | Project 25 FDMA Common Air Interface | Defines frame sync, NID encoding, rate-1/2 trellis code, CRC polynomials — all referenced but not repeated here |
| **TIA-102.AABC** | Project 25 Trunking Control Channel Messages | Defines all TSBK opcodes and per-message argument layouts that ride inside the formats defined here |
| **TIA-102.AABD** | Trunking Procedures | Defines Slotted ALOHA procedures, call setup sequences, and registration flows that use these packet structures |
| **TIA-102.BAAC** | Common Air Interface — Reserved Values | Defines MFID values referenced in the TSBK Manufacturer's ID field |

### Informative / Closely Related

| Document | Title | Note |
|---|---|---|
| TIA-102.AABB-A | Trunking Control Channel Formats (Rev A) | Predecessor document, July 2005 |
| TIA-102.AABB-C | Trunking Control Channel Formats (Rev C) | Successor document, approved ~2023 |
| TIA-102.BBAC | TDMA Trunking Control Channel Structures | Phase 2 (TDMA) equivalent of this document; different packet structures |
| APCO/NASTD/FED P25 System Standard | Project 25 System Standard | Overarching public safety communications standard framework |

---

## Standards Lineage (ASCII Tree)

```
TIA-102 (Project 25 FDMA Standards Suite)
├── AA — Trunking/Control Channel Application
│   ├── AABC — Trunking Control Channel Messages
│   │         (opcodes, argument definitions)
│   ├── AABB — Trunking Control Channel Formats  <-- THIS DOCUMENT
│   │   ├── AABB-A (2005) — Revision A
│   │   ├── AABB-B (2011) — Revision B (this document)
│   │   └── AABB-C (~2023) — Revision C (supersedes -B)
│   └── AABD — Trunking Procedures
│             (Slotted ALOHA, call setup, registration)
│
├── BA — Common Air Interface (Physical Layer)
│   ├── BAAA — FDMA Common Air Interface
│   │         (frame sync, NID, rate-1/2 trellis, CRC)
│   └── BAAC — Common Air Interface Reserved Values
│             (MFID and other reserved value tables)
│
└── BB — TDMA Physical / Control
    └── BBAC — TDMA Trunking Control Channel Structures
              (Phase 2 control channel; separate from AABB)
```

---

## Practical Context

This document defines the lowest-level packet framing that every P25 Phase 1 trunked radio must implement. It is not optional: interoperability between subscriber units from different manufacturers and with trunking controllers from different vendors depends on all parties sharing the same TSBK and multi-block formats defined here.

**In the field:** Every P25 Phase 1 trunked radio (portable or mobile) decodes TSBKs on the outbound control channel to receive call grants, group affiliations, and system status, and encodes TSBKs on the inbound channel to request service. Infrastructure equipment (site controllers, zone controllers) transmits OSPs continuously on the dedicated control channel. The composite control channel mode (section 3.2) is relevant to legacy narrow-band conventional channels repurposed for trunking, which must interrupt traffic to serve as control channels.

**Protected (encrypted) control channels** are relevant to law enforcement agencies that require communications security even on the control channel. The design in this document allows non-crypto-capable radios to remain aware of channel activity (they can validate TSBK CRCs) while being unable to extract call details from encrypted argument fields.

**Multi-block PDU** usage is typically seen for unit registration (the radio joining the system on power-on), talkgroup affiliation, and adjacent site information — transactions that carry more data than fits in a 10-octet TSBK argument space. These are less time-critical than call grants, justifying the higher latency of up to 135 ms.

---

## Key Online Resources

- **Project 25 Technology Interest Group (PTIG) — Approved Standards List (Feb 2023):**
  https://www.project25.org/images/stories/ptig/P25%20SC%2023-02-001-R1%20P25%20TIA%20Standards_Approved_16Feb2023.pdf

- **TIA-102 Series Documents — Internet Archive:**
  https://archive.org/details/TIA-102_Series_Documents

- **RadioReference Wiki — APCO Project 25:**
  https://wiki.radioreference.com/index.php/APCO_Project_25

- **Motorola P25 TDMA Trunking Suite White Paper (system-level context):**
  https://www.motorolasolutions.com/content/dam/msi/Products/apx-p25/P25_TDMA_Trunking_Standard_White_Paper.pdf

- **GlobalSpec Standards Index (companion documents):**
  - TIA-102.AABC: https://standards.globalspec.com/std/9909661/tia-102-aabc
  - TIA-102.AABD: https://standards.globalspec.com/std/9880063/tia-102-aabd
  - TIA-102.BAAA: https://standards.globalspec.com/std/10162320/tia-102-baaa
  - TIA-102.BAAC: https://standards.globalspec.com/std/10162326/tia-102-baac

---

## Open-Source Implementations

The following open-source projects implement TSBK parsing and P25 control channel decoding. All implement the structures defined in this document (TSBK 12-octet framing, DUID $7/$C dispatch, LB/P flags) in conjunction with TIA-102.AABC opcode tables.

### SDRTrunk (Java)
**https://github.com/DSheirer/sdrtrunk**

The most comprehensive open-source P25 decoder. Implements full TSBK decoding across all standard opcodes plus manufacturer-specific extensions (Motorola, Harris/M/A-COM). Handles:
- TSBK single/double/triple OSP formats
- Multi-block PDU header dispatch (both Format %10101 and %10111 header forms)
- Protected TSBK detection (P-bit handling, CRC validation on encrypted blocks)
- Cross-manufacturer TSBK variant handling (patch messages, neighbor encoding differences)

The TSBK decoder classes are the primary reference for how the LB/P/Opcode fields in Octet 0 are dispatched in practice. Look in the `src/main/java/io/github/dsheirer/module/decode/p25/` package.

### OP25 (GNU Radio / Python)
**https://wiki.radioreference.com/index.php/OP25**
Boatbod fork: https://github.com/boatbod/op25
Osmocom fork: https://osmocom.org/projects/op25/

GNU Radio-based P25 decoder supporting Phase I and Phase II. Decodes TSBKs through a three-stage pipeline: filtering → demodulation → payload decoding. The Python `trunking.py` module contains the TSBK opcode dispatch table. Useful reference for how Status Symbols on the outbound stream are used to identify inbound slot boundaries.

### Trunk-Recorder (C++)
**https://github.com/TrunkRecorder/trunk-recorder**

C++ application for recording P25 trunked calls with live control channel monitoring. The `p25_parser.cc` source file contains TSBK frame detection, LB-bit multi-TSBK sequencing logic, and multi-block PDU header parsing. This implementation shows how a real system accumulates double/triple TSBK OSPs before processing and how it handles the RFSS→SU call grant flow.

### p25.rs (Rust)
**https://github.com/kchmck/p25.rs**
TSBK docs: http://kchmck.github.io/doc/p25/trunking/tsbk/index.html

A Rust implementation providing low-level TSBK decoding. The `TsbkReceiver` state machine implements the full pipeline: dibit buffering → symbol descrambling → convolutional (rate-1/2 trellis) decode → CRC check → byte-level field extraction. This is the most directly useful reference for implementing a Rust P25 decoder. The crate demonstrates the exact byte-field layouts from this document mapped to Rust structs.

**Note for Rust implementation:** p25.rs demonstrates the NID DUID $7 dispatch path for TSBKs and the rate-1/2 trellis decode that wraps the 96-bit TSBK payload into the 196-bit on-air block. Cross-reference with TIA-102.BAAA clause 7 for the trellis encoder/decoder constants.

---

## Implementation Notes for This Document

When implementing a P25 TSBK parser, the key structural decisions from this document are:

1. **NID DUID dispatch:** Check the 4-bit DUID in the NID word. $7 → single-block TSBK path; $C → multi-block PDU path. (Frame sync and NID decoding per TIA-102.BAAA §8.3, §8.5.)

2. **TSBK Octet 0 dispatch:** Extract LB (bit 7), P (bit 6), Opcode (bits 5-0). LB=0 means accumulate the next TSBK in the same OSP before processing. P=1 means the opcode and arguments are encrypted; only CRC-validate, do not attempt field decode without the key.

3. **MFID (Octet 1) dispatch:** $00 or $01 → standard P25 opcode table (TIA-102.AABC). Any other value → manufacturer-specific parsing required.

4. **Multi-block header format selection:** Octet 0 Format field distinguishes the two header variants (%10101 vs. %10111). The alternative form (%10111) carries the opcode in header Octet 7, enabling dispatch before reading data blocks.

5. **Status symbol tracking:** OSP IDLE status (%11) marks inbound slot boundaries. During ISP reception, intraslot status symbols carry UNKNOWN (%10) — not IDLE.
