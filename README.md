# BLIP25 — TIA-P25 Standards Processing

The P25 standards suite is 129 documents of dense, cross-referencing
specifications that were never designed for easy consumption. This project
makes them accessible — extracting structured summaries, implementation-ready
specs, and original analysis that corrects long-standing misconceptions in the
P25 ecosystem.

**42 of 129 documents processed. 20 implementation specs. 2 analysis pieces.**

## What's Here

- **Implementation specs** — Self-contained, code-ready specifications for
  the FDMA air interface, TDMA MAC layer, trunking, vocoder, encryption,
  and more. Built to drive open source P25 development without needing to
  read the original PDFs.
- **Summaries** — ~1000-word technical summaries for each processed standard
- **Cross-reference index** — [`specs.toml`](specs.toml) tracks all 129
  documents with status, dependencies, and file paths
- **Analysis** — Original research that synthesizes insights across the full
  standards suite

## Analysis

- **[IMBE Frame Format vs MBE Vocoder](analysis/vocoder_wire_vs_codec.md)** —
  The widespread belief that P25 decoders must use the 1993 IMBE algorithm is
  wrong. The IMBE frame is a serialization format for MBE model parameters, not
  a vocoder. Any MBE synthesis engine can reconstruct speech from the same
  over-the-air bits. DVSI's own USB-3000 test vectors prove this: three codec
  variants (P25, P25A, P25X) use identical wire formats with different synthesis
  algorithms. TIA-102.BABG confirms the baseline IMBE vocoder cannot pass the
  enhanced vocoder quality tests. This decoupling is the single highest-impact
  improvement available to open-source P25 audio quality.

- **[The Missing Vocoder Specs](analysis/vocoder_missing_specs.md)** —
  Investigation into the P25 vocoder standards gap: where the AMBE+2 algorithm
  is (not in any TIA spec), what TIA-102.BABB actually is (MOS conformance test,
  not a codec spec), and why TIA-102.BABG is the key document for implementation
  (PESQ/MOS-LQO quality targets for enhanced vocoders).

## DVSI AMBE-3000 Software Implementation

The [`DVSI/`](DVSI/) directory contains specs for a software-equivalent
AMBE-3000 vocoder — decoder, encoder, and parametric rate converter — built
from TIA standards, expired DVSI patents, and black-box validation against
DVSI's published test vectors. The primary motivation is enabling P25/DMR/D-STAR
cross-standard bridging via parametric rate conversion instead of the lossy
tandem decode-reencode approach used by current amateur radio gateways.

See [`DVSI/AMBE-3000/AMBE-3000_Objectives.md`](DVSI/AMBE-3000/AMBE-3000_Objectives.md)
for the full methodology and patent landscape.

## Repository Layout

```
blip25-specs/
  specs.toml                    # Master index — all 129 documents, status, dependencies
  analysis/                     # Cross-document analysis and original research
  DVSI/                         # AMBE-3000 vocoder implementation specs
  standards/                    # All TIA-102, TSB, ITU-T, NTIA, J-STD documents
    TIA-102.XXXX[-rev]/         # Per-document directories (one per TIA spec)
      *_Summary.txt             # Technical summary (committed)
      *_Related_Resources.md    # Web resources and cross-references (committed)
      *_Implementation_Spec.md  # Code-ready derived spec (committed)
      *.pdf                     # Original PDF (gitignored — copyrighted)
      *_Full_Text.md            # Full text extraction (gitignored — near-verbatim)
```

## How It's Built

All specs are extracted programmatically from TIA standard PDFs using an
LLM-driven processing pipeline. Dense standards documents go in; structured,
machine-readable specs come out.

See [`docs/TIA_P25_Processing_Prompt_v2.md`](docs/TIA_P25_Processing_Prompt_v2.md)
for the extraction prompt and [`docs/specs_README.md`](docs/specs_README.md) for
the full workflow.

## Getting the Original PDFs

TIA hosts these documents publicly. The download script fetches all 129
from the TIA Connect library:

```bash
python3 tools/_download_tia.py
```

See [`docs/TIA_P25_Library_Inventory.md`](docs/TIA_P25_Library_Inventory.md)
for the full catalog.

## License

MIT — use it for anything. See [LICENSE](LICENSE).

---

*Named for the blips on the waterfall — the TDMA bursts that carry every voice on P25.*
