# Motorola Conventional SCEP vs Trunked SNDCP: Wrapper Bytes Between BAEB-* and IP Header

**Scope:** Identifies *which* encapsulation protocol carries IP datagrams
between the TIA-102.BAEB transport and the IP header on Motorola ASTRO 25
systems, and why the byte pattern differs depending on system mode.
Grounded in TIA-102.BAEB-B (the authoritative definition of both SCEP and
SNDCP V1/V2/V3) and BAEB-C / BAED-A (SNDCP-V3 and LLC details), with
Motorola System Release 7.17 / 2018 vendor docs supplying implementation
witness — which deployment uses which option, how tunables are named,
what registration forms are in the field, and where proprietary extensions
sit.

**Companion to:** `analysis/motorola_sndcp_npdu_preamble.md` (which
documents the on-the-air bytes empirically observed on trunked captures).
This note provides the architectural *why* — what layer those bytes come
from, and why conventional captures look different from trunked captures.

**Sources:**

*TIA standards (clean-room derived works only — PDFs git-ignored):*
- **TIA-102.BAEB-B** (2014-09) — **authoritative source for SCEP.**
  Defines SCEP in §4 (protocol + ARP wire format), SNDCP V1/V2/V3 in
  §§5–6, TDS/CDS state machines in §5.2/§5.3. Derived impl spec at
  `standards/TIA-102.BAEB-B/P25_IP_Data_Bearer_BAEB_B_Implementation_Spec.md`.
- TIA-102.BAEB-C (2019-12) — SNDCP-V3-centric; does **not** reproduce
  SCEP, so BAEB-B remains authoritative for any SCEP decoding.
- TIA-102.BAED-A — LLC / packet-data layer between BAEB-C reassembled
  payload and SNDCP on trunked IV&D.
- TIA-102.BAJD-A — UDP port assignments for applications above the
  stack.

*Motorola implementation witness (copyrighted vendor material, cited by
section — not quoted verbatim):*
- `MN003252A01-A` *Conventional Data Services* (2016, 76 pp) — names
  SCEP and maps Motorola's conventional tunnel architecture.
- `MN005155A01-A` *Trunked Data Services Feature Guide* (2018, 80 pp) —
  quantifies the SNDCP+UDP+IP header budget and the LLC user-plane layer.
- `MN004402A01-A` *Packet Data Gateways* (296 pp) — PDG internals;
  confirms SCEP as a TIA/APCO-standard protocol; exposes NSAPI and
  SNDCP version as provisionable fields; gives LAP-D parameter defaults.
- `ASTRO_25_Conventional_Systems_System_Planner.pdf` (older) — legacy
  RNC3000/WNG-generation context.

**Not a TIA spec amendment.** The TIA-102 byte-layout content in this
note is a pointer into the derived BAEB-B / BAEB-C / BAED-A impl specs,
not a restatement of the standard. The *Motorola* docs cited here do
not draw SCEP or SNDCP byte layouts themselves — they provide
architectural and deployment-level witness on top of the TIA specs.

---

## 1. Two Different Wrappers by System Mode

The biggest finding: **conventional and trunked ASTRO 25 do not use the
same data-encapsulation protocol.** A parser that assumes SNDCP will fail
on conventional captures, and vice versa.

| Aspect | Conventional IV&D | Trunked IV&D |
|---|---|---|
| Encapsulation above BAEB | **SCEP** = Simple Convergence Encapsulation Protocol — **TIA-102 standard**, defined in TIA-102.BAEB-B §4. Motorola's docs gloss the acronym as "Simple CAI Encapsulation Protocol"; same protocol, different name expansion. | **SNDCP** (TIA-102.BAEB-B §§5–6 and BAEB-C) + **LLC** (TIA-102.BAED-A) |
| **SCEP wrapper bytes** | **Zero.** BAEB-B §4 / §6.4: IP datagram is carried directly as a CAI logical message with SAP = Packet Data, Data Header Offset = 0. *There is no SCEP header byte layout to reverse-engineer.* | ≈30 bytes of IPv4+UDP+SNDCP plus LLC user-plane (see §2 below) |
| Air-interface block spec | TIA-102.**BAEB-A** (cited by Motorola; newer TIA editions BAEB-B/-C reproduce the same block layer) | TIA-102.**BAEB-C** (or BAEB-B for V1/V2 contexts) |
| Applicable P25 data configurations (per BAEB-B §1) | Direct Data, Repeated Data, Conventional FNE Data | Trunked FNE Data only (SNDCP mandatory) |
| Context Activation | None at the SU level (no SNDCP contexts). Infrastructure handles packet-data-registration lifecycle on the SU's behalf. | Explicit SU↔GGSN two-way exchange (SNDCPv1 or SNDCPv3) |
| IP address binding | Provisioned, or dynamic via **SCEP ARP** (BAEB-B §4.2 — 22-octet message, 3-octet LLIDs substituting for Ethernet MACs) | Static or DHCP-assigned at context activation |
| Registration | Conventional Registration Connect/Disconnect (TIA-102.BAAD-1) + Motorola-proprietary legacy form; infrastructure accepts both | Context Activation exchange (Request / Accept / Reject) |
| Header compression | N/A at the SCEP level (SCEP has no compression negotiation; any compression on conventional is in application layers or AAAD encryption) | RFC 2507 (V3+) or VJ RFC 1144 (V1/V2), negotiated in Context Activation |
| Tunnel chain | SU → Base Radio + Site GW (**SCEP tunnel**) → PDG → GGSN (**GTP tunnel**) → Border GW (**IP-in-IP / VPN**) → CEN | SU → Site Controller → RNG (**LLC / SNDCP**) → PDR → GGSN → CEN |

