# TIA-102-AAAB-A Related Resources and Context

**Document:** TIA-102.AAAB-A + TIA-102.AAAB-A-1  
**Full title:** Project 25 Security Services Overview (base) + Addendum 1: Key Management Architecture  
**ANSI designation:** ANSI/TIA-102.AAAB-A-1:2014  
**Generated:** 2026-04-12

---

## Status

**Active / Superseded status: Active (as of April 2026 to the best available knowledge)**

TIA-102.AAAB-A was published January 2005. TIA-102.AAAB-A-1 (the addendum processed here) was approved
September 9, 2014 and incorporates into AAAB-A the Key Management Architecture section (Section 7).
Together they constitute the current form of this standard.

No publicly available evidence of a subsequent full revision or supersession was found. The two incomplete
sections at publication time (IP Data Encryption §7.1.4 and Air Interface Encryption §7.1.6) were pending
ongoing ETG discussions; it is unknown whether supplemental addenda were later published to complete them.

The base document TIA-102.AAAB-A is referenced as a normative dependency by multiple other TIA-102
standards, confirming its continued role in the P25 security architecture family.

---

## Standards Family

This document is part of the **TIA-102 (Project 25) suite**, specifically the security sub-family:

### Direct companions (referenced normatively by this document)

| Document | Title | Relationship |
|----------|-------|--------------|
| TIA-102.AAAB-A | Project 25 Security Services Overview (base) | This addendum modifies it |
| TIA-102.AACA-A | Over the Air Rekeying (OTAR) Messages and Procedures | OTAR KMM details |
| TIA-102.AACD-A | KFD Interface Protocol | Key Fill Device interface protocol |
| TIA-102.AACD-A-1 | KFD Interface Protocol Addendum 1 | KFD protocol extensions |
| TIA-102.BAKA | KMF to KMF Interface | Inter-KMF S/MIME key exchange |
| TIA-102.AAAD-A | Block Encryption Protocol | CAI data encryption PDU format |
| TIA-102.AACE-A | Link Layer Authentication | LLA algorithm and message details |
| TIA-102.BACA-B | ISSI Messages and Procedures | ISSI authentication session transfer |

### Broader security context

| Document | Title | Relationship |
|----------|-------|--------------|
| TIA-102.AAAA-A | Encryption and Authentication | Core algorithm suite (DES, AES) |
| TIA-102.AABB-A | OTAR Protocol | Earlier OTAR specification |
| TIA-102.BAKA | KMF-to-KMF Interface | Detailed IKI protocol spec |
| TIA-102.AACE-A | Link Layer Authentication | Protocol-level LLA detail |
| TSB-102.BBAA | P25 System and Standards Definition | Top-level architecture overview |

### Standards lineage (ASCII tree)

```
TIA-102 Project 25 Suite
└── Security Sub-Family
    ├── TIA-102.AAAB-A  (Security Services Overview, 2005) ← base document
    │   └── TIA-102.AAAB-A-1  (Addendum 1: Key Management Architecture, 2014) ← THIS DOCUMENT
    │
    ├── Key Management Protocols
    │   ├── TIA-102.AACA-A  (OTAR Messages and Procedures)
    │   ├── TIA-102.AACD-A  (KFD Interface Protocol)
    │   ├── TIA-102.AACD-A-1  (KFD Interface Addendum 1)
    │   └── TIA-102.BAKA  (KMF-to-KMF Interface)
    │
    ├── Encryption Protocols
    │   ├── TIA-102.AAAD-A  (Block Encryption Protocol)
    │   └── TIA-102.AAAA-A  (Encryption and Authentication)
    │
    └── Authentication
        ├── TIA-102.AACE-A  (Link Layer Authentication)
        └── TIA-102.BACA-B  (ISSI — includes auth session roaming)
```

---

## Practical Context

### How this standard is used in real systems

This document is the **architectural map** for P25 security. Equipment vendors and system integrators use
it to understand how the functional components fit together before implementing the protocol-level specs:

**Key Management Facility (KMF):** Software systems sold by vendors such as Motorola Solutions
(ASTRO KMF), Harris/L3Harris (SecNet), and Thales Group implement the KMF role. These systems manage
key generation, cryptonet configuration, OTAR delivery, and inter-KMF key distribution. The functional
models in Figures 6-8 and 20 are the reference architecture for these products.

**Key Fill Device (KFD):** Physical devices (e.g., Motorola KVL 4000/5000 series, Tait KFD) implement
the wireline key provisioning path. The KFD interface protocol (TIA-102.AACD-A) governs these devices;
this overview document maps where KFDs fit in the larger architecture (Figures 9, 21, 22, 29).

**Subscriber Units (SUs):** P25 portable and mobile radios (APX, XTS, Harris XL-200P, BK Radio,
Kenwood NX series) implement the Voice Encryption Endpoint and LLA Endpoint roles. The SU's Cryptonet
membership and TEK/KEK provisioning follow the functional model in Figure 7.

**OTAR deployment:** Large P25 systems (statewide interoperability networks, FirstNet-affiliated
deployments) rely on OTAR to rekey radios without physical contact. The OTAR path through E_d →
RFSS → CAI (Figure 11) and cross-ISSI OTAR (Figure 14) are standard deployment patterns for these
networks.

