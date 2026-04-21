# 0011 — Confidence labels for passively-received PDUs (native clean / corrected / merged / partial)

**Status:** drafted (2026-04-21) — resolution in `analysis/passive_receiver_confidence_labels.md`
**Filed:** 2026-04-21
**Filer:** implementer (p25-decoder / blip25-data)
**For:** spec-author
**Related:**
- `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md` §5.3 (CRC-9), §5.4 (CRC-32)
- `standards/TIA-102.BAED-A/P25_Packet_Data_LLC_Implementation_Spec.md` §5 (ARQ procedures)
- `gap_reports/0009_passive_cross_copy_pdu_assembly.md` (related)

---

## 1. What the spec says

BAAA-B §5.3.3 and §5.4.2 describe CRC-9 and CRC-32 for *integrity checking*:
a block or packet that fails its CRC is considered errored, and the ARQ
procedures in BAED-A §5 describe what an active receiver does about it
(request retry via SACK).

The spec doesn't describe a taxonomy of *degrees of confidence* in a
received block or packet, because an active receiver only has a binary
choice: accept (CRC pass) or retry (CRC fail).

## 2. What a passive receiver actually produces

A passive (logging / monitoring / dispatch-mirror) receiver has more
granular possibilities per block:

1. **Native clean** — CRC-9 residual is 0 or 0x1FF on first Viterbi output.
   Full confidence.
2. **Single-bit-corrected** — CRC-9 failed initially, but a unique bit
   position exists whose syndrome explains the residual. Flipping that bit
   makes CRC pass. Implemented in
   `crates/blip25-core/src/pdu.rs::crc9_validate_and_correct`. Residual
   false-positive rate on 2-bit errors is ~(144 × 2) / 510 ≈ 56% per-block
   but bounded to a single bit flip, which downstream consumers may or may
   not want to trust.
3. **Cross-copy merged** — block failed CRC on its own copy, but a *different*
   transmission of the same logical PDU (per gap report 0009) had the same
   block CRC-clean. Merging yields a block that was native-clean *somewhere*,
   just not on this copy.
4. **Residual error** — block failed CRC, single-bit correction wasn't
   uniquely determinable, and no other copy of this PDU arrived with that
   block clean. Payload is best-effort garbage.

Our `PduMessage` struct tracks some of this today:
- `block_crc_ok: Vec<bool>` — per-block CRC state (after correction).
- `blocks_corrected: u8` — count of blocks that required single-bit
  correction.
- `blocks_errored: u8` (on the assembled-PDU path) — count of blocks that
  never cleaned across any copy.

But the mapping from these three counters to a human-readable confidence
label (`clean` / `merged` / `partial` / `corrected`) is ad-hoc and
inconsistent across the edge / server / data crates.

## 3. Question

**Should the derived work define a normative confidence taxonomy for
passively-received PDUs, and if so, with what labels and what rules?**

Concrete proposal:

| Label | Rule |
|-------|------|
| `native_clean` | Every block's CRC-9 passed on first Viterbi output AND per-packet CRC-32 passed. No corrections applied. |
| `corrected` | At least one block's CRC-9 passed only after single-bit syndrome correction. All blocks ultimately clean. Per-packet CRC-32 passes. |
| `merged` | The assembled PDU has every block CRC-clean, but at least one block came from a different copy than the header (cross-copy merge). |
| `partial` | One or more blocks never reached a CRC-clean state across any observed copy. Emitted on session timeout. |

Gap-9-style implementer note, not a BAAA-B amendment — this is a decoder
ergonomics question that lives above the wire.

## 4. Why this matters

Downstream consumers (dispatch UI, SIEM, LRRP location fusion, redis
tailers) make trust decisions from the quality label. Without a normative
taxonomy:

- A CAD dispatch integration that displays "SACHSE FIRE DEPT... 190 HHRS"
  has no way to distinguish that from a clean "1900 HOURS" decode except by
  heuristic text inspection.
- An LRRP location fix from a partially-errored PDU could be
  indistinguishable from a clean fix if the confidence isn't promoted to
  the event level.
- Benchmarking A/B (blip25 vs SDRTrunk vs OP25) has no common language for
  "SDRTrunk got 4 clean + 4 corrupt; we got 3 clean + 1 partial after
  merge" — each project calls these qualities something different.

## 5. What the implementer needs

An `analysis/passive_receiver_confidence_labels.md` note that:

1. Defines the four labels above (or proposes better ones).
2. States the rules clearly (what has to be true for each label).
3. Notes that the labels compose: a PDU can be `corrected AND merged`
   (some blocks bit-flipped, others from a different copy).
4. Provides a reference mapping from labels to the struct fields a
   decoder should surface (`block_crc_ok`, `blocks_corrected`,
   `blocks_errored`, `copies_seen`).

Like gap 0009, this is derived-work-only — no spec text change required.

## 6. Chip probe plausibility

Not applicable — ergonomics note, not a standards question.

## 7. Priority

Medium. The more downstream tooling that builds on blip25 events, the
more painful a retrofit becomes. Doing this now while only three consumers
exist (Pi relay, SDRTrunk A/B tool, LRRP fuser) is cheap; doing it after
ten consumers have each invented their own taxonomy is not.
