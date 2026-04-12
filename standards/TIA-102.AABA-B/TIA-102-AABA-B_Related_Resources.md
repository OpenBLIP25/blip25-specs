# TIA-102.AABA-B — Related Resources & Context

---

## Status

**Superseded.** This document (Issue B, April 2011) was superseded by **TIA-102.AABA-C**, approved December 2019 and published October 2020. The -C revision extended the trunking overview framework to reflect continued evolution of Phase 2 TDMA trunking and likely incorporated errata from the 2011 edition. The original Issue A (April 1995) was superseded by this Issue B.

Implementers and researchers should source TIA-102.AABA-C as the current baseline. This document remains useful as a reference for understanding the architectural intent of the Phase 2 TDMA additions made circa 2011.

---

## Standards Family

This document is part of the **TIA-102 "AA" trunking sub-suite** of Project 25. Within the TIA-102 series, the "AA" block defines the trunking control layer: formats, messages, procedures, and link control. This document is the overview entry point for the entire block.

**Direct companion documents (normative references from this document):**

| Reference Tag | Document                                          | Standard Number         |
|---------------|---------------------------------------------------|-------------------------|
| AABB          | Trunking Control Channel Formats                  | ANSI/TIA-102.AABB-A     |
| AABC          | Trunking Control Channel Messages                 | ANSI/TIA-102.AABC-C     |
| AABD          | Trunking Procedures                               | TIA-102.AABD-A          |
| AABD-1        | Trunking Procedures Addendum (CCC)                | TIA-102.AABD-A-1        |
| AABF          | Link Control Word Formats and Messages            | ANSI/TIA-102.AABF-B     |
| BAEB          | Packet Data Specification                         | ANSI/TIA-102.BAEB-A     |
| TIA102        | Project 25 System and Standards Definition        | PN-3-3591-UGRV1         |

**Broader TIA-102 suite context:**

- **AABA** (this series): Trunking overview — the conceptual layer
- **AABB/AABC/AABD/AABF**: Trunking control channel formats, messages, procedures, and link control — the normative implementation layer
- **BAAC/BAEB**: Conventional voice and packet data
- **BBAB/BBAC**: Physical layer (FDMA CAI and TDMA)
- **BCAA/BCAB/BCAC**: Conformance test suites

---

## Practical Context

This document describes the architectural intent behind P25 trunking systems as deployed by agencies including law enforcement, fire departments, emergency medical services, and federal agencies (e.g., DHS, DoD, state police). Real-world P25 trunked systems from vendors such as Motorola Solutions (APX series), Harris/L3Harris (XG-100, Unity XG-100), Kenwood, and Tait Communications implement the trunking model described here.

Key real-world manifestations of the concepts defined in this document:

- **System access control**: The Site Controller / Zone Controller in Motorola SmartZone and Harris VIDA systems implements the resource controller role described in §2.4.
- **Transmission vs. message trunking**: Modern P25 systems use message trunking for voice calls (the working channel is held through the entire PTT transmission and hold-off period); transmission trunking appears in data channel grants.
- **FDMA/TDMA interoperability via common control channel**: Phase 2 TDMA deployments maintain a Phase 1 FDMA control channel, exactly as described in §2.3 and §2.8. Phase 2 voice channels use TDMA (2-slot, H-DQPSK modulation), while the control channel remains Phase 1 FDMA (C4FM).
- **ISSI (Inter SubSystem Interface)**: Enables the cross-RFSS group and individual calls described in §3.1, standardized in TIA-102.BACA.
- **Composite Control Channel (CCC)**: A fallback mode described in §4 used when all working channels are occupied; implemented in some high-density urban deployments.
- **Pre-programmed Data Messaging** (§3.3.2): Corresponds to the Status/Message/Alert features found on virtually all P25 radios; encoded as short data messages on the trunking control channel or working channel.

---

## Key Online Resources

