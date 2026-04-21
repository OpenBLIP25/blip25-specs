# 0013 — Motorola SNDCP N-PDU header variants (undocumented preamble bytes before IP)

**Status:** resolved (2026-04-21) — resolution in `analysis/motorola_sndcp_npdu_preamble.md`; standard SNDCP header confirmed as 2 octets (BAEB-C §6.4.1 Figure 26); preamble bytes after the first 2 octets are NOT TIA-102 standard (three candidate explanations enumerated); heuristic IPv4-scan documented; remaining questions flagged for vendor documentation or session-activation-capture follow-up.
**Filed:** 2026-04-21
**Filer:** implementer (p25-decoder / blip25-data)
**For:** spec-author
**Related:**
- `standards/TIA-102.BAEB-C/P25_IP_Data_Bearer_Implementation_Spec.md` §2 (SNDCP PDU types)
- `standards/TIA-102.BAED-A/P25_Packet_Data_LLC_Implementation_Spec.md` §2 (LLC header)
- `analysis/fdma_pdu_frame.md` §4.3 (Unconfirmed Data PDU / SNDCP example)

---

## 1. What the spec says

BAEB-C describes the standard SNDCP N-PDU header as a 2-byte prefix before
the IP datagram:

```
SNDCP header (2 octets):
  octet 0:  PDU Type (3b) | NSAPI (5b)
  octet 1:  PCOMP (4b) | DCOMP (4b)
```

`analysis/fdma_pdu_frame.md` §4.3 shows this as a 2-byte SNDCP header plus
IP datagram.

## 2. What's actually on the wire

Every Motorola IPPKT in the Sachse gold-standard capture has a **6-byte**
(or longer) preamble before the IPv4 header `0x45` begins. Example from
SDRTrunk's decoded log for LLID 7524844 at 19:09:20:

```
400640001700284CBE   4002001600000000 000200...
└──────┬────────┘    └──────┬─────────┘
  6 bytes preamble     IP header starts here
                       (0x40 is not 0x45 — this is just 4 bytes of SNDCP
                       trailing header, then IP starts at 0x45 some bytes
                       later in other copies)
```

In the Sachse repeat-broadcast captures, the preamble bytes vary:
- `40 06 40 00 17 00 28 4C BE` — 9-byte preamble on one copy
- `40 06 40 00 17 00 B9 CC 36` — 9-byte preamble, different trailer
- `40 06 40 00 18 00 28 20 FE` — 9-byte preamble to different LLID

First byte `0x40` suggests PDU Type = 2 (DATA), NSAPI = 0, which matches
BAEB-C Table 2 (SNDCP DATA PDU). Second byte `0x06` doesn't obviously fit
PCOMP/DCOMP fields alone (would imply PCOMP=0, DCOMP=6, which is a
reserved DCOMP value per BAEB-C §2.3).

## 3. The gap

BAEB-C §2 describes the standard 2-byte SNDCP header. Real-world Motorola
traffic carries additional bytes between the 2-byte header and the IP
datagram. Candidates for what those bytes are:

1. **SNDCP "Data Compression Header"** — BAEB-C §2.3 hints at compression
   context bytes but doesn't pin down their layout or length.
2. **Motorola segmentation/reassembly prefix** — proprietary N-PDU
   fragmentation that predates the standard.
3. **LLC-layer trailing header** — BAED-A mentions LLC header extensions
   (N(S), FSNF, offsets) that *precede* the SNDCP header; my current model
   treats these as part of the PDU header block, but maybe some of those
   bytes actually live in the first data block.
4. **ARS (Automatic Registration Service) session framing** — Motorola's
   ARS protocol wraps IPPKT data in its own session layer, observable on
   port 4005 traffic.

Without knowing which, my current decoder uses
`find_and_parse_ip_udp` (`crates/blip25-core/src/pdu.rs:642`) to scan the
first 32 bytes of payload for a valid-looking IPv4 header start (version=4
+ IHL in range + sensible total length). It works empirically on every
Motorola PDU I've seen, but it's a heuristic, not a parser.

## 4. Question

**Can the derived work document the Motorola-specific SNDCP / LLC preamble
bytes that sit between the BAEB-C §2 SNDCP header and the IPv4 header?**

Even partial answers help:

- Which of the candidates above is correct, or some combination.
- Whether the preamble length is fixed or variable, and how a receiver
  determines where IP starts (length field? magic byte? sentinel?).
- Whether the preamble is Motorola-only or also appears on L3Harris /
  Kenwood / BK equipment.
- Whether BAED-A's LLC header extensions are the real explanation and
  `fdma_pdu_frame.md` needs to show those bytes inside the first data
  block, not implicit in the header.

## 5. Diagnostic evidence

SDRTrunk parses this correctly in
`src/main/java/io/github/dsheirer/module/decode/p25/phase1/message/pdu`
— specifically the `UnconfirmedDataPacket` → `IPPacket` dispatch, which
knows how to find the IP header inside Motorola's wrapper. That code is
the de facto reference but isn't currently cross-referenced from any
derived work.

The Sachse hex dumps above come from SDRTrunk's own decoded-messages log,
so SDRTrunk is reproducing the preamble bytes directly — the structure is
observable, just not documented.

## 6. What the implementer needs

An `analysis/motorola_sndcp_npdu_preamble.md` note that:

1. Captures what we've observed (hex patterns, offsets, correlations with
   PDU type / LLID / app).
2. Cross-references SDRTrunk's Java source for its interpretation.
3. Documents the heuristic that currently works (scan for
   IPv4-version-nibble + plausible IHL + in-range total length).
4. Flags open questions for a future revision (what exactly is in those
   4 extra bytes? fixed or variable? protocol-negotiated?).

Doesn't have to be exhaustive — even "SDRTrunk thinks it's a 6-byte LLC
extension + 2-byte SNDCP header, here's the layout it parses" would be
enough to move us from heuristic to parser.

## 7. Chip probe plausibility

Not applicable — this is a protocol-observation question. Chip probes are
for vocoder. Answer comes from either (a) re-reading BAED-A full text for
an LLC extension that maps to the observed bytes, or (b) decompiling /
reading SDRTrunk's IPPacket parser.

## 8. Priority

Medium. My current decoder works on live Motorola GMRS + the Sachse
capture because the scan-for-IP heuristic is robust. But:

- Any SNDCP payload that is *not* IP (pure PPP / IPX / compressed headers)
  breaks the heuristic silently.
- Any system with different vendor framing would fail differently.
- A proper parser could extract the NSAPI / PCOMP / DCOMP values for
  compression-context tracking, which is currently inaccessible.
