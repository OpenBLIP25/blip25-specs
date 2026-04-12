# TIA-102.AAAD-B — Related Resources & Context

---

## Status

**Active.** This document is currently active as ANSI/TIA-102.AAAD-B-2015. It supersedes TIA-102.AAAD-A (August 2009), which itself incorporated the original Addendum 1 covering Phase 2 Two-Slot TDMA. The -B revision was a five-year ANSI renewal with editorial changes only — no new algorithms or protocol changes were introduced.

The predecessor DES-only interim standard dates to March 2001; AES-256 and Triple-DES were incorporated at that time. This document is not expected to be superseded unless a new cipher algorithm is added to the P25 suite or the LFSR synchronization mechanism is revised.

**Algorithm deprecation note:** DES (ALGID $81) is cryptographically deprecated — NIST withdrew FIPS 46-3 in 2005. Triple-DES (ALGID $83) was deprecated by NIST in 2019 with a phase-out deadline of 2023. AES-256 (ALGID $84) remains fully recommended. In practice, many deployed P25 systems still use DES or ADP (Motorola RC4) for legacy compatibility, though modern deployments standardize on AES-256.

---

## Standards Family

This document is part of the TIA-102 Project 25 suite of approximately 50+ standards. Within that suite, it sits in the **AA series** (system-level and security specifications) alongside:

**Direct encryption companions:**
- **TIA-102.AAAB** — Digital Land Mobile Radio Security Services Overview (architecture context)
- **TIA-102.AAAC** — DES Encryption Conformance Testing
- **TIA-102.AAAD-B** — Block Encryption Protocol (this document)
- **TIA-102.AAAE** — OTAR (Over-The-Air Rekeying) Protocol

**Normative dependencies of this document:**
- **TIA-102.BAAA-A** — Project 25 FDMA Common Air Interface (superframe structure, MI field placements, LDU1/LDU2 layout)
- **TIA-102.BAAC-C** — Common Air Interface Reserved Values (ALGID and KID reserved values table)
- **TIA-102.BABA-A** — Project 25 Vocoder Description (IMBE word/octet packing — u0-u7 into w0-w10)
- **TIA-102.BBAC** — Project 25 Two-Slot TDMA MAC Definitions (ESS-A/ESS-B fields, half-rate vocoder frame structure)

**Informative reference:**
- **TIA TSB-102-B** — Project 25 TIA-102 Documentation Suite Overview (system architecture, functional reference model)

**NIST references incorporated by normative reference:**
- FIPS 197 (AES)
- FIPS 46-3 (DES)
- FIPS 81 (DES Modes)
- NIST SP 800-38A (Block Cipher Modes of Operation)
- ANSI X3.92-1981, X3.106-1983, X9.52-1998 (DES/TDEA)

---

## Standards Lineage (ASCII Tree)

```
TIA-102 Project 25 Suite
├── AA Series — System / Security
│   ├── TIA-102.AAAB — Security Services Overview
│   ├── TIA-102.AAAC — DES Conformance Testing
│   ├── TIA-102.AAAD-B — Block Encryption Protocol  ← this document
│   └── TIA-102.AAAE — OTAR Protocol
├── BA Series — FDMA Air Interface
│   ├── TIA-102.BAAA-A — FDMA CAI [normative dependency]
│   ├── TIA-102.BAAC-C — CAI Reserved Values [normative dependency]
│   └── TIA-102.BABA-A — Vocoder Description [normative dependency]
├── BB Series — TDMA / Phase 2
│   ├── TIA-102.BBAC — Two-Slot TDMA MAC [normative dependency]
│   └── TIA-102.BBAA — TDMA Physical Layer
├── BC Series — Conformance
│   └── TIA-102.BCAD/BCAE/BCAF — Conformance Test Suites
└── TSB-102-B — Documentation Suite Overview [informative]

Revision lineage of this document:
  Interim Standard (Mar 2001)
      → Original (Jul 2002)
          → Addendum 1 (Two-Slot TDMA)
              → TIA-102.AAAD-A (Aug 2009)
                  → TIA-102.AAAD-B (Dec 2015)  ← current
```

