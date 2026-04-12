# TIA-102-BAEJ Related Resources and Context

**Document:** TIA-102.BAEJ — Project 25 Conventional Management Service Specification for Packet Data  
**ANSI:** ANSI/TIA-102.BAEJ-2013  
**Approved:** September 27, 2013

---

## Status

**Active** as of April 2026. No successor revision or replacement has been published by TIA. This document is the 1st edition (Original), published October 2013. It supersedes and cancels TIA-102.BAAD-1 (an addendum to TIA-102.BAAD-B), which previously contained this material as an annex. No known withdrawal, reaffirmation, or superseding document exists in the TIA catalog as of this writing.

---

## Standards Family

This document belongs to the TIA-102 BAEX series (BAE = P25 Data), which defines the P25 Packet Data stack from the physical layer through application management.

### Standards Lineage (ASCII Tree)

```
TIA-102 (Project 25)
└── BAA-series: Physical / Air Interface
│   └── TIA-102.BAAA-A  FDMA – Common Air Interface [normative ref 3]
│       (defines CAI data packet delivery, LLID, PDU framing)
│
└── BAC-series: CAI Reserved Values / Parameters
│   └── TIA-102.BAAC-C  CAI Reserved Values [normative ref 4]
│       (defines SAP values: Registration & Authorization, Scan Preamble,
│        Packet Data Encryption Support; MFID values)
│
└── BAD-series: Conventional Procedures
│   └── TIA-102.BAAD-B  Conventional Procedures
│       └── TIA-102.BAAD-1 (addendum — SUPERSEDED by TIA-102.BAEJ)
│
└── BAE-series: Packet Data Stack
    ├── TIA-102.BAEA-B  Data Overview and Specification [normative ref 5]
    │   (defines Conventional FNE Data configuration, bearer service
    │    combinations, SNDCP/SCEP/CAI bearer services)
    ├── TIA-102.BAEJ    ← THIS DOCUMENT
    │   Conventional Management Service for Packet Data
    │   (CMS: registration, location tracking, scan mode, encryption info)
    └── TIA-102.BAEB-B  IP Data Bearer Service Specification
        (was draft SP-3-4632-RV2 at time of BAEJ publication [normative ref 6])
```

### Related TIA-102 Documents

| Document           | Title / Role                                                              |
|--------------------|---------------------------------------------------------------------------|
| TIA-102.AAAD-A     | Block Encryption Protocol — defines ES Auxiliary Header formats used by Unable to Decrypt PDU |
| TIA-102.AACA       | OTAR Protocol — defines warm-start procedure triggered by Unable to Decrypt |
| TIA-102.BAAA-A     | FDMA CAI — defines confirmed/unconfirmed delivery, CAI Response, NACK types |
| TIA-102.BAAC-C     | CAI Reserved Values — defines SAP assignments (Registration & Authorization = used by Connect/Accepted/Denied; Packet Data Scan Preamble SAP; Packet Data Encryption Support SAP) and MFID=$00 standard value |
| TIA-102.BAEA-B     | Data Overview & Specification — parent document defining the Conventional FNE Data configuration, bearer service combinations |
| TIA-102.BAEB-B     | IP Data Bearer Service — defines SNDCP and SCEP bearer layers; SNDCP scan mode and context management that supersede CMS functions in SNDCP configurations |
| TIA-102.BAAD-B     | Conventional Procedures — ancestor document; BAAD-1 addendum (now cancelled) carried CMS material |

---

## Practical Context

### What This Specification Controls

This document defines the management plane for P25 conventional packet data: how subscriber radios (SUs) register to access packet data bearer services on a conventional channel, how the FNE tracks their channel location for routing purposes, how scanning SUs can be reached by the FNE, and how encryption failures are reported.

In a deployed P25 conventional data system, this specification runs above the CAI (air interface) layer and below the application/bearer layer. The FNE is typically implemented in a dispatch console controller, a P25 conventional repeater site controller, or a software-defined radio infrastructure appliance. The SU implementation lives in the subscriber radio firmware.

### Dynamic vs. Static Registration

Static registration requires no over-the-air registration exchange and is used in simpler or legacy deployments. Dynamic registration, the more capable method, provides the FNE with explicit knowledge of when an SU enters or leaves the data-capable state, negotiates IP address binding (for SCEP-based IP data), and negotiates CMS Scan Mode. Most modern P25 radios and infrastructure use Dynamic registration.

### CMS Scan Mode and Channel Capacity

The Scan Preamble mechanism trades channel capacity for SU mobility: the FNE must hold the channel transmitting empty Scan Preamble PDUs for a duration that exceeds the SU scan period before sending data. The document explicitly warns that long SU scan periods degrade channel performance. This trade-off is a known limitation of conventional (non-trunked) P25 data operation.

