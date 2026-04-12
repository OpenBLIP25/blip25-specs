# TIA-102.BAAA-B — Related Resources & Context

## Status

**Active.** TIA-102.BAAA-B (dated June 22, 2017) is the current active revision of the
Project 25 Common Air Interface standard for Phase 1 FDMA. It supersedes Revision A
(TIA-102.BAAA-A). No further revision has been published as of early 2026. The document
corrects two errors from Revision A: the FMF flag definition (see Section 5.4.1 footnote)
and a deprecated NACK type. This document is expected to remain stable as P25 Phase 1
is a mature, widely deployed technology.

P25 Phase 1 (FDMA, specified by this document) and P25 Phase 2 (TDMA, specified by
TIA-102.BBAB and companions) coexist in the current standards suite; this document does
not cover Phase 2.

---

## Standards Family

This document is part of the TIA-102 standards suite (Project 25 digital radio). The suite
has approximately 49 separate parts. Documents directly related to this specification:

### Physical and MAC Layer (FDMA Phase 1)
- **TIA-102.BAAA-B** *(this document)* — Common Air Interface (CAI): physical/MAC layer
- **TIA-102.BAAB** — Physical Layer for FDMA (RF specifications)
- **TIA-102.BAAD** — Data Services operational detail (uses packet formats defined here)

### Voice and Encryption
- **TIA-102.BABA** — IMBE Vocoder (defines c_0 through c_7 IMBE code word encoding
  carried in LDU1/LDU2 frames)
- **TIA-102.BABB** — Vocoder Mean Opinion Score Conformance Test (MOS-based quality test)
- **TIA-102.AAAD** — Encryption and Authentication (defines encryption algorithms
  referenced by ALGID field in the CAI header)

### Link Layer and Signaling
- **TIA-102.AABC** — Assigned Numbers (SAP identifiers, Manufacturer IDs referenced
  in Section 5 data packet formats)
- **TIA-102.AABF** — Link Control Word Formats (defines LC word content carried in LDU1
  and Terminator with LC)
- **TIA-102.AABC** — Trunking Control Channel Formats (defines NID DUID values for
  trunking, reserved beyond those in Table 18)

### Measurement and Conformance
- **TIA-102.CAAA** — Transceiver Measurements (C4FM deviation test methods)
- **TIA-102.CAAB** — CQPSK Measurements (CQPSK modulation test methods)
- **TIA-102.BCAB / BCAC** — CAI conformance tests

### Phase 2 (TDMA) — for comparison
- **TIA-102.BBAB** — TDMA Common Air Interface
- **TIA-102.BBAC** — TDMA MAC/Channel Access

### Standards Lineage (ASCII Tree)

```
TIA-102 (Project 25 Standards Suite)
├── TIA-102.AAAA — System and Standards Overview
├── TIA-102.AABC — Assigned Numbers
├── TIA-102.AAAD — Encryption / Authentication
├── TIA-102.AABF — Link Control Word Formats
│
├── Phase 1 FDMA
│   ├── TIA-102.BAAA-B  ← THIS DOCUMENT (CAI physical + MAC)
│   ├── TIA-102.BAAB    (FDMA physical layer RF)
│   ├── TIA-102.BAAD    (Data services)
│   ├── TIA-102.BABA    (IMBE vocoder)
│   └── TIA-102.AAAD    (Encryption)
│
├── Phase 2 TDMA
│   ├── TIA-102.BBAB    (TDMA CAI)
│   ├── TIA-102.BBAC    (TDMA MAC)
│   └── TIA-102.BABA-A  (half-rate vocoder, Sections 13-16)
│
├── Measurements
│   ├── TIA-102.CAAA    (Transceiver measurements)
│   └── TIA-102.CAAB    (CQPSK measurements)
│
└── Conformance
    ├── TIA-102.BCAB    (CAI conformance)
    └── TIA-102.BCAC    (CAI conformance type 2)
```

---

## Practical Context

This document defines what must be implemented at the air interface level to communicate
on a P25 Phase 1 system. In practice it is used by:

**Radio manufacturers** (Motorola, Harris/L3Harris, Kenwood, Hytera) to build P25-certified
subscriber and base station radios. The standard's physical layer parameters (4800 baud
π/4 DQPSK, C4FM or CQPSK, Nyquist raised cosine filtering) define the RF emissions
characteristics. The FEC codes (Golay, Hamming, Reed-Solomon, BCH) are implemented in
hardware DSP on the radio chipset.

**Public safety communications** (police, fire, EMS, federal agencies) rely on P25
interoperability certified to this standard. The Network Access Code (NAC) field is used
by agencies to filter co-channel traffic from adjacent systems sharing the same frequency.

**Software-defined radio (SDR) developers** use this document to implement P25 decoders
for monitoring, testing, and interoperability analysis. The trellis state machine tables,
interleave table, generator matrices, and bit order tables in Annex A are the primary
implementation references.

**System integrators** implementing the ISSI (Inter-RF Subsystem Interface) or CSSI
(Console Subsystem Interface) indirectly depend on this document for the frame timing
and content of Link Control words.

---

## Key Online Resources

