# The Missing Vocoder Specs: AMBE+2, BABB, and BABG

**Category:** Vocoder / Standards Gap Analysis  
**Relevant Specs:** TIA-102.BABA-A, TIA-102.BABB, TIA-102.BABG, TIA-102.BABC  
**External Sources:** Motorola "P25 TDMA Trunking Suite of TIA-102 Standards" white paper (July 2013)  
**Date:** 2026-04-13

---

## The BABB Misattribution Problem

TIA-102.BABB is frequently misidentified in cross-references across the P25
spec suite. During extraction of this project's documents, Claude attributed
at least three different identities to BABB:

| Source Document | What It Called BABB |
|----------------|-------------------|
| BABA-A Related Resources | "Enhanced Multi-Band Excitation (AMBE+2) Vocoder" |
| BAAA-B Related Resources | "Half-rate Vocoder (AMBE)" |
| CCAA-C Related Resources | "IMBE Half-Rate Vocoder" |
| CAAA-F Related Resources | "TIA-102.BABB-B — P25 TDMA Physical Layer" (conflation with BBAB) |
| BABC Summary | "IS-102.BABB" — MOS evaluation companion (correct) |
| TSB-102-D Related Resources | "Vocoder MOS Conformance" (correct) |
| P25 Standards Baseline (April 2025) | "Vocoder Mean Opinion Score Conformance Test" (authoritative) |

**The authoritative answer:** TIA-102.BABB is the **"Project 25 Vocoder Mean
Opinion Score Conformance Test"**, published May 1999, ANSI-approved, and
re-affirmed August 2013. It defines a subjective audio quality test using MOS
scoring methodology. It is NOT a codec specification and does NOT define AMBE+2.

The BABC conformance test (Section 21 of our extraction) directly references
"IS-102.BABB" as the "alternative testing method using subjective Mean Opinion
Score (MOS) evaluation" — confirming this identification.

**Corrective actions taken:**
- Fixed BABA-A Related Resources: BABB correctly identified as MOS test
- Fixed BAAA-B Related Resources: BABB correctly identified, dependency tree updated
- Fixed CCAA-C Related Resources: BABB correctly identified
- Fixed CAAA-F Related Resources/Summary: BABB-B → BBAB (TDMA Physical Layer)
- Added BABB and BABG to specs.toml with correct titles and notes

---

## Motorola White Paper Corroboration (July 2013)

The Motorola "P25 TDMA Trunking Suite of TIA-102 Standards" white paper
(July 2013) provides independent corroboration from the dominant P25
infrastructure vendor.

### BABB Is Completely Absent

Motorola's white paper lists every TIA document needed for P25 TDMA
implementation, split into "Core Definition Documents" and "Testing Documents."
**TIA-102.BABB does not appear in either table.** The vocoder documents
Motorola references are:

| Document | Role per Motorola |
|----------|-------------------|
| TIA-102.BABA-1 | "Half-Rate Vocoder Annex — Lower bit-rate vocoder for the higher spectral efficiency of TDMA" |
| TIA-102.BABG | "Enhanced Vocoder Methods of Measurement for Perf — Defines the methods of measurement to test performance of the P25 enhanced vocoder" |

Motorola goes directly from BABA-1 (the codec) to BABG (testing the codec).
BABB is not referenced even as a historical document. This further supports
the conclusion that BABB-as-MOS-test is effectively dead and BABG replaced
it for the AMBE+2 era.

### TIA Website Confirms BABB/BBAB Confusion

When searching for TIA-102.BABB on TIA's standards store (verified April 2026),
the link for BABB redirects to the product page for **TIA-102.BBAB** (Phase 2
TDMA Physical Layer Protocol Specification). This is either:

1. A cataloging error (the BABB/BBAB transposition that pervades the ecosystem)
2. BABB was absorbed or withdrawn and the number effectively redirects
3. BABB never existed as a purchasable document and the baseline entry is a
   long-standing error

An inquiry has been sent to TIA TR-8 to resolve this. Regardless of the
outcome, BABB is not needed for implementation — BABG is the document that
matters.

### FDMA Remains Mandatory Even in TDMA Systems

The white paper explicitly states: *"The P25 TDMA standards enhance the
functionality of P25 standards and do not replace existing standards."*

Key points confirming that full-rate FDMA vocoder support is non-optional:

- **Control channel is always FDMA** — even TDMA systems use an FDMA control
  channel for call requests
- **OTAR and Location Services require FDMA** — these data services don't
  operate over TDMA voice channels
- **Dynamic Dual Mode** — Motorola's ASTRO 25 dynamically assigns FDMA or
  TDMA per call. If any participant is FDMA-only, the entire call uses FDMA.
  This means every APX radio exercises its full-rate vocoder path in
  production, regularly.

