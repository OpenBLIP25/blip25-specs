# 0014 — PDU CRC test vectors: extend coverage to pad/multi-block edges and single-bit correction

**Status:** drafted (2026-04-21) — structural fixtures delivered in `annex_tables/pdu_last_block_examples.csv` (pad-boundary layouts), `annex_tables/pdu_crc9_correction_fixtures.md` (single-bit correction bit-position fixtures), and `annex_tables/pdu_reassembly_fixtures/README.md` (cross-copy merge fixture schema). Bit-exact CRC values for new pad-boundary vectors deferred pending resolution of gap report 0016 (`pdu_crc_test_vectors.csv` existing values couldn't be reproduced under standard CRC conventions).
**Filed:** 2026-04-21
**Filer:** implementer (p25-decoder / blip25-data)
**For:** spec-author
**Related:**
- `annex_tables/pdu_crc_test_vectors.csv` (delivered in gap report 0008)
- `gap_reports/0008_pdu_crc_test_vectors_missing.md` (resolved — baseline CRC vectors)
- `gap_reports/0010_last_block_pad_crc32_layout.md` (related — needs test vectors)

---

## 1. What's already there

Gap report 0008 (resolved `e7b2ef4`) delivered
`annex_tables/pdu_crc_test_vectors.csv` with 15 vectors covering:

- **CRC-CCITT-16 (Header CRC)** — 7 vectors: all-zeros, all-ones, MSB-only,
  LSB-only, one Unconfirmed header, one Confirmed header, one Response
  header.
- **CRC-9 (per-block Confirmed)** — 4 vectors: serial=0 all-zero data,
  serial=1 all-zero data, serial=0 all-one data, canonical pattern.
- **CRC-32 (Packet CRC)** — 4 vectors: BTF=1 zeros, BTF=1 'A'×8, BTF=2
  20-octet pattern, BTF=3 32-octet pattern.

This is excellent baseline coverage — locks down bit-order, polynomial,
and init-value choices so the MSB-first vs LSB-first landmine is caught
at `cargo test` time, not on-air.

## 2. What's missing for full implementer coverage

Three categories of edge-case vectors would close the remaining gaps:

### 2.1 Pad-boundary layout (needs gap 0010 resolution first)

Once gap 0010 locks down whether pad sits between user data and CRC-32
or in some other position, we need test vectors that exercise the boundary.
Suggested additions:

| Case | Description | Why it matters |
|------|-------------|----------------|
| Unconfirmed `btf=2, pad=8` | Pad spans second-to-last block per 0007 | Catches implementations that assumed single-block pad |
| Unconfirmed `btf=1, pad=4` | Pad + CRC fit in one 12-octet block | Minimum pad spill |
| Confirmed `btf=2, pad=13` | Pad caps in Confirmed case (P_MAXC=15) | Confirms the 0007 cap with wire data |
| Confirmed `btf=3, pad=0` | Baseline — ensures CRC-32 over raw blocks only, no pad strip | Regression against future refactors |

### 2.2 Single-bit correction exercise

My decoder recovers CRC-9 single-bit errors via unique syndrome matching
(`crates/blip25-core/src/pdu.rs::crc9_validate_and_correct`). Test vectors
for this path would catch regressions where the syndrome table drifts.

| Case | Description |
|------|-------------|
| CRC-9 input + bit flipped at position 0 (DBSN MSB) | Correction via data syndrome |
| CRC-9 input + bit flipped at position 7 (CRC field MSB) | Correction via CRC-field syndrome |
| CRC-9 input + bit flipped at position 143 (last data bit) | Correction via data syndrome, end-of-block |
| CRC-9 input + *two* bits flipped with ambiguous syndrome | Should NOT correct (expected output: `crc_ok=false`) |

Each test: input bits (corrupted), expected unique position, expected
corrected block bits.

### 2.3 Cross-copy merge of errored blocks

Relates to gap 0009 (cross-copy reassembly). A test fixture of two
Confirmed PDUs:

- **Copy A** — blocks 1–3 clean, block 4 errored (CRC fail)
- **Copy B** — block 1 errored, blocks 2–4 clean

Expected merged output:
- blocks 1–4 clean (block 1 from Copy B, blocks 2–3 from either, block 4
  from Copy B)
- `blocks_errored = 0` on the assembled event

This is a protocol-behavior test, not a CRC-math test, but it lives
adjacent enough to the CRC vectors that colocating is probably convenient.

## 3. What the implementer needs

1. **Extend `annex_tables/pdu_crc_test_vectors.csv`** with the 7–8 rows
   suggested in §2.1 and §2.2.
2. **Add a new `annex_tables/pdu_reassembly_fixtures/`** directory with
   a few small `.hex` files representing multi-copy PDUs and their
   expected merged output, for gap 0009-aware decoders.
3. **Cross-verify the new vectors** against SDRTrunk's CRC test suite
   (`src/test/java/io/github/dsheirer/edac/`) to ensure the bit-order
   conventions agree.

## 4. Diagnostic evidence

My decoder's per-block CRC-9 correction rate on the Sachse gold-standard
is:
- 81 PDUs decoded
- 14 total blocks corrected via single-bit syndrome correction
  across all copies
- 0 known false-positive corrections (validated against SDRTrunk's
  decoded output which matches our clean-merged text).

That's field evidence the correction path works on one capture, but a
unit test lock-in would catch regressions that a future refactor of
`crc9_bit_syndrome` might introduce.

## 5. Chip probe plausibility

Not applicable — pure derived-work extension. No standards research
required; all inputs can be generated synthetically and cross-verified
against SDRTrunk.

## 6. Priority

Low — 0008 delivered enough coverage for the core implementer. This is a
follow-up that becomes worth doing *after* gap 0010 resolves (because the
pad-boundary vectors can't be written until we know the layout is
unambiguous) and *in parallel with* gap 0009 (because the cross-copy
fixtures overlap in scope).
