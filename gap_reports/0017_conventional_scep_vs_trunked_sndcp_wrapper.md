# 0017 — Motorola RF preamble is SCEP on conventional, SNDCP on trunked; supersedes 0013's mono-protocol framing

**Status:** resolved (2026-04-21, re-resolved with corrected attribution) — byte-layout question retired by processing TIA-102.BAEB-B through the spec pipeline. Findings: (1) **SCEP is a TIA-102 standard**, not Motorola-proprietary; defined in TIA-102.BAEB-B §4. Motorola's "Simple CAI Encapsulation Protocol" name expansion is an alternate gloss; TIA's official name is "Simple *Convergence* Encapsulation Protocol". (2) **SCEP is zero-wrapper** on data-carrying PDUs — IP datagrams are placed directly into CAI logical messages with SAP = Packet Data, Data Header Offset = 0. There is no SCEP header byte layout to reverse-engineer. (3) **SCEP ARP** (BAEB-B §4.2) is the LLID↔IPv4 binding mechanism; 22-octet message format fully drawn. The "CAI ID in a header transmitted with each datagram" language in `MN003252A01-A` §2.4 is a reference to this ARP, not to a per-datagram field. (4) Trunked IV&D wrapper remains SNDCP + LLC per TIA-102.BAEB-C / BAED-A — gap 0018. Derived works: `standards/TIA-102.BAEB-B/P25_IP_Data_Bearer_BAEB_B_Implementation_Spec.md`, `annex_tables/sndcp_reject_codes.csv`, `annex_tables/sndcp_field_definitions.csv`. Analysis notes updated: `motorola_conventional_scep_vs_trunked_sndcp.md` (SCEP attribution corrected, zero-wrapper invariant captured), `motorola_sndcp_npdu_preamble.md` (BAEB-B Figure 27 V1/V2 Reserved|DCOMP vs Figure 28 V3 PCOMP|DCOMP distinction clarified). BAEB-A remains absent from `~/blip25-specs/standards/` but is no longer on the critical path — BAEB-B is the authoritative reference.
**Filed:** 2026-04-21
**Filer:** implementer (p25-decoder / blip25-data)
**For:** spec-author
**Supersedes framing of:** `gap_reports/0013_motorola_sndcp_npdu_header_variants.md` (resolved, but its analysis note only covers the trunked case; the conventional case it treated as "undocumented extra bytes" is in fact a distinct protocol)

**Related:**
- `standards/TIA-102.BAEB-C/P25_IP_Data_Bearer_Implementation_Spec.md` §2 (SNDCP, **trunked only**)
- `standards/TIA-102.BAEB-B/` (present in spec index)
- `standards/TIA-102.BAEB-A/` — **missing from spec index; cited by name in Motorola docs**
- `standards/TIA-102.BAED-A/P25_Packet_Data_LLC_Implementation_Spec.md` §2 (LLC header, shared)
- `analysis/motorola_sndcp_npdu_preamble.md` (drafted for 0013 — correct for trunked; incorrect framing for conventional)
- `analysis/fdma_pdu_frame.md` §4.3 (Unconfirmed Data PDU example — implicitly trunked)
- Motorola `MN005155A01-A_enus_Conventional_Data_Services_Feature_Guide.pdf` §2.1.1 p. 39, §2.2 p. 41, §2.4 p. 43, §2.11.3 p. 1626–1631

---

## 1. The correction

Gap 0013 asked why Motorola traffic on the Sachse capture carried
"undocumented preamble bytes" between the TIA SNDCP header and the IP
datagram, and its analysis note listed three candidate explanations.
The Motorola Conventional Data Services Feature Guide shows the answer:
the preamble is not an SNDCP extension at all. **Conventional ASTRO 25
data does not use SNDCP. It uses SCEP — Simple CAI Encapsulation
Protocol — a Motorola-proprietary tunnel riding on TIA-102.BAEB-A.**

Explicit citations:

- **§2.1.1 p. 39:** *"Unicast datagrams are transported in CAI format on
  the over-the-air link as well as on the ASTRO 25 network links between
  the station equipment and the Conventional RNG using a SCEP tunnel.
  The confirmed and unconfirmed delivery services are implemented
  between the endpoints of this tunnel. When an IP datagram is sent by
  either a subscriber or the Conventional RNG, it is segmented into a
  number of blocks whose format depends on whether confirmed or
  unconfirmed delivery is used as described in TIA-102.BAEB-A."*

- **§2.2 p. 41 (boxed NOTICE):** *"Unlike Trunking IV&D, SCEP
  Conventional subscribers are not SNDCP-capable, are not aware of, and
  do not initiate packet data Context Activation. A Context Activation
  procedure is performed by the infrastructure on behalf of each
  subscriber, but the subscriber is not explicitly aware of this
  processing."*

So the two modes diverge:

| Mode | Over-the-air wrapper | Block format |
|------|----------------------|--------------|
| Trunked IV&D | **SNDCP** (standard, TIA-102.BAEB-C §2) | TIA-102.BAEB-C |
| Conventional | **SCEP** (Motorola-proprietary) | **TIA-102.BAEB-A** |

Both ride on top of the shared TIA PDU framing (confirmed / unconfirmed
data packet, the one consolidated in `analysis/fdma_pdu_frame.md` §4.3 /
§4.4). What sits between the PDU payload bytes and the IP datagram is
what differs.

## 2. Secondary gap — BAEB-A is missing from the spec index

`standards/TIA-102.BAEB-A/` **does not exist** in `~/blip25-specs/`.
The spec index has BAEB-B and BAEB-C, plus BAEA-B / BAEA-C (a different
doc family), but no BAEB-A. The Motorola Feature Guide cites BAEB-A by
name as the authoritative reference for the conventional confirmed /
unconfirmed block format.