This reinforces that a vendor cannot ship a TDMA-only vocoder. The "dual rate
vocoder" — Motorola's term for what's in the APX subscriber portfolio — must
handle both full-rate (7200 bps) and half-rate (3600 bps) operation.

### BABG Categorized as a TDMA Testing Document

Motorola places BABG in the "P25 TDMA Testing Docs" category alongside
TIA-102.CCAA (transceiver measurement), CCAB (transceiver performance),
BCAD (MAC conformance), and BCAF (voice channel conformance profiles). Its
description: *"Defines the methods of measurement to test performance of
the P25 enhanced vocoder."*

The phrase **"enhanced vocoder"** — not "IMBE vocoder," not "half-rate
vocoder" — confirms BABG is about testing something beyond the original
IMBE baseline. In the TDMA context, this is the dual-rate AMBE+2 vocoder.
BABG likely defines the objective quality thresholds that the AMBE+2
codec was designed to meet.

### No Mention of AMBE+2 or IMBE by Name

Notably, the white paper never uses the terms "AMBE+2" or "IMBE." It uses
"dual rate vocoder" and "enhanced vocoder." This is consistent with TIA's
approach of specifying the frame format and performance requirements without
naming the codec algorithm — because the algorithm is DVSI proprietary IP
and the standard intentionally decouples the wire format from the codec.

---

## Where Is the AMBE+2 Vocoder Specification?

The short answer: **there is no TIA specification for the AMBE+2 codec algorithm.**

The P25 vocoder standards on the April 2025 approved list are:

| Document | Title | What It Covers |
|----------|-------|----------------|
| TIA-102.BABA-A | Vocoder Description | IMBE frame format, quantization tables, baseline MBE synthesis algorithm (both rates) |
| TIA-102.BABB | Vocoder MOS Conformance Test | Subjective quality testing via MOS scores |
| TIA-102.BABG | Enhanced Vocoder Methods of Measurement | Performance measurement for "enhanced" vocoders |

Plus the withdrawn/expired:

| Document | Title | Status |
|----------|-------|--------|
| TIA-102.BABC | Vocoder Reference Test | Expired. Objective conformance testing using DVSI A25VCTS hardware |
| TSB-102.BABD | Vocoder Selection Process | Historical |
| TSB-102.BABE | Full-Rate Vocoder Evaluation MOS Test (2007) | **Withdrawn** — superseded by BABG |
| TSB-102.BABF | Half-Rate Vocoder MOS Test Plan for Phase 2 (2008) | **Withdrawn** — superseded by BABG |

None of these documents specify the AMBE+2 speech analysis or synthesis
algorithms. The AMBE+2 improvements to the MBE model are entirely
**DVSI proprietary intellectual property**, distributed as:

- Silicon: AMBE-3000, AMBE-3003 DSP chips
- USB products: USB-3000, ThumbDV
- Licensed IP cores for FPGA/ASIC integration
- Software libraries under commercial license

DVSI's own product documentation confirms this architecture: the USB-3000
"provides the enhanced AMBE+2 full-rate vocoder at 7,200 bps plus the
half-rate 3,600 bps vocoder, with the full-rate mode fully compatible with
the older IMBE vocoder."

---

## Why This Matters: The 700 MHz and Fireground Problem

The P25 Steering Committee has been pushing Phase 2 (TDMA) adoption, and the
standards baseline reflects this emphasis. However, full-rate operation remains
mandatory in several critical scenarios:

### 700 MHz Interoperability Channels

The national interoperability channels in the 700 MHz band are designated for
P25 Phase 1 (FDMA) operation. All radios operating on these channels transmit
and receive full-rate (7200 bps) voice. Modern radios (Motorola APX, Harris
XL, Kenwood VP series) use AMBE+2 for both encode and decode on these channels.

### Simplex / Fireground Communications

Fireground tactical channels operate simplex — no infrastructure, no TDMA
controller. These are inherently Phase 1, full-rate. Audio quality on
fireground channels is a life-safety concern.

### Conventional Repeaters

Many agencies operate conventional (non-trunked) repeater systems that use
full-rate FDMA. These systems are not being migrated to Phase 2.

### Phase 1 ↔ Phase 2 Bridging

When a Phase 2 TDMA system bridges to Phase 1 FDMA (e.g., inter-system
patching, conventional fallback), parametric rate conversion produces full-rate
frames. The quality of the full-rate synthesis matters for the bridged audio.

### The Implication

A vendor starting fresh today — building a radio, a dispatch console, a
gateway, or a software decoder — needs full-rate AMBE+2-quality audio for
these scenarios. The TIA specs provide:

1. **The wire format** — fully specified, interoperable, non-negotiable
2. **The quantization tables** — fully specified in BABA-A annexes
3. **The baseline synthesis algorithm** — BABA-A Sections 8, 9, 15
4. **Quality targets** — defined in BABG ("Enhanced Vocoder Methods of
   Measurement"), but we don't have this PDF yet

What the TIA specs do NOT provide:

5. **How to achieve AMBE+2-quality synthesis** — this is the gap

---

## TIA-102.BABG: Obtained and Processed

**Status: RESOLVED.** TIA-102.BABG has been obtained, extracted, and fully
processed. The full extraction is at `TIA-102.BABG/TIA-102-BABG_Summary.txt`.

TIA-102.BABG, "Enhanced Vocoder Methods of Measurement for Performance,"
was published March 2010 and approved by the P25 Steering Committee the same
month. The timing is significant: 2010 is when AMBE+2-equipped radios
(Motorola APX series) were entering production.

### What BABG Actually Contains

Our earlier speculation was largely confirmed. BABG defines:

- **PESQ-based objective quality testing** per ITU-T P.862, with raw scores
  converted to MOS-LQO via the P.862.1 mapping function
- **Independent encoder and decoder testing** — the candidate encoder is
  paired with a standard reference decoder, and vice versa. This verifies
  each component individually.
- **Dual-rate coverage** — all performance limits apply identically to both
  full-rate (7200 bps) and half-rate (3600 bps) operation
- **15 real-world noise environments** including fireground noises (PASS
  alarm, SCBA low air, fog nozzle, rotary saw, chainsaw) sourced from
  NTIA TR 08-453
- **Minimum LQO ≥ 2.0** across all noise types (with one exception: fog
  nozzle requires only LQO ≥ 1.8)
- **Standard reference encoder/decoder** — PC-executable software that
  replicates the MOS test performance (2003–2008 reference vocoder)

### What BABG Explicitly Excludes

Section 1.2 states that the baseline vocoder defined in prior versions of
TIA-102.BABA (the original Phase 1 IMBE) is **outside the scope** of these
tests — in general, the baseline vocoder cannot pass them. This confirms
that BABG is specifically an AMBE+2-class quality standard.

### Why This Matters for Implementation

BABG confirms the implementation strategy outlined in the Path Forward
section below. A conformant implementation must:

1. Implement frame unpacking per BABA-A (exact, non-negotiable)
2. Implement any MBE synthesis approach
3. **Validate against BABG's PESQ/MOS-LQO thresholds** (LQO ≥ 2.0 across
   15 noise conditions, using ITU-T P.862 PESQ)
4. Iteratively improve synthesis until thresholds are met

The standard decouples "what quality to achieve" from "how to achieve it."
DVSI's AMBE+2 is one way to hit those targets. An improved open-source MBE
synthesis engine is another. The standard doesn't care which — it only
cares about the measured output quality.

### Earlier Speculation (Confirmed)

Multiple independent sources pointed to BABG as the "enhanced vocoder"
performance specification before we obtained the document:

1. **TIA document title** — says "Enhanced Vocoder," not "IMBE Vocoder"
2. **P25 Standards Baseline (April 2025)** — lists BABG as an active,
   approved P25 standard
3. **Motorola white paper (July 2013)** — categorizes BABG as a TDMA testing
   document, describes it as defining "methods of measurement to test
   performance of the P25 enhanced vocoder"
4. **Temporal correlation** — published March 2010, same year as TDMA
   deployment; listed alongside CCAA/CCAB/BCAD/BCAF testing docs
5. **BABB absence** — Motorola doesn't reference BABB at all, going directly
   from BABA-1 (codec) to BABG (testing). BABG appears to have functionally
   replaced the 1999-era BABB/BABC test regime for the enhanced vocoder.

---

## Path Forward for Clean-Room Implementation

