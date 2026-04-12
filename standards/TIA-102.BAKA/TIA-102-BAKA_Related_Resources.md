# TIA-102.BAKA — Related Resources and Document Context

## Document Identity

| Field | Value |
|---|---|
| Document Number | TIA-102.BAKA |
| Full Title | Project 25 KMF to KMF Interface |
| Published | April 19, 2012 |
| Publisher | Telecommunications Industry Association (TIA) |
| Series | TIA-102 Series B — Infrastructure |
| Pages | 160 |
| Status | Superseded by TIA-102.BAKA-A |

---

## Document Family and Lineage

### Predecessor / Successor

| Document | Relationship | Notes |
|---|---|---|
| TIA-102.BAKA | This document | Original KMF-to-KMF interface spec, 2012 |
| TIA-102.BAKA-A | Successor (revision) | Updated version of BAKA; supersedes this document |
| TIA-102.BAKA-A-1 | Addendum to BAKA-A | Adds TLS 1.3 security solution (most recent) |

### Normative Dependencies (referenced as required by this document)

| Document | Title | Relationship |
|---|---|---|
| TIA-102.AAAA-B | P25 Security Services Overview | Defines encryption algorithms, key types (TEK/KEK/AUTHK), and security service concepts used throughout BAKA |
| TIA-102.AACA-C | P25 OTAR Messages and Procedures | Key ID format (2-byte) used in KeyDissemination is compatible with OTAR key numbering |
| TIA-102.BABA | P25 Vocoder (IMBE/AMBE+2) | Referenced for system context (informative-level reference in practice) |
| ITU-T X.509 | PKI Certificate Framework | X.509 certificates required for KMF signing and encryption |
| RFC 3851 | S/MIME Version 3.1 | Non-real-time transport security (signing + encryption of PDUs) |
| RFC 2045 | MIME Part One | MIME encoding for IKMF PDU containers |
| RFC 2822 | Internet Message Format | Email message format for non-real-time transport |
| RFC 2246 | TLS 1.0 | Real-time transport security (note: superseded by RFC 8446 TLS 1.3) |

### Related TIA-102 Series B Documents (Infrastructure)

| Document | Title | Relationship |
|---|---|---|
| TIA-102.BAAA-A | Fixed Station Interface (FSI) | Infrastructure interface; KMF may interface with fixed stations |
| TIA-102.BABA | P25 Inter-RF Subsystem Interface (ISSI) Overview | System-level context for KMF placement in P25 infrastructure |
| TIA-102.BACA-B | P25 ISSI Voice and Mobility Messages and Procedures | ISSI protocol; KMFs serve the radio subsystems interconnected via ISSI |

### Related TIA-102 Series A Documents (Air Interface / Security)

| Document | Title | Relationship |
|---|---|---|
| TIA-102.AAAA-B | Security Services Overview | Foundation for all P25 security including KMF operations |
| TIA-102.AACA-C | OTAR Messages and Procedures | Over-The-Air Re-keying; complementary to IKI (OTAR delivers keys to radios; IKI delivers keys between KMFs) |
| TIA-102.AACA-D | OTAR Messages and Procedures (2025) | Most recent OTAR revision |
| TIA-102.AACD | P25 Digital Mobile Radio Authentication | Authentication key (AUTHK) management interfaces with KMF |

---

## External Standards and References

### IETF RFCs

| RFC | Title | Role in BAKA |
|---|---|---|
| RFC 2045 | MIME Part One: Format of Internet Message Bodies | IKMF PDU encoding |
| RFC 2246 | TLS Protocol Version 1.0 | Real-time transport (superseded; BAKA-A-1 adds TLS 1.3) |
| RFC 2822 | Internet Message Format | Email headers for S/MIME transport |
| RFC 3851 | S/MIME v3.1 Message Specification | Outer-layer PDU security |
| RFC 3850 | S/MIME v3.1 Certificate Handling | PKI certificate usage for S/MIME |
| RFC 3852 | Cryptographic Message Syntax (CMS) | CMS used by S/MIME (superseded by RFC 5652) |
| RFC 5652 | Cryptographic Message Syntax (CMS) | Current CMS standard |
| RFC 8446 | TLS 1.3 | Used in TIA-102.BAKA-A-1 addendum (not normative in this base document) |

