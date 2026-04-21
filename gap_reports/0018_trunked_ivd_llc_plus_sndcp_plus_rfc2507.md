# 0018 — Trunked IV&D preamble is LLC + SNDCP + IPv4 + UDP (≈30 B), with RFC 2507 header compression after first datagram

**Status:** resolved (2026-04-21; corrected 2026-04-21 after implementer feedback from `blip25-data`) — trunked IV&D layering captured in `analysis/motorola_conventional_scep_vs_trunked_sndcp.md` (§§2–3) and `analysis/motorola_sndcp_npdu_preamble.md` (rewritten). The "extra" preamble bytes between the BAEB-C reassembled-payload boundary and IPv4 `0x45` are LLC user-plane bytes per TIA-102.BAED-A. **The SNDCP header sits at `ip_offset - 2`** — directly in front of the IPv4 version nibble — and decodes cleanly under standard BAEB-C Figure 26 semantics (no reserved-field emission). Earlier drafts mis-scoped the initial `0x40 0x06 ...` bytes as the SNDCP header; they are actually LLC user-plane, and the real SNDCP header (at `ip_offset - 2`) has PDU Type = 4/5, NSAPI in the user-context range, PCOMP = 0, DCOMP = 0. Both `motorola_sndcp_npdu_preamble.md` §3 and the BAEB-B impl spec §9 now reflect the correction. BAEB-B processing also added: (1) full SNDCP V1/V2/V3 Figures 16–28 byte layouts at `standards/TIA-102.BAEB-B/P25_IP_Data_Bearer_BAEB_B_Implementation_Spec.md` §6–§7; (2) `annex_tables/sndcp_reject_codes.csv` and `annex_tables/sndcp_field_definitions.csv`; (3) RFC 2507 sub-field semantics (MAX_INT_FHD × 16 headers, MAX_TME_FHD × 5 seconds) matching Motorola's "Max Number of Compressed Headers" and "Max Time Between Full Headers" tunables; (4) confirmation that PCOMP's meaning changed between BAEB-B (reserved) and BAEB-C (header-compression identifier). The previously-recorded "NSAPI = 0" and "DCOMP = 6" open questions are retired — they were artifacts of the mis-scoping. Remaining open question: exact LLC user-plane byte layout (BAED-A byte offsets in the first data block), now the leading unresolved item for a conformant parser.
**Filed:** 2026-04-21
**Filer:** implementer (p25-decoder / blip25-data)
**For:** spec-author
**Companion to:** `gap_reports/0017_conventional_scep_vs_trunked_sndcp_wrapper.md` (conventional / SCEP side)

**Related:**
- `standards/TIA-102.BAEB-C/P25_IP_Data_Bearer_Implementation_Spec.md` §2 (SNDCP PDU types — does not diagram the on-wire header bytes)
- `standards/TIA-102.BAED-A/P25_Packet_Data_LLC_Implementation_Spec.md` §2 (LLC header — the user-plane window layer this gap names explicitly)
- `analysis/motorola_sndcp_npdu_preamble.md` (drafted for gap 0013; its "extra preamble bytes" mystery is resolved by this gap)
- Motorola `MN005155A01-A_enus_Trunked_Data_Services_Feature_Guide.pdf` §2.1.3 p. 30-31, §2.1.9.1, p. 38 step 6, p. 1066, p. 1846-1849, p. 3056-3062

---

## 1. The correction to gap 0013's framing

Gap 0013 described "Motorola SNDCP N-PDU header variants" as
undocumented extra preamble bytes. The Motorola Trunked Data Services
Feature Guide makes the real picture explicit:

- There is no single "SNDCP preamble." The preamble is a **stack of
  layers**: LLC (TIA-102.BAED), then SNDCP (TIA-102.BAEB-C), then IPv4
  (20 B), then UDP (8 B), then app payload.
- The Feature Guide §2.1.3 p. 30 quotes the aggregate size directly:
  *"Full (uncompressed) IPv4, UDP, and SNDCP headers add up to 30
  bytes."* With IPv4 at 20 and UDP at 8, that makes **SNDCP ≈ 2 bytes**
  — matching BAEB-C §2's 2-octet header figure. The "extra bytes" gap
  0013 was seeing **aren't SNDCP at all** — they're **LLC user-plane
  fields** sitting between the BAEB-C PDU reassembly and SNDCP.
- Gap 0013's analysis note candidates "Motorola segmentation/reassembly
  prefix" and "LLC-layer trailing header" were both pointing at the
  right place. The real answer is: **LLC is a real user-plane layer
  with its own sequence numbers, retry counter, and SACK semantics**;
  it just wasn't drawn in BAEB-C. It's in BAED-A.

## 2. Wire layering, trunked IV&D

