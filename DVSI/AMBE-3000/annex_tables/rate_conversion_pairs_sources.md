# rate_conversion_pairs.csv — Sources and Classification

## Purpose

Per-pair conversion configuration for all 64×64 (source, target) chip
rate-index pairs, covering chip indices 0–61 plus project extensions
62 and 63 (P25 full-rate IMBE with and without FEC).

## Row Count

4096 data rows (64² pairs). No pair is omitted; unsupported and pending
pairs are explicit.

## Classification Categories

| Category                            | Count | supported | Rule |
|-------------------------------------|------:|-----------|------|
| `identity`                          |    64 | yes       | `source == target` — all transforms no-op, §8 passthrough |
| `p25_fullrate_fec_toggle`           |     2 | yes       | both source and target in {62, 63} — P25 full-rate IMBE FEC on/off, MbeParams identical, bit-pack only |
| `p25_fullrate_to_halfrate`          |     8 | yes       | {62, 63} ↔ {33, 34} — project `p25_fullrate ↔ p25_halfrate`; Rate Converter spec §4 primary target |
| `p25_fullrate_to_halfrate_variant`  |   108 | pending   | {62, 63} ↔ {35–61} — AMBE+2 variant bit allocation pending |
| `ambe_plus_2_halfrate_internal` (P25 subset) |  2 | yes | pairs within {33, 34} not caught by identity — same bit allocation, FEC differs only |
| `ambe_plus_2_halfrate_internal` (variants) | 810 | pending | pairs within {33–61} involving at least one of {35–61} — variant bit allocation pending |
| `ambe_1000_internal`                |   240 | no        | both in {0–15} — AMBE-1000 generation, out of spec scope |
| `ambe_2000_internal`                |   272 | no        | both in {16–32} — AMBE-2000 generation, out of spec scope |
| `cross_generation`                  |  2590 | no        | different generations — out of spec scope |
| `reserved`                          |     0 | no        | reserved index involved (currently none, since 62/63 are mapped) |

Supported: 84. Pending: 918. Unsupported: 3102 + 64 identity = 3102 `no` + rest.

## Column Definitions

| Column                  | Type    | Meaning                                       |
|-------------------------|---------|-----------------------------------------------|
| `source_rate`           | int     | Chip rate index for source side (0–63)        |
| `target_rate`           | int     | Chip rate index for target side (0–63)        |
| `supported`             | enum    | `yes`, `no`, `pending`                        |
| `voicing_xform`         | enum    | `none`, `8band_resample`, `pending`, `n/a`    |
| `magnitude_xform`       | enum    | `none`, `harmonic_resample_interp`, `pending`, `n/a` |
| `unvoiced_compensation` | enum    | `none`, `standard`, `pending`, `n/a`          |
| `predictor_rho`         | float\|string | `0.65` for AMBE+2 default, `n/a` for identity/unsupported, `pending` for variants |
| `category`              | enum    | one of the values in the table above          |
| `notes`                 | string  | Per-pair free text                            |

## Transform Semantics

For supported non-identity pairs:

- `voicing_xform = 8band_resample`: re-quantize the 8-band voicing vector
  for the target rate's V/UV codebook (Rate Converter spec §4.2).
- `magnitude_xform = harmonic_resample_interp`: resample the harmonic
  magnitudes from source L̃ to target L̂ via linear interpolation on
  the log-magnitude grid (§4.3).
- `unvoiced_compensation = standard`: apply the §4.4 scalar multiply to
  unvoiced-band magnitudes.
- `predictor_rho = 0.65`: AMBE+2 default predictor coefficient (§4.5 /
  US8595002). Applies to inter-frame log-magnitude prediction in the
  target encode direction.

For identity pairs all transforms are no-ops; the converter becomes a
format re-packer (§8 passthrough).

## Classification Logic (Reference)

```
def classify_pair(src, tgt):
    g_src = generation(src); g_tgt = generation(tgt)
    if src == tgt:
        return identity
    if both p25_fullrate_imbe (in {62, 63}):
        return p25_fullrate_fec_toggle  # supported
    if {g_src, g_tgt} == {p25_fullrate_imbe, ambe_plus_2_halfrate}:
        if the ambe_plus_2 side is 33 or 34:
            return p25_fullrate_to_halfrate  # supported
        else:
            return p25_fullrate_to_halfrate_variant  # pending
    if both ambe_plus_2_halfrate:
        if both in {33, 34}:
            return ambe_plus_2_halfrate_internal  # supported
        else:
            return ambe_plus_2_halfrate_internal  # pending
    if both ambe_1000:
        return ambe_1000_internal  # unsupported (out of scope)
    if both ambe_2000:
        return ambe_2000_internal  # unsupported
    return cross_generation  # unsupported
```

Generation bucketing:

| Chip indices | Generation            |
|--------------|-----------------------|
| 0–15         | `ambe_1000`           |
| 16–32        | `ambe_2000` (32 is AMBE-2000-compat) |
| 33–61        | `ambe_plus_2_halfrate` |
| 62, 63       | `p25_fullrate_imbe` (project extension) |

## Dependencies

This CSV becomes more useful as `halfrate_bit_allocations.csv` advances.
Specifically, each row where `supported = pending` can be upgraded when
the corresponding source/target bit allocations in
`halfrate_bit_allocations.csv` become populated.

## Multi-Subframe Rates

The AMBE-3000F manual does not mark any rate index as "multi-subframe"
explicitly. US6199037 (joint pitch/voicing quantization across
consecutive 20 ms subframes) implies the existence of 2-subframe rates
in the AMBE+2 family, but mapping US6199037 to specific chip rate
indices is a separate open question (see the "Next-Session Prompt 2"
in `docs/AMBE-3000_Next_Session_Prompts.md`). Pairs involving
prospective multi-subframe rates will need subframe-count time
alignment logic per Rate Converter spec §1.6 when that work lands.
Currently captured under the `pending` category alongside the
bit-allocation dependency.