---

## Practical Context

This document defines the cryptographic glue between standard public-domain block ciphers (DES, 3DES, AES) and the P25 radio framing layer. In practice, it answers questions like:

- Which 64 of the 72 CAI MI bits seed the LFSR?
- Which octet of the FDMA LDU1 carries voice frame 3, word w5, and what OFB block/offset does it map to?
- How does an AES-256 implementation expand a 64-bit LFSR state to fill a 128-bit input register?
- For Phase 2 TDMA, how many OFB iterations are needed per 360 ms interval?

**Equipment implementations:** Every P25 radio or console that supports encryption implements this document. This includes equipment from Motorola Solutions (APX, ASTRO series), L3Harris (XL-200P, XG-75), Kenwood (NX-5000 series), Tait (TP9400), EF Johnson, and others. The ALGID and KID fields in this document's ES Auxiliary Header are carried verbatim in BAAA CAI frames, so any CAI-compliant receiver can identify what algorithm and key slot a transmitter is using.

**Key management:** This document deliberately does not specify how keys are loaded or distributed. That is the domain of TIA-102.AAAE (OTAR) and manufacturer-proprietary key management systems (e.g., Motorola KMF, Tait VPM). Keys are physically loaded via a KVL (Key Variable Loader) or over-the-air via OTAR.

**Security posture:** AES-256 with properly randomized IVs and frequent key changes is considered strong. DES is known-broken (56-bit effective key). ADP/RC4 (not in this document, a Motorola proprietary algorithm ALGID $AA) provides minimal security. Published research (Calvert et al., 2011) demonstrated that many deployed systems use no encryption or broken algorithms, though this reflects key management failures rather than flaws in the BEP itself.

---

## Key Online Resources

**Standards purchase/reference:**
- TIA online store (active standard): https://store.accuristech.com/standards/tia-ansi-tia-102-aaad-b?product_id=2594028
- GlobalSpec standards listing: https://standards.globalspec.com/std/9973385/TIA-102.AAAD

**Project 25 Technology Interest Group (PTIG):**
- Approved P25 TIA Standards list (2022): https://project25.org/images/stories/ptig/P25_SC_22-04-003_Approved_P25_TIA_Standards_Approved_4052022.pdf
- P25 Security and Encryption Resources: https://project25.org/index.php/p25-security-and-encryption-resources

**DHS / Government references:**
- DHS P25 Compliance Assessment Program encryption requirements: https://www.dhs.gov/sites/default/files/publications/P25-CAB-ENC_REQ-508_0.pdf
- NAPCO P25 Encryption and Interoperability Guide: https://www.napco.org/documents/p25_encryption_and_interoperability.pdf

**Educational / Training:**
- Tait Radio Academy, Introduction to P25 Encryption: https://www.taitradioacademy.com/topic/introduction-to-p25-encryption-1/
- GNU Radio wiki, P25 overview (National Instruments): https://wiki.gnuradio.org/images/f/f2/A_Look_at_Project_25_(P25)_Digital_Radio.pdf
- UC San Diego thesis on P25 encryption architecture: https://escholarship.org/content/qt52w2h896/qt52w2h896.pdf

**Security research:**
- Calvert et al. (2011), "Catching Criminals and Spies: A Security Evaluation of APCO P25" — documents widespread DES/ADP deployment: https://tech.slashdot.org/story/11/09/10/1539217/security-researchers-crack-apco-p25-encryption

---

## Open-Source Implementations

### SDRTrunk
- **Repository:** https://github.com/DSheirer/sdrtrunk
- **Language:** Java
- **Encryption support:** Identifies algorithm and key ID for encrypted calls. Recognizes ALGID values for DES-OFB ($81), 2-KEY TRIPLE DES, 3-KEY TRIPLE DES ($83), AES-128, AES-256 ($84), AES-CBC, and proprietary algorithms (ACCORDION 3, BATON AUTO EVEN, FIREFLY, MAYFLY, SAVILLE, MOTOROLA PADSTONE).
- **Decryption:** Does not currently perform real-time decryption (key material would need to be supplied externally). The ALGID dispatch logic is useful as a reference for the reserved value table from BAAC.
- **Relevant file:** `src/main/java/io/github/dsheirer/module/decode/p25/reference/Encryption.java`
- **Relevance to AAAD-B:** Implements the ALGID enumeration and KID/MI extraction from CAI frames as specified by this document's ES Auxiliary Header format.

