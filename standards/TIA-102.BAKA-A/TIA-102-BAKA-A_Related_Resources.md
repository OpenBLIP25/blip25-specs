# TIA-102.BAKA-A: Related Resources and Context

## Document Lineage

| Version | Published | Notes |
|---|---|---|
| TIA-102.BAKA | ~2007 | Original KMF-to-KMF interface specification |
| TIA-102.BAKA-A | July 26, 2024 | This edition; adds Annex A deprecating RSA-PKCS#1-v1.5, mandates RSAES-OAEP + RSASSA-PSS |

The -A edition is a security-hardening update. The primary normative change is Annex A, which formally deprecates RSA-PKCS#1-v1.5 padding in both signing and encryption roles, replacing it with RSASSA-PSS and RSAES-OAEP respectively. All legacy implementations using PKCS#1-v1.5 may maintain it only for backward interoperability with pre-publication equipment.

---

## Direct Dependencies (Normative References Within This Document)

| Document | Title | Role in BAKA-A |
|---|---|---|
| TIA-102.BAJD | KMF to KMF Transport | Defines TCP port 49199, STX/ETX framing characters used by this protocol |
| TIA-102.AACA | P25 OTAR Messages and Procedures | KMF-to-radio key distribution; BAKA-A is the KMF-to-KMF counterpart |
| TIA-102.AACE | P25 Key Management | Overall key management framework; defines key classes (KEK, TEK, AUTHK) |
| TIA-102.AABC | TDMA Common Air Interface | Talkgroup addressing: WACN, System ID, Group ID used in ObjectCreate |
| RFC5751 | S/MIME Version 3.2 Message Specification | Outer-layer cryptographic envelope format |
| RFC8551 | S/MIME Version 4.0 | SHA256 digest for legacy RSA-PKCS#1-v1.5 mode (Annex A reference) |
| RFC5280 | Internet X.509 PKI Certificate and CRL Profile | Certificate structure requirements |
| RFC3447 | RSA Cryptography Specifications v2.1 (PKCS#1) | RSA-PKCS#1-v1.5 (deprecated per Annex A) |
| RFC5234 | ABNF for Syntax Specifications | Notation used for PDU grammar definitions |
| W3C XML Schema 1.0 | XML Schema Definition Language | Schema language for ikischema.xsd |

---

## Related P25 Standards (Broader Context)

| Document | Title | Relationship |
|---|---|---|
| TIA-102.BAJD-A | KMF to KMF Transport | Transport layer for BAKA-A messages (likely companion revision) |
| TIA-102.BAJB | KMF to Fixed Station Interface | Analogous interface for KMF-to-infrastructure key distribution |
| TIA-102.BAJC | KMF to Console Interface | KMF-to-dispatcher key management |
| TIA-102.BADA | Over-the-Air Rekeying (OTAR) | Radio-side key loading that KMF manages via AACA |
| TIA-102.BAAA | Common Air Interface (CAI) | P25 FDMA air interface; defines radio key loading primitives |
| TIA-102.BACA | ISSI Voice and Mobility | Inter-RF Subsystem Interface; talkgroup interop that motivates key sharing between KMFs |

---

## Relevant IETF RFCs

| RFC | Title | Relevance |
|---|---|---|
| RFC5751 | S/MIME v3.2 | Primary outer-layer format for IKMF-PDU signing/encryption |
| RFC8551 | S/MIME v4.0 | SHA-256 digest reference for legacy PKCS#1-v1.5 mode |
| RFC5280 | X.509 PKI Certificate Profile | Certificate format for KMF signing and encryption certs |
| RFC4056 | Use of RSASSA-PSS in X.509 Certificates | PSS signature use in PKI |
| RFC3560 | Use of RSAES-OAEP in CMS | OAEP key encryption in CMS/S/MIME |
| RFC3447 | PKCS#1 v2.1: RSA Cryptography Specifications | PKCS#1-v1.5 (deprecated) and OAEP/PSS definitions |
| RFC5234 | Augmented BNF | PDU grammar notation used in Section 4 |

---

## OpenSSL Implementation Notes

The normative Section 8 OpenSSL configuration (openssl.cnf) and the Section 6.4 command examples provide a complete working reference for implementing the cryptographic layer. Key observations for implementers:

**Signing (sender):**
```
openssl cms -sign -in <plaintext_pdu> -out signed.pem -outform SMIME \
  -signer <sender_signing_cert.pem> -inkey <sender_signing_key.pem> \
  -keyopt rsa_padding_mode:pss -md sha384
```
The `-keyopt rsa_padding_mode:pss` flag selects RSASSA-PSS. Without it, OpenSSL defaults to PKCS#1-v1.5 (not compliant with this standard except legacy mode).

**Encryption (sender, after signing):**
```
openssl cms -encrypt -in signed.pem -out encrypted.pem -outform SMIME \
  -recip <recipient_encryption_cert.pem> \
  -aes-256-CBC \
  -keyopt rsa_padding_mode:oaep \
  -keyopt rsa_oaep_md:sha384
```
Uses AES-256-CBC for content encryption with RSAES-OAEP (SHA-384) for the key wrap.

**Decryption (receiver):**
```
openssl cms -decrypt -in encrypted.pem -inform SMIME \
  -out decrypted.pem \
  -recip <my_encryption_cert.pem> \
  -inkey <my_encryption_key.pem>
```

**Signature verification (receiver):**
```
openssl cms -verify -in decrypted.pem -inform SMIME \
  -CAfile cacert.pem \
  -signer <sender_signing_cert.pem>
```
Output ends with "Verification successful" on success.

**CA certificate generation (3072-bit, sha384):**
```
openssl req -x509 -new -nodes -keyout cakey.pem -out cacert.pem \
  -days 700 -config openssl.cnf
```

---

## Open-Source Implementations and Ecosystem

### P25 Software Projects

**op25** (GNU Radio-based P25 receiver/transmitter)
- GitHub: boatbod/op25
- Does not implement KMF functionality; focused on PHY/MAC
- Relevant for understanding P25 system context

**p25toolkit** / various SDR P25 decoders
- Multiple projects (DSD, OP25, etc.) decode P25 voice/control channels
- None known to implement the KMF-to-KMF XML interface as of 2024

**Osmocom project** (osmocom.org)
- Has some P25 work in the GSM/cellular software radio space
- No known KMF implementation

### Why No Open-Source KMF Implementations Exist
The KMF-to-KMF interface involves handling live encryption key material for public safety systems. This creates significant barriers:
1. Legal: Key management for P25 systems is regulated; unauthorized key distribution could compromise law enforcement communications
2. Complexity: Requires full P25 system infrastructure (RFSS, radios) to be meaningful
3. Market: KMF software is sold by vendors (Motorola, Harris/L3, Tait) under CALEA-compliant licensing

### Rust Ecosystem Relevance

For implementing this spec in Rust:

**XML parsing:**
- `quick-xml` — fast event-driven XML parser (good for known schemas)
- `serde-xml-rs` — serde-based XML deserialization
- `roxmltree` — read-only XML tree (good for parsing received messages)
- Consider code-generating Rust types from ikischema.xsd using `xsd-parser-rs`

**S/MIME / CMS:**
- `openssl` crate — bindings to libssl; supports `cms_sign`, `cms_encrypt`
- `cms` crate (RustCrypto) — pure Rust CMS implementation
- `rsa` crate (RustCrypto) — RSA-OAEP and RSA-PSS implementations
- `pkcs7` / `x509-cert` / `cms` from RustCrypto collection

**TLS:**
- `rustls` — pure Rust TLS 1.2/1.3 (verify cipher suite support for ECDHE_RSA_WITH_AES_256_GCM_SHA384)
- `native-tls` — system TLS bindings

**MIME multipart:**
- `mime` crate — MIME type parsing
- `multer` — multipart body parsing (HTTP-focused but adaptable)
- Manual parsing of `--P25_IKMF_BODY` boundaries is straightforward for this constrained format

---

## Phase 3 Implementation Spec Flags

The following implementation specification documents should be produced (not included in this processing run):

### 1. `P25_IKMF_Message_Format_Implementation_Spec.md`
**Type:** MESSAGE_FORMAT  
**Scope:** Complete Rust data structures mirroring ikischema.xsd, including:
- All 9 message type structs with serde XML (de)serialization
- Elementary type newtype wrappers (RSI, keyID, WacnId, etc.) with validation
- Normative enum types (msgID, keyClass, Algorithm, Reason, Readiness)
- Return code enum with complete Table 15 values
- MIME PDU framing/parsing (single and multi-part)
- CTG correlation tag management

### 2. `P25_IKMF_Protocol_State_Machine_Implementation_Spec.md`
**Type:** PROTOCOL  
**Scope:** KMF state machine for managing IKMF procedure flows, including:
- Uncoordinated vs. coordinated procedure dispatch
- CTG correlation and timeout handling
- Long-running operation tracking (KeyDis, KeyAct, KeyDel with progress)
- Automatic activation/expiration procedure flows
- Operating profiles (Originating / Terminating / Bilateral)
- Return code handling and error recovery

### 3. `P25_IKMF_Crypto_Layer_Implementation_Spec.md`
**Type:** SECURITY  
**Scope:** S/MIME outer-layer cryptographic operations, including:
- RSASSA-PSS signing with SHA-384 (required) vs. PKCS#1-v1.5 (legacy only)
- RSAES-OAEP encryption with SHA-384 / AES-256-CBC (required)
- X.509 certificate management (dual-cert per KMF: signing + encryption)
- CA trust establishment and certificate exchange procedures
- Key material encoding (RSA-OAEP encrypted hexBinary blob format)
- TLS 1.2 transport integration (optional)

---

## Standards Purchase and Access

- TIA standards are available at: https://store.accuristech.com/tia
- Contact TIA at: standards@tiaonline.org
- TIA-102.BAKA-A is part of the TIA-102 Project 25 Standards series
