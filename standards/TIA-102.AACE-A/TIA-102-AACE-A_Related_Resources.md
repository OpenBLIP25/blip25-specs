# TIA-102.AACE-A — Related Resources & Context
# Project 25 Digital Land Mobile Radio: Link Layer Authentication

---

## Status

**Active standard.** TIA-102.AACE-A (April 2011) is the current revision of the
P25 link-layer authentication specification. It supersedes TIA 102.AACE (September
2005). As of 2026 no further revision or withdrawal has been published by TIA. The
document is listed in the TIA standards catalog and remains the normative reference
for P25 challenge-and-response authentication.

The development history (8 revisions from 2003–2010 before final ballot) reflects
significant industry review through the APCO Project 25 Interface Committee (APIC)
Encryption Task Group. The -A revision corrected sample data errors in AM2 and AM4
that were present in the original release.

---

## Standards Family

This document belongs to the **TIA-102.AAxx** security sub-suite within the larger
TIA-102 (Project 25) standards family. It defines the authentication layer that
works alongside:

| Document         | Title / Role |
|------------------|--------------|
| TIA-102.AAAD     | Encryption of voice and data (traffic encryption) |
| TIA-102.AACA     | Over-The-Air-Rekeying (OTAR) Protocol — informative ref [6] |
| TIA-102.AACD     | Key Fill Device (KFD) Interface Protocol — informative ref [5] |
| TIA/EIA-102.AABC | Trunking Control Channel Messages — normative ref [1]; defines wire format of authentication messages |
| TIA-102.BACA     | ISSI (Inter-RF Subsystem Interface) — consumes authentication results |
| TIA-102.AABB     | Project 25 Security Services Overview (architecture context) |

The document explicitly states it covers the U_m (radio) interface only. Inter-system
authentication behavior over ISSI is addressed in TIA-102.BACA.

### Standards Lineage (ASCII Tree)

```
TIA-102 Project 25 Suite
└── TIA-102.AAxx  (Security)
    ├── TIA-102.AAAD   Encryption (voice/data traffic)
    ├── TIA-102.AACA   OTAR Protocol
    ├── TIA-102.AACE-A Link Layer Authentication  ← THIS DOCUMENT
    ├── TIA-102.AACD   Key Fill Device Interface
    └── TIA/EIA-102.AABC  Trunking Control Channel Messages
                           (defines auth message wire formats)

External Normative Dependencies:
    ├── FIPS 197       AES (the cipher used by AM1–AM4)
    └── NIST SP 800-38A Block Cipher Modes of Operation (ECB mode)
```

---

## Practical Context

### How Authentication Is Used in Real Systems

P25 authentication is deployed in large-scale public safety trunked radio networks.
In practice:

- **Authentication Facility (AF)**: Typically runs inside or alongside the Fixed
  Network Equipment controller (e.g., within a Zone Controller or Master Site
  Controller). It holds the K-SUID database, generates RS, and computes KS/KS'
  using AM1/AM3 before distributing them to the RFSS.

- **RFSS implementation**: The Radio Frequency Sub-System receives KS (and KS' for
  mutual auth) from the AF without ever seeing K. It independently generates RAND1,
  computes XRES1 via AM2, and challenges the SU. Multiple authentications can reuse
  the same RS/KS with different RAND1 values, reducing AF load.

- **SU implementation**: Subscriber units store K in non-volatile memory, typically
  hardware-protected (smart card or secure element in modern equipment). K is loaded
  via Key Fill Device (using the KFD protocol, TIA-102.AACD) or OTAR (TIA-102.AACA).

- **Crypto period management**: Vendors implement RS rotation schedules to limit
  exposure of any single KS. The specification is silent on mandatory rotation
  intervals; this is a system-level policy decision.

- **Standalone vs. registration-embedded**: Most implementations trigger
  authentication during unit registration. Standalone authentication (outside
  registration) and SU-demanded authentication are less commonly implemented but
  are normatively required to be supported.

### Mutual Authentication Deployment

Mutual authentication (where the SU verifies the RFSS identity) is less commonly
enabled in deployed systems due to operational complexity—particularly the
"Change Site" behavior, where a mutually-enabled SU roams away from a site that
declines mutual auth. Some agency security policies require it for high-security
environments (federal law enforcement, DOD interoperability under FirstNet).