- **Project 25 Technology Interest Group (PTIG) — Approved Standards List:**  
  [https://www.project25.org/images/stories/ptig/P25_SC_19-06-002-R1_Approved_P25_TIA_Standards_-_June_2019.pdf](https://www.project25.org/images/stories/ptig/P25_SC_19-06-002-R1_Approved_P25_TIA_Standards_-_June_2019.pdf)

- **Project 25 Technology Interest Group (PTIG) — Approved Standards List (2022):**  
  [https://project25.org/images/stories/ptig/P25_SC_22-04-003_Approved_P25_TIA_Standards_Approved_4052022.pdf](https://project25.org/images/stories/ptig/P25_SC_22-04-003_Approved_P25_TIA_Standards_Approved_4052022.pdf)

- **TIA-102.AABA listing on GlobalSpec:**  
  [https://standards.globalspec.com/std/14211744/TIA-102.AABA](https://standards.globalspec.com/std/14211744/TIA-102.AABA)

- **TIA-102 Series Documents archive (Internet Archive):**  
  [https://archive.org/details/TIA-102_Series_Documents](https://archive.org/details/TIA-102_Series_Documents)

- **Motorola Solutions — Project 25 Phase 2 TDMA Trunking Suite White Paper:**  
  [https://www.motorolasolutions.com/content/dam/msi/docs/business/_documents/white_paper/_static_files/p25_tdma_standard_white_paper_final.pdf](https://www.motorolasolutions.com/content/dam/msi/docs/business/_documents/white_paper/_static_files/p25_tdma_standard_white_paper_final.pdf)

- **Motorola Solutions — P25 TDMA Trunking Standard White Paper (July 2013):**  
  [https://www.motorolasolutions.com/content/dam/msi/Products/apx-p25/P25_TDMA_Trunking_Standard_White_Paper.pdf](https://www.motorolasolutions.com/content/dam/msi/Products/apx-p25/P25_TDMA_Trunking_Standard_White_Paper.pdf)

- **RadioReference Wiki — APCO Project 25:**  
  [https://wiki.radioreference.com/index.php/APCO_Project_25](https://wiki.radioreference.com/index.php/APCO_Project_25)

- **RadioReference Wiki — Trunked Radio Decoders:**  
  [https://wiki.radioreference.com/index.php/Trunked_Radio_Decoders](https://wiki.radioreference.com/index.php/Trunked_Radio_Decoders)

- **TIA online standards catalog:**  
  [http://standards.tiaonline.org/standards/technology/project_25/index.cfm](http://standards.tiaonline.org/standards/technology/project_25/index.cfm)

---

## Open-Source Implementations

These projects implement the trunking services described in this document. Note: because this document is an overview, specific code references map to the normative sub-documents (AABB, AABC, AABD).

- **SDRTrunk** (Java): Full P25 Phase 1 and Phase 2 trunking decoder and monitor. Implements the complete trunking state machine including channel grants, site registration tracking, group and individual call handling, and working channel following — all of which are described architecturally here.  
  GitHub: [https://github.com/DSheirer/sdrtrunk](https://github.com/DSheirer/sdrtrunk)

- **OP25** (Python/GNU Radio): Linux-based P25 Phase 1 and Phase 2 decoder with full trunking stack. Includes a P25 Phase 1 trunking control channel / voice channel simulator ("fakecc") useful for protocol testing.  
  GitHub: [https://github.com/boatbod/op25](https://github.com/boatbod/op25)

- **GNU Radio Project 25 blocks**: Various GNU Radio out-of-tree modules implement P25 channel access and trunking control channel parsing.

Both SDRTrunk and OP25 implement the service model described in this document: group voice calls, individual voice calls, working channel following, site registration tracking, and supplementary control messaging on working channels.

---

## Standards Lineage

```
TIA-102 (Project 25 System and Standards Definition)
└── TIA-102.AA — Trunking Suite
    ├── TIA-102.AABA — Trunking Overview (this series)
    │   ├── TIA-102.AABA-A  (Issue A, April 1995)
    │   ├── TIA-102.AABA-B  (Issue B, April 2011) ← this document
    │   └── TIA-102.AABA-C  (Issue C, October 2020) [current]
    ├── TIA-102.AABB — Trunking Control Channel Formats
    ├── TIA-102.AABC — Trunking Control Channel Messages
    ├── TIA-102.AABD — Trunking Procedures
    │   └── TIA-102.AABD-A-1 — Trunking Procedures Addendum (CCC)
    └── TIA-102.AABF — Link Control Word Formats and Messages
```

This document is the conceptual root of the trunking sub-suite. All normative trunking implementation requirements flow from the companion documents listed above. The security services for trunked operation are separately addressed in the TIA-102.AAAD and related security suite documents (referenced by prior Issue A content that was removed from this revision).
