# TIA-102.BAJD-A Related Resources
## Project 25 TCP/UDP Port Number Assignments

---

## Document Status

**Active.** TIA-102.BAJD-A (January 2019) is the current revision of this
document. It supersedes TIA-102.BAJD (August 2010). It was adopted by the
Project 25 Steering Committee on October 18, 2018 as part of the Project 25
Standard. No further revision has been published as of early 2026.

The document is available for purchase from:
- TIA store: https://store.accuristech.com/tia
- Standards cataloged at: https://www.tiaonline.org/standards/catalog/

---

## Standards Family Placement

TIA-102.BAJD-A belongs to the **TIA-102 "BAJ" sub-series**, which covers
IP-based data interfaces and device connectivity for P25 infrastructure and
subscriber units. Within the broader TIA-102 numbering scheme:

- **AA series**: Air interface and common channel standards
- **BA series**: Infrastructure and inter-subsystem interfaces
  - **BAJ sub-series**: Specific data applications and device interfaces
    - BAJB: Key Fill Device (KFD) Interface — Part 1
    - BAJC: Key Fill Device (KFD) Interface — Part 2
    - **BAJD: TCP/UDP Port Number Assignments** (this document)

---

## Standards Lineage

```
Project 25 Standard (P25 Steering Committee / APCO / TIA)
└── TIA-102 Series (P25 Technical Standards)
    ├── TIA-102.AAXX  (Air Interface)
    ├── TIA-102.BAXX  (Infrastructure / Inter-subsystem)
    │   ├── TIA-102.BACA  (ISSI – Inter-RF Subsystem Interface)
    │   ├── TIA-102.BACD  (Inter-KMF Interface)  ← uses port 49199
    │   ├── TIA-102.BAEE  (Tier 2 Location Services) ← uses port 49198
    │   └── TIA-102.BAJX  (Device Interfaces)
    │       ├── TIA-102.BAJB  (Key Fill Device Interface Pt 1) ← uses 49200-49202
    │       ├── TIA-102.BAJC  (Key Fill Device Interface Pt 2) ← uses 49200-49202
    │       └── TIA-102.BAJD-A  (TCP/UDP Port Assignments) ← THIS DOCUMENT
    └── TIA-102.AAXX  (OTAR / Security)
        └── TIA-102.AACA  (OTAR Messages & Procedures)  ← uses port 64414
```

### Predecessor / Successor Chain

```
TIA-102.BAJD (August 2010, original)
    └── TIA-102.BAJD-A (January 2019, Revision A) [CURRENT]
```

---

## Practical Context

### Role in P25 IP Networking

P25 systems increasingly use IP-based backhaul and inter-subsystem interfaces.
The port assignments in this document are the "well-known port" equivalents for
P25 — the fixed constants that allow any two interoperable P25 network elements
to connect without prior negotiation. Key use cases:

1. **OTAR (port 64414)**: A P25 KMF server sends rekeying messages to subscriber
   radios using UDP/TCP to port 64414. All P25-compliant KMF implementations and
   radios with IP connectivity default to this port.

2. **Inter-KMF (port 49199)**: When two separate P25 systems need to exchange
   cryptographic key material between their Key Management Facilities, they
   connect to port 49199 on the peer KMF. Defined in TIA-102.BACD.

3. **Tier 2 Location Service (port 49198)**: Location service clients and servers
   in a P25 Tier 2 (trunked) system use port 49198 for location reporting and
   queries. Defined in the TIA-102.BAEE series.

4. **Key Fill Security Types (ports 49200-49202)**: A Key Fill Device (KFD)
   connecting to a P25 radio or console over IP uses one of these three ports
   depending on the security operation being performed:
   - 49200: Radio Authentication key fill
   - 49201: End-to-End Encryption key fill
   - 49202: Link Layer Encryption key fill
   Defined in TIA-102.BAJB-A and TIA-102.BAJC-A.

### Configurability Requirement