`MN003252A01-A` §2.2 (p. 40, boxed NOTICE) makes the contrast explicit:
SCEP conventional subscribers are *not* SNDCP-capable and do not
themselves initiate packet-data Context Activation. The infrastructure
performs an equivalent setup on the subscriber's behalf, with the
subscriber unaware that it's happening.

---

## 2. Trunked IV&D: Byte Budget for the Preamble

`MN005155A01-A` §2.1.3 (p. 30) gives the only quantitative statement of
header size found in either document: the full uncompressed IPv4 + UDP
+ SNDCP header stack totals **30 bytes**, and an Enhanced Data message
is capped at **384 bytes including all headers and payload**. That
30-byte figure is used as the admission gate for Enhanced Data — if
payload + 30 exceeds 384, the message must ride Classic Data instead.

Decomposed:

```
IPv4 header:  20 bytes  (standard, no options)
UDP header:    8 bytes  (standard)
SNDCP header: ~2 bytes  (matches TIA-102.BAEB-C §3.2 2-octet SN-Data/SN-UData header)
              --------
Total:        30 bytes
```

So on trunked IV&D with no compression, the bytes between the
BAEB-C transport and the user payload are:

```
[ BAEB-C PDU boundary ]
  [ LLC user-plane header ]     <-- TIA-102.BAEC/BAED (see §3 below)
  [ SNDCP header        (~2B) ] <-- TIA-102.BAEB-C
  [ IPv4 header         (20B) ]
  [ UDP header          (8B)  ]
  [ application payload       ]
```

**LLC is not counted in the 30-byte figure** — Motorola's statement
is scoped to IPv4, UDP, and SNDCP specifically. The LLC User Plane is
described elsewhere in the same doc (§3.1.8, p. 58) as having its own
sliding-window sequence/ACK mechanics, so its header is on the wire
too but is handled between the SU and the RNG endpoints rather than
between the SU and the CEN host.

### Header compression is the big shape variable

RFC 2507 compression is negotiated during context activation. Expected
patterns on trunked captures:

