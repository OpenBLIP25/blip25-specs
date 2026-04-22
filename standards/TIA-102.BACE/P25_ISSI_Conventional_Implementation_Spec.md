# P25 ISSI for Conventional Operation (Ec) — TIA-102.BACE Implementation Spec

**Source:** TIA-102.BACE (June 2008, reaffirmed January 2013), *Inter
RF Subsystem Interface (ISSI) for Conventional Operation — Messages
and Procedures*.

**Document type:** MESSAGE_FORMAT + PROTOCOL. Specifies the
**Ec interface** — the IP-based protocol between a **Console
Subsystem (CS)** (dispatch console) and a **Conventional Access
Radio (CAR)** (IP-to-RF gateway for conventional P25 fixed stations).
Ec is the conventional counterpart to the trunking ISSI (BACA-B),
adapted for simplex and half-duplex conventional channels.

**Scope:** defines the wire protocol, block types carried in RTP
payloads, call-control state machines, transmission-control state
machines, and timer / counter definitions. Normative for any
multi-vendor deployment of dispatch consoles and CARs.

**Not an on-air spec.** Ec is a wireline IP protocol. None of its
content appears on Um. A passive Um decoder that wants to cross-
correlate with a dispatch console would ingest Ec traffic off a
mirror port — BACE defines the protocol to parse.

**Scope of this derived work:**
- §1 — Architecture: CS, CAR, Fixed Station
- §2 — Transport: RTP/UDP, IPv4 + IPv6
- §3 — 21 RTP payload block types — catalog + key specimens
- §4 — Transmission Control (TC) protocol: packet types, TSN, Losing Audio
- §5 — Call Control (CC) protocol: 8 state machines
- §6 — PTT arbitration and priority preemption
- §7 — Voter control
- §8 — Connection establishment: request-retry-heartbeat
- §9 — Encryption: MI / AlgID / Key ID / MFID in block type 5 (ISSI Header Word)
- §10 — Key timing rules (IMBE bundling, PCM bundling, signal bit)
- §11 — Cite-to section references
- §12 — Cross-references

**Pipeline artifacts:**
- `standards/TIA-102.BACE/TIA-102-BACE_Full_Text.md` — clean-room
  extraction (copyrighted, git-ignored).
- `standards/TIA-102.BACE/TIA-102-BACE_Summary.txt` — retrieval summary.
- `standards/TIA-102.BACE/TIA-102-BACE_Related_Resources.md`.
- `annex_tables/bace_block_types.csv` — 21-row catalog of RTP payload
  block types.

---

## 1. Architecture

Per BACE §1.

```
┌───────────────────────┐   Ec interface   ┌─────────────────────┐   FSI (BAHA)   ┌───────────────────┐
│ Console Subsystem (CS)│◀────────────────▶│  Conventional       │◀──────────────▶│  Fixed Station(s) │
│  - Dispatch host      │   RTP / UDP / IP │  Access Radio (CAR) │                │  - Base repeaters │
│  - UI / audio routing │                  │  - IP↔RF gateway    │                │  - Conv receivers │
└───────────────────────┘                  │  - RTP mixer        │                └───────────────────┘
                                            └─────────────────────┘
```

- **Console Subsystem (CS)** — dispatch console host. Initiates
  connections, controls stations, requests PTT, routes audio to /
  from operator positions.
- **Conventional Access Radio (CAR)** — the IP-to-RF gateway. Manages
  one or more fixed stations via the **Fixed Station Interface
  (FSI)** per TIA-102.BAHA. Acts as an **RTP mixer** for audio routing
  between multiple consoles and the fixed station.

**Fan-in / fan-out rules:**

- **One CAR may serve multiple fixed stations** simultaneously.
- **Multiple CSes may connect to the same CAR for the same fixed
  station** — multiple dispatcher positions can monitor the same
  channel.
- **Only one CS may transmit at a time** on a given fixed station —
  PTT arbitration by priority (§6).

---

## 2. Transport

Per BACE §3.1. The protocol is **transport-agnostic at the IP layer**:

- **IPv4 or IPv6** — both supported.
- **UDP** via **RTP** (per RFC 3550).
- **RTP payload carries a sequence of "blocks"** — 21 block types
  defined (§3).

**Two logical services** share the RTP stream but typically run on
separate UDP ports or IP addresses:

| Service | Packet types carried | Purpose |
|---------|----------------------|---------|
| **Control Service** | All TC packet types except PTT Progress and TC Heartbeat | Connection management, station control, call control |
| **Voice Conveyance Service** | PTT Progress (with IMBE / PCM audio) and TC Heartbeat only | Voice audio transport |

This split mirrors the RTP design pattern where control and media
flow on distinct sockets.

---

## 3. RTP Payload Block Types (21 types)

Full machine-readable catalog: **`annex_tables/bace_block_types.csv`**.