The standard requires that all these port assignments be configurable. In
practice, P25 system administrators can change the port numbers in their
equipment's configuration to avoid conflicts with other applications on the
same IP network, or to meet site security policies. The values in the standard
are defaults only.

---

## Key Online Resources

### TIA / Official

- **TIA Standards Catalog (search for TIA-102.BAJD)**:
  https://www.tiaonline.org/standards/catalog/
- **TIA Store (purchase document)**:
  https://store.accuristech.com/tia
- **P25 Technology Interest Group (P25 TIG)**:
  https://www.project25.org/

### IANA Port Registry (Normative Reference)

- **IANA Service Name and Transport Protocol Port Number Registry**:
  https://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.xhtml
  
  Note: The P25-assigned ports (49198-49202, 64414) fall in the IANA
  "dynamic/private" range (49152-65535) and are not registered with IANA. They
  are managed solely by TIA-102.BAJD-A. IANA does not list them in its registry.

### IETF Protocol References

- **RFC 768 (UDP)**: https://www.rfc-editor.org/rfc/rfc768
- **RFC 793 (TCP)**: https://www.rfc-editor.org/rfc/rfc793
- **RFC 2780 (IANA Allocation Guidelines)**: https://www.rfc-editor.org/rfc/rfc2780

### DHS / CISA P25 Resources

- **CISA P25 Security Evaluation Program**:
  https://www.cisa.gov/safecom/p25
- **DHS SAFECOM P25 Compliance Assessment Program (CAP)**:
  https://www.dhs.gov/science-and-technology/p25-cap

---

## Open-Source Implementations

Open-source P25 projects that implement or reference the port numbers from
this document:

### OP25 (GNU Radio-based P25 decoder/gateway)
- **GitHub**: https://github.com/boatbod/op25
- Implements P25 trunked system monitoring; IP interface components may
  reference P25 port conventions.

### gr-p25 / DSD (Digital Speech Decoder)
- **GitHub (DSD)**: https://github.com/szechyjs/dsd
- P25 Phase 1 IMBE decoding; does not directly implement IP stack.

### p25rx / p25lib
- Various community implementations of P25 data layer decoding exist on
  GitHub; search: https://github.com/search?q=p25+otar

### Osmocom (OsmoP25 / osmo-p25)
- The Osmocom project has exploratory P25 work:
  https://osmocom.org/projects/p25
- IP interface work could reference BAJD-A port assignments.

### MMDVM (Multi-Mode Digital Voice Modem)
- **GitHub**: https://github.com/g4klx/MMDVM
- Implements P25 digital modes; network interfaces may use P25 port conventions.
- **MMDVMHost**: https://github.com/g4klx/MMDVMHost

### Note on Open-Source Coverage

As of the document's publication scope, none of the mainstream open-source P25
projects implement the full IP infrastructure stack (KMF, OTAR server, Tier 2
location service) where these port assignments are most critical. Those
functions are primarily implemented in commercial P25 infrastructure equipment
(Motorola, Harris/L3, Kenwood, Tait). The port assignments are most relevant
to implementers building P25-compliant IP infrastructure nodes.

---

## Related TIA-102 Documents

| Document | Title | Relationship |
|----------|-------|--------------|
| TIA-102.BAJD | TCP/UDP Port Number Assignments (original) | Predecessor, superseded |
| TIA-102.BAJB-A | Key Fill Device Interface (Part 1) | Uses ports 49200-49202 |
| TIA-102.BAJC-A | Key Fill Device Interface (Part 2) | Uses ports 49200-49202 |
| TIA-102.AACA series | OTAR Messages and Procedures | Uses port 64414 |
| TIA-102.BACD | Inter-RF Subsystem Interface KMF | Uses port 49199 |
| TIA-102.BAEE series | Tier 2 Location Services | Uses port 49198 |
| TIA-102.BACA series | ISSI Voice and Mobility | IP transport context |