- **First datagram of a session:** full 30-byte (IPv4+UDP+SNDCP) header.
- **Subsequent datagrams:** compressed header; count between full headers
  configurable via `Max Number of Compressed Headers Between Full Headers`
  and `Max Time Between Full Headers` (UNC-tunable, both can be 0 ⇒ "one
  full header per session, all compressed thereafter").
- **Broadcast data:** never compressed (one-way, no state to negotiate).
- **Encrypted data:** §2.1.3 (p. 31) rules out RFC 2507 compression of
  both inner and outer headers for encrypted traffic. Every encrypted
  datagram therefore carries the full 30-byte header stack — the
  cleanest baseline for reverse-engineering the SNDCP field layout.

### NSAPI and LAP-D named explicitly in the PDG doc

`MN004402A01-A` (Packet Data Gateways) surfaces two field names that
pin down what's actually in the trunked preamble:

- **NSAPI** (Network Layer Access Point Identity) — "Unique identifier
  of the mobile context" (§4.x, p. 4345). This matches the 4-bit NSAPI
  field in octet 0 of the BAEB-C 2-octet SN-Data/SN-UData header. The
  PDG exposes SNDCP Version and NSAPI as subscriber-record fields,
  confirming they're the on-the-wire values the radio uses.
- **LAP-D** (Link Access Protocol — D channel, ITU classic link layer):
  the doc lists `LAP-D T200` (ACK timer), `LAP-D T203` (keep-alive),
  `LAP-D N200` (max retransmissions, range 1–3), `LAP-D N201` (max
  octets in Information frame, default **260**), `LAP-D K` (window
  size). These are the actual link-layer parameters that the
  "LLC User Plane" in `MN005155A01-A` corresponds to. **N201 = 260
  octets default** is the cap on a single Information-frame payload
  below SNDCP.

`MN004402A01-A` §2.4 (p. 44) describes the DSC↔PDG *wireline* hop as
running LAP-D over UDP/IP — not the air link directly. But the same
LAP-D parameter set (T200, T203, N200, N201, K) is exposed as a
subscriber-record field on the air-facing side, which strongly suggests
the air-side LLC uses matching framing, consistent with TIA-102.BAED-A.

### SNDCPv1 vs SNDCPv3

Both versions are supported concurrently. Per `MN005155A01-A` §2.1.1.2
(p. 27), the GGSN router supports SNDCPv1 and SNDCPv3 data
registration; per §2.2.3 (p. 41), an Enhanced-Data-capable subscriber
that is denied on an SNDCPv3 context-activation request retries the
activation using SNDCPv1. APX subscribers therefore negotiate v3 → v1
on denial. If captures from the same radio show two slightly different
preamble shapes across sessions, this version fallback is a candidate
cause.

### Classic Data vs Enhanced Data — two coexisting channel types

`MN005155A01-A` §2.1.4 and §2.2 (see pp. 5, 16, 31) distinguish:

- **Classic Data channel** — unscheduled, carries both confirmed and
  unconfirmed data transmissions. Used for arbitrary-size payloads
  (larger messages, file transfers, CAD queries).
- **Enhanced Data channel** — designed for short periodic inbound
  messages like GPS location reports from many SUs. 384-byte max
  message size including all headers. Half-duplex subscriber operation
  (SU can only transmit *or* receive at a given moment). Typically
  scheduled inbound (reservation-based) rather than contention-based.
- **CCA PDCH (Controlled Channel Access)** — Transit25 variant that
  uses scheduled, unconfirmed transmissions (§2.4.2 p. 52).

Both channel types ride the same LLC/SNDCP/IPv4/UDP stack; the
difference is channel-management, not protocol layering. A message
with ≥355 bytes of user payload is always sent on Classic Data because
355 + 30-byte headers exceeds the 384-byte Enhanced Data cap (§2.1.3
pp. 30–31).

### Point-to-point link between SU and tethered mobile computer

`MN005155A01-A` §2.1.10 (p. 28): Mobile Subscriber Units carry IP to
a tethered mobile computer over either PPP or RNDIS. So below the IP
layer on the *SU↔mobile-computer* hop (not the air link) there's a
PPP or RNDIS framing. That hop is outside the RF domain but relevant
if anyone is looking at the radio's USB/serial output.

### Context Activation is a two-way exchange (unlike conventional)

`MN005155A01-A` §2.1.9.1 (p. 35): context activation is a two-way
request/response exchange between the subscriber, the subscriber's
home Packet Data Router (PDR), and the GGSN. The SU always triggers
the exchange; the infrastructure never initiates it unsolicited.

The SU can request either a static IP (preconfigured specific address)
or a dynamic one (sends `0.0.0.0` to signal DHCP); the GGSN or a
CEN-side DHCP server responds with the assigned address. Header
compression parameters are negotiated in the same exchange (§2.1.3
p. 30): the SU proposes compression parameters and the PDG either
accepts them or counter-proposes.

**Parser implication:** if capturing a session from the beginning, the
context-activation exchange reveals the compression state for the rest
of the session.

### HLR / VLR for data

`MN005155A01-A` §2.1.9.7 (p. 26): the data subsystem maintains a Home
Location Register (HLR) at the home PDR (tracks which zone a
subscriber is homed in) and a Visitor Location Register (VLR) at each
RNG (tracks which subscribers are currently affiliated to a site in
that zone). This mirrors the voice HLR/VLR. Data packets always route
through the subscriber's home PDR even when the subscriber is
roaming.

---

## 3. Trunked IV&D: LLC Is a Real Layer With Its Own Bytes

`MN005155A01-A` §3.1.8 (p. 58) lists RNG-side LLC tunables:

- `LLC Number of Attempts` — RNG retry count.
- `LLC Timer (sec)` — retry wait.
- `LLC User Plane Response Timer` — ACK-wait window.
- `LLC User Plane Window Size` — messages in flight on the sliding window.

The SACK behavior observed on the air is LLC-level. `MN005155A01-A`
§2.1.12 step 6 (p. 39) describes the RNG as reassembling inbound data
segments, checking message-level integrity, and returning either an
ACK (all blocks received cleanly) or a SACK (partial — only certain
blocks were received) before forwarding the datagram to the home PDR.

**Implication for parsing:** If the "preamble" bytes include what looks
like a small sequence number field, that is most likely the LLC user-plane
sequence, *not* SNDCP. This is consistent with TIA-102.BAED-A (LLC spec)
and is why the observed preamble sometimes has bytes that are not
accounted for by the TIA-102.BAEB-C 2-octet header.

---

## 4. Conventional IV&D: SCEP Carries the IP Datagram

**SCEP is a TIA-102 standard, defined in TIA-102.BAEB-B §4.** The
authoritative byte-level reference is the BAEB-B implementation spec
at `standards/TIA-102.BAEB-B/P25_IP_Data_Bearer_BAEB_B_Implementation_Spec.md`
(derived from the full-text extraction of the same directory).
Motorola's vendor docs (`MN004402A01-A` §1.1; `MN003252A01-A`) refer
to the same protocol using the alternate expansion "Simple CAI
Encapsulation Protocol"; the TIA standard name is **Simple
*Convergence* Encapsulation Protocol**. Same protocol.

### 4.1 The most important fact from BAEB-B — SCEP is zero-wrapper

Per BAEB-B §4.1 and §6.4 mapping rules, an IP datagram conveyed over
SCEP is placed **directly** into a P25 CAI logical message. The
carrying message uses SAP = Packet Data and Data Header Offset = 0.
**No SCEP PDU header is prepended to the IP datagram.**

Consequence for a passive decoder: on a pure-SCEP conventional
unicast datagram, the bytes from the BAEB-A/B/C block-reassembly
boundary straight to IPv4 `0x45` are **zero**. Any "extra bytes"
observed between those boundaries come from a different layer —
typically the 13-byte TIA-102.AAAD encryption header on secure
flows, Motorola's registration framing, or application-layer
wrappers (ARS session framing, OTAR envelopes) — never from SCEP
itself. This retires the earlier search for an undocumented
Motorola-proprietary SCEP header layout.

### 4.2 SCEP ARP as the LLID↔IPv4 binding mechanism

The "CAI ID in a header transmitted with each datagram" behavior
cited in `MN003252A01-A` §2.4 (p. 42) is not a per-datagram field —
it is the SCEP ARP binding exchange defined in BAEB-B §4.2. SCEP
ARP adapts RFC 826 Ethernet ARP to P25 by substituting 3-octet P25
LLIDs for 6-octet Ethernet MAC addresses. The 22-octet message
format is fully drawn in the BAEB-B impl spec §2.4. A passive
decoder that captures SCEP ARP exchanges can build the
LLID↔IPv4 table for attribution of subsequent unicast flows.

### 4.3 Tunnel architecture (from Motorola witness docs)

`MN003252A01-A` §2.1.1 (p. 38) describes the ASTRO 25 tunnel
chain: unicast datagrams are carried in CAI format on the
over-the-air link and on the wireline hop between station
equipment and the Conventional RNG through a SCEP tunnel.
Confirmed and unconfirmed delivery services are implemented
between the two tunnel endpoints. Blocking of IP datagrams into
CAI data packets follows TIA-102.BAEB-A for the original Motorola
deployments — BAEB-B and BAEB-C share the same block-layer shape
with BAEB-A for this purpose.

Key facts established by this doc:

- **Tunnel endpoints:** subscriber ↔ Conventional RNG. SCEP framing is
  present on the air-side CAI link *and* on the wireline hop between the
  station and the RNG. After the RNG, the datagram travels inside GTP
  (RNG→GGSN), then IP-in-IP/VPN (GGSN→Border Gateway).
- **Block spec is BAEB-A, not BAEB-C.** This is the older TIA-102 data
  PDU format; the 2-octet SNDCP-style header is *not* present.
- **13-byte encryption header** when secure delivery is used
  (`MN003252A01-A` §2.11.3, p. 45, around the 512/499-byte fragment
  sizing). This is the TIA-102.AAAD block-encryption protocol header, and
  it accounts for the "extra bytes" seen before the IP datagram on
  encrypted conventional captures.
- **Broadcast header carries CAI ID.** §2.4 (p. 42) specifies that each
  broadcast datagram is transmitted with a header containing the
  sending agency's CAI ID. The PDG maps destination IP →
  `255.255.255.255` at the air interface; the SU filters received
  broadcasts against its provisioned agency CAI-ID list. So SCEP's
  wrapper fields include at least one CAI-ID field for broadcast/group
  steering.
- **Registration has two accepted forms.** §2.2.1 (p. 40) notes that
  legacy Motorola Solutions conventional subscribers send a
  proprietary form of Registration Connect/Disconnect; current
  subscribers send the standard TIA-102.BAAD-1 form. The infrastructure
  accepts both, so a parser must handle both.
- **Fragmentation responsibilities:** SU does not fragment IP; mobile
  computer's IP stack does. PDG does not reassemble IP fragments;
  receiving mobile computer reassembles. Outbound fragment size: 512B
  unencrypted / 499B encrypted (13B encryption header delta).
- **Three registration types** (§2.2, p. 40): `Dynamic Registration`
  (SU sends Connect/Disconnect), `Data-Triggered Registration`
  (infrastructure auto-registers on first inbound datagram; SU has no
  way to explicitly deregister), `Manual Registration`. Each subscriber
  record in the PDG is provisioned with exactly one type.
- **Up to 20 Broadcast Data Agencies** per system (§2.4, p. 42). Each
  agency gets its own CAI ID and a statically-assigned IP address (no
  DHCP for broadcast). Each subscriber is provisioned with the CAI IDs
  of the agencies it belongs to and filters received broadcasts locally.
- **Reserved broadcast IDs** (§2.4, pp. 42–43): `0xFFFFFF` = system-wide
  All-Call; `0xFFFFFC` = reserved for ASTRO 25 infrastructure use, do
  not provision for customer agencies.
- **Separate IP addresses for the radio vs the tethered mobile computer**
  (from older `ASTRO_25_Conventional_Systems_System_Planner.pdf`
  pp. 117–118): the radio has its own IP used for control information
  between it and the connected mobile computer; the mobile computer has
  a separate unique IP used as app-layer source/destination. One
  physical radio device therefore has two IP endpoints.
- **APN-based routing at the Conventional GGSN** (§2.1.3, p. 39): when
  an inbound datagram needs to be routed to a CEN, the Conventional
  GGSN selects the tunnel to use based on an Access Point Name (APN)
  configured by NM in the subscriber's record. The provisioned APN is
  associated with the subscriber's IP address and stored at the time
  the GTP tunnel is created.
- **Confirmed vs unconfirmed retry semantics** (cross-doc, older
  `System_Planner.pdf` p. 117 and `MN003252A01-A` §2.1.1 + §2.11.6):
  4-second ACK timeout, RSS-programmable retry count, default 3 retries.
  Group/broadcast messages are always unconfirmed.
- **Unit vs group addressing** (from older `System_Planner.pdf` p. 118):
  each radio has one unique CAI Layer 2 LLID for selective unicast
  delivery, plus up to 8 group LLIDs for group data delivery.
- **Payload size limits.** MN003252A01-A §2.11.3 cites 512B fragments
  (499B with 13B encryption header). The older `System_Planner.pdf`
  p. 117 cites different numbers: "484 bytes outbound from host, 456
  bytes inbound from the mobile terminal" for the IP/FLM payload. The
  newer MN number applies to current ASTRO 25 Conventional; the older
  number reflects the earlier RNC3000/WNG generation.
- **FLM — a non-IP alternative format.** The older
  `System_Planner.pdf` p. 117 documents "Format Logical Messaging
  (FLM)" as a non-IP Motorola-proprietary data format that rides the
  same CAI transport as IP. FLM is not mentioned in MN003252A01-A
  (suggesting current ASTRO 25 conventional may have deprecated it),
  but legacy captures from older systems could show FLM-shaped payloads
  where `0x45` would be for IPv4. Worth keeping in mind as a "this
  doesn't look like IP" possibility on older equipment.

**Byte layout status:** resolved. BAEB-B §4.2 draws SCEP ARP in full;
SCEP's data-carrying form has no header of its own (the IP datagram
rides the CAI logical message directly). `MN003252A01-A` does not draw
the SCEP header because there is no SCEP header to draw — only the
block-layer PDU per BAEB-A/B/C and the TIA-102.AAAD encryption
wrapper for secure flows.

