# TIA-102.BCAD — Related Resources & Context

## Status

**Active** as of the last known public information. Published September 2011
as a final TIA Standard. No superseding document has been identified; this
remains the conformance specification for Phase 2 Two-Slot TDMA voice
services CAI. The document revision history shows development from February
2011 through September 2011 with final publication approval in July 2011.
The foreword explicitly notes that "significant work remains to fully develop
the standard series and this document will be updated as necessary," which
suggests a living document posture — check TIA's current catalog for any
revisions or amendments published after 2011.

---

## Standards Family

This document sits within the **TIA-102 (Project 25) Phase 2 TDMA** sub-family.
The broader TIA-102 suite covers FDMA (Phase 1) and TDMA (Phase 2) P25
standards across physical, MAC, trunking, and vocoder layers.

**Primary normative references:**
- **TIA-102.BBAC** — Phase 2 Two-Slot TDMA Media Access Control Description.
  This is the MAC specification that this conformance document tests. All
  burst construction rules, Reed Solomon parameters, scrambling sequences,
  ISCH encoding, DUID encoding, and PDU formats come from BBAC.
- **TIA-102.BAAC** — Common Air Interface Reserved Values. Provides ALGID
  and KID default values used in ESS test parameters.

**Informative references used in this document:**
- **TIA-102.AAAD** — Block Encryption Protocol. Source of AES encryption
  parameter definitions (ALGID=0x84 values).
- **"Half-Rate Vocoder Test Signals"** — Digital Voice Systems, Inc.,
  September 19, 2003. Source of the 1031.25 Hz tone and silence vocoder
  test patterns.
- **"Project 25 Guidelines to Assign Wide Area Communications Network and
  System Identities"** — TIA, September 29, 2000. Source of WACN ID and
  System ID assignment conventions used in test parameters.

**Related conformance and measurement documents in the P25 Phase 2 suite:**
- **TIA-102.BCAE** — Likely covers additional Phase 2 conformance areas
  (air interface data or trunking conformance)
- **TIA-102.BCAF** — Additional Phase 2 conformance specification
- **TIA-102.CCAA** — Phase 2 TDMA RF measurement specification (physical
  layer pass/fail thresholds; complements this MAC-layer conformance doc)

**Standards Lineage (ASCII tree):**

```
TIA-102 Project 25 Suite
├── Phase 1 (FDMA)
│   ├── TIA-102.BAAB — Physical Layer
│   ├── TIA-102.BAAC — Reserved Values
│   ├── TIA-102.BACA — ISSI
│   └── ... (many others)
└── Phase 2 (TDMA Two-Slot)
    ├── TIA-102.BBAB — Physical Layer Description
    ├── TIA-102.BBAC — MAC Description (normative basis for BCAD)
    ├── TIA-102.BBAD — Vocoder (Half-Rate IMBE)
    ├── TIA-102.BABA — Vocoder (Full-Rate IMBE)
    ├── TIA-102.BAAC — Reserved Values (shared with Phase 1)
    ├── TIA-102.AAAD — Block Encryption Protocol (shared)
    ├── Conformance
    │   ├── TIA-102.BCAD — CAI Conformance [THIS DOCUMENT]
    │   ├── TIA-102.BCAE — (additional conformance)
    │   └── TIA-102.BCAF — (additional conformance)
    └── Measurements
        └── TIA-102.CCAA — RF/Physical Measurements
```

---

## Practical Context

### How This Document Is Used

This specification fills the role of **developer self-test reference** for
Phase 2 TDMA chipset and software developers. Real-world usage patterns:

1. **Chipset and DSP firmware development:** Engineers implementing the TDMA
   PHY/MAC pipeline (Reed Solomon FEC, ISCH encoding, scrambling, burst
   framing) use the Annex A test vectors as golden reference outputs. If the
   implemented encoder produces C_00 through C_14 from Annex A.1 given the
   ESS inputs from Table 3, the RS encoder is considered correct.

2. **SDR (Software Defined Radio) implementation validation:** Projects
   implementing P25 Phase 2 in software (like op25 or SDRTrunk) can run
   their burst assembly code against these vectors to verify correctness of
   the RS encoder, ISCH encoder, and scrambler.

3. **Interoperability testing:** Before multi-vendor interop testing events,
   equipment manufacturers use this document to pre-verify their CAI
   implementations will produce/accept conformant bursts.

4. **MATLAB reference implementation:** Annex B provides a working MATLAB
   burst generator (available from the TIA FTP server) that generates
   exactly the Annex A vectors. This serves as an executable reference
   specification for developers who need to understand not just *what* the
   expected output is but *how* it is derived.

### What Equipment Uses This Standard

Phase 2 TDMA (also called "TDMA Phase 2" or "P25 Phase 2") is deployed
primarily by large US public safety agencies seeking higher spectral
efficiency. Equipment certified to Phase 2 includes subscriber radios from
Motorola Solutions (APX series, e.g., APX 6000, APX 8000), L3Harris
(XL-200P, P25 TDMA portables), and Kenwood/JVCKENWOOD. Infrastructure
vendors include Motorola Solutions (ASTRO 25), L3Harris (BeOn/Vida), and
Zetron. The TDMA two-slot mode doubles voice capacity by placing two calls
per 12.5 kHz channel.

