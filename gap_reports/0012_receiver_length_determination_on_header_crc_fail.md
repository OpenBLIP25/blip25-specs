# 0012 — Receiver-side PDU length determination when Header CRC-16 fails

**Status:** drafted (2026-04-21) — resolution in `analysis/fdma_pdu_frame.md` §6.1; spec is silent for passive receivers (BAED-A §3.1 says active receivers silently discard on header CRC fail), new §6.1 documents strategies A/B/C with tradeoffs and recommends Strategy B (best-effort BTF with Format-field sanity gate) as the passive-receiver default.
**Filed:** 2026-04-21
**Filer:** implementer (p25-decoder / blip25-data)
**For:** spec-author
**Related:**
- `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md` §5.2 (Header CRC), §5.7 (BTF field)
- `analysis/fdma_pdu_frame.md` §6 (dispatcher pseudocode)

---

## 1. What the spec says

BAAA-B specifies the transmitter's responsibility to set `blocks_to_follow`
(BTF, octet 6 low 7 bits of the header) correctly so the receiver knows how
many 98-dibit trellis blocks follow the header block. §5.2 defines the
Header CRC-CCITT-16 covering octets 0–9 so the receiver can verify the
header (including BTF) before trusting it.

The spec is silent on what a receiver should do when the **Header CRC-16
fails**. An active receiver presumably drops the frame and waits for
retransmission. A passive receiver / decoder has a choice:

- **Option A — Drop the frame.** Skip forward looking for the next FS+NID.
  But where is "forward"? We don't know how many blocks to skip because
  that's exactly the field whose integrity we can't trust.

- **Option B — Best-effort BTF.** Read the (CRC-failed) BTF value, cap it at
  some sanity limit, try to decode that many data blocks anyway. The data
  blocks have their own CRCs so the garbage-in test is a second check.

- **Option C — Hunt for next sync.** Search the raw dibit stream for the
  next P25 FS pattern and resume from there, even if that means dropping
  some or all of this PDU's data blocks. Less information-efficient than B
  when only a single bit flipped in BTF, but safer against pathological
  corruption.

## 2. The ambiguity

My implementation currently uses a hybrid of A/B
(`crates/blip25-core/src/pdu.rs::early_decode_pdu_blocks_to_follow`):

- Read BTF from octet 6 regardless of CRC state.
- Validate Format field is one of the three known values
  (%10110 / %10101 / %00011) — rejects frames whose octet 0 looks wrong.
- Cap BTF at 20 (sanity limit).
- Comment at pdu.rs:115 explicitly flags this as implementer's call:
  *"docs/PDU_SPEC_GAPS.md §3 — spec is silent on receiver-side PDU length
  determination, so implementer's call."*

The tradeoff: if only one bit flipped in BTF I over-read or under-read by a
small amount (the per-block CRCs catch the overshoot); if the header
corruption is worse I may collect blocks that belong to the *next* frame as
if they were mine, and subsequently miss resync for some number of bursts.

SDRTrunk (`P25P1MessageFramer.forceCompletion`, see memory
`project_sdrtrunk_forcecompletion_gap.md`) uses a completely different
strategy: infer PDU length from frame boundaries by the presence of the
*next* sync pattern, recovering frames that never had a valid BTF in the
first place. That's a pure Option C, and it recovered 9 TDULCs on one of
my captures that my decoder missed.

OP25 (`rx_pdu.cc`) drops on header CRC fail — Option A.

## 3. Question

**Does the standard express a preference for receiver behavior when Header
CRC-16 fails on a PDU header block?** Candidate resolutions:

1. **Normative answer exists in BAAA-B / BAED-A I missed.** Cite it, we
   update `fdma_pdu_frame.md` §6 dispatcher pseudocode to match.

2. **Spec is silent — document the three options as implementer strategies.**
   Add §6.1 "Recovery strategies on Header CRC-16 failure" listing A / B / C
   with tradeoff analysis. Recommend B (best-effort BTF with Format-field
   sanity check) as default since it's lowest false-miss rate when
   corruption is limited to one bit.

3. **Spec is silent, AND it's important enough to propose a normative
   behavior.** Hunt this up the TIA-102 working group chain for the next
   revision. Probably out of scope for a derived-work project.

## 4. Diagnostic evidence

On the Sachse gold-standard `.bits` capture, all 86 PDU syncs produced
Header CRC-16 pass. So header-CRC-fail isn't common enough to shape my
heuristic choice — I picked Option B based on "what if one day it does."

On live GMRS control-channel captures, the `project_sdrtrunk_forcecompletion_gap`
memory shows that SDRTrunk's forceCompletion recovered 9 TDULCs that my
decoder missed. TDULCs aren't PDUs, but the same class of problem applies:
a receiver that trusts only clean frames loses information a smarter
length-inference receiver could recover.

## 5. What the implementer needs

Either:

- A cite to BAAA-B § or BAED-A § that I missed, closing the gap, OR
- An `analysis/` note (or `fdma_pdu_frame.md` §6.1) listing the three
  options with tradeoffs, and recommending one as the passive-receiver
  default. Without that, every decoder repeats the same judgment call and
  gets slightly different answers.

## 6. Chip probe plausibility

Not applicable — wire-layer and system-behavior question.

## 7. Priority

Low for this use case (Sachse PDUs arrive clean), medium in general (any
weak-signal deployment hits header CRC failures; the right heuristic
recovers information the dumb heuristic loses).