---

## 4a. What the PDG Doc (`MN004402A01-A`) Adds on Top

The 296-page PDG manual surfaces several facts neither feature guide
draws out explicitly:

- **Three separate PDG flavors**, each a distinct VM: Conventional IV&D
  (M or K core), Trunked IV&D (L or M core), HPD (M core). "Each type
  of data service requires a separate PDG" (§1.1 p. 39). A zone can
  host all three concurrently.
- **HPD uses WAI, not CAI** (§2.x p. 43): "Fragmentation of IP datagrams
  into Wideband Air Interface (WAI) protocol data units." HPD's
  air-interface PDU format is distinct from the FDMA/TDMA CAI spec
  families — a separate investigation if decoding HPD captures.
- **LLC SARQ is HPD-only** (§2.x p. 43): "Retransmission of RF message
  segments through LLC Selective Automatic Retry Request (SARQ) (This
  feature applies to the HPD PDG only.)" So Selective ACK/retry at the
  LLC layer is an HPD feature; Trunked IV&D uses the simpler
  LLC/LAP-D retry mechanics described in §3.
- **PDG↔GGSN is GTP over UDP/IP**; PDG↔Zone Controller is multicast
  UDP/IP; PDG↔Network Manager is SNMPv3 (§2.4, p. 44). Consistent with
  what the Trunked guide says but named more precisely.
