# TIA-102.BAAC-D — Related Resources & Context

**Document:** TIA-102.BAAC-D, Project 25 Common Air Interface Reserved Values
**ANSI Designation:** ANSI/TIA-102.BAAC-D-2017
**Approved:** June 19, 2017

---

## Status

**Active.** This is the current revision (Revision D) of this document, approved June 19, 2017. It supersedes TIA-102.BAAC-C (April 2011). No subsequent revision has been published. The document is maintained by the TIA TR-8.15 Common Air Interface Subcommittee and is purchased through the TIA standards catalog.

---

## Standards Family

This document belongs to the TIA-102 Project 25 standards suite. It is a normative supplement (a "reserved values registry") that underpins all other TIA-102 specifications that use the fields it defines. It does not stand alone — it is referenced by or coordinates with the following companion documents:

**Normatively referenced by this document:**
- **TIA-102.BAAA-B** — Project 25 FDMA Common Air Interface (June 2017). The primary air interface spec; this document supplements it by defining field value semantics not enumerated there.
- **TIA-102.AABC-D** — Project 25 Trunking Control Channel Messages (April 2015). Defines trunked TGID assignments that extend Section 2.4 of this document.
- **TIA-102.AABF-D** — Project 25 Link Control Word Formats and Messages (April 2015). Contains the full set of LCF values; this document defines the four base LCF values.

**Informatively referenced:**
- **TSB-102.BBAA** — Project 25 Two-Slot TDMA Overview (March 2010). Establishes the TDMA context for which this document's field values also apply.
- **TSB-102-C** — Project 25 TIA-102 Documentation Suite Overview (March 2016). System-level architecture overview; defines "SU" referenced in Section 1.4.
- **MFID Assignments** — TIA TR-8.15 Subcommittee registry (January 2016). Governs Manufacturer's ID values beyond $00 and $01.
- **ALGID Value Assignment Guide** — TIA TR-8.15 Subcommittee registry (April 2015). Governs Algorithm ID values beyond the eleven enumerated in Table 6.

**Standards Lineage (ASCII tree):**

```
TIA-102 Project 25 Suite
│
├── TSB-102-C (Suite Overview)
│   └── (references all below)
│
├── Air Interface Layer
│   ├── TIA-102.BAAA-B  (FDMA Common Air Interface)  ← primary CAI spec
│   ├── TSB-102.BBAA    (Two-Slot TDMA Overview)
│   └── TIA-102.BAAC-D  (CAI Reserved Values)  ← THIS DOCUMENT
│       ├── supplements BAAA-B
│       └── supplements BBAA
│
├── Link Control Layer
│   └── TIA-102.AABF-D  (Link Control Word Formats & Messages)
│       └── defines additional LCF values beyond BAAC-D Table 3
│
├── Trunking Control Layer
│   └── TIA-102.AABC-D  (Trunking Control Channel Messages)
│       └── defines trunked TGID assignments extending BAAC-D Table 4
│
└── External Registries (TR-8.15 Subcommittee)
    ├── MFID Assignments (January 2016)
    └── ALGID Value Assignment Guide (April 2015)
```

---

## Practical Context

This document is fundamental to any P25 implementation. Every P25 frame carries fields whose interpretation depends on the values defined here. Practical use cases:

**Decoders and monitors (SDR-based):** Software P25 decoders (SDRTrunk, OP25, Unitrunker, DSD+) use this document's tables directly for:
- Identifying data unit type from the 4-bit DUID before any further frame processing
- Determining encryption state from ALGID ($80 = clear, others = encrypted)
- Resolving the service type from SAP in packet data headers
- Distinguishing group vs. individual calls from LCF
- Interpreting the emergency bit in Link Control words

**Infrastructure (repeaters, controllers):** Fixed Station NAC behavior ($F7F) is implemented in P25 repeaters to allow retransmission regardless of incoming NAC. The Monitor NAC ($F7E) is used in scan/monitor configurations.

