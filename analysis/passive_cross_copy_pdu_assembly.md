# Passive Cross-Copy PDU Assembly (Motorola Repeat-Broadcast)

**Scope:** FDMA Phase 1 packet data (DUID `0xC`) received by a **passive**
decoder observing infrastructure-side repeat-broadcast on live systems —
primarily Motorola dispatch and station-alerter CAD traffic. BAAA-B §5.7 and
BAED-A §5 specify active-receiver ARQ (SACK-driven retransmissions). This
note covers the distinct phenomenon of infrastructure-scheduled redundant
transmissions that a passive monitor can merge to improve receiver quality
without a protocol change.

**Not a spec amendment.** This behavior is not defined in any TIA-102 spec.
It's an empirical observation of vendor (primarily Motorola) infrastructure
behavior, together with derived implementer heuristics that have held up in
production use.

**Related gap reports:**
- `gap_reports/0009_passive_cross_copy_pdu_assembly.md` (source gap).
- `gap_reports/0011_passive_receiver_confidence_labeling.md` — the `merged`
  confidence label defined in `analysis/passive_receiver_confidence_labels.md`
  is produced by the merge procedure described here.

---

## 1. Observed Behavior

On Motorola trunked systems carrying dispatch / CAD / station-alerter traffic
via Unconfirmed Data PDUs, the same logical payload frequently appears
multiple times on the air within a short window (≤ 10 seconds). Example
pattern from a live Sachse, TX GMRS capture:

- Single 14-block Unconfirmed CAD alert transmitted as:
  - 5 copies to LLID A across ~90 seconds (two clustered bursts ~4 seconds apart)
  - 3 copies to LLID B across ~85 seconds (similarly clustered)

This is **not** ARQ retry:

1. The passive listener sees no Response Packets (ACK / NACK / SACK) on the
   channel that would explain the retransmissions.
2. Inter-copy spacing is too tight to be ARQ timeouts.
3. Different destination LLIDs receive *independent* copy runs rather than
   sharing a single acknowledged session.

The operational interpretation is scheduled repeat-broadcast for time- and
path-diversity, presumably to improve delivery to fading mobile receivers
without a feedback channel. This is consistent with Motorola's traditional
dispatch and fireground station-alerter products where delivery reliability
matters more than airtime efficiency.

**Scope note: FDMA-only.** TDMA Phase 2 MAC has a completely different PDU
framing and retry model. The passive-assembly question has to be redone for
TDMA if it arises there; nothing in this note translates directly.

---

## 2. Why a Passive Decoder Should Merge Copies

A naïve passive decoder that emits one event per physical copy produces N
events of mixed quality. Using the Sachse 14-block alert as a concrete
example: an 8-copy observation session produced 4 fully-clean events and 4
with block corruption (wrong timestamp digits, spliced null bytes, missing
letters in station names). Every copy had a different corruption pattern
because the corruption was on-air, not upstream.

A decoder that cross-merges CRC-clean blocks across copies — keying on
session identity, substituting each block with any clean copy's version —
produces N_merged ≤ N events of strictly better-or-equal quality. For the
Sachse example, the merge reduced 4/4 clean+dirty to 3 clean + 1 partial. No
protocol change, no extra compute on the radio side, purely post-processing
leverage.

---

## 3. Implementer Heuristics (The Hard Part)

The spec doesn't acknowledge passive reassembly exists, so every number here
is an implementer decision. Each decision below lists the recommended default
and the failure mode that picks the value.

### 3.1 Session Identity Key

**Recommended default:** `(src_llid, sap_id, blocks_to_follow)`, optionally
promoted to `(src_llid, sap_id, blocks_to_follow, mfid)` on mixed-vendor
systems where cross-MFID merging is semantically wrong.

