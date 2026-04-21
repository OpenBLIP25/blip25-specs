# 0010 — Last data block layout when pad > 0: user data / pad / CRC-32 octet order

**Status:** resolved (2026-04-21) — `analysis/fdma_pdu_frame.md` §4.3 and §4.4 updated with worked pad-boundary examples; layout CSV in `annex_tables/pdu_last_block_examples.csv`; BAAA-B implementation spec §5.7.3 / §5.7.4 already contains the canonical worked examples (reference).
**Filed:** 2026-04-21
**Filer:** implementer (p25-decoder / blip25-data)
**For:** spec-author
**Related:**
- `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md` §5.7.3 (Unconfirmed Data), §5.7.4 (Confirmed Data)
- `gap_reports/0007_pad_octet_distribution_across_multiple_data_blocks.md` (resolved — distribution, not placement)
- `analysis/fdma_pdu_frame.md` §7 (CRC summary)

---

## 1. What the spec says

Gap report 0007 (resolved `e7b2ef4`) settled the *distribution* question:
pad octets fill from the end, confined to the last one or two data blocks
(capped at 15 Confirmed / 11 Unconfirmed). `fdma_pdu_frame.md` §4.3 says

> Last 4 octets of final data block: Packet CRC-32 (over all user data + pad)

for the Unconfirmed case, which implies the block layout is:

```
Last data block, Unconfirmed, 12 octets:
[ user data ... ][ pad ... ][ CRC-32 (4 octets) ]
```

The Confirmed case §4.4 says

> Last data block: last 4 user octets replaced by Packet CRC-32

which could be read two ways:

- **Reading A:** the last 4 octets of *user data* (i.e., bytes that would
  otherwise have been payload) are replaced. Layout:
  `[ user data ... ][ pad ... ][ CRC-32 ]`
  — same relative order as Unconfirmed, differing only in block size
  (16 user-data octets vs 12).

- **Reading B:** the CRC-32 lives at the literal last 4 octet positions of
  the block, displacing the last 4 bytes of what would have been the
  payload region — which, under the "pad fills from the end" rule from
  0007, *is the pad region*. Layout: `[ user data ... ][ CRC-32 ][ pad ]`.

## 2. The ambiguity

For a Confirmed packet with `pad_octet_count = 7, blocks_to_follow = 3`,
last block layout under the two readings:

**Reading A (pad between user and CRC):**
```
Octets 0-4:   last 5 octets of user data
Octets 5-11:  7 pad octets
Octets 12-15: CRC-32
```

**Reading B (CRC at literal end, pad before it):**
```
Octets 0-4:   last 5 octets of user data
Octets 5-11:  7 pad octets
Octets 12-15: CRC-32
```

Wait — for a 16-octet Confirmed block these happen to coincide because the
CRC is placed in the last 4 octets in both readings. The two readings
diverge only when `pad_count > (block_size - 4)`, i.e., when pad would want
to push past the CRC region. For Confirmed (16-octet) blocks, that's
`pad > 12`; for Unconfirmed (12-octet) blocks, `pad > 8`.

With the 0007 cap of `P_MAXU = 11` for Unconfirmed, pad up to 11 on a
12-octet block means pad *must* straddle the CRC region under any reading,
and the spec then genuinely needs to say whether:

- pad sits *in* octets 8-11 of the last block with CRC in its own octets
  8-11 (impossible — overlap), or
- pad cascades into the second-to-last block as 0007 resolved, and the last
  block has `user | pad | CRC` with at most 8 pad octets in it.

The 0007 resolution strongly suggests the second interpretation. But neither
BAAA-B full text nor `fdma_pdu_frame.md` §4.3/§4.4 shows a worked hex example
for a case with pad > 4.

## 3. Question

**For both Confirmed and Unconfirmed Data Packets with pad > 0, what is the
exact octet layout of the last data block?** Specifically:

1. Does CRC-32 always occupy the literal last 4 octets of the last data
   block, regardless of pad count?
2. Are pad octets placed immediately before the CRC-32 (filling upward from
   octet `block_size - 5`), or before the user data (filling downward from
   octet 0)?
3. When pad reaches the boundary, does additional pad go into the
   second-to-last block at the *end* (octets `block_size - 1` backward) or
   at the *start* (octets 0 forward)?

## 4. What we do today

My implementation (`crates/blip25-core/src/pdu.rs::decode_pdu` line ~217) assumes:

```rust
// Strip padding from end
if pad > 0 && pad < payload_bytes.len() {
    payload_bytes.truncate(payload_bytes.len() - pad);
}
// Strip trailing CRC-32 (4 bytes) for unconfirmed data only
if !header.confirmed && payload_bytes.len() >= 4 {
    payload_bytes.truncate(payload_bytes.len() - 4);
}
```

i.e., "user data first, pad next, CRC last" — Reading A, which works on
every PDU I've seen in the Sachse capture. But no edge-case validation
with `pad > 8` on Unconfirmed or `pad > 12` on Confirmed.

## 5. What the implementer needs

A **worked hex example** in BAAA-B §5.7.3 (Unconfirmed) and §5.7.4
(Confirmed) showing the last-block octet layout for each of:

- `pad = 0` (baseline)
- `pad = 4` (small, within block)
- `pad = 10` Unconfirmed / `pad = 13` Confirmed (touches / straddles the
  CRC region)

Even just one figure per case would kill this ambiguity. Alternatively, a
`annex_tables/pdu_last_block_examples.csv` with a few (headers + payload,
encoded last-block octets) pairs.

## 6. Chip probe plausibility

Not applicable — wire-layer derived-work question. Answerable by reading
BAAA-B full text §5.4.3 carefully and cross-checking against SDRTrunk's
`module/decode/p25/phase1/message/pdu` layout.

## 7. Priority

Low — in practice P25 CAD / SNDCP payloads I've observed stay under
`pad = 8` because MTUs are small. The gap is mostly a future-proofing concern
and a landmine for a decoder that gets deployed on a system sending larger
fragmented packets.