**Encryption-aware equipment:** The ALGID table governs key management negotiation — knowing whether AES-128 ($85), AES-256 ($84), or a legacy algorithm is in use determines which key material and IV handling applies. DES ($81) and 3DES ($83) remain in the field on older infrastructure but are not authorized for new procurements under modern CJIS and federal security requirements.

**Conventional vs. trunked address mapping:** Section 2.4's 16-to-24-bit TGID mapping rule ($0001–$FFFF → $FEF001–$FFEFFF) is essential for correct message routing in mixed conventional/trunked systems.

**Data services:** The SAP table is the dispatch table for P25 data — packet data, key management, location services, and trunking control are all distinguished by SAP value before any deeper protocol parsing.

---

## Key Online Resources

- **TIA Standards Catalog:** https://www.tiaonline.org/standards/catalog/
  Purchase point for the official document. Search "TIA-102.BAAC".

- **TIA TR-8.15 Subcommittee (Public FTP):** Accessible via the TIA website under Standards → Engineering Committees → TR-8 → TR-8.15 Common Air Interface → Contributions (FTP) → Public. Hosts the MFID Assignments and ALGID Value Assignment Guide referenced in this document.

- **APCO P25 Technology Interest Group:** https://www.apcointl.org/technology/p25/
  Industry context for P25 standards adoption and procurement.

- **DHS SAFECOM P25 CAP:** https://www.cisa.gov/safecom/p25
  Federal compliance context; P25 Compliance Assessment Program tests against standards including this document's field values.

- **RadioReference Wiki — P25:** https://wiki.radioreference.com/index.php/P25
  Community documentation on P25 field values including NAC, TGID, and DUID usage in practice.

---

## Open-Source Implementations

These projects implement decoding and/or encoding of the fields defined in this document:

**SDRTrunk** (Java, actively maintained)
- https://github.com/DSheirer/sdrtrunk
- Implements full P25 Phase 1 (FDMA) and Phase 2 (TDMA) decoding
- DUID dispatch: `src/main/java/io/github/dsheirer/module/decode/p25/phase1/message/P25P1DataUnitID.java`
- ALGID decoding: `src/main/java/io/github/dsheirer/module/decode/p25/reference/Encryption.java`
- SAP handling: present in packet data and trunking message decoders
- NAC filtering: configurable per channel in the channel editor

**OP25** (C++/Python, GNU Radio-based)
- https://github.com/boatbod/op25
- Implements P25 Phase 1 FDMA decoding and limited trunking
- DUID, NAC, and ALGID handling in `op25/gr-op25-legacy/lib/p25_frame_assembler_impl.cc` and related files
- Python trunking logic references TGID, SAP, and ALGID values extensively

**Unitrunker** (Windows, closed-source decoder with open protocol documentation)
- https://www.unitrunker.com/
- Widely used field tool; implements NAC scanning and TGID tracking

**DSD / DSD+** (C, demodulation-focused)
- https://github.com/szechyjs/dsd (original open-source DSD)
- Decodes DUID, LCF, ALGID; limited trunking awareness

**p25rx** and related Rust projects:
- No mature standalone Rust P25 library as of 2025; SDRTrunk is the most complete reference implementation for cross-checking field value handling

---

## Extraction Notes

All tables in this document were visually read from PDF page renders (pages 11–18 of the 20-page PDF). No programmatic extraction was used — the document contains only simple two- to four-column tables with no complex spanning cells, making visual reading reliable. Spot-check against the rendered pages was performed for all numeric values.

Table 6 (ALGID) was given particular attention: the gap between $04 ($SAVILLE, decimal 4) and $41 (BATON Auto Odd, decimal 65) is intentional — values in between are assigned via the external ALGID Value Assignment Guide and are not enumerated here. The gap between $41 and $80 is similarly governed by the external registry.

---

*No formulas or algorithms are present in this document. Phase 3 implementation specs are not applicable (MESSAGE_FORMAT classification, value-table document only).*
