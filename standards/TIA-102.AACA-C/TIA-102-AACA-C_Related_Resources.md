# TIA-102.AACA-C Related Resources
## Digital Radio Over-The-Air-Rekeying (OTAR) Messages and Procedures

**Document ID:** TIA-102.AACA-C  
**Published:** August 2, 2023  
**Status:** Active (supersedes TIA-102.AACA-B)  
**Classification:** MESSAGE_FORMAT + PROTOCOL + ALGORITHM

---

## Processing Status

| File | Status |
|------|--------|
| `TIA-102-AACA-C_Full_Text.md` | Complete |
| `TIA-102-AACA-C_Summary.txt` | Complete |
| `TIA-102-AACA-C_Related_Resources.md` | This file |
| Phase 3 Implementation Specs | Not yet produced (see flags in Summary.txt) |

---

## Standards Family: TIA-102 Cryptographic Sub-Family

### Directly Related (must read together)

| Document | Title | Relationship |
|----------|-------|--------------|
| TIA-102.AACA-B | OTAR Messages and Procedures (Rev B) | Predecessor; superseded by this document |
| TIA-102.AAAA | P25 Encryption Standards | Defines DES/TDES/AES-256 algorithm parameters used by OTAR |
| TIA-102.AAAB | P25 Message Encryption | Defines CAI-layer encrypted message format; OTAR TEKs activate this |
| TIA-102.AAAC | P25 Key Management Facility Interface | KMF-side interface; OTAR is the air-side complement |
| TIA-102.AACE | P25 OTAR Test Procedures | Conformance tests; directly dependent on AACA-C message formats |

### Broader P25 Air Interface Context

| Document | Title | Relationship |
|----------|-------|--------------|
| TIA-102.AAAB-A | Conventional P25 Message Encryption | CAI encryption layer (DLD transport context) |
| TIA-102.AAAB-B | TDMA P25 Message Encryption | TDMA-layer encryption (Phase 2 TDMA systems) |
| TIA-102.BAAA | Common Air Interface (CAI) Overview | Foundation for DLD transport mode |
| TIA-102.BACA | ISSI (Inter-RF Subsystem Interface) | Multi-site coordination; KMF topology |
| TIA-102.BCCA | CSSI (Console Subsystem Interface) | Dispatch console interface; key management coordination |
| TIA-102.BBAB | P25 SNDCP | Subnetwork Dependent Convergence Protocol — carries KMMs in DLI mode |

---

## NIST Cryptographic References (Normative)

| Standard | Title | Used For |
|----------|-------|----------|
| NIST SP800-38B | Recommendation for Block Cipher Modes of Operation: CMAC | Enhanced CMAC authentication (Version 1 MAC frame, new in Rev C) |
| NIST SP800-38F | Recommendation for Block Cipher Modes of Operation: Methods for Key Wrapping | Enhanced Key Wrap (KW-AE / KW-AD) for TEK protection |
| NIST SP800-108r1 | Recommendation for Key Derivation Using Pseudorandom Functions | KDF in Counter mode with HMAC-SHA-256; derives MAC key for CMAC |
| FIPS 197 | Advanced Encryption Standard (AES) | AES-256 block cipher primitive |
| FIPS 46-3 | Data Encryption Standard (DES) | DES/TDES legacy cipher (deprecated in Rev C context) |
| FIPS 81 | DES Modes of Operation | CBC-MAC mode (legacy, deprecated) |

---

## Standards Lineage

```
TIA-102.AACA (original)
    └─► TIA-102.AACA-A (Revision A)
            └─► TIA-102.AACA-B (Revision B)
                    └─► TIA-102.AACA-C (Revision C, 2023) ◄── CURRENT
                            │
                            ├── Added: Enhanced CMAC authentication (Opt Svc $1F)
                            ├── Added: SP800-38F Enhanced Key Wrap
                            ├── Deprecated: D-bit in Enhanced MAC Frame Version 0
                            └── Deprecated: DES ECB wrap, DES CBC-MAC (legacy only)
```

---

## Key Revision C Changes vs Revision B

1. **Enhanced CMAC Authentication** (Optional Service ID `$1F`): New Version 1 Enhanced
   MAC Frame using CMAC per SP800-38B with a derived MAC key (SP800-108r1 KDF,
   HMAC-SHA-256, Label = "OTAR MAC"). This is now the recommended MAC method.

