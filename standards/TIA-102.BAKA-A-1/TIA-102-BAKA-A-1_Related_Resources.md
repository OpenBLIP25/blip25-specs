# TIA-102-BAKA-A-1 Related Resources and Context

**Document:** TIA-102.BAKA-A-1 — Project 25 KMF to KMF Interface, Addendum 1  
**Published:** December 2024  
**Addendum to:** TIA-102.BAKA-A

---

## Status

**Active.** This addendum was published December 2024 as the first addendum to TIA-102.BAKA-A.
It does not supersede BAKA-A; it supplements it by adding TLS 1.3 as a second optional
transport security suite. TIA-102.BAKA (April 2012) was the original KMF-to-KMF interface
standard, subsequently revised to TIA-102.BAKA-A. TIA-102.BAKA-A-1 is the current most recent
update to that revision.

The document lineage:
- TIA-102.BAKA (original, ~2012)
- TIA-102.BAKA-A (revision, added TLS 1.2 optional security)
- TIA-102.BAKA-A-1 (December 2024 addendum, adds TLS 1.3 optional security)

---

## Standards Family

This document belongs to the TIA-102 encryption and key management sub-family within the P25
suite. The key management documents relevant to this addendum include:

| Document            | Title                                              | Relationship |
|---------------------|---------------------------------------------------|--------------|
| TIA-102.BAKA        | KMF to KMF Interface (original)                   | Ancestor     |
| TIA-102.BAKA-A      | KMF to KMF Interface (current base standard)      | Parent       |
| TIA-102.BAKA-A-1    | KMF to KMF Interface — Addendum 1 (this document) | This doc     |
| TIA-102.BAJD        | P25 TCP Port Assignments                          | Referenced (port 49199) |
| TIA-102.BAJD-A      | P25 TCP/UDP Port Assignments (revision)           | Referenced   |
| TIA-102.AACA        | OTAR Messages and Procedures                      | Companion    |
| TIA-102.AACA-B/C/D  | OTAR Messages revisions                           | Companion    |
| TIA-102.AACD        | KMF Messages and Procedures                       | Companion    |
| TIA-102.AACD-A/B    | KMF Messages revisions                            | Companion    |

The KMF-to-KMF interface (Inter-KMF Interface, IKI) enables two KMF systems operated by
different agencies to exchange key material, commands, and acknowledgments using S/MIME-wrapped
payloads over TCP, optionally secured by TLS. This is distinct from the KMF-to-radio (OTAR)
path defined in TIA-102.AACA.

---

## Practical Context

Key Management Facilities are centralized servers that manage encryption keys for P25 radio
fleets. In single-agency deployments, one KMF manages all radios. In multi-agency deployments
(e.g., mutual aid, regional interoperability), radios from different agencies must share
encryption keys managed by different KMFs — this is the use case the BAKA interface addresses.

The KMF-to-KMF interface allows:
- Inter-agency key distribution (shared keys pushed between agencies)
- Coordinated rekeying across agency boundaries
- Return-receipt confirmation of key delivery

The transport architecture uses S/MIME for message-layer encryption and signing, with optional
TLS as a transport-layer security wrapper. TLS 1.2 was the prior state of the art; this
addendum's addition of TLS 1.3 matters because TLS 1.3 eliminates weak cipher suites, mandates
forward secrecy, and reduces handshake round-trips.

**Real-world usage:** Major P25 KMF vendors include Motorola Solutions (ASTRO 25 KMF), L3Harris,
and Tait Communications (EnableProtect). These platforms implement the BAKA interface for
inter-agency key coordination in public safety radio networks. Full implementation details are
vendor-proprietary and not publicly documented beyond product datasheets.

**TCP port 49199** is the standardized port for KMF real-time transport as defined in
TIA-102.BAJD. KMFs may use other ports by administrative agreement.

---

## Key Online Resources

- **TIA Press Release — BAKA-A-1 publication:**
  https://tiaonline.org/press-release/tia-publishes-new-standard-tia-102-baka-a-1/

