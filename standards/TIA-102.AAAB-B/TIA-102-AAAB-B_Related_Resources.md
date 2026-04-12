# TIA-102-AAAB-B Related Resources & Context
# Document: TIA-102.AAAB-B — Project 25 Security Services Overview
# ANSI/TIA-102.AAAB-B-2019

---

## Status

**Active.** This document (ANSI/TIA-102.AAAB-B-2019, approved February 5, 2019) is the current revision as of April 2026. It supersedes TIA-102.AAAB-A (January 2005), which in turn superseded the original TIA/EIA-102.AAAB (July 2002). No further revision has been issued post-2019.

Two sections within the document were explicitly marked incomplete at time of publication:
- **Section 7.1.4 — IP Data Encryption**: Pending completion of P25 IP Data Encryption architecture discussions within the APIC Encryption Task Group. No successor document resolving this has been identified as of 2026.
- **Section 7.2.2 — Air Interface Encryption (Link Layer Encryption)**: Similarly on hold. The P25 Standards Update (March 2025) confirms that Link Layer Encryption (LLE) development is still in progress within the ETG; TDMA air interface material is substantially complete but FDMA modifications and Key Fill Interface changes remain under review.

The document is referenced in the P25 Technology Interest Group (PTIG) approved standards list and in CISA public safety communications guidance.

---

## Standards Family

This document is the **security overview anchor** for the TIA-102 suite. The P25 standard comprises approximately 80+ documents organized in a multi-tier hierarchy:

```
TSB-102-C (System and Standards Definition — Shell)
│
├── TIA-102.AAAB-B  ← THIS DOCUMENT
│   (Security Services Overview — top-level reference for all security)
│
├── End-to-End Encryption
│   ├── TIA-102.AAAD-A  (Block Encryption Protocol)
│   ├── TIA-102.AACA-A  (OTAR Messages and Procedures)
│   └── TIA-102.AACD-A  (Key Fill Device Interface Protocol)
│       └── TIA-102.AACD-A-1  (KFD Interface Addendum — ETG review)
│
├── Link Layer Authentication
│   └── TIA-102.AACE-A  (Digital LMR Link Layer Authentication)
│
├── Link Layer Encryption
│   └── [In development — no published document number assigned as of 2026]
│
├── Key Management Infrastructure
│   └── TIA-102.BAKA  (KMF-to-KMF Interface)
│
├── Inter-RF Subsystem (ISSI) — relevant to roaming auth session transfer
│   └── TIA-102.BACA-B  (ISSI Messages and Procedures)
│
└── TIA-905 Series
    (Phase 2 TDMA security — designed to interoperate with TIA-102 security)
```

The TSB-102-C shell document is the entry point for navigating the full suite. ANSI and TIA maintain the authoritative registry of active standards.

---

## Practical Context

This document defines the "why" and "what" of P25 security without specifying the "how." It is the reference that procurement documents, system specifications, and conformance test plans cite when invoking P25 security requirements at an architectural level.

**Encryption types in deployment:**
- Type I (classified) and Type III (sensitive but unclassified) are the most relevant to public safety. AES-256 (FIPS 197) is the current preferred algorithm across the suite; DES and 3DES are deprecated but supported for legacy interoperability.
- Type IV covers commercial/exportable encryption used in non-government systems.

**Key management in practice:**
- Physical Key Fill (via KFD) remains prevalent for initial radio provisioning and in smaller agencies that cannot justify a full KMF deployment.
- OTAR via KMF is the operational standard for large deployments (state/federal systems, large municipal systems). The KMF vendor ecosystem includes Motorola (KMF product line), Tait (Unified Key Management), and others.
- The Unique Key Encryption Key (UKEK) provisioned into a radio via Key Fill is the bootstrap credential that enables subsequent OTAR sessions.

**Link Layer Authentication (LLA) deployment:**
- LLA (defined in TIA-102.AACE-A) has been adopted in some trunked P25 Phase 2 deployments to counter subscriber duplication threats. It operates only on the Trunking Control Channel. The AF-to-FNE interface for authentication material distribution remains non-standardized, requiring proprietary vendor implementation.
- Mutual authentication (SU challenges FNE identity) is optional and less commonly deployed than unit authentication alone.

**Air Interface Encryption / Link Layer Encryption (LLE) deployment:**
- LLE is not yet deployed in production P25 systems as of 2026, as the standard remains incomplete. The need for LLE is driven by traffic flow analysis threats — specifically the exposure of talkgroup IDs and radio identities in clear air interface signaling even when voice traffic is encrypted.
- CISA has published guidance (August 2024) addressing LLA/LLE readiness: "LLA, LLE — Are You Really Secure?" highlighting that most agencies have neither service deployed.

