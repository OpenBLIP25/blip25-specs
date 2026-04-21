# 0008 — PDU CRC test vectors missing from derived works

**Status:** resolved (2026-04-21, commit `e7b2ef4`)
**Filed:** 2026-04-21
**Filer:** spec-author agent
**For:** spec-author agent (enhancement; not a standard ambiguity)
**Resolution:** Generated `annex_tables/pdu_crc_test_vectors.csv` with 15
canonical (input → expected wire CRC) pairs covering CRC-CCITT-16 (80-bit
header, 7 vectors), CRC-9 (135-bit confirmed data block, 4 vectors), and
CRC-32 (1/2/3-block unconfirmed payloads, 4 vectors). Implementation
cross-verified against SDRTrunk `edac.CRCP25` checksum tables on 8 independent
table entries. Added BAAA-B §13.4 documenting the post-inversion wire format
and §13.5 pointing implementers at the CSV.
**Related:**
- `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md` §13 (CRC polynomials)
- `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md` §5.7 (PDU subtypes)
- `analysis/fdma_pdu_frame.md` §7 (three-CRC summary)

---

## 1. Nature of the gap

This is *not* a standard ambiguity — BAAA-B §5.2, §5.3.3, and §5.4.2 (full
text) fully specify the three PDU CRCs (Header CRC-CCITT-16, per-block CRC-9,
Packet CRC-32) including polynomial, inversion polynomial, and MSB-first bit
order. Our impl spec §13 has the polynomials and initial values. The spec is
complete.

What's missing is **test vectors**: concrete (input octets → expected CRC)
pairs that an implementer can run against their CRC routine to verify
correctness before exposing bugs on-air. P25's bit-order convention (MSB-first
with reflected output = false) is the opposite of the LSB-first convention
used in most consumer modem code (Kermit, XMODEM, Ethernet FCS, PNG). An
implementer who grabs a CRC-CCITT library off the shelf will silently get
wrong answers; a test vector would catch this instantly.

## 2. What we have

- CRC polynomials and initial values in BAAA-B Impl Spec §13.
- Reference to SDRTrunk's `TSBKMessage.java` CRC routine in AABB-B Impl Spec
  §4.5.
- No CSV of (input, output) test vectors in `annex_tables/`.

## 3. What we need

A new `annex_tables/pdu_crc_test_vectors.csv` (or similar) containing, for
each of the three CRCs:

| CRC type | Input octets (hex) | Expected CRC (hex) | Notes |
|----------|-------------------|--------------------|-------|
| Header CRC-16 | `55 FD 00 11 22 33 82 00 00 00` | `AB CD` | Example Standard MBT header |
| CRC-9 | `02 00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF` | `1 FB` (9 bits) | Serial #1 + 16 data octets |
| Packet CRC-32 | ... 48-octet payload ... | `12 34 56 78` | 4-block Unconfirmed packet |

Sources for test vectors (in decreasing order of trustworthiness):

1. Captured on-air traffic with known plaintext (if we have any clean
   `*.wav` captures where the PDU contents can be reconstructed from
   context).
2. SDRTrunk unit tests under `src/test/java/.../p25/phase1/message/pdu/`.
3. Synthetic test vectors generated from a reference CRC implementation
   validated against SDRTrunk.

## 4. Chip probe plausibility

Not applicable — pure derived-work enhancement. No standards research needed;
this is an extraction task against already-consulted full-text + reference
implementations.

## 5. Priority

Low-to-medium. The implementer can verify their own CRC routines against
SDRTrunk's Java code directly, so this gap is an ergonomics issue rather than
a blocker. But CRC bit-order bugs are notoriously silent (wrong answers look
plausible), so having a CSV of test vectors in the repo makes the derived
work more reusable by implementers who don't want to pull down a whole Java
project to cross-check.

## 6. Related future work

Similar test-vector CSVs would also help for:
- NID BCH(63,16,23) encode/decode (specific input → encoded 64-bit NID)
- Golay(24,12,8) for TDULC
- RS(24,12,13) for LDU1 LC outer code
- RS(36,20,17) for HDU outer code

All of these are already fully specified in the impl specs, but test vectors
would be a valuable addition. This report is scoped to the three PDU CRCs
because they're the most recently consolidated and the most bit-order-sensitive.
