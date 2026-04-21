# 0009 — Passive cross-copy PDU assembly (Motorola repeat-broadcast) is unspecified

**Status:** resolved (2026-04-21) — resolution in `analysis/passive_cross_copy_pdu_assembly.md`
**Filed:** 2026-04-21
**Filer:** implementer (p25-decoder / blip25-data)
**For:** spec-author

**Related:**
- `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md` §5.7 (PDU subtypes, ARQ, SACK)
- `standards/TIA-102.BAED-A/P25_Packet_Data_LLC_Implementation_Spec.md` §4 (fragmentation / reassembly)
- `analysis/fdma_pdu_frame.md` — single-PDU frame reference, no repeat-broadcast coverage

---

## 1. What the spec says

BAAA-B §5.7.5 defines Response Packets (ACK / NACK / SACK) for *active*
receivers performing ARQ: a unit that owns the session NAKs bad blocks via
SACK bitmap, the transmitter retries those blocks, and the session completes
once every block is clean on the receiver's side.

BAED-A §4 describes LLC-layer fragmentation and reassembly on the active
receiver side.

Neither document addresses the *passive* receiver case: a third-party listener
(monitoring / logging / dispatch-mirror role) that sees every copy of a PDU
that the infrastructure transmits but never participates in the ARQ loop.

## 2. What we observed on-air

The Sachse gold-standard capture
(`/mnt/share/P25-IQ-Samples/GMRS_station_alert_20260409_1900-772106250/`)
includes 8 physical copies of one 14-block CAD text alert within ~6 seconds:

- LLID 7524844 — 5 copies at 19:09:20, 19:09:24, 19:09:25, 19:10:45, 19:10:49
- LLID 7524845 — 3 copies at 19:09:24, 19:09:25, 19:10:49

This is **not** ARQ retry — the passive listener sees no SACK traffic driving
the retransmissions, the copies are too closely spaced, and the two LLIDs
receive *independent* copy runs rather than a shared acknowledged session.
This appears to be Motorola's scheduled "repeat-broadcast" behavior for
dispatch / station-alerter CAD traffic: the infrastructure resends the same
payload to multiple destinations with redundancy, presumably to combat
fading / time-diversity on mobile receivers.

A passive decoder that just emits one event per physical copy produces 8
events of mixed quality (4 clean, 4 with various block corruption — including
SDRTrunk's own log output, which shows `1902 HOURS` in one copy where the
correct timestamp is `1900 HOURS`, a null byte spliced into `1900\x00 HOURS`
in another, and a missing letter in `SACHSE FIK` in a third).

A passive decoder that cross-merges CRC-clean blocks across copies can
produce *3 clean* + *1 partial* event instead. That's measurable receiver
quality improvement purely from post-processing, with no protocol change.

## 3. What I had to decide without spec guidance

Building `crates/blip25-core/src/pdu_session.rs` (the `PduAssembler`), I
had to pick values for the following, all guided by what worked on-air
rather than by any normative source:

1. **Session identity key.** I use `(src_llid, sap_id, blocks_to_follow)`.
   - Including `pad_octet_count` caused spurious session splits when a noisy
     header decode flipped one bit in the pad field.
   - Including a sequence number or message-ID would be ideal, but neither
     BAAA-B nor BAEB-C exposes one at the header level for unconfirmed /
     confirmed data.
   - Adding `mfid` to the key is the safest next extension, since Motorola
     is the only vendor observed emitting repeat-broadcast and
     cross-MFID merging seems semantically wrong — but I don't have
     spec basis to require it.

2. **Idle-timeout for session eviction.** I picked **30 seconds**. The real
   value is bounded below by the max spacing between repeat-broadcast
   copies (observed up to ~5s on Sachse), and bounded above by how long the
   memory can hold state without accumulating zombie sessions. 30s was a
   guess that has held up in four weeks of live use but is entirely
   empirical.

3. **Emission semantics.** Three plausible behaviors:
   - **"First clean wins"** — emit once, the first time every block has a
     CRC-clean copy on record; suppress further copies for the same session.
     This is what I implemented.
   - **"Best-effort running update"** — emit on every copy, downstream
     dedupes. Higher bus traffic, simpler receiver.
   - **"Emit on session-end only"** — wait for idle-timeout, emit the final
     merged state. Higher latency; better for batched logging.

   The spec doesn't prefer any of these because it doesn't acknowledge
   passive reassembly exists.

4. **What to emit on partial-only sessions.** If a session ages out with
   some blocks still lacking a CRC-clean copy, do I emit a degraded event
   (labeled "partial") or drop it? I chose emit-partial with a
   `blocks_errored` counter; SDRTrunk prints the corrupted text unmerged on
   every copy, so there's no reference behavior to align with.

## 4. What would help

An `analysis/` note — `passive_cross_copy_pdu_assembly.md` or similar —
that documents:

1. **The observed repeat-broadcast pattern.** What Motorola actually
   transmits (copy count, spacing, per-LLID vs shared), distinct from ARQ.
   Scoped as empirical observation, not normative.

2. **Implementer heuristics.** The four decisions above, each with a
   recommended default and the reason (what fails if you pick a different
   value). This is the part that's pure derived work — no spec reading, just
   distilled operating experience.

3. **Confidence labeling.** How to tag the merged emission: `clean` vs
   `merged` (assembled from multiple copies' worth of clean blocks) vs
   `partial` (some blocks never cleaned). See also gap report 0011.

4. **Scope note.** That this is FDMA-only (TDMA MAC has a completely
   different PDU framing and retry mechanism, so the passive-assembly
   question has to be redone for TDMA if it ever arises).

## 5. Chip probe plausibility

Not applicable — this is an empirical-observation note, not a standards
question. No chip can answer "how does Motorola schedule repeat-broadcast."
Answer comes from packet captures against live systems.

## 6. Priority

Medium. The blip25 decoder works fine today with the heuristics above. But
the heuristics are undocumented in the derived work, so the next implementer
will rediscover them from scratch — or worse, pick different values and
silently regress. This gap exists precisely because it's not a spec question
and therefore keeps falling off both sides of the spec-author / implementer
boundary.
