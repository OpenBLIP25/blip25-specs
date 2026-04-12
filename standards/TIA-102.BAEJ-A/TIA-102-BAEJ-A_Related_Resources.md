# TIA-102.BAEJ-A — Related Resources & Context

**Document:** ANSI/TIA-102.BAEJ-A-2019  
**Title:** Project 25 Conventional Management Service Specification for Packet Data  
**Approved:** March 8, 2019

---

## Status

**Active.** This document (Revision A, approved March 8, 2019) is the current edition. It cancels and replaces TIA-102.BAEJ (original edition, October 2013). The revision refined the CMS Scan Mode operational text (FNE Scan Preamble duration and SU scan period description) and updated normative references to point to the then-current editions of companion documents. No subsequent revision has been identified as of April 2026.

The document carries ANSI approval (ANSI/TIA-102.BAEJ-A-2019) and was adopted by the Project 25 Steering Committee on October 18, 2018.

---

## Standards Family

This document belongs to the **TIA-102 Series B (Radio Systems Interface)** sub-family for **Conventional Data** services. It specifies a management layer that sits between the air interface (TIA-102.BAAA-B) and the packet data bearer services (TIA-102.BAEA-C, TIA-102.BAEB-B).

### Normative Dependencies (inbound — documents this spec depends on)

| Document | Title | Role |
|---|---|---|
| TIA-102.BAAA-B | Project 25 FDMA Common Air Interface | Confirmed/unconfirmed delivery service definitions; CAI Response PDU (NACK–Invalid User) |
| TIA-102.BAAC-D | Project 25 CAI Reserved Values | SAP assignments (Registration & Authorization SAP; Packet Data Scan Preamble SAP; Packet Data Encryption Support SAP); LLID definition; MFID values |
| TIA-102.BAEA-C | Project 25 Data Overview and Specification | Overall data framework; Conventional FNE Data configuration; local policy definitions; SNDCP Context Management |
| TIA-102.BAEB-B | Project 25 IP Data Bearer Service Specification | SNDCP (Subnetwork Dependent Convergence Protocol) scan mode; SNDCP IP address binding |
| TIA-102.AAAD-B | Project 25 Block Encryption Protocol | ES Auxiliary Header format; 2ndary SAP (EP SAP); KID (KEY ID); ALG ID fields referenced in Unable to Decrypt PDU |
| TIA-102.AACA-A | Project 25 OTAR Protocol | KM SAP; KEYID; warm-start procedure triggered by Unable to Decrypt |
| RFC 791 | Internet Protocol | IPv4 address format (32-bit, 4-octet) |

### Directly Replaced

| Document | Status |
|---|---|
| TIA-102.BAEJ (2013) | Cancelled and replaced by TIA-102.BAEJ-A |
| TIA-102.BAAD-1 (addendum) | Original source material for the CMS packet data registration specification |

### Companion Documents in the CMS / Conventional Data Sub-family

| Document | Title | Relationship |
|---|---|---|
| TIA-102.BAEJ-A | This document | CMS for conventional packet data (registration, scan, encryption info) |
| TIA-102.BAEA-C | Data Overview and Specification | Master framework; defines Conventional FNE Data configuration |
| TIA-102.BAEB-B | IP Data Bearer Service Specification | SNDCP bearer; IP address binding for SNDCP configurations |
| TIA-102.BAAD-B | Conventional Procedures | Broader conventional channel procedures |
| TIA-102.BAAC-D | CAI Reserved Values | SAP table; MFID registry |
| TIA-102.BAAA-B | FDMA Common Air Interface | Physical and data-link layer |

---

## Practical Context

### Where This Fits in P25 Conventional Packet Data

In a conventional P25 system, a channel is typically designated as a packet data channel. Subscriber radios (SUs) must register with the Fixed Network Equipment (FNE) before they can send or receive IP or CAI data over that channel. This document defines that registration handshake and the management signals that accompany it.

Three packet data bearer service modes are addressed:
- **CAI Data Bearer Service** — raw CAI data frames; IP address binding N/A; scan mode managed by this spec
- **IP Data Bearer Service with SCEP** — IP packets over Simple CAI Encapsulation Protocol; IP binding and scan mode both managed by this spec
- **IP Data Bearer Service with SNDCP** — IP packets over Subnetwork Dependent Convergence Protocol; IP binding and scan mode managed by SNDCP's own Context Management function (TIA-102.BAEB-B), not by this spec

### Scan Mode in Practice

CMS Scan Mode allows a radio to interleave monitoring of a packet data channel with scanning of other conventional channels (e.g., voice channels). The FNE sends empty "Scan Preamble" packets — just a CAI header block with no payload — to alert a scanning radio that data is coming. The radio must return to the packet data channel within its scan period. System designers must balance scan period length against channel throughput: long scan periods force the FNE to pre-amble for longer, wasting channel capacity.

