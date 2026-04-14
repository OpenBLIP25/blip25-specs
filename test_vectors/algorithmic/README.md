# Algorithmic Test Vectors

Deterministic input→output pairs for the pure-math stages of P25 vocoder
processing. Every file here is regenerable from the TIA spec alone; no
proprietary inputs or outputs.

## Files

| File | Rows | What it validates |
|------|-----:|-------------------|
| `golay_23_12_roundtrip.csv` | 4096 | [23,12] Golay encode: 12-bit info → 23-bit codeword |
| `golay_24_12_roundtrip.csv` | 4096 | [24,12] Extended Golay encode: 12-bit info → 24-bit codeword |
| `hamming_15_11_roundtrip.csv` | 2048 | [15,11] Hamming encode: 11-bit info → 15-bit codeword |
| `gf64_exp_log_pairs.csv` | 63 | GF(64) exp/log round-trip: `EXP[LOG[b]] == b` for b=1..63 |
| `bit_prioritization_samples.csv` | variable | Worked §1.4 scan for representative L values |

All files have a `#`-prefixed header block with the source equations /
tables / CSVs in this repo, regeneration command, and invariant summary.
Data rows use standard CSV (comma-separated, LF line endings).

## Regeneration

```
python3 tools/gen_algorithmic_vectors.py
```

This reads the authoritative matrices/polynomials from `standards/*/` and
`standards/*/annex_tables/*.csv`, regenerates every file here, and
verifies each against its invariant before writing.

**If a regenerated file differs from the committed file:**
- If the change is intentional (spec update, new test coverage), commit
  both the tool change and the new outputs.
- If the change is unexpected, investigate the generator or the source
  data — don't silently overwrite. The committed files are the audit
  trail.

## What to Use This For

A conformant implementation should:

- **Reproduce** every `<input> → <output>` pair in these CSVs bit-for-bit.
- **Fail fast** on any mismatch during the build's test suite, not at
  runtime.
- **Log the mismatch** to your CI output with the row number of the
  failing input — these CSVs are small enough to inspect by hand.

If your implementation disagrees with one of these files, either
(a) your implementation has a bug, (b) the generator has a bug, or
(c) the underlying spec data (in `standards/*/annex_tables/`) has a
bug. Investigate in that order — the generators are simple and the
spec data has been invariant-checked.

## Coverage

These vectors cover the **deterministic, L-independent** parts of the
pipeline. They do NOT cover:

- Audio synthesis (no audio anywhere in this directory).
- L-dependent paths that only exercise at specific L values (for full
  coverage of every L ∈ [9, 56], use the tables in
  `standards/*/annex_tables/` directly as test inputs).
- Cross-frame state paths (log-mag prediction, frame repeat, frame mute).
  Those need multi-frame inputs from `../synthetic/`.
- Dequantization with non-trivial step sizes (implicitly tested via
  `bit_prioritization_samples.csv` for a few L values, but not
  exhaustive across all B_m values).
