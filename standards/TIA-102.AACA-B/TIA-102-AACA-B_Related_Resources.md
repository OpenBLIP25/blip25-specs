# TIA-102.AACA-B — Related Resources and Context

## Status

**Active.** ANSI/TIA-102.AACA-B was published September 8, 2021 and is the current
revision of the P25 OTAR messages and procedures standard. It supersedes ANSI/TIA-102.AACA-A
(and its addendum AACA-A-1, which is incorporated into revision B). Revision B is the document
referenced by current conformance test specifications.

DES-based OTAR (ALGID $81) is technically still normative in this document but is deprecated
for new deployments per FPIC (Federal Partnership for Interoperable Communications) memo
recommending against DES in P25 systems.

---

## Standards Family

This document sits within the TIA-102 suite (Project 25 Standards), in the key management
subsection:

```
TIA-102 Suite
└── Key Management (AACA series)
    ├── TIA-102.AACA-B  ← THIS DOCUMENT
    │   Over-The-Air-Rekeying Messages and Procedures
    │
    ├── TIA-102.AACD    (Subscriber Unit Management / Programming)
    │   Defines SU parameters referenced by OTAR (MNP, keyset config)
    │
    ├── TIA-102.AACB    (KMF Interface Specification)
    │   Defines the northbound interface to the Key Management Facility
    │
    ├── TIA-102.AACE    (OTAR Conformance Test Specification)
    │   Test cases verifying compliance with AACA-B
    │
    └── TIA-102.AAAA    (Common Air Interface / Algorithm IDs)
        Source of Algorithm ID assignments ($80 clear, $81 DES, $83 TDEA, $84 AES-256)
```

**Companion documents referenced normatively:**

| Reference | Document |
|-----------|----------|
| [1]  | TIA-102.AAAA — Common Air Interface (CAI) definition, Algorithm IDs |
| [2]  | TIA-102.BAAA — Data Services CAI standard |
| [4]  | TIA-102.BABA — Enhanced Address / Symmetric Addressing |
| [5]  | TIA-102.AAAA — Algorithm ID assignments |
| [6]  | TIA-102.BAEB — IP Data Bearer Service |
| [7]  | TIA-102.BAEA — Packet Data (SNDCP) |
| [9]  | TIA-102.BAEB-C — P25 OTAR port number |
| [10] | NIST FIPS 46-3 — Data Encryption Standard |
| [11] | ANSI X9.52 — Triple DES (TDEA) |
| [12] | NIST FIPS 197 — Advanced Encryption Standard |
| [14] | NIST SP 800-38F — Key Wrapping (KW-AE / KW-AD) |
| [15] | ANSI X9.52 (also TDEA) |
| [19] | IETF RFC 768 — UDP |
| [20] | TIA-102.BAAA — Common Air Interface |

---

## Practical Context

**How this standard is used in real-world systems:**