This matters because the trunked BAEB-C spec assumes SNDCP context;
applying it to conventional SCEP traffic leads an implementer to look
for SNDCP fields that aren't there. Without BAEB-A in the derived-works
set, conventional-mode implementers are reading the wrong spec.

**Ask:** locate TIA-102.BAEB-A, ingest it, and produce a derived
impl-spec note covering the confirmed / unconfirmed block format under
the *conventional* profile. This will likely look very similar to the
existing BAEB-C derivation at the TIA-block layer and differ above it
(no SNDCP header, SCEP instead).

## 3. SCEP byte layout — still TBD from this Feature Guide

The Feature Guide names SCEP and describes its semantics; it does not
diagram the SCEP header bytes. Known contents (from scattered sections):

| Field | Present when | Size | Source |
|-------|-------------|------|--------|
| CAI ID | Group / broadcast (dst IP = 255.255.255.255) | unknown | §2.4 p. 43 |
| Encryption header | Secure delivery | **13 bytes** | §2.11.3 p. 1626–1631 |
| Registration form discriminator | Standard vs Motorola-proprietary | unknown | §2.2.1 p. 41 |

Section 2.11.3 additionally constrains outbound fragment size to
**≤ 512 bytes** clear / **≤ 499 bytes** encrypted — the 13-byte delta
matches the encryption-header size, consistent with SCEP carrying the
BAEB/AAAD encryption context directly in its wrapper.

Section 2.2.1 notes that legacy Motorola subscribers use a proprietary
Registration Connect / Disconnect form, while newer subscribers use the
standard TIA-102.BAAD-1 form. Both are accepted. This implies the SCEP
header has a type discriminator byte early in the layout.

**Full-byte layout sources (in decreasing order of likely yield):**

1. **RNG Operations Manual** — Motorola-internal / vendor-documentation
   tier. Would have the actual SCEP header byte map if the user can
   obtain one.
2. **SDRTrunk source** — `UnconfirmedDataPacket.java` + `IPPacket.java`
   in `module/decode/p25/phase1/message/pdu/`. De facto reference —
   SDRTrunk successfully extracts IP datagrams from conventional SCEP
   traffic, so its parser encodes the layout it uses.
3. **Reverse engineering from multi-message A/B** — already 8 Sachse
   copies in hand with known plaintext; the preamble varies at known
   offsets and a differential analysis might pin down field boundaries.

Priority on (2) remains highest — ~30 min of Java reading, result
applies to every Motorola conventional capture.

## 4. What this changes for derived works

1. **`analysis/fdma_pdu_frame.md` §4.3** — the SNDCP example is
   trunked-mode-specific. Either add a sibling §4.3 for conventional
   SCEP (with the BAEB-A block layer beneath), or add a top-level note
   that the example assumes trunked IV&D.

2. **`analysis/motorola_sndcp_npdu_preamble.md`** (drafted for 0013) —
   rename or split. The "SNDCP" half applies to trunked IV&D; the
   conventional/SCEP case belongs in a new `analysis/motorola_scep_tunnel.md`
   keyed to this gap report. The Sachse capture referenced in 0013's
   analysis is **conventional-mode** and should migrate to the SCEP
   analysis doc.

3. **New `analysis/motorola_scep_tunnel.md`** — scope:
   - Protocol name and endpoints (per §2.1.1).
   - Tunnel diagram (Figure 5 p. 40, SU ↔ Base Radio / Site Gateway —
     SCEP — PDG — GTP — GGSN — IP-in-IP/VPN — Border Gateway — CEN).
   - Known header fields (CAI ID, 13-byte encryption header,
     registration discriminator).
   - Byte-level layout: cite SDRTrunk source once read.
   - Scope note that SCEP is Motorola-only; other P25 vendors on
     conventional may use different wrappers.

4. **Receiver dispatch** — a conformant implementer must dispatch on
   channel mode (trunked vs conventional) before choosing wrapper
   parser:

```
ON decoded PDU data payload (format 0x15/0x16, user-data SAP):
  IF channel_context == TRUNKED_IVD:
     → parse per TIA-102.BAEB-C §2 SNDCP header, then IP
  ELIF channel_context == CONVENTIONAL_ASTRO_25:
     → parse per SCEP (Motorola-proprietary wrapper), then IP
  ELSE:
     → heuristic: scan for IPv4 version nibble (current blip25 behavior)
```

## 5. Companion reading queued

Per the user's `~/blip25-motospec/recent/` review, the sibling document
is `MN005155A01-A_enus_Trunked_Data_Services_Feature_Guide.pdf`. That
one would confirm whether Motorola trunked IV&D adds proprietary header
bytes *beyond* the TIA-102.BAEB-C §2 SNDCP standard, or stays
spec-clean. If trunked adds nothing proprietary, then the conventional
SCEP wrapper is the only Motorola-specific item left to document. If
trunked does add bytes, that's a third gap to file.

## 6. Chip probe plausibility

Not applicable — protocol documentation question.

## 7. Priority

Medium-high. The original 0013 resolution pointed decoders at BAEB-C
for traffic that's actually BAEB-A + SCEP. A decoder following that
guidance on a conventional system won't break (the IPv4-scan heuristic
works either way), but will misattribute the wrapper bytes and can't
extract CAI-ID, encryption-context, or registration-discriminator
fields. Renaming the protocol in documentation is cheap and prevents
future misrouting of implementer effort.

## 8. Acknowledgment

Protocol naming correction credited to the user's Motorola literature
review in `~/blip25-motospec/recent/` — specifically the Conventional
Data Services Feature Guide. The user explicitly noted "this is for
conventional — trunking next," which is the hook for filing this
separately from whatever the trunked Feature Guide will reveal.