### OP25 (Open Source Mobile Communications)
- **Repository:** https://github.com/boatbod/op25
- **Language:** Python/C++ (GNU Radio blocks)
- **Encryption support:** Active decryption support for DES-OFB and RC4/ADP. AES-256 decryption proposed/in progress.
- **Key configuration:** Supports configuring multiple keyid/algid/key triples for FDMA Phase 1 traffic. Keys supplied in a JSON config file.
- **Relevance to AAAD-B:** The DES-OFB decryption path implements the MI→LFSR→OFB chain described in sections 3 and 4 of this document. The voice frame octet ordering in sections 5.3 (Tables 5-5/5-6) is directly reflected in the voice frame assembly logic.
- **Osmocom wiki:** https://projects.osmocom.org/projects/op25/wiki/WikiStart

### DSD / DSD-FME (Digital Speech Decoder)
- **Repository (DSD-FME fork):** https://github.com/lwvmobile/dsd-fme
- **Language:** C
- **Encryption support:** Partial P25 encryption support including key-supplied DES decryption.
- **Reference:** https://deepwiki.com/lwvmobile/dsd-fme/4.4-p25-encryption-and-key-management
- **Relevance to AAAD-B:** Implements the MI extraction and OFB mode for voice decryption. The FDMA encryption schedule (Tables 5-5 and 5-6) governs how this implementation orders the IMBE voice octets before XOR with keystream.

### p25rx / robotastic p25-decoder
- **Repository:** https://github.com/robotastic/p25-decoder
- **Language:** Python/C
- **Note:** Primarily a protocol decoder without encryption support, but useful for understanding CAI frame parsing that precedes encryption.

### Rust / P25 Rust crates (community)
- No mature standalone Rust P25 encryption crate has been identified at time of this writing. The LFSR, OFB, and voice frame schedule are straightforward to implement from this document. AES-256 and DES are available via the `aes` and `des` crates on crates.io.

---

## Implementation Notes for This Document

**LFSR polynomial taps** (for direct implementation):
```
Feedback stages: 15, 27, 38, 46, 62, 64
Polynomial: C(x) = 1 + x^15 + x^27 + x^38 + x^46 + x^62 + x^64
Register: 64 bits, shifts left 64 times per MI
```

**ALGID dispatch table** (from BAAC, referenced here):
```
$80 = Unencrypted
$81 = DES-OFB          (n=64,  m=8,   B_FDMA=28, B_TDMA=17)
$83 = TDEA (3DES-OFB)  (n=64,  m=8,   B_FDMA=28, B_TDMA=17)
$84 = AES-256-OFB      (n=128, m=16,  B_FDMA=15, B_TDMA=9)
```

**MI expansion needed when n > 64:** Use LFSR-based shift register expansion (section 4.2 / Table 4-1). For AES-256, the 128-bit input register is filled by concatenating the 64 LFSR bits with the next 64 bits produced by shifting the LFSR 64 more times in an n-stage register. The canonical expanded values for the test MI `1234 5678 90AB CDEF` are given in Table 4-1 of this document.

**Cross-validation test vectors:** The canonical test case used across all three annexes:
- MI (header): `1234 5678 90AB CDEF 00`
- DES key: `0123 4567 89AB CDEF`
- DES B#2 keystream: `5D97 6A50 4786 581F`
- TDEA B#2 keystream: `F2EF 4174 6B0B EE27`
- AES-256 B#2 keystream: `A4A4 5220 6523 E33B AD35 8AF5 71CB 029A`

These values can be used to validate a from-scratch implementation before testing with live P25 traffic.