- **NSAPI and SNDCP Version are per-subscriber configurable fields**
  on the PDG (§4.x pp. 4343–4345, 6028–6030). Exposing NSAPI as
  provisionable confirms it's the same NSAPI field that appears in
  octet 0 of the BAEB-C SN-Data header.
- **LAP-D parameter defaults** (§4.x, multiple locations, pp. 4615–4627,
  4697–4718, 5777–5787):
  - `T200` (ACK timer) — tunable ms
  - `T203` (keep-alive) — tunable ms
  - `N200` (max retransmissions) — range 1–3
  - `N201` (max octets in Information frame) — **default 260**
  - `K` (window size, max outstanding I-frames)

  These are standard ITU Q.921/LAP-D parameters. The N201 = 260 octets
  default caps a single Information-frame payload; IP datagrams larger
  than this must be fragmented at the LAP-D layer.
- **PDG config mostly lives in the RNG/PDR via NM provisioning** —
  the manual is dense with `pdr*` parameters that influence on-air
  timing and queueing. Any byte-layout differences between deployments
  could be caused by different PDG provisioning, so capture-vs-spec
  comparisons should note the target system's PDG generation (M vs K
  core).

---

## 5. Application-Layer Observations

### ARS and CAD share the same preamble because they share the same stack

The observation from the implementor's original question — that port
4005 (ARS) and port 2002 (CAD) show identical preamble bytes — is
consistent with both being UDP applications riding the same
LLC/SNDCP/IPv4/UDP stack (trunked) or the same SCEP/IPv4/UDP stack
(conventional). They differ only in UDP destination port; all preceding
bytes are app-agnostic.