### Encryption Support

The Unable to Decrypt PDU provides a lightweight out-of-band signal for encrypted data failures. It is particularly useful in OTAR (over-the-air rekeying) deployments, where a key management server can automatically warm-start an SU when the SU reports it cannot decrypt a data frame. The PDU carries enough information (SAP, key ID, algorithm ID, current keyset index) for the key management service to identify and resolve the failure.

### Motorola MFID=$90 Compatibility (Annex A)

Before this document's standardization, Motorola's proprietary implementation used MFID=$90 in the CAI Data Packet Header for CMS packet data PDUs. The standard uses MFID=$00. As of Revision A, the MFID=$00 and MFID=$90 PDU formats are identical in content; the MFID byte is the only difference. Interoperable systems should accept both forms. Manufacturers deploying MFID=$90 PDUs are advised to license Motorola's specification to receive future updates.

---

## Key Online Resources

- **TIA Standards Catalog** (purchase/access): https://store.accuristech.com/tia
- **TIA TR-8 committee page** (TR-8.5 Signaling and Data Transmission Subcommittee): https://www.tiaonline.org/standards/engineering-committees/
- **P25 Technology Interest Group (PTIG)**: https://www.project25.org — official P25 steering committee resource; documents adoption status
- **DHS SAFECOM P25 resources**: https://www.dhs.gov/safecom/p25 — federal procurement and interoperability guidance
- **NPSTC (National Public Safety Telecommunications Council)**: https://www.npstc.org — operational guidance for P25 deployments
- **IHS Markit / Accuris** (standard purchase): https://store.accuristech.com/tia

---

## Open-Source Implementations

The CMS packet data registration protocol is implemented in several open-source P25 software stacks, primarily in the FNE and gateway components:

### SDRTrunk
- **URL:** https://github.com/DSheirer/sdrtrunk
- **Relevance:** SDRTrunk decodes P25 conventional and trunked channels. Its data channel decoders parse CAI data packet headers including the Registration and Authorization SAP traffic; relevant to understanding the on-air encoding that carries CMS PDUs.

### OP25
- **URL:** https://github.com/boatbod/op25
- **Relevance:** GNU Radio-based P25 decoder. Handles conventional data channel framing. SAP-layer parsing for Registration PDUs would be found in the data block assembly code.

### dvmhost / DVM Project (Digital Voice Modem)
- **URL:** https://github.com/DVMProject/dvmhost
- **Relevance:** Open-source FNE implementation targeting P25 (and DMR). The `dmr`/`p25` sub-projects include conventional channel management logic. The CMS registration and packet data management described in this document is directly relevant to the FNE-side implementation in dvmhost.
- **Network host:** https://github.com/DVMProject/dvmhost

### p25-and-friends / p25lib
- Various Python and C implementations of P25 framing exist as research/educational tools; search GitHub for `p25 packet data registration` for current active forks.

*Note: Full protocol-level CMS registration (RT/RO field parsing, state machine, Unable to Decrypt) is a relatively niche layer; most open-source implementations focus on voice and trunking. The FNE-side logic is more commonly found in proprietary dispatch console and gateway software.*

---

## Standards Lineage

```
TIA-102 (Project 25 Standards Suite)
│
├── Series A: Common Air Interface
│   └── TIA-102.BAAA-B  FDMA Common Air Interface (physical + data link)
│   └── TIA-102.BAAC-D  CAI Reserved Values (SAP assignments, MFID registry)
│
├── Series B: Radio Systems Interface / Conventional
│   │
│   ├── TIA-102.BAEA-C  Data Overview and Specification
│   │   └── Conventional FNE Data configuration
│   │   └── SNDCP Context Management (via TIA-102.BAEB-B)
│   │
│   ├── TIA-102.BAEB-B  IP Data Bearer Service Specification
│   │   └── SNDCP bearer; scan mode for SNDCP configurations
│   │
│   ├── TIA-102.BAAD-1  [Addendum — original source material for CMS PD Registration]
│   │
│   ├── TIA-102.BAEJ    [2013 — original CMS for Packet Data; CANCELLED]
│   │
│   └── TIA-102.BAEJ-A  [2019 — THIS DOCUMENT — CMS for Packet Data, Revision A]
│       ├── Packet Data Registration (Static + Dynamic)
│       ├── SU Location Tracking
│       ├── CMS Scan Mode
│       └── Packet Data Supplementary Encryption Info (Unable to Decrypt)
│
├── Series A (Encryption):
│   ├── TIA-102.AAAD-B  Block Encryption Protocol (ES Auxiliary Header)
│   └── TIA-102.AACA-A  OTAR Protocol (warm-start, key management)
│
└── (Other series: trunking, ISSI, CSSI, vocoder, testing...)
```
