# Passive Receiver Confidence Labels for P25 PDUs

**Scope:** FDMA Phase 1 packet data (DUID `0xC`) decoded by a **passive**
receiver — one that monitors the air interface without participating in ARQ.
The normative BAAA-B / BAED-A CRC story is binary (accept or retry), which
suits an *active* receiver but under-serves a monitor that must surface
degraded-but-useful decodes to downstream consumers (dispatch UIs, SIEM,
LRRP fusers, A/B benchmarking). This note defines a four-label taxonomy for
passive-receiver confidence and the composition rules between labels.

**Not a spec amendment.** BAAA-B §5.3 / §5.4 define the CRC polynomials and
integrity semantics; this note sits strictly above them in the decoder API
and doesn't alter any wire-level behavior.

**Related gap reports:**
- `gap_reports/0009_passive_cross_copy_pdu_assembly.md` (cross-copy merge produces
  the `merged` label defined here).
- `gap_reports/0011_passive_receiver_confidence_labeling.md` (source gap).

---

## 1. Why a Taxonomy

An active receiver's CRC outcome is binary: the block passes and joins the
reassembled packet, or it fails and goes into the SACK retry bitmap. BAAA-B
§5.3.3 / §5.4.2 never ask the receiver to publish anything except pass/fail,
because a downstream application is only ever handed a packet after every
block has CRC-passed.

A passive receiver sees more:

1. A block may CRC-pass on first Viterbi output (the common case on strong
   signals).
2. A block may initially CRC-fail, but a single bit flip explains the
   syndrome uniquely — the block can be deterministically corrected and then
   passes. This is the BAED-A §3.2.6 ARQ signal ("retry") repurposed as
   local correction.
3. A block may fail on one physical copy of the PDU but succeed on another
   copy that the transmitter sent for time/path diversity (Motorola's
   repeat-broadcast behavior — see `passive_cross_copy_pdu_assembly.md`).
   Cross-copy merging produces a packet that was block-for-block clean
   *somewhere*, just not all on the same transmission.
4. A block may stay dirty across every observed copy. The PDU can still be
   delivered if downstream consumers accept partials — e.g., a human
   dispatcher reading CAD text is better served by "SACHSE F?RE DEPT" than by
   no event at all.

The normative retry / drop dichotomy collapses all four cases into one of
two answers, so distinguishing them requires an extra layer.

---

## 2. Four Labels

Each reassembled PDU is tagged with exactly one of four confidence labels,
ordered from highest to lowest confidence:

| Label | Rule | What a consumer should trust |
|-------|------|------------------------------|
| `native_clean` | Every block's CRC-9 (Confirmed) or Packet CRC-32 (Unconfirmed) passed on first Viterbi output, with no bits corrected and no cross-copy substitution. | Treat as ground truth; equivalent to an active receiver's ACK'd packet. |
| `corrected` | At least one block's CRC-9 passed only after applying single-bit syndrome correction (`crc9_validate_and_correct` or equivalent). Every block is ultimately clean on this copy; per-packet CRC-32 passes. No cross-copy substitution. | Trust the payload; flag the block(s) whose bits were corrected so downstream can decide policy on corrected-byte provenance. |
| `merged` | The assembled PDU has every block CRC-clean, but at least one block came from a *different physical copy* than the header (cross-copy merge). | Trust the payload; note that the PDU was reassembled across multiple transmissions and therefore has higher latency than a single-copy decode. |
| `partial` | One or more blocks never reached a CRC-clean state across any observed copy. Payload contains best-effort Viterbi output for dirty blocks. | Do not trust dirty block contents; use only clean block contents. Surface the dirty-block mask so consumers can render `?` / `.` placeholders. |

Labels are **mutually exclusive** at the PDU level, but the lower three are
structurally composable on a per-block basis (see §3). The whole-PDU label is
the *worst* (lowest-confidence) label that any constituent block earned.

### 2.1 Label Promotion Order

When consolidating per-block states into a PDU-level label:

1. If any block is `partial` (never clean on any copy) → PDU label = `partial`.
2. Else if any block came from a different copy than the header → PDU label =
   `merged`.
3. Else if any block required single-bit correction → PDU label = `corrected`.
4. Else → PDU label = `native_clean`.

This is a lattice: `partial` < `merged` < `corrected` < `native_clean`,
with "takes the minimum" as the consolidation operator.

---

## 3. Composition (Per-Block State)

A decoder that wants to surface richer state per block should record an
independent per-block confidence:

```c
// Per-block state tracked during assembly.
typedef struct {
    bool     crc_ok;              // block ultimately clean (any copy, any correction)
    bool     crc_ok_first_try;    // block clean on first Viterbi without correction
    bool     bit_corrected;       // single-bit syndrome correction was applied
    uint8_t  source_copy_index;   // which physical copy supplied the accepted block
    uint8_t  copies_seen;         // how many physical copies of this block arrived
} pdu_block_confidence_t;
```

Composite per-block states a PDU can carry:

| Per-block composite | Produces PDU label | Example |
|--------------------|-------------------|---------|
| all `crc_ok_first_try=true` | `native_clean` | Strong signal, no retries. |
| some `bit_corrected=true`, all `crc_ok=true`, all `source_copy_index=header_copy` | `corrected` | Marginal signal, Viterbi soft outputs recovered by CRC-9 syndrome. |
| all `crc_ok=true` but some `source_copy_index != header_copy` | `merged` | Motorola repeat-broadcast rescued one copy's bad block with another's good copy. |
| any `crc_ok=false` | `partial` | Block never cleaned across all observed copies. |