```
What we have:
  ┌─────────────────────────────────────────────┐
  │ Frame format          ✓ Complete (BABA-A)    │
  │ FEC codes             ✓ Complete (BABA-A)    │
  │ Quantization tables   ✓ Complete (BABA-A)    │
  │ Baseline synthesis    ✓ Specified (BABA-A)   │
  │ Enhanced perf metrics ✓ Complete (BABG)      │ ← obtained
  │ PESQ/MOS-LQO targets ✓ Complete (BABG)      │ ← obtained
  │ PESQ test tooling     ✓ Available (ITU P.862)│
  └─────────────────────────────────────────────┘

What is dead/irrelevant:
  ┌─────────────────────────────────────────────┐
  │ MOS subjective test   BABB (dead/absorbed)   │
  └─────────────────────────────────────────────┘

What doesn't exist in any TIA spec:
  ┌─────────────────────────────────────────────┐
  │ AMBE+2 synthesis      ✗ DVSI proprietary     │
  │ algorithm               (foundational patents│
  │                         expired; one sibling │
  │                         half-rate patent     │
  │                         US8359197 active     │
  │                         until 2028-05-20)    │
  │                         see DVSI/AMBE-3000/  │
  └─────────────────────────────────────────────┘

Implementation strategy:
  1. Implement frame unpacking exactly per BABA-A
  2. Implement baseline IMBE synthesis per BABA-A (functional)
  3. Validate against BABG PESQ/MOS-LQO thresholds (LQO ≥ 2.0)
  4. Iteratively improve synthesis using expired patent techniques:
     - Phase regeneration from spectral envelope (US5701390)
     - Per-harmonic voicing decisions (US8595002)
     - Joint voicing/pitch quantization (US6199037)
     - Enhanced spectral amplitude processing
     - Improved V/UV transition smoothing
  5. Black-box validate against DVSI AMBE-3000 test vectors
  6. Test both full-rate and half-rate paths (BABG confirms both
     are covered with identical performance limits)
```

### Patent Status Caveat (added 2026-04-27)

The summary above is technically correct but legally incomplete. The
foundational AMBE+2 disclosure is documented in five DVSI patents that
were all expired prior to April 2026 (US5701390, US6199037, US7634399,
US8315860, US8595002 — see `DVSI/AMBE-3000/AMBE-3000_Patent_Reference.md`).
However, **US8359197 — a sibling of US8595002 with the same 2003-04-01
priority date — remains in force until 2028-05-20** because it received
substantial patent term adjustment for USPTO prosecution delays.

US8359197 actually contains **two independent patentable inventions**:

1. **Mixed pitch+voicing+gain FEC-protected first codeword** (claims 1,
   42, 60 and their dependents). Narrowed during prosecution to require
   the first codeword to contain "less than all of the quantizer bits
   for the frame" — distinguishing Hardwick '089 (US 6,161,089) which
   FEC-protected pitch + gain MSBs but excluded voicing bits from the
   protected codeword.
2. **Bit-count-dependent spectral codebook indexing** (claims 72–87).
   Allowed as filed — never amended during prosecution.

US8359197's claims read directly onto an AMBE+2 half-rate
encoder/decoder that uses the BABA-A wire format:
- 4-pitch + 4-voicing + 4-gain MSB grouping into a 12-bit first codeword
  (claims 7–8) — first codeword is a strict subset of ~49 voice bits, so
  meets the "less than all quantizer bits" limitation
- [24,12] extended Golay protecting that codeword (claim 9)
- Data-dependent scrambling keyed from the first codeword (claim 15)
- Redundant-index voicing codebook (claim 6)
- Tone-frame identifier in disallowed pitch-bit values (claims 16–19)
- Variable spectral codebook selection from 25–32 bit allocation
  (claim 72) — AMBE+2's variable spectral bit count drives codebook
  index choice

**Full-rate (7200 bps) IMBE implementations are likely outside claims
1, 42, 60** — these claims chain to dependent claims (7–14) specifying
bit allocations particular to the half-rate format. Full-rate
infringement on claim 1 alone depends on whether IMBE's first FEC-
protected codeword groups pitch + voicing + gain bits and is a strict
subset of total quantizer bits. **However, claim 72 (variable spectral
codebook indexing) may apply to full-rate IMBE if its spectral codebook
selection depends on a per-frame bit count** — a separate question for
the BABA-A full-rate FEC structure.

For implementation planning:
- Half-rate (3600 bps) AMBE+2-compatible decoders/encoders shipped
  before 2028-05-20 carry patent risk under US8359197
- Full-rate IMBE decoders/encoders are not encumbered (Phase 1 channels)
- Receive-only software for research / personal use has lower practical
  enforcement risk than vendor product, but is still inside the claim
  scope
- Additional active DVSI patents exist (US11715477, US11990144,
  US12254895, US12451151, US12462814) — these are 2022+ filings likely
  covering the next-generation AMBE-4020 chip rather than AMBE+2; they
  are noted here for completeness and not detailed in the AMBE-3000
  reference

See `DVSI/AMBE-3000/AMBE-3000_Patent_Reference.md` §6 for full claim
analysis of US8359197.

The Motorola white paper's use of "dual rate vocoder" and "enhanced vocoder"
— never "IMBE" or "AMBE+2" — mirrors TIA's approach: the standard specifies
the wire format and the quality bar, not the algorithm. A conformant
implementation hits the BABG performance targets using whatever synthesis
approach it chooses. The MBE model parameters on the wire are the same
regardless of which synthesis engine processes them.

The DVSI AMBE-3000 software implementation specs (see `DVSI/AMBE-3000/`)
document the expired patent algorithms, black-box validation methodology,
and test vector reference for this effort.