2. **D-bit deprecated**: The Derived Key indicator bit in the Enhanced MAC Frame Version 0
   is deprecated; implementations should not set or rely on it.

3. **SP800-38F Key Wrap clarified**: Enhanced Key Wrap using NIST KW-AE/KW-AD is
   further specified with test vectors.

---

## Open Source / Reference Implementations

| Resource | Notes |
|----------|-------|
| [OP25 (osmocom-digital)](https://github.com/osmocom/op25) | Open-source P25 receiver; partial OTAR support in Python |
| [gr-p25](https://github.com/bitmunch/op25) | GNU Radio P25 implementation; OTAR receive path |
| [p25rx](https://github.com/szechyjs/dsd) | DSD (Digital Speech Decoder); limited OTAR parsing |
| [Wireshark P25 dissector](https://gitlab.com/wireshark/wireshark) | P25 CAI dissector includes partial KMM display |

> **Note:** No known fully compliant open-source KMF/OTAR implementation exists as of
> the processing date of this document. Most open-source tools implement receive-side
> parsing only, not key management or key wrapping.

---

## Practical Context

### Who Uses This

- **Public safety agencies** deploying P25 encrypted radio systems (law enforcement, fire,
  EMS, federal agencies)
- **Radio vendors** implementing P25 subscriber units with OTAR support (Motorola APX,
  Harris XL/Unity, Kenwood NX-5000 series, etc.)
- **KMF vendors** implementing key management servers (Motorola KMFe, Harris KMF, etc.)
- **Security researchers** analyzing P25 encryption key management

### What You Need to Implement OTAR

To implement a compliant OTAR KMF or SU, you need this document plus:
- **TIA-102.AAAA** for algorithm parameter definitions (IV sizes, key sizes per algorithm)
- **TIA-102.AAAB** (appropriate variant) for the encrypted traffic format that OTAR keys activate
- **TIA-102.AAAC** for KMF interface (if building a networked KMF)
- **NIST SP800-38F, SP800-38B, SP800-108r1** for the cryptographic primitives

### Implementation Complexity Notes

- **KMM parsing** is straightforward (fixed header + tagged body fields)
- **Key wrap** requires careful implementation of KW-AE/KW-AD; the provided test vectors
  are essential for validation
- **Enhanced CMAC** requires implementing the SP800-108r1 KDF correctly; the Label string
  "OTAR MAC" and the counter/context construction are precisely specified
- **Anti-replay (MN)** must handle modulo 2^16 wrap-around correctly
- **SLN mapping** to radio key storage is vendor-specific (AACA-C specifies the address
  space, not the internal key storage format)

---

## Phase 3 Implementation Spec Flags

The following implementation specs should be produced in a follow-up processing run:

1. **`P25_OTAR_KMM_Protocol_Implementation_Spec.md`**
   - KMM header encoder/decoder
   - All 32 message body formats with field validation
   - Message ID dispatch table ($00–$27)

2. **`P25_OTAR_Key_Wrap_Implementation_Spec.md`**
   - Enhanced Key Wrap (SP800-38F KW-AE/KW-AD) — primary implementation target
   - TDES-based wrap
   - DES ECB wrap (legacy, receive-only compatibility)
   - Full test vectors from Annex

3. **`P25_OTAR_MAC_Implementation_Spec.md`**
   - Enhanced CMAC (Version 1, SP800-38B + SP800-108r1 KDF) — primary target
   - Enhanced CBC-MAC (Version 0) — compatibility
   - DES CBC-MAC (legacy, receive-only)
   - Full test vectors from Annex

4. **`P25_OTAR_Procedures_Implementation_Spec.md`**
   - State machine for each of the 18 procedures
   - KMF-side and SU-side state diagrams
   - MN anti-replay logic with modulo wrap handling
   - SLN key storage addressing

5. **`P25_OTAR_Transport_Implementation_Spec.md`**
   - DLD framing (CAI-layer transport)
   - DLI framing (UDP/SNDCP, KMM Preamble `$1E` handling)
   - Max KMM length constraints per transport (512 DLD, 465 DLI)
