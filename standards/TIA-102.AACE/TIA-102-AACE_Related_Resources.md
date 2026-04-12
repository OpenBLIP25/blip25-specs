# TIA-102-AACE Related Resources & Context

## Status

**Active.** This document was released for publication on September 9, 2005 and published as a TIA Standard in December 2005. As of the time of writing (2026), no superseding document has replaced it. It remains the normative specification for P25 link-layer authentication using the AES-128 Challenge and Response mechanism. The TIA TR-8 committee continues to maintain the P25 security suite.

Note: The document predates widespread P25 Phase 2 (TDMA) deployment. Its scope explicitly covers FDMA and TDMA systems with an FDMA control channel, which encompasses the majority of deployed trunked P25 infrastructure. Phase 2 TDMA traffic channels reuse the same authentication mechanism.

---

## Standards Family

This document sits within the **TIA-102 AAC** security sub-series of the Project 25 standards suite. The TIA-102 suite is organized into lettered families:

- **AAA** — General System Design (architecture, overview)
- **AAB** — Common Air Interface (FDMA physical/MAC layers)
- **AAC** — Encryption, Key Management, and Authentication ← *this document lives here*
- **BAA/BAB/BAC** — TDMA physical and MAC layers
- **BBA/BBB/BBC** — Trunking and MAC procedures
- **CAA/CAB** — Conformance testing

The authentication family within AAC includes:

| Document       | Title                                              | Role relative to this document |
|----------------|----------------------------------------------------|--------------------------------|
| TIA-102.AABC   | Trunking Control Channel Messages                  | Normative ref [1]: defines wire format of all 6 auth messages |
| TIA-102.AACA   | Over-The-Air Rekeying (OTAR) Protocol              | Informative ref [5]: key delivery path |
| TIA-102.AACD   | Key Fill Device (KFD) Interface Protocol           | Informative ref [4]: physical key provisioning |
| **TIA-102.AACE** | **Link Layer Authentication** ← *this document* | Challenge/Response + Mutual Auth protocol and algorithms |
| TIA-102.AACF   | Authentication Conformance Test Procedures         | Conformance testing for this standard |
| TIA-102.AACG   | Security Architecture                              | Upper-level security design context |
| TIA-102.AACC   | End-to-End Encryption                              | Voice/data payload encryption (separate from link auth) |

The AES standard referenced as [2] is FIPS PUB 197 (NIST, November 2001). The ECB mode specification referenced as [3] is NIST SP 800-38A.

---

## Standards Lineage

```
TIA-102 (Project 25 Standards Suite)
└── TIA-102.AAC (Security)
    ├── TIA-102.AACA  — OTAR Protocol (key delivery over-air)
    ├── TIA-102.AABC  — Control Channel Messages (wire format for auth PDUs)
    ├── TIA-102.AACC  — End-to-End Encryption (voice/data payload)
    ├── TIA-102.AACD  — Key Fill Device Interface (physical key load)
    ├── TIA-102.AACE  — Link Layer Authentication ← THIS DOCUMENT
    │       │
    │       ├── Uses: FIPS 197 (AES-128)
    │       ├── Uses: NIST SP 800-38A (ECB mode)
    │       └── Auth PDU formats → TIA-102.AABC
    └── TIA-102.AACF  — Authentication Conformance Tests
```

---

## Practical Context

### Deployment in P25 Systems

P25 trunked radio systems used by public safety agencies (police, fire, EMS, federal agencies) implement link-layer authentication to prevent unauthorized radios from registering on the system and consuming talkgroup capacity. Without authentication, any radio programmed with the correct system parameters (WACN, System ID, NAC) can register and potentially receive encrypted voice traffic or disrupt operations.