```
┌──────────────────────────────────────────────────────────┐
│  CAI PDU (TIA-102.BAEB-C blocks, reassembled payload)    │
│  └── LLC user-plane PDU (TIA-102.BAED-A)                 │
│      • sequence / window fields                          │
│      • ACK / SACK (selective acknowledgment, §p. 39      │
│        step 6 of Feature Guide)                          │
│      └── SNDCP header (~2 bytes per BAEB-C §2)           │
│          • PDU Type + NSAPI + PCOMP + DCOMP              │
│          └── IPv4 (20 B) + UDP (8 B) + app payload       │
└──────────────────────────────────────────────────────────┘

Total pre-payload preamble (no compression): ≈30 bytes.
```

The Feature Guide's "30 bytes" figure is the clean invariant a
passive-decoder implementer can key on: **if your preamble scan comes
in under 30 bytes from the BAEB-C block boundary to the first `0x45`
IPv4 version nibble, the capture has RFC 2507 header compression
active.**

## 3. RFC 2507 header compression changes the parse problem

**§2.1.3 pp. 30-31** establishes:

- UDP/IP headers are **RFC 2507-compressed** after the first datagram
  of a session.
- **First datagram of a session:** full 30-byte stack on the wire.
- **Subsequent datagrams in the same session:** compressed headers
  (much smaller). RFC 2507 replaces the IPv4/UDP fields with a
  context identifier and a delta encoding.
- **Broadcast:** never compressed (no per-receiver session state).
- **Encrypted:** *"Inner and outer header compression is not supported
  for encrypted data."* (§2.1.3 p. 31). Encrypted captures always carry
  full uncompressed headers.

Two RNG-configurable knobs:

- **Max Number of Compressed Headers Between Full Headers**
- **Max Time Between Full Headers**

Both zero means "one full header per session, everything after it
compressed." Non-zero means the session periodically re-emits full
headers for decoder resync. A passive receiver needs to track RFC 2507
context to decompress compressed headers; **a passive receiver that
ignores this will simply see "malformed IPv4" on every datagram after
the first in a session**. Current blip25 IPv4-scan heuristic
accidentally works because we're decoding *broadcast* CAD traffic in
the Sachse capture — broadcast never gets compressed.

## 4. SNDCPv1 vs SNDCPv3

**p. 1066:** *"Supports SNDCPv1 and SNDCPv3 data registration."*
**pp. 1846-1849:** radio negotiates v3 → falls back to v1 if denied.

Both versions live on the wire. Same radio can appear as either across
sessions. Header formats differ slightly (SNDCPv3 adds fields for
explicit Context Activation state; SNDCPv1 is the original BAEB spec).
A passive parser has to handle both.

