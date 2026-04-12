# TIA-102.BAAB-B: Related Resources and Context

## Document Context

TIA-102.BAAB-B is the **conformance test specification** for the P25 Phase 1 FDMA Common
Air Interface. Its role in the TIA-102 family is to provide bit-exact test vectors that
validate implementations of the algorithms defined in the normative CAI specification
(TIA-102.BAAA). It is the ground truth for P25 frame encoding correctness.

---

## TIA-102 Standards Family — Direct Dependencies

### Normative References (specs this document tests)

| Document | Title | Relationship |
|----------|-------|-------------|
| **TIA-102.BAAA** | Project 25 FDMA Common Air Interface | Primary subject — defines the frame structure, FEC codes, and field formats that BAAB-B tests |
| **TIA-102.BABA** | IMBE Vocoder (Full Rate) | Defines the IMBE u-vector format used as input to LDU1/LDU2 test vectors |
| **TIA-102.BABA-A** | IMBE Vocoder Phase 2 (Half Rate) | Related vocoder for P25 Phase 2 |
| **TIA-102.AABF** | Project 25 Security | Defines ALGID, KID, MI fields tested in encrypted variants |

### Related TIA-102 Conformance Documents

| Document | Title | Relationship |
|----------|-------|-------------|
| **TIA-102.BAAB** | Conformance Tests for P25 CAI (Rev A) | Predecessor to this document |
| **TIA-102.CAAB** | Conformance Tests for P25 TDMA CAI | Analogous conformance spec for Phase 2 TDMA |

### Standards That Consume These Test Results

| Document | Title | Relationship |
|----------|-------|-------------|
| **TIA-102.AAAA** | P25 Overview and Architecture | Top-level system document |
| **TIA-102.AABG** | P25 Inter-RF Subsystem Interface | ISSI; relies on correct CAI framing |
| **TIA-102.BAAC** | P25 CAI Performance Recommendations | Complements conformance tests with performance metrics |

---

## FEC Algorithm Resources

### BCH Codes
- **BCH(64,16,23)** — used for NID encoding.
- Lin & Costello, *Error Control Coding* (2nd ed., Prentice-Hall) — authoritative textbook treatment of BCH codes.
- The generator polynomial for BCH(64,16,23) over GF(2^6) is defined in TIA-102.BAAA Appendix.

### Reed-Solomon Codes
- **RS(24,12,13), RS(24,16,9), RS(36,20,17)** — all over GF(2^6) with primitive polynomial x^6 + x + 1.
- Blahut, *Algebraic Codes for Data Transmission* (Cambridge, 2003).
- The `multgf26()` function in Annex A implements GF(2^6) multiplication with mod-reduction against 0x43.

### Golay Code
- **Golay(23,12)** — perfect binary code, t=3.
- The Golay generator matrix used in P25 is given explicitly in TIA-102.BAAA and implemented in `golay_23_encode()`.

### Hamming Codes
- **Hamming(15,11,3)** and **Hamming(10,6,3)** — standard shortened Hamming codes.
- The generator matrices are given as integer arrays in Annex A.

### Trellis Codes
- **Trellis 3/4 and 1/2 rate** — 16-state CQPSK trellis codes for data packets.
- Uses a fixed dibit interleave rule (`intlvRule[98]`) before constellation mapping.
- Signal point mapping via `signalP[16][2]` table (CQPSK 4-level FSK/PSK hybrid).

### CRC
- **CRC-CCITT (16-bit)**: polynomial 0x1021.
- **CRC-32**: standard IEEE 802.3 polynomial.

---

## Open-Source P25 Implementations

These projects implement P25 CAI encoding and/or decoding. The test vectors in BAAB-B
are useful for validating their FEC stages.

### Software-Defined Radio (SDR)
- **op25** (GNU Radio P25 implementation): https://github.com/boatbod/op25
  - Implements P25 Phase 1 and Phase 2 decoders; uses FEC matching BAAB-B vectors.
- **gr-p25** (legacy GNU Radio P25): historically significant early implementation.
- **DSD (Digital Speech Decoder)**: https://github.com/szechyjs/dsd
  - Decodes P25, DMR, and D-STAR; P25 FEC matches BAAB-B structure.
- **dsd-fme** (DSD fork with extended P25): https://github.com/lwvmobile/dsd-fme
  - Extended P25 support including Phase 2.

### Rust Implementations
- **p25** crate (Rust): https://github.com/kchmck/p25.rs
  - Rust P25 library implementing CAI frame structure; FEC stages should match BAAB-B.
- This TIA-P25 project: intended to drive a Rust implementation via spec-driven development.
  See `docs/CLAUDE_for_rust_project.md` for the Rust project CLAUDE.md template.

### Reference C Code
- Annex A of this document (TIA-102.BAAB-B) contains the original authoritative C source.
- The five programs (hdr.c, term.c, ldu1.c, ldu2.c, data.c) can be extracted and compiled
  to generate additional test vectors or validate an implementation against specific inputs.

---

## P25 Compliance Assessment Program (CAP)

The TIA P25 CAP certifies equipment for P25 interoperability. BAAB-B test vectors are
used in CAP testing to verify conformant CAI framing.

- **PTIG (P25 Technology Interest Group)**: https://www.ptig.org
  - Administers CAP; maintains the list of CAP-tested equipment.
- **NPSTC (National Public Safety Telecommunications Council)**: https://www.npstc.org
  - Promotes P25 adoption; publishes P25 implementation guides.
- **DHS SAFECOM**: https://www.dhs.gov/safecom
  - Federal agency supporting emergency communications interoperability; references P25 CAP.

---

## Educational Resources

### P25 Overview
- APCO International: https://www.apcointl.org — the standards development organization for P25.
- *Project 25 Technology Interest Group Technology Advisory Committee* white papers — available from ptig.org.
- RadioReference wiki P25 article: community-maintained P25 protocol overview.

### IMBE Vocoder
- The IMBE algorithm is covered in: Hardwick & Lim, *Multiband Excitation Vocoder*, 1988.
- P25 uses IMBE at 4,400 bps voice + 2,800 bps FEC overhead.

### FDMA vs TDMA (P25 Phase 1 vs Phase 2)
- P25 Phase 1 FDMA: 12.5 kHz channels, C4FM or CQPSK modulation.
- P25 Phase 2 TDMA: 12.5 kHz channels, 2 time slots (TDMA), DQPSK modulation.
- This document (BAAB-B) covers Phase 1 FDMA only.

---

## LLM / RAG Context Notes

For LLM-based code generation or retrieval-augmented generation against P25 implementations:

1. **Frame Sync constant** is `{0x5575, 0xf5ff, 0x77ff, 0x0000}` — appears at start of every data unit.
2. **DUID lookup**: 0=HDU, 3=Terminator, 5=LDU1, 0xa=LDU2, 0xc=Data Packet.
3. **NID structure**: `[NAC[11:0] | DUID[3:0] | BCH_parity[47:0]]` in 64 bits total.
4. **GF(2^6) primitive polynomial**: x^6 + x + 1 = 0x43 (mod-reduction constant).
5. **IMBE PN seed**: `p = 16 * u[0]`, recurrence `p = (173 * p + 13849) & 0xffff`.
6. **Status Symbol insertion**: SSym dibits are interleaved at fixed positions within LDU frames.
7. **Test vector format**: each microslot = 6 twelve-bit words (3 hex digits), two microslots per display line.
8. **NAC 0x293** (hex) is the primary test NAC used throughout Annex B.

These constants and structures are implementation-invariant — any conformant P25 implementation
must use exactly these values.