### OTAR is special-cased on conventional

`MN003252A01-A` §1.3 (p. 23) flags OTAR as *not* an IP application
from the SU's perspective. On conventional, OTAR rides the SCEP tunnel
but does not appear as a UDP/IP flow to the subscriber. On trunked,
OTAR is a normal UDP application layered above SNDCP/LLC. Parser
behavior must differ by mode.

---

## 6. Implementation Recommendations

1. **Branch by system mode before parsing.** Conventional → SCEP parser;
   Trunked → LLC+SNDCP parser. A capture's channel context (conventional
   site vs trunked control channel) determines which applies.
2. **Use encrypted captures as parsing baselines for trunked.** Encrypted
   traffic cannot use RFC 2507 compression, so every datagram carries the
   full 30-byte IPv4+UDP+SNDCP header — the cleanest reference case.
3. **Handle SNDCPv1 ↔ SNDCPv3 version negotiation.** Both coexist on
   live systems; APX falls back v3→v1 on denial. Do not assume the
   higher version.
4. **LLC sequence field is likely in the "preamble" bytes on trunked.**
   The observed bytes not accounted for by BAEB-C's 2-octet SNDCP header
   are most likely LLC user-plane overhead (TIA-102.BAED-A).
5. **For the conventional SCEP byte layout, the authoritative source
   is TIA-102.BAEB-B §4** (derived impl spec:
   `standards/TIA-102.BAEB-B/P25_IP_Data_Bearer_BAEB_B_Implementation_Spec.md`).
   SCEP's data-carrying form has no wrapper bytes; SCEP ARP has a
   22-octet message whose layout is fully drawn. SDRTrunk's
   `UnconfirmedDataPacket.java` / `IPPacket.java` serve as
   implementation witness for Motorola-specific behavior *around*
   SCEP (registration-form discriminator emission, legacy
   BAAD-1-vs-proprietary handling), not as the SCEP byte-layout
   source.
6. **13-byte encryption header applies to conventional-mode secure
   traffic.** This is TIA-102.AAAD (Block Encryption Protocol) territory
   and is already documented in our standards tree under
   `TIA-102.AAAD-B/`.

---

## 7. Documentation Evidence Ladder

The Motorola docs reviewed sit on a spectrum of usefulness for the
byte-layout question. Recording which docs had what, so future
investigators don't re-mine the empty ones:

**Highest value — TIA standards (authoritative, clean-room derived):**
- **TIA-102.BAEB-B** (2014-09) — **defines SCEP, including SCEP ARP
  byte layout; defines SNDCP V1/V2/V3 PDU formats and state
  machines.** Authoritative byte-level reference. Derived impl spec at
  `standards/TIA-102.BAEB-B/P25_IP_Data_Bearer_BAEB_B_Implementation_Spec.md`;
  CSV annexes at `annex_tables/sndcp_reject_codes.csv` and
  `annex_tables/sndcp_field_definitions.csv`.
- TIA-102.BAEB-C (2019-12) — SNDCP-V3 spec (supersedes BAEB-B for
  SNDCP V3 only; does not reproduce SCEP).
- TIA-102.BAED-A — LLC / packet-data layer.
- TIA-102.BAJD-A — UDP port assignments for the apps above.

**High value — Motorola implementation witness:**
- `MN004402A01-A` *Packet Data Gateways* (296 pp) — independently
  confirms SCEP is a TIA/APCO standard (prompted the BAEB-B lookup);
  exposes NSAPI + LAP-D parameters as provisionable fields; confirms
  HPD uses WAI and LLC SARQ while Trunked IV&D does not; documents
  all three PDG flavors (Conventional, Trunked, HPD).
  `MN003338A01-B` is an earlier revision of the same manual.