- **TIA Standards store (purchase required):**
  https://store.accuristech.com/tia

- **CISA P25 Security User Needs Statement (DHS/CISA):**
  https://www.cisa.gov/sites/default/files/publications/21_0609_p25_the-spun_508c.pdf

- **P25 Technology Interest Group (PTIG) — Encryption Resources:**
  https://project25.org/index.php/p25-security-and-encryption-resources

- **Motorola Solutions ASTRO 25 KMF:**
  https://www.motorolasolutions.com/en_us/products/p25-products/security/kmf.html

- **L3Harris KMF Datasheet:**
  https://www.l3harris.com/sites/default/files/2025-06/l3harris-kmf-key-management-facility-datasheet-a-cs-pspc.pdf

- **RFC 8446 — TLS 1.3 (normatively referenced):**
  https://www.rfc-editor.org/rfc/rfc8446

- **RFC 5289 — TLS Elliptic Curve Cipher Suites with SHA-256/384 and AES GCM:**
  https://www.rfc-editor.org/rfc/rfc5289

- **RFC 5280 — Internet X.509 PKI Certificate and CRL Profile:**
  https://www.rfc-editor.org/rfc/rfc5280

- **RFC 793 — Transmission Control Protocol:**
  https://www.rfc-editor.org/rfc/rfc793

---

## Open-Source Implementations

No open-source projects are known to implement the KMF-to-KMF interface. The KMF interface
involves key material handling under encryption, making it a vendor-controlled space in
practice:

- **OP25** (https://github.com/boatbod/op25) — P25 signal decoder, no KMF support
- **SDRTrunk** (https://github.com/DSheirer/sdrtrunk) — P25 signal decoder/recorder, no KMF support
- **JMBE** — vocoder library only, unrelated

The KMF-to-KMF interface (BAKA) is not implemented in any known open-source project. The
inter-agency key management path requires handling live cryptographic key material, which places
it outside the scope of passive monitoring/decoding tools.

---

## Standards Lineage

```
TIA-102 (P25 Standards Suite)
└── Encryption and Key Management Sub-Family
    ├── TIA-102.AAAD-A      Key Management (OTAR algorithms)
    ├── TIA-102.AACA-D      OTAR Messages and Procedures (radio-to-KMF path)
    ├── TIA-102.AACD-B      KMF Messages and Procedures
    └── KMF-to-KMF Interface (Inter-KMF Interface, IKI)
        ├── TIA-102.BAKA    (original, ~2012)
        ├── TIA-102.BAKA-A  (revision, added TLS 1.2 optional transport security)
        └── TIA-102.BAKA-A-1 (December 2024 addendum — THIS DOCUMENT)
                             Adds TLS 1.3 cipher suite (TLS_AES_128_GCM_SHA256,
                             RSA 4096, RFC 8446) as second optional security option

Port Assignment Reference:
        └── TIA-102.BAJD / BAJD-A  (TCP/UDP port 49199 for KMF real-time transport)
```

---

## Phase 3 Implementation Notes

This document is classified **PROTOCOL**. A Phase 3 implementation spec could be produced
covering:

1. **TLS connection establishment state machine** — TCP connect → TLS handshake (client/server
   role selection based on TCP initiator/acceptor), cipher suite negotiation per section 3.2,
   fallback to no-TLS for interoperability.

2. **Certificate management spec** — The three-certificate model (signature cert, encryption
   cert, TLS cert) with key size requirements (3072 vs 4096 by TLS version), PKI hierarchy
   per RFC 5280, and optional dual-purpose cert path.

3. **Cipher suite selection logic** — Conditional support: TLS 1.2 mandatory if TLS supported,
   TLS 1.3 optional; administrative agreement path for other cipher sets.

However, given the narrow scope of this addendum (it only modifies 6 sections of BAKA-A),
the full Phase 3 spec for the KMF-to-KMF interface should be produced against TIA-102.BAKA-A
(the parent document), with this addendum's TLS 1.3 additions incorporated as a delta. Flag
for a follow-up pass after TIA-102.BAKA-A is processed.
