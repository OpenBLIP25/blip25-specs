# TIA-102.BAJD Related Resources and Context
# Project 25 — TCP/UDP Port Number Assignments

---

## Status

**Active.** TIA-102.BAJD was published in October 2010 as a first-issue TIA Standard. No
superseding revision has been identified. As a port number registry document, it is not
expected to change frequently; revisions would be triggered only by new P25 subsystems
requiring named port assignments or significant expansion of the reserved block.

The document is not listed as obsolete or withdrawn in available TIA catalog information.
It remains the canonical reference for P25 TCP/UDP port number defaults within the
TIA-102 suite.

---

## Standards Family

This document belongs to the **TIA-102 Project 25** standards suite, specifically the
**BAJ-series** which covers P25 data application and network interface topics.

### Companion Documents in the BAJ Series

| Document          | Title (approximate)                                      |
|-------------------|----------------------------------------------------------|
| TIA-102.BAJA      | P25 Inter-RF Subsystem Interface (ISSI) — Data          |
| TIA-102.BAJB-A    | P25 Console Subsystem Interface (CSSI)                   |
| TIA-102.BAJB-B    | P25 CSSI (revision)                                      |
| TIA-102.BAJC-A    | P25 Fixed Station Interface (FSI)                        |
| TIA-102.BAJC-B    | P25 FSI (revision)                                       |
| **TIA-102.BAJD**  | **TCP/UDP Port Number Assignments (this document)**      |

### Documents that Consume the Port Assignments

| Document              | Port(s) Used                         |
|-----------------------|--------------------------------------|
| TIA-102.AACA series   | Port 64414 (OTAR), Port 49199 (KMF)  |
| TIA-102.BACA series   | ISSI voice/data — IP transport       |
| TIA-102.BAEA/BAEB     | Console interfaces — IP transport    |
| TIA-102.BAHA series   | Fixed Station Interface — IP transport|

### Standards Lineage (ASCII Tree)

```
TIA-102 Project 25 Standards Suite
└── BAJ Series: Data Applications & Network Interfaces
    ├── TIA-102.BAJA — ISSI Data
    ├── TIA-102.BAJB-A/B — CSSI
    ├── TIA-102.BAJC-A/B — FSI
    └── TIA-102.BAJD — TCP/UDP Port Number Assignments (this document)
         │
         ├── References (normative):
         │   └── IANA Port Number Assignments Registry
         │
         └── References (informative):
             ├── RFC 768 — UDP (IETF, 1980)
             ├── RFC 793 — TCP (IETF, 1981)
             └── RFC 2780 — IANA IP Header Allocation Guidelines (IETF, 2000)
```

---

## Practical Context

### How This Document Is Used

P25 systems increasingly use IP backhaul for inter-subsystem communication. The ISSI
(Inter-RF Subsystem Interface), CSSI (Console Subsystem Interface), FSI (Fixed Station
Interface), KMF (Key Management Facility), OTAR, and location services all operate over
standard TCP/UDP/IP transport. Without a coordinated port assignment scheme, equipment
from different manufacturers could have non-interoperable default configurations or
conflict with other applications running on the same IP infrastructure.

This document solves that problem by providing a registry that:
1. Tells manufacturers which ports are reserved for P25 and must not be used for other purposes
2. Tells implementers what default port numbers to configure in their applications
3. Provides a basis for firewall rules and network ACLs in P25 deployments

The three specifically named ports are the most implementation-relevant:
- **49198** — Tier 2 Location Service (GPS/AVLS data)
- **49199** — Inter-KMF Interface (key distribution between Key Management Facilities)
- **64414** — OTAR (Over-the-Air Rekeying, used for encryption key distribution to subscribers)

All three should be configurable per the document's requirements, which is important in
environments where these ports may conflict with existing network services or where
security policies require non-default port assignments.

### Deployment Implications

Network engineers deploying P25 systems should be aware of the following:
- The OTAR port (64414) is in the high end of the dynamic/private range, near the top of
  the 16-bit port space (max 65535). Some firewall implementations or NAT devices may
  treat high-numbered ports differently.
- The reserved blocks are non-contiguous, which complicates writing precise firewall rules
  without either listing each range individually or using broader rules that include
  available gaps.
- The "should be configurable" requirement means deployed systems should expose port
  configuration in their management interfaces.

---

## Key Online Resources

- **IANA Port Number Registry** (normative reference):
  https://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.xhtml

- **TIA Standards Catalog** (for ordering or status checking):
  https://www.tiaonline.org/standards/catalog/

- **RFC 768 — User Datagram Protocol** (informative reference):
  https://www.ietf.org/rfc/rfc0768.txt

- **RFC 793 — Transmission Control Protocol** (informative reference):
  https://www.ietf.org/rfc/rfc0793.txt

- **RFC 2780 — IANA Allocation Guidelines For Values In the Internet Protocol and Related Headers** (informative reference):
  https://www.ietf.org/rfc/rfc2780.txt

- **APCO Project 25 Technology Interest Group**:
  https://www.apcointl.org/technology/project-25/

- **P25 Standards Documents (DHS SAFECOM)**:
  https://www.cisa.gov/safecom/p25

---

## Open-Source Implementations

Open-source P25 implementations that use TCP/UDP transport and would be affected by these
port assignments:

- **OP25** (Osmocom P25 receiver/decoder):
  https://github.com/boatbod/op25
  OP25 implements P25 trunk/control channel decoding. Network-facing features would use
  these port assignments for any IP interface components.

- **SDRTrunk**:
  https://github.com/DSheirer/sdrtrunk
  SDRTrunk is a Java-based P25 trunked radio decoder. It includes P25 Phase 1 and Phase 2
  decoding. Any network streaming or ISSI/CSSI interface implementation would reference
  these port assignments.

- **p25tools / p25lib**:
  Various community repositories implement P25 framing. Search GitHub for `p25 otar`,
  `p25 issi`, or `p25 kmf` for implementations that would use ports 64414, 49199, and 49198.

- **Osmocom**:
  https://osmocom.org/projects/p25
  Osmocom has P25-related projects that may implement IP interfaces using these port values.

*Note: As of the processing date, no open-source project has been identified that
explicitly implements all three named P25 application ports (49198, 49199, 64414) in a
complete end-to-end manner. The OTAR port (64414) is most likely to appear in KMF or
OTAR server implementations.*

---

## Phase 3 Implementation Notes

**No Phase 3 implementation spec is needed for this document.**

This document is classified as OVERVIEW/registry. Its entire normative content is Table 1
(the port assignment table), which is fully reproduced in the Full Text extraction. There
are no algorithms, message formats, state machines, or protocol procedures to implement
beyond configuring the correct port numbers in application code.

For any P25 application using TCP/UDP, the implementation guidance is:

```rust
// P25 TCP/UDP Port Assignments (TIA-102.BAJD Table 1)
pub const P25_TIER2_LOCATION_SERVICE_PORT: u16 = 49198;
pub const P25_INTER_KMF_INTERFACE_PORT: u16    = 49199;
pub const P25_OTAR_PORT: u16                   = 64414;

// Reserved ranges (do not assign to non-P25 applications)
// 49200-49213, 49226-49249, 49257-49279, 49281-49291

pub fn is_p25_reserved_port(port: u16) -> bool {
    matches!(port,
        49198 | 49199 |
        49200..=49213 |
        49226..=49249 |
        49257..=49279 |
        49281..=49291 |
        64414
    )
}
```