### 3.1 Block categories

| Range | Category |
|-------|----------|
| **Type 0** | Voice (IMBE digital or μ-law PCM analog) |
| **Type 1** | Transmission Control header |
| Types 2–4 | Call parameters / session params |
| **Types 5–7** | Conventional Header + IMBE Voice Headers |
| Types 16–19 | PTT control, source identification, link control |
| Types 21–23 | Station control (channel, repeat, squelch, CTCSS, RF mode) |
| Types 24–26 | Single Block Commands, Voter Command/Status |
| Types 27–30 | Connection management (Connect Req, Heartbeat, ACK, Disconnect) |
| **Types 63–127** | Manufacturer-specific (MFID + Length + data) |

### 3.2 Block Type 0 — Voice (the most-carried block)

The E bit (Encoded flag) distinguishes **IMBE voice** (`E = 1`) from
**PCM audio** (`E = 0`):

#### 3.2.1 IMBE Voice Blocks (`E = 1`, per BACE §3.1.2.1)

18 octets each. **Frame Type** field in octet 0 identifies which
IMBE frame in the 18-frame superframe and selects the supplemental
data layout:

| Frame Type | Meaning | Supplemental data |
|------------|---------|-------------------|
| `$62` / `$63` | Frames 1 / 2 | None (reserved bytes 14–17) |
| `$64`–`$69` | Frames 3–8 | **Link Control** — 4 of 24 6-bit RS-encoded code words per frame (per BAAA-B §5.5, decoded via AABF-D) |
| `$6A` | Frame 9 | **Low Speed Data** — 2 octets (LSD0 / LSD1) |
| `$6D`–`$72` | Frames 12–17 | **Encryption Sync** — 4 of 24 6-bit RS-encoded code words per frame (per BAAA-B §5.4) |
| `$73` | Frame 18 | **Low Speed Data** — 2 octets (LSD2 / LSD3) |

Reserved for future use: other frame types not in the above list.

**Error status encoding (octet 17 of LC / ES frames):** base-3
encoding of per-code-word error status:
```
STATUS = ST3 * 3^3 + ST2 * 3^2 + ST1 * 3^1 + ST0 * 3^0
```
where `STn ∈ {0, 1, 2}` means `{no errors, 1 error, erasure}` for
code word n.

#### 3.2.2 PCM Audio Blocks (`E = 0`)

160-octet μ-law PCM per 20 ms (G.711 encoding, 8 kHz / 8-bit μ-law).
Used for analog RF mode or analog FSI connections where the CAR
doesn't have an IMBE vocoder in the data path.

### 3.3 Block Type 1 — TC Packet Type

2-octet Transmission Control header:

```
Bit 7                              0
┌───────────────────────────────────┐
│ Packet Type (4b) │ Reserved (4b) │   Octet 0
├───────────────────────────────────┤
│ TSN (7b)                   │ L   │   Octet 1 (TSN 7 bits, Losing Audio bit in LSB)
└───────────────────────────────────┘
```

**Packet Type values:**

| Value | Meaning |
|-------|---------|
| `0` | PTT Request |
| `1` | PTT Grant |
| `2` | **PTT Progress** (carries voice) |
| `3` | PTT End |
| `4` | PTT Start |
| `5` | Mute |
| `6` | Unmute |
| `7` | Wait |
| `8` | Deny |
| `9` | **TC Heartbeat** |
| `11` | PTT Transmit Data |

**TSN (Transmission Sequence Number):** 7-bit identifier for a PTT
session. See §4.

**Losing Audio bit (L):** marks this stream as the "losing" side
when two PTT streams overlap; CAR marks the non-selected stream
L=1. Receivers may use this to suppress audio playback for losing
streams.

### 3.4 Block Types 5, 6, 7 — ISSI Header Word and Voice Header

**Block Type 5 (Conventional Header Word):** a **direct copy of the
ISSI Header Word from BACA** (trunking ISSI). Carries:
- MI (Message Indicator)
- AlgID (Algorithm ID)
- Key ID (KID)
- MFID (Manufacturer ID)
- Group ID
- NID (Network Identifier)

This is the structure that lets a CS know the encryption state of an
incoming transmission before voice blocks arrive.

**Block Types 6, 7:** IMBE Voice Header Parts 1 and 2 — the TIA-102
voice-header content unpacked for digital subscriber transmissions.

### 3.5 Block Types 21–23 — Station Control

Per-fixed-station state and commands:

- **Channel selection**
- **Repeat mode** (on / off / per-channel)
- **Squelch mode** — including **CTCSS code selection A–H**
- **Second receiver** enable
- **Protected mode** (encryption-only lockout)
- **Wildcard** (any-channel scan)
- **RF mode** — digital-only, analog-only, mixed

**Full report request/response cycle** — CS sends a report request
(Type 21), CAR returns the current state (Type 22).