- `MN003252A01-A` *Conventional Data Services* (2016, 76 pp) — names
  SCEP, documents tunnel chain, registration types, broadcast agency
  model, 13-byte encryption header, fragment sizes.
- `MN005155A01-A` *Trunked Data Services Feature Guide* (2018, 80 pp) —
  quantifies the 30-byte IPv4+UDP+SNDCP header budget; documents
  SNDCPv1/v3 negotiation, RFC 2507 compression, context activation,
  LLC user-plane tunables. Confirms what BAEB-B / BAEB-C specify at
  the spec level.
- `ASTRO_25_Conventional_Systems_System_Planner.pdf` (older, redacted in
  some versions) — Element 4 of the IV&D section, pp. 105–130. Documents
  RNC3000/WNG/DIU legacy architecture, FLM as non-IP alternative,
  payload limits (484/456B), dual IP addressing (radio + mobile
  computer), LLID Layer-2 addressing. Useful backward context.

**No value for the byte-layout question (marketing/datasheet grade):**
- `ASTRO_25_Data_Solutions_Brief.pdf` — 12pp marketing
- `ASTRO_25_Packet_Data_Gateway_Spec_Sheet.pdf` — 2pp server datasheet
- `ASTRO_Transport_Networking_Data_Sheet.pdf` — hardware brochure
- `APX_Data_Modem_Tethering_Fact_Sheet.pdf` — LTE offload marketing
- `ASTRO_25_Text_Messaging_Service.pdf` — 4pp brochure
- `apx6000xe_service_manual_68012002026_c.pdf` — RF/DSP manual, no data-stack
  content (grep for sndcp/llc/preamble empty)

**Referenced but not in our collection — prioritized hunt list:**

*Byte-layout question — RESOLVED:*
- ~~`Packet Data Gateways` manual~~ — acquired as `MN004402A01-A`
  (and earlier rev `MN003338A01-B`); promoted to High Value tier.
- ~~**TIA-102 SCEP spec**~~ — **located and processed** as
  TIA-102.BAEB-B §4 (document was already on disk — just not Phase-3
  derived). Derived impl spec produced 2026-04-21. SCEP is
  zero-wrapper in its data-carrying form; the 22-octet SCEP ARP
  message is the only SCEP-specific wire format and is fully drawn.
- `RNC3000 Host Application Programmer's Manual` — cited by the
  older System Planner for the pre-K-core generation. Still useful
  as legacy context; no longer needed for byte layout.
- `RNC3000 Operator's Manual` — same as above.

*Encryption-related (for the 13-byte encryption header and secure-delivery variants):*
- `CAI Data Encryption Module User Guide` — cited multiple times in
  `MN003252A01-A` (pp. 19, 1837, 2571, 2572, 2582). Describes the CDEM
  component that handles encryption at the PDG. Secure-delivery byte
  layout is likely detailed here.
- `Secure Communications Feature Guide` — cited `MN003252A01-A` p. 20.
  System-wide secure comms reference.
- `Key Management Facility User Guide` — cited `MN003252A01-A` p. 20
  and pp. 2378, 2598. Manages keys for encrypted data and OTAR.
- `Encrypted Integrated Data Feature Guide` — cited `MN005155A01-A`
  pp. 393, 1995. Trunked-side EID documentation.

*Call-processing and mobility (for HLR/VLR/registration byte formats):*
- `Call Processing and Mobility Management Feature Guide` — cited
  `MN005155A01-A` pp. 1613, 1624. Documents the HLR/VLR database
  structures that back data registration.

*Related data-services docs (sibling features):*
- `HPD Packet Data Resource Management Reference Guide` — cited in
  both docs (`MN005155A01-A` pp. 399, 3314–3315). High Performance Data
  is the 25 kHz dedicated-data variant; likely parallels the byte-layout
  question for its data stack. **Already in our `recent/` folder** as
  `MN005099A01-A`.
- `Dynamic System Resilience Feature Guide` — cited `MN005155A01-A`
  pp. 388, 762. Context for HA Data behavior under core failover.

*Configuration tools (where UNC/CPS parameters get set — useful for
understanding what the radio negotiates):*
- `Configuration Manager for Conventional Systems User Guide` —
  K-core conventional configuration.
- `Unified Network Configurator User Guide` (Voyence Control + UNCW) —
  network-device configuration including RNG/PDG parameters.
- `Provisioning Manager User Guide` — subscriber-record provisioning.
- `Customer Programming Software (CPS)` online help — radio-side
  subscriber configuration (registration type, IP address, header
  compression parameters).

*Infrastructure hardware:*
- `GGM 8000 System Gateway` manual — gateway router used as GGSN in
  current systems.
- `S6000 and S2500 Routers` manual — older GGSN router line.
- `Virtual Management Server Hardware User Guide` and
  `Virtual Management Server Software User Guide` — VMware host that
  runs the virtualized PDG.

