# TSB-102-A -- Related Resources & Context

## Status

This document is **historically foundational but partially superseded**. TSB-102-A (November 1995, revision of TSB-102 from March 1994) remains the original system architecture definition for APCO Project 25. It has never been formally withdrawn or replaced as a single document. However, portions of its content have been superseded or expanded by later publications:

- **TSB-102-C** (P25 Documentation Suite Overview): Takes over the standards catalogue and document description role of Section 4. TSB-102-C is periodically updated to reflect the current state of the TIA-102 document family, which has grown far beyond the ~30 documents catalogued in TSB-102-A to over 129 documents.
- **TSB-102-D** (P25 Detailed System and Standards Definition): Provides a more detailed and current version of the technical requirements and system model originally defined in Sections 3 and 5 of TSB-102-A. TSB-102-D incorporates Phase 2 TDMA, ISSI/CSSI refinements, and other architectural developments that postdate 1995.

Despite partial supersession, TSB-102-A remains important because it is the original source for several foundational concepts that pervade the entire P25 suite: the six open interface definitions, the functional group model, the reference configuration diagrams, the document numbering taxonomy, and the APCO Statement of Requirements. Virtually every normative TIA-102 document references TSB-102-A (or its predecessor TSB-102) either directly or through the conventions it established.

The document's designation as [4.2.2] in the BABG references table confirms its role as the root architectural reference.

---

## Standards Family Position

TSB-102-A sits at the **root of the P25 document tree**. It is not part of the A (services), B (systems), or C (equipment) categories -- it is the parent document that defines those categories. The numbering ***102 with no suffix letter is unique to this document.

### Document Hierarchy

```
TSB-102-A  --  APCO Project 25 System and Standards Definition (1995)
|
+-- ***102A___  SERVICES
|   +-- AAAA  DES Encryption Protocol
|   +-- AAAB  Security Services Overview
|   +-- AABB  Trunking Control Channel Formats
|   +-- AABC  Trunking Control Channel Messages
|   +-- AABD  Trunking Procedures
|   +-- AABF  Link Control Word Formats and Messages
|   +-- AABG  Conventional Control Messages
|   +-- AACA  OTAR Protocol
|   +-- AACB  OTAR Operational Description
|   +-- (and many more)
|
+-- ***102B___  SYSTEMS
|   +-- BAAA  Common Air Interface (CAI) -- Phase 1 FDMA
|   +-- BABA  IMBE Vocoder Description
|   +-- BACA  Inter RF-Subsystem Interface (ISSI) Message Definitions
|   +-- BADA  PSTN Interconnect Interface Definition
|   +-- BAEA  Data Overview
|   +-- BAEB  Packet Data Specification
|   +-- BAEC  Circuit Data Specification
|   +-- BAEE  Radio Control Protocol
|   +-- BAFA  Network Management Interface Definition
|   +-- BBAA  Two-Slot TDMA Overview (Phase 2)
|   +-- BBAB  TDMA Physical Layer
|   +-- BBAC  TDMA MAC Layer
|   +-- (and many more)
|
+-- ***102C___  EQUIPMENT
|   +-- CAAA  Digital C4FM/CQPSK Transceiver Measurement Methods
|   +-- CAAB  Digital C4FM/CQPSK Transceiver Performance Recommendations
|   +-- CCAA  TDMA Transceiver Measurement Methods
|   +-- (and many more)
|
+-- TSB-102-C  Documentation Suite Overview (supersedes Section 4)
+-- TSB-102-D  Detailed System and Standards Definition (supersedes Sections 3/5)
```

---

## Documents That Reference TSB-102-A

Because TSB-102-A is the foundational architecture document, it is referenced by virtually every document in the TIA-102 suite, either directly or transitively. Key documents that explicitly depend on it include:

| Document | Title | Nature of Reference |
|---|---|---|
| TIA-102.BABG | Conventional Control Messages | Lists TSB-102-A as reference [4.2.2] |
| TIA-102.BAAA (all revisions) | Common Air Interface | References TSB-102 system model and naming |
| TIA-102.BACA (all revisions) | ISSI Message Definitions | References ISSI architecture from TSB-102 |
| TIA-102.BADA | PSTN Interconnect Interface | References Et interface definition |
| TIA-102.BAFA | Network Management Interface | References En interface definition |
| TSB-102.BBAA | Two-Slot TDMA Overview | References TSB-102-A as root system definition |
| TSB-102-C | Documentation Suite Overview | Supersedes and expands Section 4 |
| TSB-102-D | Detailed System Definition | Supersedes and expands Sections 3 and 5 |
| All conformance documents (BC/CC series) | Various | Reference system model indirectly |

---

## APCO Project 25 History

### Timeline

