# Test Vectors for P25 Vocoder Implementations

This directory holds **test fixtures** that implementers can run against
their code to catch spec-correctness regressions. They are **not**
implementations themselves.

## Categories

Three kinds of test vectors, with different legal and practical properties:

### 1. `algorithmic/` — Pure-math reference pairs (committed, public)

Golay/Hamming/BCH codeword round-trips, GF(64) exponential/logarithm pairs,
bit-prioritization expansion samples, and other deterministic functions of
the TIA spec. Every entry is derivable from the BABA-A PDF alone, so there
is no licensing or trademark issue. Small (hundreds of KB).

Regenerate with `tools/gen_algorithmic_vectors.py`. If the generator output
disagrees with the committed files, either the generator has a bug or the
underlying spec CSV (e.g. `annex_tables/annex_e_gain_quantizer.csv`) has
changed — investigate before overwriting.

### 2. `synthetic/` — Our-own-authorship PCM + expected MBE parameters

PCM audio you record yourself (or synthesize from known test signals like
sine waves, silence, tones) paired with the MBE parameters your encoder
should produce. `.pcm` and `.wav` files are `.gitignored` by default to
avoid accidental commits before you've decided the content is publishable;
the `expected/*.json` files with per-frame MBE parameters can be committed
once their inputs are cleared for publication.

The point of this category is to validate the **algorithmic part** of the
encode/decode pipeline (bitstream ↔ MBE parameters) without depending on
proprietary test vectors. It does *not* validate final PCM synthesis
quality — for that you need DVSI or a listening test.

### 3. `dvsi_manifest/` — SHA-256 hashes of DVSI test vectors (committed, no DVSI content)

DVSI distributes `tv-std` and `tv-rc` test vector sets (a few GB each,
publicly downloadable). They are DVSI's copyright and cannot be
redistributed here. Instead, this directory holds `.sha256` manifests
that let anyone verify their local DVSI copy is bit-identical to the one
we validated against.

Run `tools/dvsi_manifest.py --verify <path>` to check a local copy
against a committed manifest. Run `tools/dvsi_manifest.py --generate
<path> <out>` to create a new manifest from a local DVSI mirror.

## What Goes Where

| Scenario | Location |
|----------|----------|
| "Golay decode of 0x000001 should be..." | `algorithmic/golay_23_12_roundtrip.csv` |
| "This 20 ms of my voice should decode to these MBE parameters" | `synthetic/expected/<sample>.json` |
| "The tv-std file `r0_u.pcm` is SHA-256 abc123..." | `dvsi_manifest/tv-std.sha256` |
| "SDRTrunk's BCH decoder uses a different error limit than ours" | `analysis/oss_implementations_lessons_learned.md` |

## Legal Framework

- `algorithmic/` content is derived from TIA-102 standards, which are
  copyright-protected but whose algorithmic content (matrices, polynomials,
  formulas) is not copyrightable. Our expressions are original.
- `synthetic/` PCM is your authorship — record, synthesize, or commission
  original audio. Do NOT commit content whose copyright you don't hold.
- `dvsi_manifest/` contains only hashes. Hashes of DVSI content are not
  themselves DVSI content.

If in doubt about whether a specific test vector is safe to commit,
err on the side of caution — keep it in the `synthetic/` tree with a
note in `synthetic/README.md` until the question is resolved.