**What the derived works should say:** cite BAEB-C §2 for SNDCPv1;
document the SNDCPv3 delta (which is likely in a BAEB-C errata or
addendum we don't have ingested yet).

## 5. LLC user-plane — it's a real, config-tunable layer

**pp. 3056-3062** list LLC-layer RNG config:

- LLC Number of Attempts (retry count)
- LLC Timer (sec) (retry wait)
- LLC User Plane Response Timer
- LLC User Plane Window Size (messages in flight)

**p. 39 step 6:** *"replies with an acknowledgment that the message
was received without errors (ACK) or replies that only certain blocks
were successfully received (SACK)."* This is **LLC-level SACK**, not
the BAAA-B packet-level SACK covered by gap 0006. There are **two**
SACK mechanisms in P25 data:

| Layer | Spec | Purpose |
|-------|------|---------|
| Packet (BAAA-B §5.7.5) | Response Packet with SACK flag bitmap | Block-level retry within one PDU |
| LLC user-plane | BAED-A | Message-level retry across a session |

Gap 0006 (BAAA-B packet-SACK with >64 blocks) and this LLC-SACK are
distinct mechanisms. Worth calling out in `analysis/fdma_pdu_frame.md`
to prevent future cross-contamination.

## 6. Classic Data vs Enhanced Data

**§2.1.3 p. 30:** *"An Enhanced Data message can contain a maximum of
384 bytes of data, including user payload and all headers."*

Two message-sizing profiles exist:

- **Classic Data** — older, longer max message, no/optional compression.
- **Enhanced Data** — 384-byte cap, compression mandatory for efficiency.

A passive decoder doesn't strictly need to distinguish these for
parsing (the layering is the same), but knowing the 384-byte ceiling
helps flag implausible reassembly.

## 7. ARS and OTAR ride on this stack

**Feature Guide comment:** OTAR is "not an IP application from the
perspective of the SU" on conventional, but **on trunked it IS an IP
app** riding on UDP above SNDCP.

Practical: ARS (port 4005) and CAD (port 2002) packets differ only in
their UDP destination port. Everything up to and including the UDP
header is identical. So the common pre-payload bytes across all
trunked Motorola data traffic are:

```
[LLC user-plane] [SNDCP ~2 B] [IPv4 20 B] [UDP 8 B]
                                           └── dst port differentiates app
```

This is useful for the confidence-label taxonomy in gap 0011: the
LLC+SNDCP+IPv4+UDP path up to dst-port is the "same-preamble"
invariant that multi-app decoders can rely on.

## 8. Reverse-engineering strategy for the SNDCP byte layout

The Feature Guide does not diagram SNDCP header bytes. Best leverage:

1. **Encrypted traffic as anchor** — encrypted data forbids compression.
   Every datagram carries the full 30-byte stack. Pick an encrypted
   capture (Sachse is clear; need a different capture) and the SNDCP
   header bytes are at fixed offsets from the IPv4 `0x45` marker.
2. **SDRTrunk `SNDCPPacket.java`** — module/decode/p25/phase1/message/
   pdu/sndcp/. Implementation witness for both v1 and v3 parsers. ~30
   min read; yields the byte map directly.
3. **Context Activation capture** — the SU↔GGSN Context Activation
   exchange (§2.1.9.1) sets up the RFC 2507 compression context. A
   capture that includes the activation handshake gives you the
   full-header / compression-state baseline to differentially decode
   compressed packets downstream.

## 9. Impact on derived works

1. **`analysis/fdma_pdu_frame.md` §4.3** — SNDCP example should be
   re-annotated as trunked-mode-specific and extended to mention that
   the LLC user-plane layer sits between the BAEB-C blocks and the
   SNDCP header, not drawn in the current §4.3 diagram.

2. **`analysis/motorola_sndcp_npdu_preamble.md`** (drafted for gap 0013)
   — its "Motorola-proprietary preamble" framing is wrong. The extra
   bytes it labeled as mystery are **LLC user-plane** bytes per BAED-A.
   Rename or rewrite: scope becomes "LLC + SNDCP preamble on trunked
   IV&D" with byte layout from SDRTrunk.

3. **New `analysis/rfc2507_compression_on_p25.md`** — scope: what
   changes in the on-wire preamble when header compression activates,
   which capture conditions force full headers (broadcast, encrypted,
   first-in-session), and how a passive decoder can either decompress
   or skip compressed datagrams gracefully.

4. **Cross-reference with BAED-A** — the spec-author's existing BAED-A
   derivation in `standards/TIA-102.BAED-A/` should be audited for
   whether it covers LLC user-plane window / sequence / SACK, which
   this Feature Guide calls out as a first-class layer. If yes, wire
   it up; if no, that's a further spec gap.

## 10. Matrix: conventional vs trunked (from 0017 + 0018)

| Aspect | Conventional IV&D | Trunked IV&D |
|--------|-------------------|--------------|
| Encapsulation | SCEP (Motorola proprietary) | SNDCP (TIA) + LLC (TIA) |
| Block spec | TIA-102.BAEB-A | TIA-102.BAEB-C |
| Context Activation | Infra-only, SU unaware | Explicit SU↔GGSN (§2.1.9.1) |
| Header compression | Not mentioned | RFC 2507, post-first-datagram |
| Registration | Connect/Disconnect (std + Motorola) | Context Activation exchange |
| SACK layers | 1 (packet-level, BAAA-B) | 2 (packet-level BAAA-B + LLC-level BAED) |
| Known wrapper bytes | 13 B encryption header, CAI ID for broadcast | ≈30 B total (IP 20 + UDP 8 + SNDCP ~2), LLC header not yet pinned |
| Spec in blip25-specs | **BAEB-A MISSING** (per gap 0017 §2) | BAEB-C present; BAED-A present |

## 11. What the implementer takes away

1. **Dispatch by system mode.** Trunked → LLC+SNDCP parser. Conventional
   → SCEP parser. Same BAEB TIA-block layer underneath, different
   wrappers above.

2. **Budget 30 bytes of preamble on first-of-session trunked traffic**;
   less thereafter (compression). Encrypted traffic is always full 30.

3. **LLC is a real layer.** If a capture's preamble looks like it has
   a sequence number field before SNDCP, that's LLC, not SNDCP
   proprietary. Covered by BAED-A.

4. **Handle v1 and v3 SNDCP.** Same radio emits both.

5. **RFC 2507 is the shape wildcard.** Blip25's current IPv4-scan
   heuristic works only because the Sachse capture is broadcast CAD
   (never compressed). Unicast Motorola IV&D will fail that heuristic
   on every datagram after the first in a session unless decompression
   is implemented or compression is side-stepped via encryption-forced
   full headers.

6. **Dst-port is the app discriminator** once the LLC+SNDCP+IPv4+UDP
   invariant is parsed. ARS=4005, CAD=2002, LRRP=4001, etc.

## 12. Chip probe plausibility

Not applicable — protocol documentation question.

## 13. Priority

High for trunked IV&D correctness. The blip25 Pi deployment works
today on Sachse (conventional GMRS, broadcast CAD) largely by
accident — the capture's conditions bypass every Motorola-specific
wire-format complication. Real-world trunked monitoring hits RFC 2507
compression, SNDCPv1/v3 divergence, and LLC user-plane SACK
immediately. Worth resolving before the Pi migrates from GMRS
monitoring to a trunked target.

## 14. Acknowledgment

Findings credited to the user's Motorola literature review in
`~/blip25-motospec/recent/` — specifically the Trunked Data Services
Feature Guide (`MN005155A01-A`), sibling to the Conventional Feature
Guide cited in gap 0017.