### Official / Standards Bodies
- **TIA Standards Portal** — https://standards.tiaonline.org/all-standards/p25-downloads-application
  (P25 standard download index; some documents freely available)
- **Project 25 PTIG** — https://www.project25.org
  (P25 Technology Interest Group; published list of approved TIA standards)
- **P25 Approved Standards List (2023)** — https://www.project25.org/images/stories/ptig/P25_SC_23-02-001-R1_P25_TIA_Standards_Approved_16Feb2023.pdf
- **GlobalSpec TIA-102.BAAA entry** — https://standards.globalspec.com/std/10162320/tia-102-baaa

### Amateur / Public Safety Reference
- **RadioReference Wiki — P25** — https://wiki.radioreference.com
  (Community documentation on P25 systems, NID/NAC values for known systems)

---

## Open-Source Implementations

The following open-source projects implement significant portions of what this document
specifies. They are useful for cross-validation of algorithm implementations.

### SDRTrunk
- **URL:** https://github.com/DSheirer/sdrtrunk
- **Language:** Java
- **Relevance:** Most comprehensive open-source P25 Phase 1 decoder. Implements:
  - Frame sync detection (48-bit FS pattern)
  - NID decoding with BCH error correction
  - LDU1/LDU2 decoding with Golay, Hamming, and Reed-Solomon FEC
  - Trellis decoding for PDU data blocks
  - All six data unit types
  - JMBE integration for IMBE audio
- **Key paths:** `src/main/java/io/github/dsheirer/module/decode/p25/`
  - `P25P1MessageFramer.java` — frame sync and NID
  - `audio/codec/` — IMBE codec integration
  - FEC implementations under `edac/` or `module/decode/p25/`

### OP25
- **URL:** https://github.com/osmocom/op25
- **Language:** Python + C++ (GNU Radio blocks)
- **Relevance:** GNU Radio-based P25 Phase 1 and 2 decoder/transceiver. Implements
  the complete CAI receive chain including trellis decoding, NID, and all voice/data units.
  References TIA-102.BAAA-A compliance.
- **Key paths:** `op25/gr-op25-repeater/lib/` — C++ implementations of P25 algorithms

### p25.rs
- **URL:** https://github.com/kchmck/p25.rs
- **Language:** Rust
- **Relevance:** Pure Rust P25 CAI implementation. Implements NID, all FEC codes (BCH,
  Golay, Hamming, Reed-Solomon), LDU1/LDU2 parsing, and trellis decoding. Well-documented
  and structured for library use. Directly relevant for Rust-based implementation work.
- **Key modules:**
  - `src/coding/` — BCH, Golay, Hamming, Reed-Solomon implementations
  - `src/voice/` — LDU1/LDU2 frame decoding
  - `src/data/` — PDU decoding with trellis
  - `src/trunking/` — NID and DUID handling

### JMBE (Java IMBE/AMBE)
- **URL:** https://github.com/DSheirer/jmbe
- **Language:** Java
- **Relevance:** Open-source IMBE vocoder decoder (audio synthesis from IMBE code words).
  Decodes the c_0 through c_7 IMBE frames carried in LDU1/LDU2 voice channels.
  Used by SDRTrunk for voice output.

### imbe_vocoder (GNU Radio)
- **URL:** https://github.com/balr0g/imbe_vocoder
- **Language:** C++
- **Relevance:** Standalone IMBE encoder/decoder GNU Radio block. Useful for implementing
  IMBE encoding in a software transmitter or for testing FEC decode chains.

### robotastic/trunk-recorder
- **URL:** https://github.com/robotastic/trunk-recorder
- **Language:** C++
- **Relevance:** P25 trunked system recorder built on GNU Radio. Uses OP25 for P25 decoding.
  Shows practical integration of CAI decoding in a production monitoring system.

---

## Implementation Notes for This Document

**BCH (NID protection):** The (63,16,23) BCH code is the most computationally intensive
NID operation. SDRTrunk and p25.rs both implement syndrome-based decoding. The generator
polynomial g(x) = 6331 1413 6723 5453 (octal) can be verified against the 16×64 generator
matrix in Table 19.

**Reed-Solomon (voice FEC):** The three shortened RS codes use GF(2^6) with primitive
polynomial α^6 + α + 1. The GF(2^6) log/antilog tables (Table 6) are essential for
efficient implementation. SDRTrunk implements these in Java; p25.rs implements in Rust.

**Golay (header/LC FEC):** The (18,6,8) shortened Golay code is used in the most
performance-critical path (every IMBE frame header). The generator matrix in Table 3
is in systematic form and directly usable for encoding; decoding typically uses syndrome
table lookup.

**Trellis (data block FEC):** The rate-3/4 and rate-1/2 trellis decoders use Viterbi
algorithm with the state transition tables from Table 12. The interleave table (Table 14)
must be applied before Viterbi decoding (de-interleave on receive). Both SDRTrunk and OP25
implement this.

**Frame sync:** The 48-bit sync word (24 dibits) is typically detected by correlating
against the known dibit pattern from Table 15. A Hamming distance threshold of 4 or fewer
bit errors is commonly used in practice.
