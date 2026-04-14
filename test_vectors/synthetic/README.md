# Synthetic PCM Test Vectors

## What this directory is for

Pair an input PCM waveform you authored (or can prove you have rights to)
with the MBE parameters an encoder should produce from it, or with the
PCM a decoder should produce from a known bitstream. This is the
**synthetic** equivalent of DVSI's test vectors — lower coverage, but
fully clear of third-party IP.

## Layout

```
synthetic/
├── README.md          (this file)
├── .gitignore         (excludes .pcm/.wav/.raw/... by default)
└── expected/          (committable: per-frame MBE parameters as JSON)
```

PCM files themselves are gitignored until you've decided the content is
publishable. Keep source waveforms here anyway — the `.gitignore` just
prevents them from being committed accidentally.

## Expected Output Format

For each `<name>.pcm` you drop here, create `expected/<name>.json` with:

```json
{
  "source": "silence_20ms.pcm",
  "rate": "full",                    // "full" (IMBE 7200 bps) or "half" (AMBE+2 3600 bps)
  "copyright": "Your Name / CC0",    // who authored the PCM, and under what terms
  "sample_rate_hz": 8000,
  "frame_count": 1,
  "frames": [
    {
      "frame_idx": 0,
      "omega_0_rad_per_sample": 0.0510,
      "L": 9,
      "K": 3,
      "v_l": [1, 1, 1, 1, 1, 1, 1, 1, 1],
      "M_l": [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0],
      "b_hat": {
        "b0_pitch": 0,
        "b1_vuv": "0x07",
        "b2_gain": 16,
        "b3_b7": [10, 9, 9, 9, 9],
        "b8_onward": []
      }
    }
  ]
}
```

All fields are optional — commit whatever stages your encoder can
independently produce. The important thing is that each JSON file
documents the **exact expected output** of the algorithm under test, so
any implementation can compare its output field-by-field without audio
similarity metrics (which are noisy).

## Suggested Starter Vectors

A handful of synthetic cases would catch a broad class of decode bugs
without requiring real speech recordings:

| Case | Input | Catches |
|------|-------|---------|
| Silence | All-zero PCM, 20 ms | Silence-frame handling, b̂₁ = 0 path |
| Sine 440 Hz | Single-tone, pitched | Pitch estimation + V/UV (all voiced low bands) |
| Sine 3500 Hz | High single-tone | Pitch estimation + V/UV (highest-band voicing) |
| White noise | Band-limited noise | V/UV (all unvoiced) + noise synthesis path |
| Two-tone | 440 Hz + 880 Hz | Harmonic separation, L estimation |
| Chirp 100→3500 Hz | Linear frequency sweep | Pitch tracking across frames |
| Tone frame bitstream | DTMF or silence tone ID | §2.6 tone-frame decode path |

PCM length per file: one 20 ms frame is enough for the per-frame stages.
Multi-frame files (say, 200 ms = 10 frames) test cross-frame state —
useful for the log-magnitude prediction (§1.8.5) and frame concealment
paths.

## What this does NOT validate

- Final PCM synthesis quality. Two correct decoders can produce audibly
  different PCM because the MBE synthesis window, voiced/unvoiced
  excitation mixing, and phase regeneration have implementation leeway.
  For synthesis validation, you need DVSI or a listening test.
- The 1993-vs-2014 IMBE algorithm difference. Your impl targets 2014
  (BABA-A); mbelib targets 1993. Expected values here should follow
  BABA-A.

## Contribution Rules

- Every PCM you put here must either be your own recording (CC0 dedication
  recommended) or a synthesized signal (sine waves are uncopyrightable).
- No voice recordings of other people without explicit written permission.
- No commercial speech databases (TIMIT, etc.) — those are licensed.
- Mark every `expected/*.json` with the `copyright` field so licensing
  status is self-documenting.
