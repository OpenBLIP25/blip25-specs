# AMBE-3000 Test Vector Reference

**Source:** DVSI publicly available test vector archives (tv-std, tv-rc)  
**Date:** 2026-04-13  
**Purpose:** Document the structure, contents, and usage of DVSI's test
vectors for validating a software AMBE-3000 implementation against the
hardware reference.

**Location:** `~/p25-decoder/iq-samples/P25-IQ-Samples/DVSI Vectors/`

---

## 1. Overview

DVSI distributes two test vector sets with the USB-3000™:

| Set | Directory | Purpose | Rate Configs | Test Lines |
|-----|-----------|---------|--------------|------------|
| tv-std | `tv-std/tv/` | Standard encode/decode | 62 (r0–r61) + P25/P25X | 21,200 |
| tv-rc | `tv-rc/` | Rate conversion | 64 (r0–r63) + D-STAR + P25 | 20,391 |

Each set contains:
- **Input files** — raw PCM audio and/or encoded bit streams at the top level
- **Output directories** — reference output for each rate configuration
- **Test command files** — `.txt` files defining the exact test commands

A conformant implementation must produce bit-identical output to the
reference files for all test cases.

---

## 2. Input Test Files

### 2.1 File Inventory (tv-std)

96 PCM input files covering diverse audio conditions. All files are
16-bit signed little-endian PCM at 8 kHz sample rate.

#### Speech Signals

| File(s) | Description | Count |
|---------|-------------|-------|
| `clean.pcm` | Clean reference speech, no noise | 1 |
| `noisy.pcm` | Speech with background noise | 1 |
| `knox_1..16.pcm` | Knox voice corpus — standardized reference phrases | 16 |
| `dam.pcm` | Dynamic Audio Message — speech with varying dynamics | 1 |
| `dam10, dam20, dam40, dam80` | DAM at different SNR/dynamic levels | 4 |
| `dam_10ov.pcm` | DAM with 10 dB overload/clipping | 1 |
| `damcar.pcm` | DAM with car background noise | 1 |
| `fambf22c, fambm22a` | Female/male voice samples | 2 |
| `tambf22a, tambf22b, tambf32b, tambf32e` | Transformed voice samples | 4 |
| `ucarf15a, ucarm15a, ucarm20a` | Additional voice samples | 3 |
| `t01, t02, t03, t04` | Generic numbered test sequences | 4 |

#### Tone Signals

| File(s) | Description | Count |
|---------|-------------|-------|
| `dtmf.pcm` | Standard DTMF sequence | 1 |
| `dtmf4, dtmf8, dtmf50, dtmf60, dtmf100, dtmf180` | DTMF at various counts/durations | 6 |
| `dtmf15n, dtmf15p` | 15 DTMF tones with different characteristics | 2 |
| `dtmf4r, dtmf8r` | Repeated DTMF sequences | 2 |
| `dtmf25.pcm` | 25 DTMF tones | 1 |
| `dtmfvbad.pcm` | DTMF with error/voice-bad flags | 1 |
| `dtone_1..16` | Dual-tone test signals | 16 |
| `sine0_1k, sine0_4k` | Pure sine waves at 1 kHz and 4 kHz | 2 |
| `tia11.pcm` | TIA reference signal | 1 |
| `irstia.pcm` | IRS-filtered TIA reference signal | 1 |

#### Special / Edge Case Signals

| File(s) | Description | Count |
|---------|-------------|-------|
| `alert.pcm` | Alert tone | 1 |
| `cp0, cp1, cp2, cp31` | Control packet / minimal frames | 4 |
| `cpvbad.pcm` | Control packets with voice-bad flags | 1 |
| `grbge.pcm` | Garbage / corrupted input | 1 |
| `zero.pcm` | Silence (all zeros) | 1 |
| `xfer.pcm` | Transfer / transition signal | 1 |
| `mark.pcm` | Marker / reference tone | 1 |
| `lvl.pcm` | Level / amplitude variation | 1 |
| `p01mirs.pcm` | P25 protocol test with MIRS signal | 1 |
| `ptrain0..12` | Pseudo-random pattern trains | 13 |

### 2.2 Additional Files in tv-rc

The tv-rc set includes the same base PCM inputs plus additional files
with companded variants:

| Format | Extension | Description |
|--------|-----------|-------------|
| µ-law | `.pcmu` | G.711 µ-law compressed (1 byte/sample) |
| A-law | `.pcma` | G.711 A-law compressed (1 byte/sample) |

The tv-rc D-STAR directory (`dstar/`) contains pre-encoded `.bit` files
paired with reference `.pcm` decode outputs for D-STAR codec testing.

---

## 3. Output Directory Structure

### 3.1 tv-std Output

```
tv-std/tv/
├── r0/ through r61/     ← 62 standard rate configs (encode-decode output)
│   ├── alert.pcm         (decoded PCM for each input)
│   ├── clean.pcm
│   └── ...
├── p25/                  ← P25 full-rate with FEC
│   ├── alert.pcm         (encode-decode PCM output)
│   ├── alert.bit         (encode-only bit output)
│   └── ...
├── p25_nofec/            ← P25 full-rate without FEC
│   ├── alert.pcm
│   ├── alert.bit
│   └── ...
├── p25x/                 ← P25 extended with FEC
│   ├── alert.pcm
│   ├── alert.bit
│   └── ...
└── p25x_nofec/           ← P25 extended without FEC
    ├── alert.pcm
    ├── alert.bit
    └── ...
```

### 3.2 tv-rc Output

```
tv-rc/
├── r0/ through r63/     ← 64 rate configs (encode-decode output)
├── p25/                  ← P25 with FEC
│   ├── *.pcm, *.bit      (encode-decode and encode-only outputs)
│   ├── r0/ through r63/  (rate-converted from P25 to each rate)
│   ├── dstar/            (rate-converted from P25 to D-STAR)
│   └── p25_nofec/        (rate-converted P25 FEC → P25 no-FEC)
├── p25_nofec/            ← P25 without FEC
│   ├── *.pcm, *.bit
│   ├── r0/ through r63/
│   ├── dstar/
│   └── p25/
└── dstar/                ← D-STAR codec
    ├── *.pcm, *.bit
    ├── r0/ through r63/  (rate-converted from D-STAR to each rate)
    ├── p25/              (rate-converted D-STAR → P25 FEC)
    └── p25_nofec/        (rate-converted D-STAR → P25 no-FEC)
```

---

## 4. Test Command Files

### 4.1 Command Format

Each line in a `.txt` file is a complete test command:

```
-c <CODEC> -q <OPERATION> -r <RATE> <input_file> -cmp <reference_file>
```

| Flag | Description |
|------|-------------|
| `-c CODEC` | Codec mode: `STD`, `P25`, `P25A`, `P25X`, `RC` |
| `-q` | Quiet mode (suppress output except errors) |
| `-enc` | Encode only: PCM in → bit out |
| `-dec` | Decode only: bit in → PCM out |
| `-encdec` | Round-trip: PCM in → encode → decode → PCM out |
| `-rc` | Rate conversion: bit in → rate convert → bit out |
| `-r N` | Rate table index (for STD/RC modes) |
| `-r RCW0..RCW5` | Rate parameters as 6 hex words (for P25 modes) |
| `-rd <rate>` | Decoder (input) rate for rate conversion |
| `-re <rate>` | Encoder (output) rate for rate conversion |
| `-cmp <file>` | Compare output against reference file |

### 4.2 Test File Summary

| File | Codec | Tests | Lines | Description |
|------|-------|-------|-------|-------------|
| `cmpstd.txt` | STD | encode-decode | 19,472 | All inputs × 62 rates (r0–r61) |
| `cmpp25.txt` | P25 | enc, dec, encdec | 576 | P25 full-rate, FEC and no-FEC |
| `cmpp25a.txt` | P25A | enc, dec, encdec | 576 | P25 enhanced variant |
| `cmpp25x.txt` | P25X | enc, dec, encdec | 576 | P25 extended variant |
| `cmprc.txt` | RC | encdec, rc | 20,391 | Rate conversion: all rates + cross-standard |

### 4.3 P25 Test Structure (cmpp25.txt)

For each input file, 6 test commands are generated:

```
1. -c P25 -encdec -r <FEC_RATE>    input.pcm -cmp p25/input.pcm        # round-trip with FEC
2. -c P25 -encdec -r <NOFEC_RATE>  input.pcm -cmp p25_nofec/input.pcm  # round-trip no FEC
3. -c P25 -enc    -r <FEC_RATE>    input.pcm -cmp p25/input.bit         # encode with FEC
4. -c P25 -enc    -r <NOFEC_RATE>  input.pcm -cmp p25_nofec/input.bit   # encode no FEC
5. -c P25 -dec    -r <FEC_RATE>    p25/input.bit -cmp p25/input.pcm     # decode with FEC
6. -c P25 -dec    -r <NOFEC_RATE>  p25_nofec/input.bit -cmp p25_nofec/input.pcm  # decode no FEC
```

Where:
- FEC rate: `0x0558 0x086b 0x1030 0x0000 0x0000 0x0190`
- No-FEC rate: `0x0558 0x086b 0x0000 0x0000 0x0000 0x0158`

### 4.4 Rate Conversion Test Structure (cmprc.txt)

The rate conversion tests cover three categories:

**1. Standard encode-decode** (same as cmpstd but for RC codec, 64 rates):
```
-c RC -encdec -r <N> input.pcm -cmp ./rN/input.pcm
```

**2. Cross-standard rate conversion:**
```
# D-STAR → various rates
-c RC -rc -rd <DSTAR_RATE> -re <TARGET_RATE> ./dstar/input.bit -cmp ./dstar/rN/input.bit

# P25 → various rates
-c RC -rc -rd <P25_FEC_RATE> -re <TARGET_RATE> ./p25/input.bit -cmp ./p25/rN/input.bit

# P25 no-FEC → various rates
-c RC -rc -rd <P25_NOFEC_RATE> -re <TARGET_RATE> ./p25_nofec/input.bit -cmp ./p25_nofec/rN/input.bit
```

**3. Cross-standard bidirectional:**
```
# D-STAR ↔ P25
-c RC -rc -rd <DSTAR_RATE> -re <P25_RATE> ./dstar/input.bit -cmp ./dstar/p25/input.bit
-c RC -rc -rd <P25_RATE> -re <DSTAR_RATE> ./p25/input.bit -cmp ./p25/dstar/input.bit
```

---

## 5. File Formats

### 5.1 PCM Files (.pcm)

| Property | Value |
|----------|-------|
| Encoding | 16-bit signed integer |
| Byte order | Little-endian |
| Sample rate | 8,000 Hz |
| Samples per frame | 160 |
| Frame duration | 20 ms |
| Bytes per frame | 320 |
| Header | None (raw data) |

**Note:** Byte order in files is little-endian, but the USB-3000 packet
protocol uses big-endian for PCM samples over the wire. The test vector
files represent the on-disk format, not the wire format.

### 5.2 Bit Files (.bit)

| Property | Value |
|----------|-------|
| Encoding | One byte per bit (0x00 or 0x01) |
| Bits per frame | Depends on rate (e.g., 144 for P25 full-rate FEC) |
| Bytes per frame | Equal to bits per frame |
| Header | None (raw data) |

**Important:** This is not packed binary — each codec bit occupies a full
byte. A 144-bit P25 frame is stored as 144 bytes, not 18 bytes.

### 5.3 Companded Files (.pcmu, .pcma)

| Property | Value |
|----------|-------|
| Encoding | G.711 µ-law (.pcmu) or A-law (.pcma) |
| Bytes per sample | 1 |
| Sample rate | 8,000 Hz |
| Bytes per frame | 160 |

---

## 6. Validation Methodology

### 6.1 Bit-Exact Comparison

The `-cmp` flag in the test commands performs byte-for-byte comparison
between the implementation output and the reference file. For a
conformant implementation:

- **Encoder tests** (`-enc`): Output `.bit` file must be byte-identical
  to reference `.bit` file
- **Decoder tests** (`-dec`): Output `.pcm` file must be byte-identical
  to reference `.pcm` file
- **Round-trip tests** (`-encdec`): Output `.pcm` must be byte-identical
  to reference `.pcm`
- **Rate conversion tests** (`-rc`): Output `.bit` must be byte-identical
  to reference `.bit`

### 6.2 Progressive Validation Strategy

For a software implementation, validate in this order:

