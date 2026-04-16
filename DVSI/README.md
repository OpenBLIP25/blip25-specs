# DVSI AMBE-3000 Software Implementation Specs

## Scope

Implementation specifications for a software clone of the DVSI AMBE-3000
vocoder chip, covering decode, encode, and parametric rate conversion.

These specs are derived from:
- DVSI's publicly available USB-3000™ software and documentation
- DVSI's publicly available test vector files (tv-std, tv-rc)
- TIA-102.BABA-A vocoder frame format and MBE synthesis algorithms

## Relationship to TIA-102 Standards

The TIA-102.BABA-A standard specifies the P25 vocoder **wire format** (IMBE
frame structure, FEC, quantization tables, baseline MBE synthesis) but does
not specify the AMBE+2 codec algorithm. TIA intentionally decouples the wire
format from the codec — the standard defines what goes over the air, not how
to produce or consume it.

DVSI's AMBE-3000 chip is the de facto implementation used by all major P25
radio vendors (Motorola APX, L3Harris, Kenwood). A software implementation
that produces bit-identical output to the AMBE-3000 at each rate configuration
is functionally equivalent to the hardware.

## Legal Basis

Software implementations of the AMBE vocoder have existed and been publicly
distributed for many years (mbelib, JMBE, codec2 AMBE support, OP25). DVSI's
patents on MBE vocoders have expired. The USB-3000 software, documentation,
and test vectors are publicly downloadable from DVSI's website. These specs
describe the functional behavior of the chip as documented in DVSI's own
public materials and validated against their published test vectors.

## Source Material

All source material is publicly available from DVSI:
- USB-3000 Manual (protocol spec, rate tables, packet formats)
- USB-3000 host software (C source for client/server applications)
- Test vectors: tv-std (62 rate configs + P25 variants) and tv-rc (64 rate
  configs + D-STAR + P25 rate conversion)

## Deliverables

| Spec | Status | Description |
|------|--------|-------------|
| `AMBE-3000_Objectives.md` | done | Scope, methodology, patent landscape, implementation order |
| `AMBE-3000_Patent_Reference.md` | done | Algorithmic detail from 5 key expired DVSI patents |
| `AMBE-3000_Protocol_Spec.md` | done | USB-3000 packet protocol (wire format) |
| `AMBE-3000_Operational_Notes.md` | done | Reset timing, init sequencing, firmware errata, integration gotchas |
| `AMBE-3000_Test_Vector_Reference.md` | done | Test vector structure and validation methodology |
| `annex_tables/rate_index_table.csv` | done | 62-rate table (AMBE-1000/2000/+2 total/speech/FEC bps) |
| `annex_tables/rate_control_words.csv` | done | Table 120 RCWs + hardware config pins for 60+ built-in rates |
| `AMBE-3000_Decoder_Implementation_Spec.md` | draft | Channel bits → PCM synthesis (phase regen per US5701390, AMBE+2 FEC per US8595002) |
| `AMBE-3000_Encoder_Implementation_Spec.md` | draft | PCM → channel bits (forward VQ search, closed-loop predictor; analysis layer deferred to `analysis/vocoder_analysis_encoder_addendum.md`) |
| `AMBE-3000_Rate_Converter_Implementation_Spec.md` | draft | Parametric MBE rate conversion per US7634399 (no PCM round-trip) |

## Validation

A conformant software implementation must produce bit-identical output to the
AMBE-3000 chip for all test vectors at all rate configurations. The tv-std and
tv-rc vector sets provide comprehensive coverage:

- **tv-std**: 62 standard rate configurations (r0-r61) plus P25 with/without
  FEC and P25X variants
- **tv-rc**: 64 rate conversion configurations (r0-r63) plus D-STAR and P25
  rate conversion

Each test vector provides paired .pcm (PCM audio) and .bit (encoded channel
bits) files. Encode validation: pcm in → bit out must match. Decode
validation: bit in → pcm out must match.

## Trademarks

USB-3000, USB-3003, USB-3012, AMBE, AMBE+, and AMBE+2 are trademarks or
registered trademarks of Digital Voice Systems, Inc. (DVSI). IMBE is a
trademark of DVSI. This project is not affiliated with, endorsed by, or
sponsored by DVSI. All trademarked names are used here solely for
identification and reference to publicly available products and
documentation, under nominative fair use.
