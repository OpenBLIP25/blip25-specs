# TIA-102.BAAD-B Related Resources and Context

**Document:** ANSI/TIA-102.BAAD-B-2015  
**Topic:** Project 25 Conventional Procedures  
**Compiled:** 2026-04-12

---

## Status

**Active.** This document was approved July 28, 2015, and published August 2015. It cancels and replaces TIA-102.BAAD-A (February 2010). No superseding document has been published as of the research date.

This document is actively referenced in FCC regulations (47 CFR § 90.548), which mandates P25 interoperability compliance for public safety LMR systems. Conventional-mode squelch behavior (NAC, talk group), repeater procedures, and channel access parameters from this document are among the behaviors tested under the DHS P25 Compliance Assessment Program (P25 CAP).

The document is available for purchase through the TIA standards store (accuristech.com/tia) and through IHS Markit.

---

## Standards Family

This document is one of approximately 49 documents in the TIA-102 suite. It was formulated under TIA TR-8 Mobile and Personal Private Radio Standards, TR-8.15 Common Air Interface Subcommittee, at the direction of the Project 25 Steering Committee.

**Standards Lineage (ASCII Tree)**

```
TIA-102 Project 25 Suite
├── Common Air Interface (CAI) Layer
│   ├── TIA-102.BAAA-A  FDMA Common Air Interface (physical + data unit formats)
│   └── TIA-102.BAAC-C  Reserved Values (NAC, UID, GID, ALGID ranges)
│
├── Link Control & Signaling
│   ├── TIA-102.AABF-D  Link Control Word Formats and Messages
│   └── TIA-102.AABG    Conventional Control Messages (TSBK formats)
│
├── Conventional Procedures  ←── THIS DOCUMENT
│   └── TIA-102.BAAD-B  Conventional Procedures (Revision B, 2015)
│       Supersedes: TIA-102.BAAD-A (2010)
│       Supersedes: TIA-102.BAAD Original (2003)
│       Supersedes: TIA-102.BAAD Interim Standard (1993)
│
├── Packet Data
│   ├── TIA-102.BAEA-B  Data Overview and Specification
│   └── TIA-102.BAED    Packet Data Logical Link Control Procedures
│                       (split from BAAD in Revision B)
│
├── Trunking Procedures
│   └── TIA-102.AABD-B  Trunking Procedures
│
├── Telephony Interconnect
│   └── TSB-102.BADA-A  Telephone Interconnect Overview (Voice Service)
│
├── Encryption
│   └── TIA-102.AAAD-A  Block Encryption Protocol
│
├── Vocoder
│   └── TIA-102.BABA-A  Vocoder Description (IMBE full-rate)
│
└── Suite Overview
    └── TSB-102-B       TIA-102 Documentation Suite Overview
```

**Companion Documents Referenced Normatively:**

| Designator | Document |
|---|---|
| TIA-102.AAAD-A | Project 25 Digital LMR Block Encryption Protocol |
| TIA-102.AABF-D | Project 25 Link Control Word Formats and Messages |
| TIA-102.AABG | Project 25 Conventional Control Messages |
| TIA-102.BAAA-A | Project 25 FDMA Common Air Interface |
| TIA-102.BAAC-C | Project 25 CAI Reserved Values |
| TIA-102.BABA-A | Project 25 Vocoder Description |
| TIA-102.BAEA-B | Project 25 Data Overview and Specification |
| TIA-102.BAED | Project 25 Packet Data LLC Procedures |

---

## Practical Context

Conventional P25 systems represent the most widely deployed P25 configuration in North America. Unlike trunked systems, conventional systems do not require a control channel; subscriber units monitor a single working channel for voice, data, and supplementary data. This architecture is common in:

- Small and mid-size public safety agencies that cannot justify trunked infrastructure
- Interoperability channels between agencies (many "mutual aid" and NPSPAC channels are conventional P25)
- Military and federal government radio networks
- Simplex/direct (talkaround) modes that are used when infrastructure fails or in building interiors

The procedures in this document govern:
- How a radio unit politely accesses a shared channel before transmitting (avoiding collisions)
- How repeaters relay inbound audio to outbound channels and to dispatch consoles
- How subscriber units determine which transmissions to unmute (NAC filtering, talk group filtering, selective squelch)
- How emergency alarms, radio inhibit/uninhibit, and other out-of-band control messages are delivered reliably over a congested shared channel
- How telephone interconnect calls are set up and torn down

A key addition in Revision B was the formalization of packet data procedures and the separation of Packet Data LLC into TIA-102.BAED. This was driven by the need to develop packet data conformance tests and interoperability testing scenarios for data-capable P25 radios.

The P25 CAP, administered by DHS through the National Institute of Standards and Technology (NIST), tests radios and infrastructure against interoperability parameters drawn from this document and companion standards. Tested behaviors include NAC filtering, TGID squelch, emergency call signaling, and radio inhibit.

---

## Key Online Resources

- **TIA Standards Store:** https://store.accuristech.com/tia  
  (Purchase the document)

