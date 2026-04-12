# TIA-102.AACA-D — Related Resources & Context

## Status

**Active.** TIA-102.AACA-D was published April 2025 (final published document revision 1: 2025-05-16).
It supersedes TIA-102.AACA-C. This is the current normative OTAR standard for P25 systems.
No known replacement is in progress as of the document publication date.

The document is issued under ANSI accreditation by the Telecommunications Industry Association
(TIA) Engineering Committee TR-8. It is sponsored by APCO International, NASTD, and US federal
agencies. Compliance with prior revisions (AACA-B, AACA-C) does not impugn compliance with
earlier versions; equipment must declare which version of the OTAR document it implements.

## Standards Family

This document sits in the TIA-102 (Project 25) suite under the AACA sub-series:

```
TIA-102 (Project 25 Digital Radio Suite)
├── AAAB-x  Common Air Interface (physical, MAC layers)
├── AAAC-x  Data Link Independent (now absorbed into other docs)
├── AAAD-x  Encryption and Authentication Algorithms (ALGID registry)
│            └── Referenced normatively by AACA for ALGID values
├── AACA-x  Over-The-Air-Rekeying (OTAR) — THIS DOCUMENT
│   ├── AACA-A   (initial release)
│   ├── AACA-A-1 (addendum: single-key/multi-key Key ID interop)
│   ├── AACA-B   (FPIC DES note; absorbed AACA-A-1)
│   ├── AACA-C   (added CMAC; deprecated D bit in MAC frame)
│   └── AACA-D   (refined CMAC context; 2025 — CURRENT)
├── BAEB-x  IP Data Bearer Service (DLI OTAR transport layer)
├── BBAC-x  Trunking Control Channel MAC procedures
├── BACA-x  ISSI (Inter-RF-Subsystem Interface)
└── [AACD]  OTAR Conformance Test Document (companion to AACA)
```

Companion documents referenced normatively within this document:
- **[1]** APCO Project 25 System and Standards Definition (TSB-102.BAAA)
- **[2]** TIA-102.AAAB — Common Air Interface
- **[4]** TIA-102.AAAC — Packet Data
- **[5]** TIA-102.AAAD — Encryption and Authentication (ALGID registry)
- **[6]** TIA-102.BAEB — IP Data Bearer Service
- **[7]** TIA-102.AAAC-C or equivalent for SNDCP
- **[9]** Assigned P25 OTAR UDP port number
- **[10]** FIPS 46-3 (DES)
- **[11]** ANSI X3.92 (DES input/output conventions)
- **[12]** FIPS 197 (AES)
- **[14]** NIST SP 800-38F (KW-AE key wrapping)
- **[15]** ANSI X9.52 (Triple DES)
- **[19]** RFC 768 (UDP)
- **[24]** NIST SP 800-108 (KDF in Counter mode)
- **[25]** NIST SP 800-38B (CMAC)
- **[26]** RFC 4493 / SP 800-38B (AES-CMAC)

## Practical Context

### How OTAR Is Used in Real-World Systems

P25 OTAR is the standard mechanism by which public safety radio systems — police, fire, EMS,
federal law enforcement — manage encryption keys across potentially thousands of subscriber
radios. Without OTAR, each radio must be physically brought to a technician for key loading
(KVL, Key Variable Loader). With OTAR:

- The KMF (Key Management Facility) is typically a dedicated server appliance at the
  dispatch/system manager site (e.g., Motorola KMF, Harris SecNet KMF, EF Johnson KMF).
- During normal operation, the KMF sends Rekey-Command KMMs to SUs over the traffic channel
  or control channel, wrapped in DLD OTAR (CAI data packets) or DLI OTAR (IP/UDP/SNDCP).
- SUs acknowledge receipt with Rekey-Acknowledgment (Response Kind 3/Immediate) or
  Delayed-Acknowledgment (Response Kind 2).
- The Hello message flow allows SUs to self-identify and request rekey when they know they
  are out of sync.
- The Warm-Start procedure handles the case where an SU has a KEK but no valid TEKs —
  the KMF delivers TEKs encrypted with the KEK without outer-layer CAI encryption.
- The Zeroize-Command remotely deletes all keys; the SU must then execute the New Unit
  procedure to rejoin OTAR service.
- OTAR can operate in trunked (TDMA/FDMA) and conventional FNE configurations.

### Security Hierarchy in Practice

KMMs shall not use a weaker algorithm to wrap or authenticate a stronger-keyed payload
(Table 81). Recommended hierarchy:
- Transporting DES keys: inner layer DES or AES-256; outer layer DES or AES-256; MAC DES or AES-256
- Transporting TDES keys: inner/outer/MAC TDES or AES-256
- Transporting AES keys: inner/outer/MAC AES-256 only

CMAC (Version 1 MAC frame) is the recommended MAC method for AES-keyed systems as of AACA-C/D.
CBC-MAC (Version 0 MAC frame) remains supported for backward compatibility.

### Known Interoperability Considerations

- Mixed single-key and multi-key equipment deployments require careful Key ID alignment
  (Annex B, Historical Key ID).
- Equipment compliant with AACA-B may not support CMAC (added in AACA-C).
- The D (Derived Key) bit in the MAC Format field was deprecated in AACA-C; implementations
  shall set it to 1 for backward compat but ignore it for decision-making.