### Encryption Info and OTAR Integration

The Unable to Decrypt PDU provides a lightweight feedback path from the SU to the FNE when decryption fails. It carries the key ID, algorithm ID, current keyset index, and an error code distinguishing key mismatch from hardware/operational failures. This allows the OTAR service to trigger a warm-start re-key without requiring the user to manually intervene. The document notes this is optional — OTAR systems detect stale keys through other mechanisms — but it improves user experience.

### MFID=$90 Legacy Compatibility

The informative Annex A addresses real-world backward compatibility with legacy Motorola equipment, which used MFID=$90 in these PDUs before the P25 standard absorbed the material from BAAD-1. Modern implementations use MFID=$00. Equipment that supports both MFID values enables mixed fleets of legacy Motorola SUs and P25-standard FNEs (or vice versa) to coexist. This is a common scenario in public safety agencies that have mixed-vintage equipment.

---

## Key Online Resources

- **TIA Standards Catalog** — TIA-102.BAEJ purchase/status:  
  https://www.tiaonline.org/standards/catalog/

- **PTIG (Project 25 Technology Interest Group)** — P25 standards overview, document list, and implementation guidance for public safety:  
  https://www.project25.org/

- **APCO International** — P25 standards adoption, compliance testing program (CAP):  
  https://www.apcointl.org/technology/p25/

- **DHS SAFECOM P25 CAP** — Compliance Assessment Program for P25 equipment; tests interoperability of P25 data including CMS functions:  
  https://www.cisa.gov/safecom/p25

- **IHS Markit / S&P Global (Accuristech)** — TIA standards distributor:  
  https://store.accuristech.com/tia

---

## Open-Source Implementations

The CMS protocol defined in this document is not widely implemented in isolation in open-source projects, because conventional P25 packet data is far less common in open-source SDR implementations than trunked voice. However, the following projects contain relevant code:

### OP25 (GNURadio-based P25 SDR)

- **Repository:** https://github.com/boatbod/op25  
- **Relevance:** OP25 implements P25 voice and some data channel monitoring. CAI data packet handling exists but CMS (registration/management layer) is not fully implemented as of the last public commits. The project's `op25/gr-op25-r2/lib/` directory contains MAC layer code relevant to understanding the data packet framing on which CMS PDUs are carried.

### SDRTrunk

- **Repository:** https://github.com/DSheirer/sdrtrunk  
- **Relevance:** SDRTrunk is the most feature-complete open-source P25 receiver. It decodes P25 FDMA data channels and displays data headers. CMS-specific PDU parsing (Registration Request/Response, Scan Preamble, Unable to Decrypt) is not separately enumerated in its decoder as of the last public release, but the CAI data packet layer on which these PDUs ride is decoded. The SAP field dispatch that would be needed to route to CMS handlers is visible in the P25 CAI data layer code.
  - Data header handling: `src/main/java/io/github/dsheirer/module/decode/p25/phase1/message/pdu/`

### p25-toolkit / related tools

Various smaller P25 analysis tools on GitHub parse CAI data packets but generally do not implement the CMS management layer. Search GitHub for `p25 packet data registration` or `p25 CMS` for current state.

### Implementation Notes for Open-Source Developers

To implement CMS per this document, the key integration points are:

1. **SAP dispatch in the CAI data receiver:** Route inbound PDUs to the CMS handler based on the SAP field value from TIA-102.BAAC-C:
   - SAP = Registration & Authorization → CMS registration PDU handler
   - SAP = Packet Data Scan Preamble → Scan Preamble handler
   - SAP = Packet Data Encryption Support → Unable to Decrypt handler

2. **RT field dispatch within CMS:** Parse octet 0, bits [7:4] as the RT value (Table 3) to distinguish Connect / Disconnect / Accepted / Denied.

3. **State machine:** Implement the SU three-state machine (Closed → Register → Open) and FNE two-state machine (Closed ↔ Open) per Section 3.2.

4. **MFID handling:** Check the MFID field in the CAI Data Packet Header Block; accept both $00 and $90 per Annex A guidance.

---

## Document History and Lineage Notes

- The CMS material in this document originated in the TIA-102.BAAD-1 addendum, which extended TIA-102.BAAD-B (Conventional Procedures). When the P25 data documents were reorganized, CMS for Packet Data was extracted into this standalone document.
- TIA-102.BAAD-1 is cancelled by this document.
- The BAEJ document was adopted August 17, 2013 and published October 4, 2013 as ANSI/TIA-102.BAEJ-2013.
- No revision (BAEJ-A, -B, etc.) has been published as of April 2026. The reference [6] (TIA-102.BAEB-B) was a draft at time of publication; its final publication may have resulted in editorial updates to this document's scope, but no formal revision is known.