- **TIA Standard Announcement (BAAD-B issuance):**  
  http://standards.tiaonline.org/tia-issues-new-project-25-conventional-procedures-standard  
  (Announcement of BAAD-B publication)

- **Project 25 Technology Interest Group (PTIG):**  
  https://www.project25.org  
  (P25 Steering Committee, approved standards list)

- **FCC Rule 47 CFR § 90.548 (P25 Compliance):**  
  https://www.law.cornell.edu/cfr/text/47/90.548  
  (Regulatory context; mandates P25 interoperability testing)

- **DHS P25 Compliance Assessment Program:**  
  https://www.dhs.gov/safecom/p25  
  (Testing program that exercises behaviors defined in this standard)

- **NIST P25 Certified Devices Process Document:**  
  https://www.nist.gov/system/files/documents/2024/01/19/Process%20Document%20for%20the%20NIST%20List%20of%20Certified%20Devices%20v2.50.pdf

- **RadioReference P25 Wiki:**  
  https://wiki.radioreference.com/index.php/P25  
  (Community documentation on P25 system types, NAC codes, talkgroups)

- **RadioReference P25 Conventional Header Collection (forum thread):**  
  https://forums.radioreference.com/threads/p25-conventional-header-info-collection.435808/  
  (Community data collection of conventional P25 parameters observed in real systems)

- **TIA-102 Wikipedia Overview:**  
  https://en.wikipedia.org/wiki/TIA-102  
  (Summary of the TIA-102 standards suite)

---

## Open-Source Implementations

The following open-source projects implement the conventional P25 air interface procedures specified in this document and its companion standards:

### SDRTrunk
- **Repository:** https://github.com/Dsheirer/sdrtrunk
- **Language:** Java
- **Description:** Cross-platform application for decoding P25 Phase I and Phase II (and DMR) using software-defined radio receivers. Implements conventional P25 voice and data decoding, talkgroup and unit ID filtering (Selective Squelch equivalent), NAC filtering, and supplementary data message decoding. Actively maintained.

### OP25 (Osmocom)
- **Repository:** https://github.com/osmocom/op25
- **Language:** C++/Python (GNU Radio blocks)
- **Description:** GNU Radio-based P25 decoder and recorder. Implements FDMA P25 physical layer decoding, NAC validation, voice frame extraction (feeding IMBE/AMBE vocoders), and control/supplementary data message parsing. Supports both conventional and trunked monitoring.

### OP25 (boatbod fork — more frequently updated)
- **Repository:** https://github.com/boatbod/op25
- **Language:** C++/Python
- **Description:** Active fork of Osmocom OP25 with additional features for trunked and conventional logging.

### TrunkRecorder
- **Repository:** https://github.com/TrunkRecorder/trunk-recorder
- **Language:** C++/Python (GNU Radio based)
- **Description:** Focused on trunked system recording, but includes conventional channel support. Implements channel access and call boundary detection logic.

### p25lib / p25craft (various)
- Various smaller repositories on GitHub implement portions of the P25 conventional protocol for specific research or testing purposes. Search GitHub for `p25 conventional` or `p25 tsbk` for current projects.

**Implementation Notes for SDRTrunk and OP25:**
- Both implement the three squelch modes (Monitor, Normal, Selective) as defined in sections 6.1.1.3 and 6.4.1.3 of this document.
- NAC filtering corresponds to the receiver NAC qualification procedures of sections 2.5 and 7.2.
- Call termination detection relies on TDU/ETDU recognition as defined in sections 2.4 and 6.1.1.4.
- Supplementary data message (TSBK) decoding reads from TIA-102.AABG for format and from this document for procedural context.
- Neither project implements the transmit (inbound) path — they are receive-only. For transmit implementations, see commercial P25 radio firmware or the Hamlib/Codec2 ecosystem.

---

## Regulatory and Compliance Context

The P25 Compliance Assessment Program (P25 CAP) directly tests behaviors described in this document:

- **NAC filtering compliance:** Radios must correctly reject transmissions with non-matching NACs (section 2.5)
- **TGID selective squelch:** Radios must unmute only for assigned talk groups (section 6.1.1.3, Table 3)
- **Emergency call signaling:** Emergency bit in LC field must be correctly set and recognized (section 6.2)
- **Radio Inhibit/Uninhibit:** Inhibited radios must behave as described in sections 8.4–8.5 (this was the subject of a published P25 CAP advisory after real-world field failures)

The CSSI (Console Subsystem Interface) and DFSI (Digital Fixed Station Interface) standards, while not referenced in this document, depend on the conventional procedures defined here for their over-the-air behavior.

---

## Notes on Revision History

The Interim Standard (1993) predates digital P25 deployments and defined only basic voice and data procedures. The Original (2003) edition added AFC description and corrected address and monitor function bugs. Revision A (2010) was a major normative expansion that added formatted procedures matching the trunking procedures document structure and resolved multiple interoperability issues identified through the CAPPTG (Compliance Assessment Program Technical Project Group). Revision B (2015) specifically addressed packet data interoperability failures and relocated the LLC procedures to TIA-102.BAED to facilitate independent packet data conformance testing.