*Note on the `MNxxxxxxA01-A` numbering:* The 2016–2018 docs use this
format (e.g. `MN003252A01-A`, `MN005155A01-A`). The sibling docs
referenced by title should have matching MN numbers — worth searching
motorolasolutions.com or the Motorola Online customer portal by title
to find the current revision. The `recent/` folder already contains
several (`MN005099A01-A` HPD Resource Management, `MN004904A01-B`
System Overview and Documentation Reference, `MN004686A01-AG` Radio
Management System Planner, `MN004285A01-A` ASTRO 25 vCenter Setup) —
check there before hunting externally.

**Implementation witness (not a standard):**
- SDRTrunk source at `src/main/java/io/github/dsheirer/module/decode/p25/
  phase1/message/pdu/` — particularly `UnconfirmedDataPacket.java`,
  `IPPacket.java`, `SNDCPPacket.java`. Useful for observing
  Motorola-specific behavior that sits *around* the TIA byte layout
  (registration-form discriminator emission, legacy
  BAAD-1-vs-proprietary handling, reserved-field values used in the
  field). Not the authoritative byte-layout source — that's BAEB-B /
  BAEB-C / BAED-A.

---

## 8. Open Questions for Follow-Up

**Resolved by BAEB-B processing (2026-04-21):**
- ~~Locate the TIA-102 SCEP spec~~ — **done.** TIA-102.BAEB-B §4
  defines SCEP. The PDF was already on disk; it just needed
  Phase-3 derivation.
- ~~Exact SCEP header layout~~ — **resolved.** No SCEP header
  exists in the data-carrying form (zero wrapper bytes per §4.1
  / §6.4). SCEP ARP byte layout is drawn at §4.2.
- ~~CAI ID "header transmitted with each datagram"~~ — **resolved.**
  This is SCEP ARP, not a per-datagram field. The 22-octet ARP
  message carries LLID + IPv4 binding state; subsequent data
  datagrams rely on the established binding and don't repeat the
  CAI-ID in-band.

**Still open — these are vendor-behavior questions, not spec questions.**

TIA says what the standard wire format is; it does not say what
Motorola-specific extensions an APX radio actually emits in reserved
bits, nor whether SDRTrunk's parser is faithfully showing those bits
vs. introducing artifacts. Each item below needs either a clean live
capture with known context state or SDRTrunk source inspection —
re-reading TIA will not resolve them.

- **Does SCEP use LLC semantics internally in Motorola deployments?**
  `MN003252A01-A` doesn't mention LLC on the conventional side, and
  BAEB-B §4 says SCEP delivers via "confirmed" or "unconfirmed" CAI
  services directly — suggesting BAEB-A block-level ACK/retry rather
  than a BAED LLC layer. **Spec answer is clear; the open question is
  what Motorola's RNG actually does on the wire.** Resolve via:
  (a) end-to-end capture of a conventional confirmed-data exchange
  with retries observable, or (b) SDRTrunk's conventional SCEP parser
  path (if one exists).
- **SNDCPv3 vs SNDCPv1 wire shape on Motorola APX radios.** BAEB-B
  Figures 16–18 (Context Activation Request) and 19–21 (Accept) draw
  the standard formats. **Spec is unambiguous; the open question is
  which variant an APX radio emits for a given context-activation
  attempt, and what reserved/vendor fields (if any) it populates.**
  Resolve via clean capture of a radio's first packet-data session
  after power-up, or SDRTrunk source (`SNDCPPacket.java` family).
- **DCOMP = 6 emission on trunked.** Observed Motorola trunked
  traffic sets `DCOMP = 6`, which is reserved in both BAEB-B and
  BAEB-C. **No spec will resolve this — reserved means reserved.**
  Resolution requires distinguishing:
  (a) A de-facto Motorola vendor extension (the standards-compliant
  alternative in V3 would be MSO/DCOMP-via-SNDCP-option);
  (b) An SDRTrunk parsing artifact misinterpreting the field;
  (c) A capture-chain bit-error that happens to land in that nibble.
  Resolve via clean capture with corroboration from multiple radios
  on multiple systems, plus SDRTrunk source read to see how the
  parser derived the value.

**What a good capture campaign would look like** (if the implementor
wants to close these):
1. Known-good APX radio (firmware version recorded) on a known-good
   trunked system.
2. Capture from power-on through first packet-data session — the
   Context Activation Request/Accept pair reveals the SNDCP version
   negotiated and provisions the compression state.
3. Capture at least one encrypted data exchange on the same session —
   encrypted traffic cannot use RFC 2507 compression, so every
   datagram carries the full 30-byte header stack and `DCOMP` /
   `PCOMP` should be fixed at known values. Any `DCOMP = 6` here is
   definitely vendor behavior, not a compression-state artifact.
4. Cross-check against SDRTrunk's decode of the same capture to
   isolate parser artifacts from real-radio behavior.