P25 OTAR is deployed by law enforcement, fire, EMS, and federal agencies using digital
radio infrastructure. The KMF is typically a server appliance or software system (e.g.,
Motorola Solutions' KMF, Harris/L3Harris KMF, EF Johnson KMF) that maintains key material
for a fleet of subscriber units. When a key rollover is due, the KMF operator initiates a
rekey operation, and the KMF pushes new TEKs to all affected radios via the Rekey-Command
procedure — transparently, while radios are on the channel.

Key operational points:
- OTAR operates over the existing P25 voice/data infrastructure; no dedicated key
  management channel is required.
- DLD (Data Link Dependent) OTAR is the traditional mode, using P25 packet data with
  CAI-layer encryption. The KMM rides in an encrypted CAI packet.
- DLI (Data Link Independent) OTAR uses the P25 IP data bearer service (SNDCP/UDP/IP),
  enabling OTAR over any interface supporting IP (including backhaul-side IP paths).
  DLI is required when the radio is in a Conventional FNE Data Configuration.
- The maximum KMM size is 509 octets (DLD) or 462 octets (DLI after preamble overhead).
  Large rekey operations covering many keysets must be split across multiple KMMs.
- The Warm-Start and Registration procedures address the bootstrap problem: how does an
  SU establish encrypted OTAR communication if it has no keys in common with the KMF?
  The answer is that the SU provides a unique session TEK encrypted under the device's
  UKEK, which the KMF then uses to establish the secure channel.

**Equipment vendors implementing this standard (as of 2024):**
- Motorola Solutions (APX series, ASTRO 25)
- L3Harris Technologies (BeOn, XL series)
- EF Johnson (VP series)
- Kenwood (NX-5000 series, NEXEDGE)
- BK Radio (KNG series)
- Tait Communications (TP series)

---

## Key Online Resources

- **TIA Online (purchase):** https://global.ihs.com/TIA
- **TIA Standards:** https://www.tiaonline.org/
- **PTIG (Project 25 Technology Interest Group):** https://www.project25.org/
  - Hosts P25 CAP (Compliance Assessment Program) resources
  - P25 CAP tests include OTAR conformance per AACE
- **CISA P25 OTAR Resources:** https://www.cisa.gov/sites/default/files/publications/
  SAFECOM-NCSWIC_P25_OTAR_Guide.pdf
  (SAFECOM guidance on OTAR deployment — practical operational guidance)
- **FPIC DES Recommendation:** Referenced in Annex A as the source of the DES deprecation
  notice incorporated into revision B.
- **NIST SP 800-38F (Key Wrapping):** https://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-38F.pdf
  (defines the KW-AE/KW-AD algorithm used for enhanced key encryption in this document)
- **NIST FIPS 197 (AES):** https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197-upd1.pdf

---

## Open-Source Implementations

**SDRTrunk** — Primary open-source P25 decoder/monitor. Implements OTAR message
decoding (parses KMMs from the P25 data channel), though it does not implement the KMF
key management role or decrypt encrypted KMMs without key material.
- Repository: https://github.com/DSheirer/sdrtrunk
- OTAR-relevant code: `src/main/java/io/github/dsheirer/module/decode/p25/phase1/message/`
  and related `otar` subdirectory
- SDRTrunk can display OTAR traffic in plaintext, showing message type, RSI, and field values

**OP25** — GNU Radio-based P25 decoder. Also parses OTAR messages.
- Repository: https://github.com/boatbod/op25
- Relevant files: `op25/gr-op25-dev/lib/p25p1_voice_decode.cc` and related Python layer

**p25dcodr** — Python P25 packet decoder with OTAR message type identification.

**Note on OTAR KMF implementation:** No production-quality open-source KMF server
is publicly known. The key management server role involves handling key material that
would typically be classified or controlled; open implementations are limited to receivers
(decoders) rather than transmitters (key distributors). A complete open-source KMF
implementing this spec would require handling DES/3DES/AES key wrapping (NIST SP 800-38F),
CBC-MAC authentication, message number management, and the full procedure state machines.

**Rust/systems-level references:**
- The `aes` crate (https://crates.io/crates/aes) provides AES block cipher primitives
  needed for key wrapping and CBC-MAC.
- NIST SP 800-38F key wrap is available via the `aes-kw` crate (https://crates.io/crates/aes-kw).
- Note: the enhanced key wrap in this document uses a custom IV pattern (0xA6 repeated)
  and a specific 6-round structure defined in Section 13.3, which matches RFC 3394 /
  NIST SP 800-38F "KW" mode. The `aes-kw` crate implements this directly.

---

## Standards Lineage

```
P25 Key Management Document History
====================================

TIA-102.AACA        (original OTAR spec, pre-2000)
    │
    ├── TIA-102.AACA-A      (Revision A, circa 2005-2009)
    │       │
    │       └── AACA-A-1    (Addendum: single-key / multi-key interoperability)
    │
    └── TIA-102.AACA-B      (Revision B, September 2021)  ← CURRENT
            Incorporates AACA-A-1
            Adds DES deprecation notes (FPIC)
            Clarifies KMM protection layer rules

Related key management documents:
    TIA-102.AACD    SU Programming & Management Parameters
    TIA-102.AACB    KMF Interface (northbound)
    TIA-102.AACE    OTAR Conformance Testing

Algorithm IDs sourced from:
    TIA-102.AAAA    Common Air Interface (defines $80-$FF algorithm space)
```

---

## Notes on Interoperability

A key complication acknowledged in the document (and motivating Annex B) is the
distinction between single-key and multi-key subscriber equipment. Single-key equipment
(pre-AACA-A-1) uses Key ID $0000 as a default and ignores multi-key messages. Multi-key
equipment uses explicit Key IDs. When mixed in the same system, the KMF must account
for Key ID $0000 behavior on legacy equipment.

Revision B explicitly states that compliance with AACA-B does not invalidate compliance
with prior versions — a nod to the installed base of AACA-A equipment in operational systems.