### 3.6 Block Types 25–26 — Voter

Used on multi-receiver conventional sites where several receivers
simultaneously decode the same transmission; a **voter** arbitrates
which decoded stream is forwarded.

**Voter Commands (Type 25):**
- `REPORT` — request current voter state
- `NORMAL` — resume automatic voting
- `SELECT` — force a specific receiver
- `DISABLE` — drop a receiver from consideration

**Voter Status (Type 26):**
- `NO_SIGNAL` / `SELECTED` / `GOOD_P25` / `GOOD_FM` / `BAD_P25` /
  `BAD_FM` / `NOT_EQUIPPED` / `FAILED`

### 3.7 Block Types 27–30 — Connection Management

**Connection Request (Type 27):** includes port negotiation — the CS
proposes UDP ports for the Control Service and Voice Conveyance
Service, the CAR may accept or counter-propose.

**Control Service Heartbeat (Type 28):** periodic liveness check.
Configurable via `TControlHBTimer`; if `NControlHBLoss` consecutive
heartbeats are missed, the session is declared failed.

**Control Service Acknowledgement (Type 29):** ACK / NAK response
code set (codes 0–9). Enables richer error reporting than a binary
success / fail.

**Disconnect Request (Type 30):** explicit session teardown.

### 3.8 Block Types 63–127 — Manufacturer Specific

`MFID (1B) | Length (variable) | data`. Vendor-proprietary
extensions. BACE encourages manufacturers to propose general-use
packet types to TIA for standardization.

---

## 4. Transmission Control (TC) Protocol

Per BACE §5.

### 4.1 TSN (Transmission Sequence Number) — 7 bits

**Parity split:**
- **Console Subsystems use even TSNs** (≥ 2).
- **CARs use odd TSNs.**
- **TSN 0 is reserved** for TC Heartbeats during muted transmissions.

**TSN advance rule:** `next_TSN = (TSN + 2) mod 128, skipping 0`.
Both parties increment their own TSN by 2 each new transmission.

**Processing rule:** all RTP packets for a given TSN are processed
in RTP sequence-number order. Late-arriving packets (sequence number
before the current processed position) are **discarded**.

### 4.2 Four TC State Machines

| Role | States |
|------|--------|
| **Console Send** | Requesting → Transmitting |
| **Console Receive** | Receiving |
| **CAR Transmit** | (CAR is always ready; state driven by inbound RF) |
| **CAR Receive** | Arbitrating → Receiving / Wait / Deny |

### 4.3 Voice Conveyance vs Control Service

- **Voice Conveyance Service carries ONLY:** PTT Progress (with
  voice blocks) and TC Heartbeat.
- **Control Service carries everything else:** PTT Request / Grant /
  End / Start / Mute / Unmute / Wait / Deny and all non-voice
  blocks.

Keeping voice and control on separate sockets simplifies QoS and
prevents control-plane congestion from affecting voice jitter.

### 4.4 Signal bit

Per BACE §5.3. The Signal bit in the TC header indicates which
phase of a transmission the packet belongs to:

| Phase | Signal bit |
|-------|------------|
| PTT Request | `0` |
| PTT Grant, PTT Progress | `1` |
| PTT End | `0` |

---

## 5. Call Control (CC) Protocol

Per BACE §4.

Eight state machines handle four services, each split by endpoint role:

| Service | CS-side state machine | CAR-side state machine |
|---------|------------------------|------------------------|
| **Connection Establishment** | CS Connection | CAR Connection |
| **Station Control** | CS Station | CAR Station |
| **Single Block Control** | CS SBC | CAR SBC |
| **Voter Control** | CS Voter | CAR Voter |

**Connection Establishment** uses a request-retry-heartbeat pattern:
- `NControlRetry` — max retry count.
- `TControlRetryTimer` — inter-retry interval.
- `TControlHBTimer` — heartbeat period after session established.
- `NControlHBLoss` — consecutive missed heartbeats before declaring
  session lost.

Default values live in BACE Annex A (timer table).

---

## 6. PTT Arbitration and Priority Preemption

Per BACE §5.7.

The **CAR is the arbiter.** When multiple CSes request PTT on the
same fixed station:

1. The CAR inspects the **TX Priority field** from each request.
   Range: `$10` (lowest) through `$F0` (highest).
2. Highest priority wins.
3. Ties are resolved by the CAR's local policy (typically first-come-first-served).

**PTT Wait vs PTT Deny** — the CAR has two ways to decline:
- **PTT Wait** — defers the decision; the CS should wait and retry.
- **PTT Deny** — refuses outright; the CS should not retry without
  operator intervention.

**Console Priority Preemption:** a higher-priority CS can take over
from a lower-priority one already transmitting. The CAR sends
`NTCResend` copies of the PTT End packet to the losing CS to ensure
it learns about the preemption. Then the higher-priority CS is
granted.