`corrected` and `merged` are *conceptually* composable (`corrected AND merged`
— some blocks fixed via bit-flip, others from different copies) but for
PDU-level reporting use the promotion order in §2.1 to collapse to a single
label. Consumers that need finer granularity read the per-block fields
directly.

---

## 4. Reference Field Mapping

Rust-ish field names for a decoder implementing this taxonomy:

```c
// Whole-PDU confidence.
typedef enum {
    PDU_CONF_NATIVE_CLEAN = 0,
    PDU_CONF_CORRECTED    = 1,
    PDU_CONF_MERGED       = 2,
    PDU_CONF_PARTIAL      = 3,
} pdu_confidence_label_t;

typedef struct {
    pdu_confidence_label_t label;
    uint8_t blocks_native_clean;   // blocks with crc_ok_first_try=true
    uint8_t blocks_corrected;      // blocks with bit_corrected=true (implies crc_ok=true)
    uint8_t blocks_merged_from_other_copy;  // blocks with source_copy_index != header_copy
    uint8_t blocks_errored;        // blocks with crc_ok=false (only non-zero when label=PARTIAL)
    uint8_t copies_seen;           // total physical copies of this PDU observed
} pdu_confidence_t;
```

**Invariants:**

- `blocks_native_clean + blocks_corrected + blocks_merged_from_other_copy
   + blocks_errored == BTF` (the block count from the header).
- `label == PARTIAL  ↔  blocks_errored > 0`.
- `label == MERGED   ↔  blocks_errored == 0 AND blocks_merged_from_other_copy > 0`.
- `label == CORRECTED ↔ blocks_errored == 0 AND blocks_merged_from_other_copy == 0
                       AND blocks_corrected > 0`.
- `label == NATIVE_CLEAN ↔ blocks_native_clean == BTF`.

---

## 5. Boundary Cases

### 5.1 Header CRC-16 vs. confidence label

The confidence label describes the *payload* confidence. The Header CRC-16
gates whether the PDU enters the confidence pipeline at all (per
`fdma_pdu_frame.md` §6.1, header CRC failure triggers one of the three
recovery strategies A/B/C; only a surviving frame gets a confidence label).
A decoder running Strategy B (best-effort BTF) that produced a payload from a
CRC-failed header should additionally flag the PDU with a separate
`header_was_corrupted` boolean — this is orthogonal to the confidence label,
which assumes a trusted header.

### 5.2 Confirmed-delivery ARQ-driven retries

If a Confirmed PDU's SACK-driven retransmission produces a block that a
passive receiver sees cleanly on the retry, the retry is itself "a different
copy" of the block. The passive receiver's merge policy should treat it
identically to Motorola repeat-broadcast copies: `merged` label, tagged with
the source copy index. There's no need for a separate "ARQ-merged" label.

### 5.3 Encrypted payloads

Block CRCs cover the encrypted ciphertext, not plaintext. The confidence
label reflects *wire integrity*, not decryption success. A PDU can be
`native_clean` in this taxonomy but still produce unusable plaintext if a key
is unavailable or the MI is wrong. Downstream crypto processing is a separate
stage with its own error taxonomy.

### 5.4 Single-copy vs. multi-copy receivers

A decoder that doesn't implement cross-copy assembly (e.g., emits one event
per physical copy) will never produce the `merged` label. That's a valid
operating mode; the label is an output of the assembly layer, not a required
capability of every decoder. A single-copy decoder should use the three
remaining labels (`native_clean`, `corrected`, `partial`) and document that
it does not emit `merged`.

---

## 6. Why Not More Labels

Tempting additions that were considered and rejected:

- **`soft_corrected`** (Viterbi soft-output recovered a block without CRC
  syndrome correction). Collapsed into `native_clean` because the Viterbi
  already fired before CRC checking — if the block passes first-try CRC, the
  soft-decoding path has already done its job and there's no user-visible
  difference to track.
- **`crypto_labeled`** (payload decrypted but with MI resync). Belongs in a
  separate crypto-layer confidence taxonomy; mixing wire and crypto
  confidence in one label makes promotion rules incoherent.
- **`stale_merged`** (merge happened but more than N seconds after the
  header's first arrival). Timing is session-metadata territory; put it
  alongside `copies_seen` and `first_seen_timestamp` rather than in the
  label enum.

Keeping the label enum at four values keeps downstream dispatch compact and
matches how humans reason about "how much should I trust this?" — native,
auto-fixed, cross-copy-rescued, or best-effort.

---

## 7. Cross-References

- `analysis/passive_cross_copy_pdu_assembly.md` — the cross-copy assembly
  procedure that produces the `merged` label.
- `analysis/fdma_pdu_frame.md` §6.1 — header CRC-fail recovery, which precedes
  confidence labeling.
- `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md`
  §5.3 (CRC-9) / §5.4 (CRC-32) — the integrity primitives these labels sit on
  top of.
- `standards/TIA-102.BAED-A/P25_Packet_Data_LLC_Implementation_Spec.md` §3.1,
  §3.2.6 — the active-receiver ARQ model that this taxonomy extends for
  passive use.