**Conventional CAI encryption:** County-level and municipal Conventional FDMA systems use CAI Data
Encryption (Section 7.1.3) to protect packet data. The SU-to-FNE and SU-to-SU configurations
(Figures 18-19) describe the two primary deployments; the Block Encryption Protocol (AAAD-A) defines
the PDU format.

**Link Layer Authentication:** Deployed in Trunked systems to prevent rogue radio registration. LLA is
implemented in modern P25 trunking infrastructure (Motorola ASTRO 25, Harris Unity XG-100P systems)
and in SUs that support the AACE-A specification.

### Known incomplete sections at publication

Two sections were explicitly deferred at time of this addendum's publication (September 2014):
- **§7.1.4 IP Data Encryption** — architecture discussions were ongoing within the P25 ETG
- **§7.1.6 Air Interface Encryption (Link Layer Encryption)** — same status

It is unknown from public sources whether subsequent addenda to AAAB-A were published to complete
these sections, or whether this material was addressed in other P25 standards.

---

## Key Online Resources

### TIA Standards
- TIA standards catalog (requires purchase): https://www.tiaonline.org/standards/catalog/
- IHS Markit/S&P Global (TIA standards distributor): https://store.accuristech.com/tia

### P25 Technology Interest Group (PTIG)
- Overview of P25 security features: https://www.project25.org/index.php/p25-technology/security
- The PTIG publishes implementation guides and conformance test documents supplementing the TIA specs.

### APCO International
- P25 standards overview: https://www.apcointl.org/technology/standards/p25/
- APCO hosts the APIC (P25 Interface Committee) that governs the standard.

### DHS SAFECOM / FirstNet
- P25 Security Overview (SAFECOM): https://www.cisa.gov/safecom/p25
- DHS/CISA publishes procurement guidance citing TIA-102.AAAB-A as the security architecture reference.

### NIST
- NIST SP 800-53 references P25 security in LMR context for federal systems.
- AES (FIPS 197) and the use of AES in P25: https://csrc.nist.gov/publications/fips/fips197/fips-197.pdf

### RFC 4301 (informative reference cited in the document)
- Security Architecture for the Internet Protocol: https://www.rfc-editor.org/rfc/rfc4301

---

## Open-Source Implementations

The following open-source projects implement components of the P25 security architecture described
in this document:

### OP25 (GNU Radio-based P25 receiver/decoder)
- Repository: https://github.com/boatbod/op25
- Relevance: Implements P25 Phase 1 and Phase 2 decoding. Handles encrypted voice frames (recognizes
  TEK/MI/ALGID in the CAI header per AAAD-A); does NOT decrypt without keys (by design).
  The codebase reflects the Voice Encryption endpoint model from Section 7.1.1.

### SDRTrunk
- Repository: https://github.com/DSheirer/sdrtrunk
- Relevance: Java-based P25 trunking decoder. Handles encrypted traffic channel identification and
  CAI data parsing. Implements the CAI PDU structure relevant to Section 7.1.3.

### DSD (Digital Speech Decoder) / DSD+
- Repository: https://github.com/szechyjs/dsd (original; DSD+ is closed-source)
- Relevance: Early P25 decoder; does not implement key management but processes the voice frame
  structures described in Section 7.1.1.1.

### p25lib / gr-p25 (GNU Radio P25 blocks)
- Various forks exist; the primary gnuradio-based implementation is within OP25 above.

### JMBE (Java Multi-Band Excitation) / AMBE codec
- Repository: https://github.com/DSheirer/jmbe
- Relevance: P25 IMBE/AMBE vocoder library; required alongside encryption for encrypted voice decode
  once keys are available. Referenced by SDRTrunk.

**Note on key management open-source:** No open-source KMF implementations were identified. KMF
software is commercially sensitive and sold by a small number of LMR vendors. The OTAR protocol
(TIA-102.AACA-A) and KFD interface (TIA-102.AACD-A) have no known public open-source implementations
as of April 2026.

---

## Document Hierarchy (detailed)

```
TIA-102.AAAB-A (Security Services Overview, Jan 2005)
│  Defines: security service categories (confidentiality, integrity, authentication),
│            key management overview, physical/OTAR key distribution summary
│
└── TIA-102.AAAB-A-1 (THIS DOCUMENT, Sep 2014)
       Adds: Section 7 — Key Management Architecture (informative)
       Modifies: Section 6 intro text
       Defers: IP Data Encryption (§7.1.4), Air Interface Encryption (§7.1.6)

Protocol-level dependencies (normative, called out in §C.1):
  ├── TIA-102.AACA-A ─── OTAR KMM formats, procedures, MAC, sequence numbering
  ├── TIA-102.AACD-A ─── Key Fill Device Interface (KFD↔SU physical provisioning)
  ├── TIA-102.AACD-A-1 ─ KFD Interface extensions (addendum, under development at pub time)
  ├── TIA-102.BAKA ───── KMF-to-KMF Interface (S/MIME protected key exchange between KMFs)
  ├── TIA-102.AAAD-A ─── Block Encryption Protocol (CAI PDU encryption format)
  ├── TIA-102.AACE-A ─── Link Layer Authentication (AM1–AM4 algorithms, LLA message formats)
  └── TIA-102.BACA-B ─── ISSI Messages and Procedures (auth session roaming via ISSI)

Informative references:
  ├── TSB-102.BBAA ────── P25 System and Standards Definition (shell document)
  └── RFC 4301 ─────────── IETF Security Architecture for IP (background context)
```
