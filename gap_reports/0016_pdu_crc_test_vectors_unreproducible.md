# 0016 — pdu_crc_test_vectors.csv CRC values don't reproduce under standard conventions

**Status:** drafted (2026-04-21) — fixed by swapping column header names in `annex_tables/pdu_crc_test_vectors.csv` (Option 1 from §0 below). Row values remain valid under the field-validated convention in blip25's `crc_ccitt` (init=0, poly=0x1021, MSB-first, final XOR 0xFFFF), which also empirically passes Header CRC-16 on 81/86 PDUs from the Sachse `.bits` capture. No downstream decoder changes needed.
**Filed:** 2026-04-21
**Filer:** spec-author (while drafting resolution of gap 0014)
**For:** user / implementer

**Related:**
- `gap_reports/0008_pdu_crc_test_vectors_missing.md` (resolved commit `e7b2ef4` — delivered the CSV in question)
- `gap_reports/0014_pdu_crc_test_vectors_pad_and_correction_coverage.md` (wants to extend the CSV)
- `annex_tables/pdu_crc_test_vectors.csv`
- `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md` §13 (CRC polynomials)

---

## 0. Implementer diagnostic (2026-04-21)

Ran the Sachse-proven `crc_ccitt` from `crates/blip25-core/src/tsbk.rs`
(init=0, poly=0x1021, MSB-first, final XOR 0xFFFF) against three rows:

| Row | Message | CSV `raw_remainder_hex` | CSV `wire_crc_hex` | Our impl returns |
|-----|---------|------------------------|-------------------|------------------|
| 1 (all-zeros) | `00000000000000000000` | `0xFFFF` | `0x0000` | **`0xFFFF`** |
| 2 (all-ones) | `FFFFFFFFFFFFFFFFFFFF` | `0xB827` | `0x47D8` | **`0xB827`** |
| 5 (TGID grant) | `55FD0011223382000000` | `0x7267` | `0x8D98` | **`0x7267`** |

Our impl matches `raw_remainder_hex` on all three. Since the impl is
empirically correct on 81/86 PDUs from the Sachse `.bits` capture (the
header CRC comparison is `compute(bits[0..80]) == extracted(bits[80..96])`
and passes on those 81), the 0x7267-class values **are** the bits on the
wire.

Conclusion: the CSV's column names are reversed. The `raw_remainder_hex`
column holds wire-CRC bytes (post-inversion, what's transmitted);
`wire_crc_hex` holds the pre-inversion register state. XOR of the two is
0xFFFF as expected (the inversion polynomial I(x) for CRC-16), so the
data is internally consistent — only the labels are swapped.

**Fix options:**

1. **Rename columns** in `pdu_crc_test_vectors.csv`: swap `raw_remainder_hex`
   ↔ `wire_crc_hex`. Minimal change; existing row values remain valid.
2. **Swap values** in place and keep the existing column names. More
   intrusive; any downstream consumer that already read the CSV under the
   current labels breaks.

Option 1 is cheaper and keeps already-authored values load-bearing.

Also applies to the CRC-9 and CRC-32 rows by symmetry — both use the
same "invert with all-ones I(x)" transmission convention per BAAA-B §5.2 /
§5.3.3 / §5.4.2. If any single row's labels are correct, all rows are; if
any are swapped, all are.

---

## 1. What happened

While drafting pad-boundary CRC-32 vectors for gap 0014, I attempted to
verify my CRC-32 implementation by reproducing the CRC values already in
`annex_tables/pdu_crc_test_vectors.csv`. Every computation I tried
disagreed with the existing values, across all four CRC-32 rows and all
seven CRC-CCITT-16 rows.

Specifically, for CRC-32 `BTF=1, 8 zero octets`, the CSV claims:

| column | CSV value |
|--------|----------|
| `raw_remainder_hex` | `0xFFFFFFFF` |
| `wire_crc_hex` | `0x00000000` |

I attempted three conventions for the CRC-32 math:

| Convention | Produces | Matches CSV? |
|------------|----------|--------------|
| MSB-first, init=`0xFFFFFFFF`, no reflection, raw only | `0x6904BB59` | no |
| MSB-first, init=`0x00000000`, no reflection, raw only | `0x00000000` | no — swapped from CSV |
| LSB-first, init=`0xFFFFFFFF` (Ethernet / zlib.crc32) | `0x6522DF69` | no |

Same pattern for all other rows. The CSV's `raw` and `wire` columns do XOR
to the inversion polynomial (all ones, correct per BAAA-B §5.3.3), so the
two columns are self-consistent, but I can't identify the CRC algorithm
that produces those specific remainders.

## 2. Possibilities

1. **The CRC is correct under a convention I haven't tried.** There are
   many CRC-32 variants (BZIP2, MPEG-2, POSIX, JAMCRC, Castagnoli) and a
   convention that combines one of those polynomials with the spec's
   inversion polynomial might reproduce the values.
2. **The CRC values were computed with a buggy or novel implementation.**
   The CSV was generated during gap 0008 resolution; there's no
   cross-verification path documented. If the generator had a specific
   bug (e.g., inverted init condition, wrong bit-ordering per octet), the
   values would be consistently wrong in a way that's still internally
   consistent (raw XOR wire = I(x)).
3. **The CSV column headers are mis-labeled.** If "raw_remainder_hex" is
   actually "verification residual" (register state after processing
   message + wire CRC) and "wire_crc_hex" is the actual on-air CRC, the
   convention might differ from what the column names suggest.

## 3. What would close this

Run the rows in the CSV through a known-good CRC implementation — ideally
one validated against a non-P25 reference — and compare:

- **SDRTrunk:** `src/test/java/io/github/dsheirer/edac/CRC9Test.java`,
  `CRC32Test.java` if present. These are per-block, not per-PDU, but they
  lock down the polynomial conventions.
- **OP25:** `op25/gr-op25_repeater/apps/...` has CRC utilities per block.
- **A from-scratch calculator.** Implement BAAA-B §5.3.3 literally
  (`F_M(x) = (x^32 M(x) mod G_M(x)) + I_M(x)`) and verify against a hand
  computation on a 1-bit message. The CSV's MSB-only / LSB-only rows are
  the most diagnostic here.

Either the CSV is correct under a convention I didn't try (in which case
document the convention in the CSV header), or the CSV has bug(s) and
needs regeneration. Either way, a ground-truth fix here is load-bearing
for any decoder that uses the vectors as unit tests.

## 4. Impact on gap 0014 resolution

Because I couldn't verify the existing CRC conventions, I declined to
extend `pdu_crc_test_vectors.csv` with new pad-boundary rows — new vectors
computed under a different convention would introduce a second
inconsistency. Instead, gap 0014 was resolved with **structural** fixtures:

- `annex_tables/pdu_last_block_examples.csv` — octet-by-octet layout for
  pad-boundary cases (no CRC values).
- `annex_tables/pdu_crc9_correction_fixtures.md` — bit-flip position
  fixtures for single-bit correction testing.
- `annex_tables/pdu_reassembly_fixtures/README.md` — cross-copy merge
  fixture schema.

Once gap 0016 resolves, the last-block-examples CSV can be promoted to
carry specific computed CRC values and merged back into
`pdu_crc_test_vectors.csv`.

## 5. Chip probe plausibility

Not applicable — this is a CRC-math / test-vector-correctness question.
No chip involved.

## 6. Priority

Medium. The existing vectors are consumed by at least one test suite
(blip25 `pdu_crc_test_vectors_match` or similar), so they're either
already wrong-in-a-way-that-matters or right-under-an-unstated-convention.
Either outcome warrants pinning down before more vectors accumulate.