- **1989**: APCO (Association of Public-Safety Communications Officials) initiates Project 25, bringing together APCO, NASTD (National Association of State Telecommunications Directors), and federal agencies to define next-generation digital public safety radio.
- **1989-1993**: The APCO Project 25 Steering Committee develops the Statement of Requirements. The TIA TR-8 committee (Mobile and Personal Private Radio Standards) begins standards development under the APCO/TIA Interface Committee (APIC), chaired by Stuart Meyer.
- **March 1994**: TSB-102 published -- the original System and Standards Definition.
- **June 1995**: Ballot Version 6 with update changes circulated.
- **November 1995**: **TSB-102-A published** (Revision A) with ballot comment revisions. This is the document under discussion.
- **1995-1998**: Phase 1 standards published -- CAI (C4FM/CQPSK at 12.5 kHz), IMBE vocoder, DES encryption, trunking control channel formats and messages.
- **Early 2000s**: ISSI specifications developed, enabling multi-vendor system interconnection. Packet and circuit data specifications published.
- **2009-2010**: Phase 2 TDMA standards published (TIA-102.BBAB physical layer July 2009, TSB-102.BBAA overview March 2010), introducing two-slot TDMA for 6.25 kHz equivalent spectrum efficiency.
- **2010s**: CSSI (Console Subsystem Interface) specifications developed. AES-256 encryption added alongside legacy DES. Link layer authentication (TIA-102.AACE) introduced.
- **2019**: Major consolidation of Phase 2 MAC specifications (TIA-102.BBAC-A, BBAD-A, BBAE).

### Organizational Context

The P25 standards process involves three key organizations:
- **APCO International**: Represents the public safety user community. Developed the original Statement of Requirements.
- **NASTD**: Represents state-level telecommunications directors. Co-sponsor of the user requirements.
- **TIA (Telecommunications Industry Association)**: The standards development organization (SDO) that publishes the formal TIA-102 documents through its TR-8 committee structure.

Federal participation comes through NTIA (National Telecommunications and Information Administration) and the IRAC (Interagency Radio Advisory Committee), as P25 applies to equipment licensed under both FCC and NTIA rules.

---

## Relationship to TSB-102-C and TSB-102-D

### TSB-102-C (Documentation Suite Overview)
TSB-102-C takes over the "catalogue" function of TSB-102-A Section 4. Where TSB-102-A listed approximately 30 documents (many still in draft), TSB-102-C provides a current index of all 129+ documents in the TIA-102 family, with descriptions, revision histories, and cross-references. TSB-102-C is periodically updated by the P25 Steering Committee.

### TSB-102-D (Detailed System and Standards Definition)
TSB-102-D takes over and significantly expands the "system model" and "technical requirements" functions of TSB-102-A Sections 3 and 5. It incorporates:
- Phase 2 TDMA architecture (not anticipated in 1995)
- CSSI (Console Subsystem Interface) -- a major interface not defined in TSB-102-A
- Refined ISSI architecture based on actual deployment experience
- IP-based networking concepts
- Updated functional group definitions reflecting modern system architectures
- Expanded security architecture (AES, link layer authentication)

Together, TSB-102-C and TSB-102-D form the modern equivalent of TSB-102-A. However, TSB-102-A remains the historical anchor and is still cited as the original system definition.

---

## Key Concepts Originated in TSB-102-A

Several concepts introduced in TSB-102-A became permanent features of the P25 architecture:

1. **RF Subsystem (RFSS)**: The concept of the RFSS as the fundamental building block -- bounded by open interfaces and internally proprietary -- originated here and remains central to all P25 system design.

2. **Six Open Interfaces**: The architectural principle of defining the system by its interfaces rather than by internal implementations was established in TSB-102-A and has guided all subsequent standards work.

3. **WACN/System/RFSS Hierarchy**: The naming structure for wide-area communications networks (Appendix A) is used throughout the trunking and ISSI specifications.

4. **Service Taxonomy**: The classification of services into bearer services, teleservices, and supplementary services (Section 5.4) established the framework used in all subsequent service definitions.

5. **Conventional/Trunked Duality**: The principle that the same subscriber equipment must operate in both conventional and trunked modes, with the difference being only in the supported feature set and access method, was established here.

---

## Standards Catalogs and Registries

- **P25 Approved TIA Standards List** (Project 25 Steering Committee, most recent available)
  https://www.project25.org/images/stories/ptig/P25%20SC%2023-02-001-R1%20P25%20TIA%20Standards_Approved_16Feb2023.pdf
- **P25 Document Suite Reference** (QSL.net mirror of Jan 2010 edition)
  https://www.qsl.net/kb9mwr/projects/dv/apco25/P25_Standards.pdf
- **Internet Archive -- TIA-102 Series Documents collection**
  https://archive.org/details/TIA-102_Series_Documents

## Community and Reference Sites

- **RadioReference.com** -- Extensive P25 system databases
  https://www.radioreference.com
- **P25 Technology Interest Group (PTIG)**
  https://www.project25.org

## Informational Open-Source References

Several open-source projects implement portions of the P25 protocols defined by the normative standards that TSB-102-A catalogues. These may be useful as informational references for understanding how the architecture maps to practice:

- **OP25** (boatbod fork) -- P25 Phase 1/Phase 2 decoder: https://github.com/boatbod/op25
- **SDRTrunk** -- Cross-platform P25 trunking decoder: https://github.com/DSheirer/sdrtrunk
- **Trunk-Recorder** -- P25 trunked system recorder: https://github.com/TrunkRecorder/trunk-recorder

Note: These are informational references only, not authoritative sources for protocol specifications.