---

## Key Online Resources

### TIA Standards

- **TIA Standards Catalog:** https://www.tiaonline.org/standards/catalog/
  (Search for "102.BCAD" to find current status and purchase options)
- **TIA TR-8 Committee page:** https://www.tiaonline.org/standards/
  (TR-8.12 Two-Slot TDMA subcommittee governs this document)

### APCO Project 25 Technology Interest Group (PTIG)

- **APCO P25 PTIG:** https://www.apcointl.org/technology/project-25/
  P25 technology documentation, compliance assessment program information,
  and interoperability resources.

### DHS SAFECOM / CISA P25

- **CISA SAFECOM P25:** https://www.cisa.gov/safecom/p25
  Federal guidance on P25 procurement, including Compliance Assessment
  Program (CAP) which formally tests P25 equipment including Phase 2.

### TIA FTP Server (MATLAB Reference Implementation)

Per Annex B of this document: `http://ftp.tiaonline.org/tr-8/tr-8.12/public`
Contains the MATLAB M-Files for the Phase 2 Two-Slot TDMA CAI Burst Generator
that generate the expected test vectors in Annex A. Note: FTP access may
require TIA membership or have changed since 2011 publication.

---

## Open-Source Implementations

The following open-source projects implement Phase 2 TDMA P25 CAI and
are directly relevant to validating or cross-referencing the components
tested in this conformance specification:

### OP25 (GNU Radio P25 receiver)

- **Repository:** https://github.com/boatbod/op25
- **Relevance:** Implements P25 Phase 2 TDMA decoding including RS
  decoding, DUID decoding, ISCH parsing, and voice burst unscrambling.
  The RS decoder in op25 can be compared against the error correction
  capability parameters in CP 3.1 (Tables 37, 38). The scrambling
  sequences generated by `Generate_OB_Scr_Seq.m` / `Generate_IB_Scr_Seq.m`
  correspond to op25's `p25_tdma.cc` scrambling logic.
- **Key files:** `op25/gr-op25-repeater/lib/p25p2_tdma.cc`,
  `op25/gr-op25-repeater/lib/rs.cc`

### SDRTrunk

- **Repository:** https://github.com/DSheirer/sdrtrunk
- **Relevance:** Java-based P25 Phase 2 decoder. Implements TDMA
  burst decoding, Reed Solomon FEC (ESS, IEMI, I-OEMI, S-OEMI), DUID
  decoding, I-ISCH decoding, scrambling, and MAC PDU parsing —
  exactly the functions exercised by CPs 1–4.
- **Key classes:** `io.github.dsheirer.module.decode.p25.phase2.*`,
  particularly `P25P2DataUnitID`, `P25P2SuperFrameFragment`, and the
  Reed-Solomon implementations in the `edac` package.
- **Cross-reference note:** The Reed Solomon code parameters in Tables 37–38
  (ESS: 44,16,29; IEMI: 46,26,21; S-OEMI: 45,26,20; I-OEMI: 52,30,23)
  should match the RS code parameters in SDRTrunk's P25 Phase 2 FEC classes.

### JMBE (Java Multi-Band Excitation vocoder)

- **Repository:** https://github.com/DSheirer/jmbe
- **Relevance:** Implements AMBE+2 / IMBE vocoder used in P25 voice bursts.
  Relevant to the vocoder test patterns (1031.25 Hz and silence) used in
  this conformance spec's voice burst construction tests (CP 2.2, CP 2.3).

### Osmocom / gr-osmosdr

- **Relevance:** Base SDR hardware support used by OP25. Not directly
  implementing P25 Phase 2 protocol but provides the RF acquisition
  layer beneath it.

---

## Cross-Validation Notes for Implementers

When validating a Phase 2 TDMA implementation against this document:

1. **Reed Solomon encoder:** The most important check. Feed Table 3 ESS
   inputs into your RS encoder and compare against Annex A.1 codewords.
   The RS mother code parameters are (n=44, k=16, GF(64)) for ESS —
   all other codes are punctured/shortened versions of this mother code.

2. **Scrambling sequences:** The outbound scrambling seed is derived from
   (WACN ID, System ID, Color Code). With the test values (0x47D89, 0x65C,
   0x65C), the Annex A.7 outbound 4V burst vectors provide ground truth for
   the complete scrambled burst. op25 and SDRTrunk both implement compatible
   scramblers that can be used for cross-check.

3. **I-ISCH encoding:** The (40,9,16) binary code is unusual. The six ISCH
   codewords in Table 48 (C_00=0x1442F705EF through C_05=0x18B37C5AC9)
   and the Annex A.6 decoded messages provide a full test set.

4. **MAC PDU construction:** The MAC_ACTIVE, MAC_PTT, and MAC_END_PTT PDU
   structures with their CRC12 computation are exercised by CP 4. The
   Compute_CRC12.m MATLAB function in Annex B provides the CRC algorithm.

5. **Burst framing:** The Annex A vectors encode the full burst structure
   including ISCH tail bits, DUID, ESS-B fields, and voice/EMI payload.
   Preceding slot partial ISCH values are labeled but are informative only
   (not part of the normative burst).
