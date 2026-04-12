# TIA-102.AAAD-A Related Resources & Context

**Document:** TIA-102.AAAD-A — Project 25 Digital Land Mobile Radio Block Encryption Protocol  
**ANSI Approval:** August 20, 2009  
**Supersedes:** TIA-102.AAAD (June 2002), which superseded TIA-102.AAAA (February 2001)

---

## Status

**Active.** This document is the current normative encryption protocol standard for P25 systems as of 2026. It was approved as ANSI/TIA-102.AAAD-A in August 2009 and has not been superseded. The 2009 revision added Two-Slot TDMA (Phase 2) encryption schedules and elevated AES to mandatory status (Section 7). No subsequent lettered revision (AAAD-B) is known to have been published as of the knowledge cutoff.

The predecessor TIA-102.AAAA defined only DES. TIA-102.AAAD added TDEA and AES annexes. This revision (AAAD-A) adds TDMA support and mandates AES.

---

## Standards Family

This document sits in the TIA-102 (Project 25) encryption sub-suite within the broader P25 standards suite:

```
TIA TSB-102-A  (P25 System and Standards Definition — overview)
    │
    ├── TIA-102.AAAA  — P25 DES Encryption Protocol (Feb 2001, superseded)
    │       └── TIA-102.AAAD  — Block Encryption Protocol (Jun 2002, superseded)
    │               └── TIA-102.AAAD-A  — Block Encryption Protocol, Rev A (Aug 2009) ← THIS DOCUMENT
    │
    ├── TIA-102.BAAA-A  — P25 FDMA Common Air Interface (normative, Ref 3)
    ├── TIA-102.BABA    — P25 Vocoder Description (normative, Ref 2)
    ├── TIA-102.BAAC-B  — P25 CAI Reserved Values (normative, Ref 4)
    └── TIA-102.BBAC    — P25 Two-Slot TDMA MAC Definitions (normative, Ref 6)
```

**Companion documents:**
- **TIA-102.BAAC-B** defines the ALGID reserved values ($80=unencrypted, $81=DES, $83=TDEA, $84=AES) and the all-zero MI reserved value that signals unencrypted traffic.
- **TIA-102.BBAC** specifies where the MI is embedded in the TDMA MAC frame (the Encryption Sync Signaling / ESS word).
- **TIA-102.BAAA-A** specifies the FDMA superframe structure, LDU1/LDU2 format, MI field placement in the Header and LDU2, and the 72-bit MI field layout.
- **TIA-102.BABA** defines the IMBE vocoder frame structure (words u0–u7) from which the voice plaintext octets (w0–w10) are derived.

**External normative references:**
- NIST FIPS 46-3 / ANSI X3.92-1981 — DES
- NIST FIPS 81 / ANSI X3.106-1983 — DES Modes of Operation (OFB)
- ANSI X9.52-1998 — Triple DES Modes of Operation
- NIST FIPS 197 — Advanced Encryption Standard (AES)
- NIST SP 800-38A — Recommendation for Block Cipher Modes of Operation (OFB, Section 6.4)

---

## Practical Context

### How This Standard Is Used in Real Systems

P25 radios (portable and mobile), base stations, and consoles that implement voice privacy use this protocol. The encryption and decryption functions are implemented in the radio equipment near the IMBE vocoder — plaintext is the vocoder output (or input on receive), and ciphertext is what goes over the air.

**Key management** is out of scope for this document. In practice, keys are loaded into radios via the P25 Key Management Facility (KMF) over the Key Fill Device (KFD) interface, or over-the-air via OTAR (Over-The-Air Rekeying, specified in TIA-102.AACE). Common key fill devices include the Motorola KVL series, Harris SecuNet, and others.

**Algorithm selection in deployed systems:**
- Modern P25 equipment mandates AES-256 (ALGID $84) per Section 7 of this document
- TDEA (ALGID $83) is common in older legacy equipment
- DES (ALGID $81) is considered cryptographically weak and deprecated in practice; NIST withdrew FIPS 46-3 in 2005
- The all-zero ALGID ($80) and all-zero MI indicate unencrypted (clear) operation

**Interoperability:** Because the ALGID and KID are transmitted with each message, a receiver can determine which algorithm and key to use. Cross-vendor encrypted interoperability requires shared key management and the same ALGID support.

**Known deployment issue — IV reuse:** This standard does not mandate IV freshness mechanisms. Several documented attacks on P25 systems (notably the 2011 research by Futoransky, Waisman, et al., and broader analysis by Nohl & Evans) showed that some commercial implementations reused IVs, enabling keystream recovery. This is a protocol implementation concern, not a flaw in the OFB construction itself, but it underscores the importance of Section 3.1's guidance on IV generation.

---

## Key Online Resources