**Phase 1 — Decoder (highest priority):**
```
# For each input .bit file in p25/:
# Decode and compare against reference .pcm
-c P25 -dec -r 0x0558 0x086b 0x1030 0x0000 0x0000 0x0190 p25/<file>.bit -cmp p25/<file>.pcm
```
Start with `clean.bit` (clean speech), then `dam.bit` (dynamic speech),
then `dtmf.bit` (tones), then edge cases (`zero`, `grbge`, `cpvbad`).

**Phase 2 — Encoder:**
```
# For each input .pcm file:
# Encode and compare against reference .bit
-c P25 -enc -r 0x0558 0x086b 0x1030 0x0000 0x0000 0x0190 <file>.pcm -cmp p25/<file>.bit
```

**Phase 3 — Rate conversion:**
```
# P25 → D-STAR
-c RC -rc -rd 0x0558 0x086b 0x1030 0x0000 0x0000 0x0190 \
         -re 0x0130 0x0763 0x4000 0x0000 0x0000 0x0048 \
         ./p25/<file>.bit -cmp ./p25/dstar/<file>.bit

# D-STAR → P25
-c RC -rc -rd 0x0130 0x0763 0x4000 0x0000 0x0000 0x0048 \
         -re 0x0558 0x086b 0x1030 0x0000 0x0000 0x0190 \
         ./dstar/<file>.bit -cmp ./dstar/p25/<file>.bit
```

**Phase 4 — Full regression:**
Run all 41,591 test lines from all five `.txt` files.

### 6.3 When Bit-Exact Match Is Not Achievable

For an independent software implementation, bit-exact match with the
AMBE-3000 hardware may not be achievable for the encoder (analysis is
more sensitive to floating-point differences than synthesis). In that
case, validate using:

1. **Round-trip quality:** Software-encode → hardware-decode and
   hardware-encode → software-decode must produce acceptable audio
2. **Parameter-level comparison:** Extract MBE parameters from both
   encoder outputs and compare pitch, voicing, and spectral amplitudes
3. **PESQ scoring:** Use ITU-T P.862 PESQ to measure objective quality
   difference between software and hardware encode-decode chains
4. **BABG thresholds:** Both paths must meet TIA-102.BABG LQO ≥ 2.0
   across all 15 noise conditions

### 6.4 P25 Codec Variants (P25 vs P25A vs P25X)

The test vectors include three P25 codec variants that use identical
rate control words but may produce different output:

| Variant | Test File | Likely Interpretation |
|---------|-----------|----------------------|
| P25 | cmpp25.txt | Baseline P25 IMBE codec |
| P25A | cmpp25a.txt | Enhanced P25 codec (AMBE+2 improvements) |
| P25X | cmpp25x.txt | Extended P25 codec (further enhancements) |

All three share the same `p25/` and `p25_nofec/` output directories in
tv-std, suggesting the reference outputs are identical for these inputs.
The codec variant affects the internal algorithm path, not the wire
format. Validation against any one variant that matches the reference
output confirms conformance.

---

## 7. Quick Reference: P25 Test Commands

### Decode P25 full-rate with FEC
```
usb3k_client -c P25 -dec -r 0x0558 0x086b 0x1030 0x0000 0x0000 0x0190 p25/clean.bit
```

### Encode P25 full-rate with FEC
```
usb3k_client -c P25 -enc -r 0x0558 0x086b 0x1030 0x0000 0x0000 0x0190 clean.pcm
```

### Round-trip P25 full-rate
```
usb3k_client -c P25 -encdec -r 0x0558 0x086b 0x1030 0x0000 0x0000 0x0190 clean.pcm -cmp p25/clean.pcm
```

### Rate convert P25 → D-STAR
```
usb3k_client -c RC -rc \
  -rd 0x0558 0x086b 0x1030 0x0000 0x0000 0x0190 \
  -re 0x0130 0x0763 0x4000 0x0000 0x0000 0x0048 \
  p25/clean.bit -cmp p25/dstar/clean.bit
```

### Rate convert D-STAR → P25
```
usb3k_client -c RC -rc \
  -rd 0x0130 0x0763 0x4000 0x0000 0x0000 0x0048 \
  -re 0x0558 0x086b 0x1030 0x0000 0x0000 0x0190 \
  dstar/clean.bit -cmp dstar/p25/clean.bit
```