| Field in key? | Yes / No | Why |
|---------------|----------|-----|
| Source LLID | Yes | Distinct sessions will always have distinct source addresses. |
| SAP ID | Yes | Two simultaneous PDUs from the same source to different SAPs (e.g., CAD vs LRRP) must stay distinct. |
| Blocks To Follow | Yes | Cheap discriminator between a long PDU and a short one from the same source. Reject merges where BTF disagrees — strong signal the payloads are different. |
| MFID | Maybe | Promote into the key when a capture spans mixed-vendor infrastructure. Default-off to avoid splitting sessions when a single header bit flip corrupts MFID on one copy. |
| Pad Octet Count | **No** | Spurious session splits when a noisy header decode flips a single bit in the 5-bit pad field. Pad is redundant with payload length once BTF is in the key. |
| Sequence number / message ID | Would be ideal, not available | Neither BAAA-B nor BAEB-C exposes one at the PDU header level for Unconfirmed / SNDCP data. Confirmed data has N(S) but that's a sequence number mod 8, not a message ID — not enough entropy to uniquely identify a session. |
| MFID-private fields (e.g., Motorola session ID bytes in the SNDCP preamble) | Not yet | The Motorola SNDCP preamble bytes documented in `motorola_sndcp_npdu_preamble.md` may carry a usable session correlator; this is an open investigation, don't rely on it yet. |

### 3.2 Idle-Timeout for Session Eviction

**Recommended default:** **30 seconds** of inactivity before emitting a
partial session and evicting.

- **Lower bound:** must exceed the maximum observed inter-copy spacing. On
  the Sachse capture the widest interior gap was ~5 seconds; Motorola's
  documented "burst plus retry after 60 seconds" pattern for some
  station-alerter modes pushes the lower bound up toward 90 seconds for those
  specific flows.
- **Upper bound:** bounded by how long the assembler can hold state without
  accumulating zombie sessions. A receiver sitting on a busy control channel
  sees thousands of short-lived source LLIDs; unbounded session lifetime
  produces an unbounded working set.
- **30s rationale:** large enough to catch the common "burst-of-4-in-10-seconds"
  pattern plus a safety margin; small enough that a receiver restart after
  lost sync doesn't bleed over into the next operational window. The number
  is empirical, not normative — production decoders should make it tunable.

### 3.3 Emission Semantics

Three plausible behaviors; pick one per-decoder:

| Mode | Trigger | Latency | Downstream complexity |
|------|---------|---------|----------------------|
| **First-clean-wins** | Emit once, the first time *every* block has at least one CRC-clean copy on record. Suppress subsequent copies of the same session. | Low (emits as soon as mergeable) | Low (downstream gets one event per PDU) |
| **Best-effort running update** | Emit on every physical copy, with a de-dup hint (session key + copy index). Downstream dedupes. | Low | Higher bus traffic; downstream must implement dedup |
| **End-of-session** | Wait for idle-timeout, emit the final merged state only. | High (bounded by timeout) | Low; good for batched logging |

**Recommended default:** **first-clean-wins** for operational use
(dispatch-UI-class consumers need low latency and a single clean event).
Reserve **end-of-session** for logging / archival consumers that can tolerate
the latency and want the final consolidated confidence score. **Best-effort
running update** is useful during development / A/B benchmarking but
generates too much downstream churn for production.

### 3.4 Partial-Session Handling

When a session ages out with some blocks still never-clean, the decoder has
two reasonable choices:

- **Emit partial** — produce a `partial`-labeled event (per
  `passive_receiver_confidence_labels.md`) with the dirty-block mask
  surfaced, so downstream can render `?` placeholders for unreadable bytes.
  SDRTrunk prints the corrupted text unmerged on every copy; this variant
  is strictly more useful to downstream consumers because the confidence
  label is explicit.
- **Drop silently** — simpler, but downstream loses a strong link-quality
  signal (the existence of a partial is itself evidence of poor coverage).

**Recommended default:** **emit partial with dirty-block mask.** Drop-silent
is a valid operating mode for consumers that explicitly don't want degraded
events, but it should be opt-out, not default.

---

## 4. Merge Procedure

The block-level merge is straightforward once the session-identity key is
chosen:

```
ON receiving a physical copy C of a PDU:
    key = session_key_from_header(C.header)
    session = sessions.upsert(key, fresh_session_state())
    session.last_seen = now()
    session.copies_seen += 1

    FOR i IN 0..BTF-1:
        block_i_crc_ok = verify_block_crc(C.blocks[i])
        IF block_i_crc_ok AND NOT session.blocks[i].crc_ok:
            // First clean copy of this block → adopt it.
            session.blocks[i] = C.blocks[i]
            session.blocks[i].crc_ok = true
            session.blocks[i].source_copy_index = session.copies_seen
        // Existing clean blocks are sticky: don't overwrite with a later
        // clean copy, even if it arrived first on this copy. This keeps
        // the merge monotonic.

    IF every block in session.blocks has crc_ok == true:
        emit(session)            # first-clean-wins semantics
        session.emitted = true
        # Still track copies_seen for metrics; evict on idle timeout.

ON idle_timer_tick:
    FOR session IN sessions:
        IF now() - session.last_seen > IDLE_TIMEOUT:
            IF NOT session.emitted:
                emit_partial(session)
            sessions.evict(session)
```

Confidence labeling per
`analysis/passive_receiver_confidence_labels.md` §2.1:

- `native_clean` if every block was clean on copy 1 with no correction.
- `corrected` if any block needed single-bit correction but all came from
  copy 1.
- `merged` if any block's `source_copy_index > 1`.
- `partial` if any block stayed `crc_ok == false` at emission time.

---

## 5. What the Spec Actually Says vs. What We Do

| Question | Spec answer | Passive-decoder choice (this note) |
|----------|-------------|----------------------------------|
| How many copies of a PDU should the infrastructure send? | Unspecified. Infrastructure is a black box per BAAA-B. | Observed to be 3–8 on Motorola CAD flows. |
| Should a passive receiver reassemble across copies? | Spec is silent — ARQ is defined only for active receivers (BAED-A §5). | Yes; see §4. |
| When can a session be evicted? | Not applicable (spec doesn't define passive sessions). | 30s idle-timeout (§3.2). |
| What confidence does a partially-merged PDU have? | N/A. | Use the four-label taxonomy from `passive_receiver_confidence_labels.md`. |
| Should cross-MFID merging happen? | N/A. | Default no; promote MFID into the session key on mixed-vendor systems. |

The pattern: the spec answers the *what* (wire format, CRC polynomials,
ARQ procedures for active receivers) and the passive-decoder implementer
answers the *how* (session tracking, timeouts, emission semantics). This
split is load-bearing — a future TIA-102 revision could standardize passive
monitoring without invalidating anything here, but is unlikely to.

---

## 6. Non-Goals

This note intentionally does not:

- **Describe Motorola's scheduling algorithm.** We don't know why copy
  spacing is what it is, or what triggers a second burst 90 seconds later.
  Pure observation, no reverse engineering of infrastructure.
- **Make normative claims about copy counts or spacing.** Other operators
  (L3Harris, Kenwood, BK) may behave differently. The session-identity and
  timeout choices in §3 are designed to be robust across vendor differences
  without requiring per-vendor tuning.
- **Cover Confirmed-delivery ARQ retransmissions.** Those *are* in the
  spec (BAED-A §5). A passive receiver that observes a SACK-driven retry is
  seeing a different phenomenon; the merge procedure in §4 will handle it
  correctly (the retry looks like another copy), but the motivation is
  different.

---

## 7. Cross-References

- `analysis/passive_receiver_confidence_labels.md` — confidence taxonomy
  produced by this merge procedure.
- `analysis/motorola_sndcp_npdu_preamble.md` — the vendor-specific bytes in
  Motorola IP PDUs that may eventually supply a stronger session key than
  `(src_llid, sap_id, BTF)`.
- `analysis/fdma_pdu_frame.md` — PDU frame structure that defines what a
  "block" is in §4's merge procedure.
- `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md`
  §5.7 — the Unconfirmed / Confirmed / Response header formats this note
  keys sessions from.