---

## Key Online Resources

- **P25 Technology Interest Group (PTIG) — Approved Standards List (June 2019)**:
  https://www.project25.org/images/stories/ptig/P25_SC_19-06-002-R1_Approved_P25_TIA_Standards_-_June_2019.pdf

- **P25 Standards Update March 2025** (includes ETG status for LLE and security documents):
  https://www.project25.org/images/stories/ptig/P25%20Standards%20Update%20March%202025%20.pdf

- **CISA — LLA and LLE Public Safety Guidance (August 2024)**:
  https://www.cisa.gov/sites/default/files/2024-09/24_0828_s-n_LLA_LLE_are_you_really_secure_2022_final_508C.pdf

- **Project25.org — P25 Security and Encryption Resources** (maintained by PTIG):
  https://project25.org/index.php/p25-security-and-encryption-resources

- **GlobalSpec — TIA-102.AAAB Standards Reference**:
  https://standards.globalspec.com/std/13348990/TIA-102.AAAB

- **APCO/OJP — P25 Security Services Overview Reference** (iepd #438):
  https://it.ojp.gov/NISS/iepd/438

- **Internet Archive — TIA-102 Series Documents** (historical document set):
  https://archive.org/details/TIA-102_Series_Documents

- **TIA Store** (official purchase channel for current standards):
  https://store.accuristech.com/tia

---

## Open-Source Implementations

The security services defined in this document (encryption, OTAR, LLA) are among the most proprietary aspects of the P25 ecosystem. Open-source P25 tools generally implement the *reception* and *decoding* of unencrypted P25 traffic but do not implement the cryptographic services defined in this document. This is because:
1. Traffic Encryption Keys are secret and not obtainable from the air interface.
2. OTAR key management messages are themselves encrypted.
3. Algorithm implementations for Type I/III are controlled under export restrictions.

**SDRTrunk** (https://github.com/DSheirer/sdrtrunk):
- Implements P25 Phase 1 and Phase 2 decoding, trunking, and call following.
- Recognizes encrypted voice frames and OTAR key management PDU structure, but cannot decrypt without keys.
- Identifies MI fields and ALGID values in captured frames — useful for understanding the Esync/MI synchronization described in this document.
- No OTAR key management client or LLA implementation.

**OP25 / boatbod/op25** (https://github.com/boatbod/op25):
- GNU Radio-based P25 decoder. Similar capabilities to SDRTrunk for decoding, no encryption implementation.
- Phase 2 TDMA support includes recognition of air interface encryption markers.

**p25craft** (https://github.com/gr-p25):
- GNU Radio P25 blocks; no known encryption or key management implementation.

**No public open-source implementation of OTAR (KMM client), LLA challenge-response, or P25 KMF has been identified** as of April 2026. This is a significant gap for open-source P25 stack development. The OTAR message structure is defined in TIA-102.AACA-A; an open-source KMF client implementing that spec (referencing this document for architectural context) would be a meaningful contribution to the ecosystem.

---

## Standards Lineage

```
TIA/EIA-102.AAAB (July 2002)
│   Original publication. Minor editorial corrections from letter ballot.
│
└── TIA-102.AAAB-A (October 2004 / January 2005)
    │   Minor editorial corrections from letter ballot.
    │
    └── TIA-102.AAAB-B (October 2018 approved / February 5, 2019 published)
        │   Minor editorial corrections from letter ballot.
        │   Added Section 7 (Key Management Architecture) — the major
        │   substantive addition over AAAB-A, providing functional models
        │   and architecture diagrams for all security sub-domains.
        │   IP Data Encryption (7.1.4) and Air Interface Encryption (7.2.2)
        │   left incomplete pending ETG architectural discussions.
        │
        └── [No further revision as of April 2026 — AAAB-B is current]

Related active companion documents:
    TIA-102.AACE-A (2011) — Link Layer Authentication
    TIA-102.AACA-A (2014) — OTAR Messages and Procedures
    TIA-102.AACD-A (2014) — Key Fill Device Interface
    TIA-102.AAAD-A (2009) — Block Encryption Protocol
    TIA-102.BAKA (2012)   — KMF-to-KMF Interface
    TIA-102.BACA-B (2012) — ISSI Messages and Procedures

    [LLE specification] — In development, no designation assigned
    [IP Data Encryption] — Architecture discussions ongoing
```

---

*Context file generated April 2026. Document status verified against P25 Standards Update March 2025 and PTIG approved standards list.*
