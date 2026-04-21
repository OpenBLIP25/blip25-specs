# CRC-9 Single-Bit Correction Fixtures

Structural test cases for passive receivers that implement CRC-9 single-bit
syndrome correction (e.g., `crc9_validate_and_correct` in blip25). These
fixtures describe the input / expected-output relationship in terms of
**bit positions** rather than computed CRC values — the implementer
generates the specific CRC-9 bit patterns using their own validated
CRC-9 implementation, then locks them in as unit tests.

## Format per fixture

Each fixture is a row in the table below. A unit test instantiates the
fixture by:

1. Taking a **reference** clean Confirmed data block (17 octets ×
   {`DBSN`, CRC-9, 16 user-data octets}) where the CRC-9 validates.
2. Flipping the bit(s) at `corrupt_bits` (0-indexed, where bit 0 is MSB
   of octet 0 / DBSN MSB, bit 143 is LSB of octet 17 / last user-data
   octet LSB, for a 144-bit block).
3. Running the decoder's CRC-9 validate-and-correct path.
4. Comparing against `expected_behavior`.

## Fixtures

| id | corrupt_bits | region | expected_behavior | notes |
|----|--------------|--------|-------------------|-------|
| c9_flip_dbsn_msb | bit 0 | DBSN MSB | decoder corrects; recovered bit position = 0; `bit_corrected = true`; output CRC-9 passes | Earliest bit in the block — sensitive to "start of syndrome" indexing off-by-one bugs |
| c9_flip_dbsn_lsb | bit 6 | DBSN LSB | corrects; recovered position = 6 | Last bit of DBSN |
| c9_flip_crc_msb | bit 7 | CRC-9 field MSB | corrects; recovered position = 7 | Error is in the CRC field itself, tests correction across the "dbsn \| CRC" boundary |
| c9_flip_crc_lsb | bit 15 | CRC-9 field LSB | corrects; recovered position = 15 | Last bit of CRC-9 |
| c9_flip_user_first | bit 16 | user data octet 0, MSB | corrects; recovered position = 16 | First user-data bit |
| c9_flip_user_mid | bit 79 | user data octet 7, LSB | corrects; recovered position = 79 | Middle of user data region |
| c9_flip_user_last | bit 143 | user data octet 15, LSB | corrects; recovered position = 143 | Last bit in the block — sensitive to "end of syndrome" indexing off-by-one |
| c9_flip_two_bits_ambiguous | bits 20 and 100 | user data, 2 flips with non-unique syndrome | **does not correct**; output `crc_ok = false` | Ensures the corrector doesn't claim success when the syndrome doesn't uniquely identify a single-bit flip |
| c9_flip_two_bits_near | bits 42 and 43 | user data, adjacent bits | **does not correct**; output `crc_ok = false` | Adjacent bit flips should be rejected because the syndrome is unambiguous but wrong for a single-bit model |

## How to generate the reference block

Pick a reference DBSN + user-data payload that's easy to inspect in hex.
Suggested: DBSN = 0x05, user-data = `00 11 22 33 44 55 66 77 88 99 AA BB CC DD EE FF` (the same
canonical 16-octet test pattern as row 12 of `pdu_crc_test_vectors.csv`).

Compute CRC-9 over `serial_number (7 bits) || 16 data octets (128 bits) =
135 bits` per BAAA-B §5.4.2 to produce the reference clean block. That
gives you a known-good 144-bit block. Flip one or two bits per the table
above to generate the test input; the expected corrected output (for
single-bit cases) is the original reference block.

**Note on test-vector consistency:** `pdu_crc_test_vectors.csv` (delivered
under gap 0008) contains rows claiming specific CRC-9 values for the
canonical pattern. As of 2026-04-21 I was unable to reproduce those values
under standard CRC-9 conventions while drafting gap 0014; implementers
should validate their CRC-9 output against a second reference (e.g.,
SDRTrunk's `edac/CRC9` or OP25) before depending on the gap-0008 vectors.
That discrepancy is a separate question from the layout/correction
fixtures documented here.

## Scope

- FDMA Phase 1 Confirmed Data Packets only. Unconfirmed blocks have no
  CRC-9 (they rely on the whole-packet CRC-32).
- Single-bit correction only. Multi-bit error recovery is out of scope for
  CRC-9; it lives in the cross-copy merge layer
  (`pdu_reassembly_fixtures/`).
- These fixtures describe *decoder correction behavior*. They don't
  replace the integrity-level CRC test vectors in
  `pdu_crc_test_vectors.csv`.