### NIST Publications

| Publication | Title | Role |
|---|---|---|
| NIST SP 800-57 Part 1 | Recommendation for Key Management: General | Key lifecycle guidance applicable to KMF design |
| NIST SP 800-56B | Pair-Wise Key Establishment Using Integer Factorization | RSA key establishment (inner-layer key wrapping) |
| NIST SP 800-131A | Transitioning Cryptographic Algorithms and Key Lengths | Algorithm transition guidance; relevant as TLS 1.0 and RSA-1024 age out |
| FIPS 197 | Advanced Encryption Standard (AES) | AES algorithm used for key encryption and S/MIME outer layer |

### ITU-T Standards

| Standard | Title | Role |
|---|---|---|
| ITU-T X.509 | PKI Certificate Framework | X.509 v3 certificates required for all KMF PKI operations |

---

## P25 Standards Context

### Where BAKA Fits in the P25 Security Architecture

```
P25 Radio Network Security Stack
─────────────────────────────────────────────
Layer                 Document
─────────────────────────────────────────────
Policy / Overview     TIA-102.AAAA-B
                       (Security Services Overview)
                              │
               ┌──────────────┼──────────────┐
               │                             │
Air Interface                          Infrastructure
Key Delivery                            Key Sharing
               │                             │
     TIA-102.AACA-C/D                TIA-102.BAKA / BAKA-A
     (OTAR — delivers keys            (IKI — shares keys
      to radio subscribers)           between KMFs)
               │                             │
               └──────────────┬──────────────┘
                              │
                    Keys in subscriber
                    units and KMFs
─────────────────────────────────────────────
```

The IKI (BAKA) and OTAR (AACA) are complementary:
- OTAR delivers keys from a KMF downward to field radios
- IKI moves keys laterally between KMFs at the infrastructure level
- A key typically originates at one KMF, is shared to peer KMFs via IKI, then
  distributed to radios by each KMF via OTAR

### Key Management in Interoperability Scenarios

When two agencies join forces for a joint operation:
1. Agency A's KMF creates an Encryption Object (ObjCre) in Agency B's KMF via IKI
2. The object contains shared TEKs, KEKs, or AUTHKs for the joint operation
3. Each agency's KMF distributes those keys to its own radios via OTAR
4. Coordinated activation (KeyAct) ensures both agencies activate keys simultaneously
5. After the operation, coordinated deletion (KeyDel) removes the shared keys

---

## Implementation Guidance Notes

### For Implementers

1. **XML validation:** Section 7 XSD is normative. All IKMF messages must be
   schema-valid. Use a validating XML parser.

2. **Inner-layer encryption is mandatory:** Keys must never appear in plaintext
   in IKMF XML. Always RSA-encrypt with the recipient's public key before
   base64-encoding into the `<key>` element.

3. **Two-certificate requirement:** Each KMF needs separate signing and
   encryption X.509 certificates. A single combined-use certificate is not
   compliant with S/MIME best practices.

4. **TLS version:** This document normalizes TLS 1.0. Implementations should
   use TIA-102.BAKA-A-1 which adds TLS 1.3 support, or implement TLS 1.2/1.3
   as a local policy upgrade.

5. **ctag management:** Coordinated procedures can span many minutes (15-minute
   progress intervals). Implementors must persist ctag state across connection
   interruptions.

6. **Return code -19:** The duplicate F_NOT_READY code (-19) is specifically
   used for automatic key expiration failures, distinct from -8 (general
   not-ready). Implementations should handle both.

7. **Multi-part MIME:** For non-real-time (email) transport, batching multiple
   key disseminations in a single multi-part MIME message reduces email overhead
   and is the recommended approach for large key sets.

---

## Document Status and Availability

- TIA-102.BAKA is a Final Publication (2012) and is available from TIA (tiaonline.org)
  and IHS Markit standards distributors.
- It is superseded by TIA-102.BAKA-A, which should be consulted for current
  implementations.
- TIA-102.BAKA-A-1 (addendum) adds TLS 1.3 and is the most current revision.
- All three documents should be referenced together for a complete picture of
  the current IKI specification.

---

*Generated from TIA-102.BAKA (April 2012 Final Publication, 160 pages)*