In practice:
- **Authentication is optional per SU** — an RFSS can choose to authenticate specific SUIs or all SUs.
- **Mutual authentication is rarely deployed** in practice as of the mid-2020s, despite being mandatory in the standard when implemented in the FNE. The overhead (additional round-trip messages) and operational complexity have limited adoption.
- **The Authentication Key (K)** is typically loaded via Key Fill Device (KFD) using the KFD protocol (TIA-102.AACD), often through Motorola KVL (Key Variable Loader) or similar tools.
- The **Authentication Facility (AF)** is typically implemented within the zone controller or site controller of the trunked system (e.g., Motorola ASTRO 25 systems, Harris OpenSky/BeOn systems, Kenwood NEXEDGE).
- **RS refresh** (changing the random seed to generate new KS/KS' values) is implemented as a configurable crypto period, typically measured in hours or days.

### Security Posture

The scheme has the following security properties:
- K never traverses the air interface — only RS, RAND1, RAND2, RES1, RES2 are sent over-the-air.
- RES1 and RES2 are 32-bit values — a brute-force guess succeeds with probability 1/2^32 per attempt. Repeated failed authentications are detectable.
- AES-128 in ECB mode for a single block is cryptographically sound; ECB mode concerns (identical plaintext → identical ciphertext) do not apply to single-block operations.
- The bitwise complement in AM3 ensures KS ≠ KS' even when K and RS are identical inputs to AM1 and AM3.

### Known Implementation Status in Open Source

The P25 authentication protocol defined here is implemented or partially implemented in the following open-source projects:

**OP25** (https://github.com/boatbod/op25)
- Python/GNU Radio-based P25 receiver
- Implements P25 control channel decoding including authentication message parsing
- Does not implement the cryptographic authentication response (would require K)
- Decodes and logs Authentication Demand, Authentication Response, and related PDUs
- Relevant source: `op25/gr-op25-r2/lib/p25_frame.cc`, `trunk.py`

**SDRTrunk** (https://github.com/DSheirer/sdrtrunk)
- Java-based P25 decoder and scanner
- Decodes authentication control channel messages
- Implements P25 trunking control channel message parsing including auth PDUs
- Does not implement the AES authentication response computation
- Relevant package: `io.github.dsheirer.module.decode.p25.message.tsbk`

**p25rx / DSD (Digital Speech Decoder)**
- Older decoders that may parse auth messages at the TSBK layer
- No cryptographic implementation

**Implementing AM1–AM4 in open source** is straightforward — they are AES-128 ECB operations with simple bit-width transforms. Any AES library (Rust `aes` crate, Python `cryptography`, OpenSSL) can implement them. The test vectors in Section 6.6 of this document provide full validation data.

**Rust implementation note:** The `aes` crate (https://crates.io/crates/aes) provides AES-128 ECB block encryption. AM1 is: zero-pad RS to 16 bytes, encrypt with K. AM2 is: zero-pad RAND1 to 16 bytes, encrypt with KS, take first 4 bytes. AM3 is: zero-pad RS to 16 bytes, bitwise NOT all 16 bytes, encrypt with K. AM4 mirrors AM2 with KS' and RAND2.

---

## Key Online Resources

- **TIA Standards store** — The authoritative source for purchasing current TIA-102 documents:
  https://store.accuristech.com/tia

- **APCO Project 25 Technology Interest Group (PTIG)** — Industry organization that coordinates P25 testing and interoperability, including authentication conformance:
  https://www.project25.org

- **NIST FIPS 197 (AES)** — Referenced normatively as the base algorithm:
  https://csrc.nist.gov/publications/detail/fips/197/final

- **NIST SP 800-38A (Block Cipher Modes)** — Normative reference for ECB mode:
  https://csrc.nist.gov/publications/detail/sp/800-38a/final

- **DHS SAFECOM P25 resources** — Federal guidance on P25 procurement and security:
  https://www.cisa.gov/safecom/p25

- **RadioReference P25 wiki** — Community documentation on P25 system parameters, control channel messages, and authentication field observations:
  https://wiki.radioreference.com/index.php/APCO_Project_25

- **SDRTrunk GitHub** — Open-source Java P25 decoder that parses authentication PDUs:
  https://github.com/DSheirer/sdrtrunk

- **OP25 GitHub** — Open-source GNU Radio P25 receiver with control channel decoding:
  https://github.com/boatbod/op25

---

## Open-Source Implementations of AM1–AM4

No known public repository implements the complete AM1–AM4 authentication response computation (which would require possession of K, a controlled key material). This is expected: K is a security-sensitive value distributed only through controlled provisioning channels.

However, any developer can implement AM1–AM4 using standard AES libraries given the test vectors in Section 6.6. The algorithms are:

```
AM1(K, RS)     → KS  : AES_ECB_Encrypt(key=K,  plaintext=zero_pad_to_16(RS))
AM2(KS, RAND1) → RES1: AES_ECB_Encrypt(key=KS, plaintext=zero_pad_to_16(RAND1))[0:4]
AM3(K, RS)     → KS' : AES_ECB_Encrypt(key=K,  plaintext=bitwise_NOT(zero_pad_to_16(RS)))
AM4(KS', RAND2)→ RES2: AES_ECB_Encrypt(key=KS',plaintext=zero_pad_to_16(RAND2))[0:4]
```

Where `zero_pad_to_16(RS)` means right-pad an 80-bit (10-byte) RS value with 6 zero bytes, and `zero_pad_to_16(RAND)` means right-pad a 40-bit (5-byte) RAND value with 11 zero bytes.

Test vector verification (from Section 6.6):
- AM1: K=`000102030405060708090a0b0c0d0e0f`, RS=`38aec82933b17f80249d` → KS=`052430bdaf39e82fd0ddd698c02fb036`
- AM2: KS=`052430bdaf39e82fd0ddd698c02fb036`, RAND1=`4d925af608` → RES1=`3e00faa8`
- AM3: same K and RS → KS'=`69d5dc08023c4652cc71d5cd1e74e104`
- AM4: KS'=`69d5dc08023c4652cc71d5cd1e74e104`, RAND2=`6e784f75bd` → RES2=`b3ad16e1`

---

## Phase 3 Implementation Specs — Flagged for Follow-Up

This document warrants two Phase 3 implementation specs that were **not produced in this run** and are flagged for a follow-up pass:

1. **`TIA-102-AACE_Authentication_Algorithm_Implementation_Spec.md`**
   - AM1–AM4 complete pseudocode with bit-width expansion/reduction operations
   - AES-128 ECB interface definition
   - All four test vectors with worked examples
   - Rust implementation notes using the `aes` crate
   - Cross-references to OP25 / SDRTrunk parsing code

2. **`TIA-102-AACE_Authentication_Protocol_State_Machine_Spec.md`**
   - RFSS and SU state machines as formal state tables
   - Complete PDU sequencing for all 11 MSC scenarios
   - Authentication Timer behavior and timeout handling
   - REG_REFUSED vs. Deny Response dispatch logic
   - SU Change Site behavior for adversary FNE detection
   - Integration points with unit registration and location registration flows