---

## 7. Voter Control

Per BACE §§3.1.2 Types 25–26. Applies only on multi-receiver sites
where the CAR has a voter subsystem.

**Internal voter hardware behavior is out of scope** for BACE — only
the ISSI-visible commands and status updates are specified. Vendors
implement the voting algorithm internally.

**Command path:** CS → CAR. Status path: CAR → CS. A voter may
emit unsolicited status updates when its internal state changes
(e.g., a selected receiver drops to BAD_P25).

---

## 8. Timing Rules for Voice Transport

Per BACE §§3.2, 5.5, 5.6.

### 8.1 IMBE bundling

- **1–3 IMBE blocks per PTT Progress packet** — up to 60 ms of audio.
- **All IMBE-bearing packets for a given TSN carry the same block
  count** — do not vary bundling mid-transmission.
- **IMBE1 (frame 1 of a superframe)** must be first in any packet
  that contains it.
- **IMBE18 (frame 18) and IMBE1 (frame 1)** must NOT be in the same
  packet — they belong to different superframes.

### 8.2 PCM bundling

- **1–3 PCM blocks per PTT Progress packet** — up to 60 ms of audio
  (20 ms per block).

### 8.3 Losing Audio rule

When two PTT streams overlap on the TIA-102 side of the CAR, the
non-selected stream is marked **L=1 (Losing)** in its TC packets.
Receiving CSes may mute L=1 streams to prevent operator confusion
from hearing two overlapping speakers.

---

## 9. Encryption (Block Type 5)

The **ISSI Header Word** (Block Type 5) carries the crypto state:

| Field | Source spec |
|-------|-------------|
| MI (Message Indicator) | AAAD-B |
| AlgID | BAAC-D |
| Key ID (KID) | BAAC-D |
| MFID | BAAC-D |
| Group ID | BAAC-D / per-deployment provisioning |
| NID | BAAA-B |

This is the **same field layout as BACA** (trunking ISSI) — hence
"direct copy of the ISSI Header Word from BACA." A CS that already
parses trunking ISSI Header Words can reuse the parser for
conventional ISSI.

---

## 10. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify:

- Architecture and CAR/CS roles — TIA-102.BACE §1.
- Normative references — §2.
- RTP payload definitions — §3.
- RTP packet structure — §3.1.
- P25 Block Types — §3.1.2.
- IMBE Voice Block and per-frame variants — §3.1.2.1 and subsections.
- TC Packet Type — §3.1.2.2.
- Block Types 5–7 (ISSI Header / IMBE Voice Headers) — §3.1.2.
- Construction of RTP packets — §3.2.
- Call Control — §4.
- CC-SAP — §4.1.
- Call Control state machines (8 total) — §4.2.
- Transmission Control — §5.
- TC-SAP — §5.1.
- TC state machines (4 total) — §5.2.
- TC packets and voice conveyance / control services — §5.3.
- RTP header management — §5.4.
- TSN management — §5.5.
- Losing Audio — §5.6.
- Transmission Control procedures (PTT arbitration, preemption) — §5.7.
- Timer and counter definitions — §6 / Annex A.

---

## 11. Cross-References

**Upstream (this doc depends on):**
- **TIA-102.BACA-B** — parent trunking ISSI spec. Block Type 5 (ISSI
  Header Word) is a direct copy. **Not yet processed in this repo.**
- `standards/TIA-102.BAHA-A/…` (pending processing) — FSI (Fixed
  Station Interface); the CAR's interface to fixed stations.
- `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md`
  — IMBE frame structure (§5.5), link-control code-word encoding
  (§5.4–5.6), HDLC data units.
- `standards/TIA-102.AABF-D/P25_Link_Control_Word_Implementation_Spec.md`
  — LCO values and LC data blocks referenced by IMBE Frames 3–8.
- **TIA-102.AABC-B** — TSBK opcodes; Single Block Control uses them.
  (Repo has AABC-E processed, which is AABC-B's successor.)
- `standards/TIA-102.AABG/…` — Conventional Control Messages; SBC
  also references these.
- `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` —
  IMBE vocoder specification.
- **RFC 3550** (RTP), **RFC 3551** (RTP/AVP Profile), **ITU-T G.711**
  (μ-law PCM).

**Companion P25 ISSI work:**
- `standards/TIA-102.BACA-B-3/P25_ISSI_IWF_Interworking_Implementation_Spec.md`
  — IWF interworking addendum for the trunking ISSI.
- **TIA-102.BACF** — ISSI Messages and Procedures for Packet Data.
  (Phase 2 run in background during this processing pass.)

**Supporting annex table:**
- `annex_tables/bace_block_types.csv` — 21-row catalog of RTP payload
  block types with category, purpose, and source section.