- **TIA Standards Store:** https://store.accuristech.com/tia — Primary source for purchasing the official standard
- **APCO Project 25 Technology Interest Group (PTIG):** https://www.project25.org — APCO's P25 standards coordination body; publishes P25 CAP (Compliance Assessment Program) requirements that reference this document
- **NIST FIPS 197 (AES):** https://csrc.nist.gov/publications/detail/fips/197/final
- **NIST SP 800-38A (Block Cipher Modes):** https://csrc.nist.gov/publications/detail/sp/800-38a/final
- **NIST FIPS 81 (DES Modes):** https://csrc.nist.gov/publications/detail/fips/81/final (historical)
- **P25 CAP ICTAP Documentation:** https://www.dhs.gov/p25-compliance-assessment-program — DHS program that tests P25 equipment interoperability including encryption

---

## Open-Source Implementations

The following open-source projects implement the encryption protocol described in this document:

### OP25 (GNURadio-based P25 receiver)
- **Repository:** https://github.com/boatbod/op25
- **Relevance:** Implements P25 FDMA receive including encrypted traffic detection (identifies ALGID and KID from headers). Decryption requires key material; the OFB keystream generation and LFSR MI tracking logic maps directly to this document.
- **Key files:** `op25/gr-op25-legacy/src/lib/p25p1_fdma.cc`, voice frame assembly

### SDRTrunk
- **Repository:** https://github.com/DSheirer/sdrtrunk
- **Relevance:** Full P25 Phase 1 and Phase 2 decoder. Implements encrypted traffic detection with ALGID/KID display. Decryption support with user-supplied keys. The MI tracking and LFSR counter logic is implemented.
- **Language:** Java
- **Key classes:** `src/main/java/io/github/dsheirer/module/decode/p25/` — P25 decoding modules including encryption state tracking

### DSD (Digital Speech Decoder) / DSD+
- **Repository (DSD):** https://github.com/szechyjs/dsd
- **Relevance:** Decodes P25 voice frames; encryption detection based on MI and ALGID fields. Does not implement decryption but correctly handles encrypted superframes.

### gr-p25 (older GNURadio P25 blocks)
- **Repository:** https://github.com/argilo/sdr-examples (and forks)
- **Relevance:** Early P25 SDR work; encryption-aware at the frame level.

### p25rx (Rust-ecosystem interest)
- No mature Rust implementation of P25 voice decryption is known as of 2026, making this a gap for the open-source P25 Rust project.

---

## Standards Lineage (ASCII Tree)

```
NIST FIPS 46-3 (DES, 1999) ──────────────────────────────┐
NIST FIPS 81 (DES OFB, 1980) ────────────────────────────┤
ANSI X9.52 (TDEA, 1998) ─────────────────────────────────┤
NIST FIPS 197 (AES, 2001) ───────────────────────────────┤
NIST SP 800-38A (Block Modes, 2001) ─────────────────────┤
                                                          │
TIA TSB-102-A (P25 System Definition, 1995)               │
    │                                                     │
    └── TIA-102.AAAA (P25 DES Protocol, Feb 2001) ────────┤
            │  (absorbed by)                              │
            └── TIA-102.AAAD (Block Enc. Protocol,        │
                              Jun 2002)                   │
                    │  (revised to)                       │
                    └── TIA-102.AAAD-A ◄──────────────────┘
                        (Block Enc. Protocol Rev A,
                         Aug 2009, THIS DOCUMENT)
                         - Adds Two-Slot TDMA support
                         - Mandates AES-256
                         - Adds TDMA encryption schedules
```

---

## Phase 3 Implementation Spec — Flagged for Follow-Up

This is an **ALGORITHM** document. The following implementation specs should be produced in a follow-up pass:

1. **`TIA-102-AAAD-A_LFSR_MI_Implementation_Spec.md`** — LFSR polynomial, 64-bit register, clocking schedule, MI expansion for n>64, IV validity check, LFSR test vectors from Table 3-1 and Table 4-1.

2. **`TIA-102-AAAD-A_OFB_Keystream_Implementation_Spec.md`** — Complete OFB keystream generation pipeline, FDMA and TDMA superframe encryption schedules (Tables 5-5, 5-6, 5-8, 5-9), DES/TDEA/AES binding with bit correspondence notes, and all test vectors from Annexes A, B, C.

3. **`TIA-102-AAAD-A_Data_Encryption_Implementation_Spec.md`** — ES Auxiliary Header structure, packet encryption/decryption procedure, CRC boundary rules, pad octet handling, secondary SAP placement.

These are deferred because they require careful cross-referencing of the large encryption schedule tables (Tables 5-5/5-6/5-8/5-9) and all annex test vectors. Programmatic extraction from the PDF is recommended for those tables to avoid transcription errors.