### Relationship to Encryption

Authentication and encryption are independent supplementary services in P25. A call
can be authenticated but unencrypted, encrypted but unauthenticated, both, or neither.
Authentication prevents unauthorized device access; encryption (TIA-102.AAAD) protects
call content.

---

## Key Online Resources

- **TIA Standards Catalog** — Primary source for the current published document:
  https://www.tiaonline.org/standards/catalog/

- **APCO Project 25 Technology Interest Group (PTIG)** — Standards coordination body;
  publishes implementation guides and conformance test documents:
  https://www.project25.org/

- **DHS SAFECOM Program** — Federal guidance on P25 procurement and security for
  public safety agencies; references authentication requirements:
  https://www.cisa.gov/safecom

- **NIST FIPS 197 (AES)** — The underlying cipher normatively referenced:
  https://csrc.nist.gov/publications/detail/fips/197/final

- **NIST SP 800-38A (Block Cipher Modes)** — ECB mode definition referenced:
  https://csrc.nist.gov/publications/detail/sp/800-38a/final

- **IHS Markit / Accuristech** — Commercial source for purchasing the TIA document:
  https://store.accuristech.com/tia

---

## Open-Source Implementations

The four authentication mechanisms (AM1–AM4) are straightforward compositions of
AES-128-ECB with fixed padding/truncation. Several open-source P25 projects
implement or reference this standard:

### OP25 (GNU Radio-based P25 decoder)
- **Repository**: https://github.com/boatbod/op25
- Authentication support: OP25 is primarily a passive monitor/decoder. It does not
  implement SU-side authentication (which requires K), but the codebase decodes
  authentication control channel messages (Authentication Demand, Response, etc.)
  to log authentication events.
- Relevant files: `op25/gr-op25-legacy/lib/p25p1_fdma.cc` and related trunking logic.

### SDRTrunk (Java-based P25 decoder/monitor)
- **Repository**: https://github.com/DSheirer/sdrtrunk
- Authentication support: SDRTrunk decodes and logs P25 trunking control channel
  messages including authentication messages. It does not implement the cryptographic
  mechanisms (AM1–AM4) as that requires K.
- Message parsing for TIA/EIA-102.AABC messages (including auth messages):
  `src/main/java/io/github/dsheirer/module/decode/p25/phase1/message/`

### Osmocom P25 / osmo-p25
- The Osmocom project has explored P25 protocol stacks. Search the Osmocom
  repository list for P25-related projects:
  https://gitea.osmocom.org/

### Direct AES-128-ECB Implementation
The AM1–AM4 algorithms reduce entirely to standard AES-128-ECB with fixed-width
zero-padding and MSB-truncation. Any language with an AES library can implement
them directly. The test vectors in Section 6.6 are sufficient for verification.

**Note on XRES1/XRES2 computation**: The RFSS computes XRES1 = AM2(KS, RAND1) and
compares it to the SU's RES1. Since this uses the same function, the network side
and the subscriber side run identical code paths with the same inputs. Implementations
for testing can verify both sides with the Section 6.6 vectors.

---

## Implementation Notes for Open-Source P25 Projects

Any implementation of P25 authentication for a software-defined SU or FNE needs:

1. **AES-128-ECB**: Standard library call (OpenSSL `EVP_aes_128_ecb`, Rust `aes`
   crate with `ecb` mode, Python `cryptography` package, etc.).

2. **RS expansion** (80→128 bits): Concatenate the 10-byte RS with 6 zero bytes.

3. **RAND expansion** (40→128 bits): Concatenate the 5-byte RAND with 11 zero bytes.

4. **RES truncation** (128→32 bits): Take the first 4 bytes (MSB) of the AES output.

5. **AM3 complement**: Bitwise NOT of the 16-byte expanded RS before encryption.

6. **Control channel message parsing**: Requires TIA/EIA-102.AABC for the actual
   bit-field layouts of the six authentication messages. This document provides the
   message names and MSC flows but not the wire format.

7. **K management**: Secure storage is required. Hardware security modules or
   platform TEEs are appropriate for production implementations.
