# TIA-102.BAEE-C Related Resources & Context
# Project 25 Radio Management Protocols
# Generated: 2026-04-13

---

## Status

**Active.** This document (Revision C, December 2015) is the current edition
of the P25 Radio Management Protocols specification. It supersedes and cancels
TIA-102.BAEE-B (February 2010). No subsequent revision has been identified.

The document was adopted by the Project 25 Steering Committee on October 29,
2015 as part of the Project 25 Standard, giving it ANSI designation
ANSI/TIA-102.BAEE-C-2015.

**Revision lineage:**
- Interim Standard (December 1995) — Initial RCP definition
- Original (February 2000) — Formal publication
- Revision A (August 2004) — Added SNMP support, IANA OID
- Revision B (February 2010) — Reaffirmation update
- Revision C (November 2015) — Current; errata, suite alignment

---

## Standards Family

This document is part of the TIA-102 (Project 25) documentation suite,
specifically within the **BA (Packet Data)** subseries:

```
TIA-102 Project 25 Standard Suite
└── BA — Packet Data
    ├── BAAA — ...
    ├── BAEA-C  — P25 Data Overview and Specification         [parent arch]
    ├── BAEB    — ...
    ├── BAEC    — ...
    ├── BAED    — ...
    ├── BAEE-C  — Radio Management Protocols                  [THIS DOCUMENT]
    ├── BAEF    — ...
    └── BAEG    — Mobile Data Peripheral Interface            [A Interface]
```

**Normative companions (directly referenced):**
- **TIA-102.BAEA-C** (P25 Data Overview and Specification) — Defines the
  overall P25 Packet Data architecture including the A Interface concept that
  this document's protocols operate over.
- **TIA-102.BAEG** (P25 Mobile Data Peripheral Interface) — Defines the
  physical and link-layer A Interface that carries RMP messages.
- **TIA-102.BAAC-C** (P25 Common Air Interface Reserved Values) — Defines
  MFID values used in RCP and SNMP SDU headers.
- **TSB-102-B** (P25 TIA-102 Documentation Suite Overview) — Describes where
  this document fits in the broader 129-document P25 suite.

**IETF dependencies (all normative):**
- RFC 768 (UDP), RFC 1155/1157/1212/1213 (SNMPv1/MIB-II),
  RFC 2578/2579/2580 (SMIv2), RFC 3411–3418 (SNMPv2/v3 framework)

---

## Practical Context

### How this standard is used in real-world equipment and systems

The A Interface exists in P25 mobile data systems where a laptop, tablet,
or ruggedized terminal (the MDP) connects to a P25 radio via a serial or
IP link. Common deployments include:

- **Law enforcement vehicle installations** — A laptop or MDT connects via
  the A Interface to a P25 mobile radio. The MDT uses RCP to query the radio's
  registration status, signal strength, and battery level before transmitting
  data packets. If the registration status changes (e.g., the radio moves out
  of coverage), the radio sends an unsolicited RPRT SDU to alert the MDP.

- **Dispatch consoles and gateway systems** — Fixed Network Equipment (FNE)
  data gateways may use RMP to monitor and configure attached radios or radio
  interfaces in FNE Data Configuration mode.

- **Fleet management / remote diagnostics** — The GET_STATS operator provides
  IP datagram counters that can be used for performance monitoring and
  troubleshooting data throughput issues.

### Protocol selection in practice

TIA-102.BAEE-C defines RCP and SNMP as alternative protocols — a system
implements one or the other, not both simultaneously on the same interface.
RCP is the simpler, purpose-built option favored in embedded MDP software.
SNMP is available for operators who have existing SNMP-based network
management infrastructure and want to manage P25 radios alongside other
network elements using standard tools (Net-SNMP, Nagios, Zabbix, etc.).

### Port assignments

- **RCP**: UDP port 469 (IANA-registered as "radio control protocol")
- **SNMP queries**: UDP port 161 (standard SNMP)
- **SNMP traps**: UDP port 162 (standard SNMP traps)

### MFID encoding

The MFID byte ($00 = standard) in RCP SDUs uses the same encoding as the
P25 CAI Manufacturer ID field. This allows vendor-specific SDU extensions
to be cleanly identified and ignored by non-vendor implementations.

---

## Key Online Resources

- **TIA standards catalog** — Purchase or access the current document:
  https://store.accuristech.com/tia

- **IANA port number registry** (confirms port 469 = "radio control protocol"):
  https://www.iana.org/assignments/port-numbers