- DLI OTAR is restricted to Trunked and Conventional FNE Data configurations; Conventional
  SCEP uses DLD even when the IP Data Bearer is available.

## Key Online Resources

- **TIA Standards Store** — purchase the official document:
  https://store.accuristech.com/tia
- **TIA TR-8 Committee** — P25 standards development:
  https://www.tiaonline.org/standards/technology/p25/
- **APCO P25 Technology Interest Group (PTIG)**:
  https://www.apcointl.org/technology/p25/
- **NIST P25 CAP (Compliance Assessment Program)** — tests equipment against P25 standards
  including OTAR; DHS Science & Technology funded:
  https://www.dhs.gov/science-and-technology/p25-cap
- **FPIC (Federal Partnership for Interoperable Communications)** — issued the DES
  deprecation memo referenced in Annex A:
  https://www.dhs.gov/sites/default/files/publications/SAFECOM-NCSWIC_DES_Memo.pdf
- **NIST SP 800-38B (CMAC specification)**:
  https://csrc.nist.gov/publications/detail/sp/800-38b/final
- **NIST SP 800-38F (Key Wrap specification)**:
  https://csrc.nist.gov/publications/detail/sp/800-38f/final
- **NIST SP 800-108r1 (KDF in Counter Mode)**:
  https://csrc.nist.gov/publications/detail/sp/800-108/rev-1/final

## Open-Source Implementations

OTAR key loading and protocol parsing appear in several open-source P25 projects. Full
KMF implementation (key generation, distribution management) is not open-source due to
operational security requirements of the public safety community.

### SDRTrunk (Java)
- **URL**: https://github.com/DSheirer/sdrtrunk
- Implements P25 Phase 1 and Phase 2 decoding including trunked systems.
- Parses OTAR KMM message headers and displays message type/RSI fields for monitoring.
- Does not implement key unwrapping (requires key material not available to passive receivers).
- Relevant packages: `io.github.dsheirer.module.decode.p25.message.pdu.otar`

### OP25 (Python/C++ / GNU Radio)
- **URL**: https://github.com/boatbod/op25
- GnuRadio-based P25 receiver and decoder.
- OTAR message parsing present; key loading is done via separate KMF interface when
  keys are known (for authorized monitoring/testing scenarios).
- Relevant file: `op25/gr-op25-r2/lib/p25p2_duid.cc` and OTAR-related Python scripts.

### p25-analyzer / p25craft (various GitHub forks)
- Various researchers have published OTAR frame parsers for forensic/testing purposes.
- Search GitHub for "p25 otar" to find current active forks.

### libp25 / p25lib
- Several C/Rust/Python libraries implement P25 frame structures; OTAR message formats
  (as defined in this document's Section 10) are included in more complete implementations.

### Interoperability Test Tools
- The NIST P25 CAP testing infrastructure exercises OTAR procedures as part of conformance
  testing. Test vectors in Sections 14.1-14.3 of this document are used for verifying
  KMM construction and MAC computation.

## Standards Lineage (ASCII Tree)

```
APCO Project 25 (P25) Digital Radio Standards
└── TIA-102 Series (ANSI/TIA-102, maintained by TR-8)
    │
    ├── Physical / MAC Layer
    │   ├── TIA-102.AAAB-A/B  Common Air Interface (CAI)
    │   └── TIA-102.BBAB      TDMA Physical Layer (Phase 2)
    │
    ├── Data Layer
    │   ├── TIA-102.AAAC      Packet Data (SNDCP)
    │   └── TIA-102.BAEB      IP Data Bearer Service
    │
    ├── Encryption & Key Management
    │   ├── TIA-102.AAAD      Encryption & Authentication (ALGID registry)
    │   │                      DES ($81), TDES ($83), AES-256 ($84), ...
    │   │
    │   └── TIA-102.AACA      Over-The-Air-Rekeying (OTAR)
    │       ├── AACA-A         Initial release
    │       ├── AACA-A-1       Addendum: single/multi-key Key ID interop
    │       ├── AACA-B         DES deprecation note; absorbed AACA-A-1
    │       ├── AACA-C         Added CMAC; deprecated D bit
    │       └── AACA-D         THIS DOCUMENT (2025)
    │
    ├── Trunking & Control
    │   ├── TIA-102.AABC      Trunking Control Channel Messages
    │   └── TIA-102.BBAC      MAC/Scheduler Procedures
    │
    └── Conformance Testing
        └── [AACD]            OTAR Conformance Test Document
```

## Notes on Document Evolution

The four revisions of this document reflect the P25 community's incremental adoption of
stronger cryptography:

- **AACA-A**: Original OTAR, DES-based, single-key equipment model.
- **AACA-B**: Clarified key protection hierarchy; added multi-key equipment support;
  incorporated FPIC guidance on DES deprecation (DES retained for backward compat only).
- **AACA-C**: Added CMAC (AES-256 based, SP 800-38B) as the recommended MAC algorithm,
  superseding the CBC-MAC derived-key approach for new deployments.
- **AACA-D** (this document): Tightened CMAC key derivation context to fully specify the
  KDF input parameters, enabling interoperable CMAC MAC computation across vendors.

Compliance is per-version; declaring AACA-D compliance requires support for CMAC with the
KDF defined in Section 13.5.2.2.2, while an AACA-B-only device need only support CBC-MAC.