- **IANA Private Enterprise Number registry** (confirms PEN 17405 = "TIA TR8dot5"):
  https://www.iana.org/assignments/enterprise-numbers

- **DHS/CISA P25 suite overview** — The DHS SAFECOM program and CISA publish
  P25 compliance guidance that references the data suite:
  https://www.cisa.gov/safecom/P25

- **P25 Technology Interest Group (PTIG)** — Industry body for P25 equipment
  interoperability; references the data suite standards:
  https://www.project25.org

- **APCO International** — The organization whose APIC Data Task Group
  originated the materials underlying this document:
  https://www.apcointl.org

---

## Open-Source Implementations

No open-source project implementing the full RCP or the BAEE-C SNMP MIB has
been identified. However, the following projects handle P25 data services or
adjacent areas:

- **OP25** (Osmocom) — Software-defined P25 decoder/transceiver; handles
  CAI-layer packet data but does not implement the A Interface or RMP layer:
  https://github.com/osmocom/op25

- **SDRTrunk** — P25 trunking decoder for SDR receivers; focuses on voice and
  trunking control, not packet data management:
  https://github.com/DSheirer/sdrtrunk

- **Net-SNMP** — General-purpose SNMP toolkit that could be used to implement
  the BAEE-C SNMP MIB on a P25 gateway system; would require the MIB from
  Annex A to be loaded:
  https://sourceforge.net/projects/net-snmp/

- **Wireshark** — Contains a P25 dissector (APCO P25); does not appear to
  include specific RCP dissection as of 2026, but the protocol is simple enough
  that a dissector could be written using the SDU formats in this document.

**Implementation note for open-source developers:** The RCP protocol is
straightforward to implement. The complete SDU format is defined by:
- Type byte dispatch: $43 (RQST), $52 (RESP), $45 (RPRT)
- 2-byte length field for the Data payload
- 2-byte SDU Tag for request/response correlation ($FFFF = unsolicited report)
- 1-byte MFID ($00 = standard)
- 1-byte operator / report ID in Data[0]
- Operand or response payload in Data[1+]

The SNMP MIB ASN.1 definition is provided verbatim in Annex A of this
document and can be loaded directly into any SMIv2-compliant SNMP toolkit.

---

## Standards Lineage

```
IETF SNMPv1 (1990)                   IETF SNMPv2/v3 (1999-2002)
RFC 1155, 1157, 1212, 1213           RFC 2578-2580, 3411-3418
           |                                    |
           +------------------------------------+
                           |
                   SNMP support added
                   to RMP (Rev A, 2004)
                           |
P25 MOU (April 1992)       |
TIA / APCO DTG             |
           |               |
           v               v
TIA-102.BAEE Interim Standard (Dec 1995)
           |
           | Radio Control Protocol (RCP) only
           v
TIA-102.BAEE Original (Feb 2000)
           |
           v
TIA-102.BAEE-A (Aug 2004) — SNMP addendum, IANA OID 17405
           |
           v
TIA-102.BAEE-B (Feb 2010) — Reaffirmation update
           |
           v
TIA-102.BAEE-C (Dec 2015) [CURRENT] — Errata, suite alignment
  ANSI/TIA-102.BAEE-C-2015
  Adopted by P25 Steering Committee: Oct 29, 2015
           |
           | Used by / relates to:
           v
TIA-102.BAEA-C — P25 Data Overview and Specification (Dec 2015)
TIA-102.BAEG   — P25 Mobile Data Peripheral Interface (May 2013)
TIA-102.BAAC-C — P25 CAI Reserved Values (April 2011)
```

---

## Phase 3 Flag: Implementation Specs Needed

This document is classified **MESSAGE_FORMAT + PROTOCOL**. The following
implementation spec should be produced in a follow-up pass:

**`TIA-102-BAEE-C_RCP_Message_Parsing_Spec.md`** — Should include:
- Complete Type byte dispatch table (RQST=$43, RESP=$52, RPRT=$45)
- Operator dispatch table for RQST SDUs (7 operators)
- Report ID dispatch table for RPRT SDUs (3 report types)
- Byte-level format diagrams for all 10 RQST variants, 8 RESP variants,
  3 RPRT variants
- SDU Tag correlation logic (request/response matching, $FFFF sentinel)
- MFID handling (passthrough for $00, skip logic for unknown MFIDs)
- Failure response handling (abbreviated form: no payload after result code)
- Parser pseudocode in Rust showing the full dispatch + extraction pattern
- The complete SNMP MIB ASN.1 (already in Annex A of this document, verbatim)
- Notes on the rmpConfigStatusMode state machine (volatile/NV boundary)
