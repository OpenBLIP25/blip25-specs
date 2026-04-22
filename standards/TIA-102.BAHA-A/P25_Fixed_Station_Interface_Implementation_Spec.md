# P25 Conventional Fixed Station Interface (CFSI) — TIA-102.BAHA-A Implementation Spec

**Source:** TIA-102.BAHA-A (June 2017), *Fixed Station Interface
Messages and Procedures*. Supersedes TIA-102.BAHA (original).
Revision A adds **Protocol Version 2 (PV2)** with a dedicated Data
Conveyance service and expanded receiver / transmitter control.

**Document type:** MESSAGE_FORMAT + PROTOCOL. Specifies the
**Conventional Fixed Station Interface (CFSI)** — the link between a
**Conventional Fixed Station Subsystem (CFSS)** (the RF hardware: a
base radio with transmitter / receiver) and a **Conventional Fixed
Station Host (CFSH)** (software inside an RFSS or Console Subsystem).

Two interface variants:
- **AFSI** — Analog Fixed Station Interface (4-wire audio + E&M).
  Legacy, not IP-based.
- **DFSI** — Digital Fixed Station Interface. IP/UDP transport. **The
  primary focus of this impl spec.**

**Not an on-air spec.** CFSI is a wireline link (analog 4-wire or
UDP/IP) between a fixed station and its controlling host. Nothing
defined here appears on Um. For P25 passive decoders, CFSI only
matters if a tool consumes CFSI traffic from a site's internal
network for correlation purposes.

**Scope of this derived work:**
- §1 — CFSS / CFSH architecture
- §2 — AFSI analog interface (4-wire + E&M, Tone Remote Control)
- §3 — DFSI overview: three services (Control, Voice Conveyance, Data Conveyance)
- §4 — DFSI Control Service: 17 FSC messages, state machine, retry protocol
- §5 — DFSI Voice Conveyance Service: RTP payload structure, 18-block catalog
- §6 — Stream framing: Start of Stream, Stream Ack, End of Stream
- §7 — Voice transport: IMBE (14-octet blocks) and PCM μ-law
- §8 — Receiver reports and voter control
- §9 — DFSI Data Conveyance Service (PV2 only)
- §10 — PV1 / PV2 differences summary
- §11 — Cite-to section references
- §12 — Cross-references

**Pipeline artifacts:**
- `standards/TIA-102.BAHA-A/TIA-102-BAHA-A_Full_Text.md` — clean-room
  extraction (copyrighted, git-ignored).
- `standards/TIA-102.BAHA-A/TIA-102-BAHA-A_Summary.txt` — retrieval summary.
- `standards/TIA-102.BAHA-A/TIA-102-BAHA-A_Related_Resources.md`.
- `annex_tables/baha_dfsi_control_messages.csv` — 17-message FSC catalog.
- `annex_tables/baha_dfsi_block_types.csv` — 18-block-type RTP payload catalog.

---

## 1. CFSS / CFSH Architecture

Per BAHA-A §4.

```
┌───────────────────────┐    CFSI (AFSI or DFSI)    ┌──────────────────────┐
│ CFSH                  │◀─────────────────────────▶│ CFSS                 │
│ (Conventional Fixed   │                            │ (Conventional Fixed  │
│  Station Host)        │                            │  Station Subsystem)  │
│                       │                            │                      │
│  - inside RFSS or CS  │                            │  - base radio hw     │
│  - software control   │                            │  - TX / RX RF        │
│  - audio routing      │                            │                      │
└───────────────────────┘                            └──────────────────────┘
                                                               │
                                                               │ Um air interface
                                                               ▼
                                                     (subscriber units)
```

- **CFSH** is the "brain" — typically inside a console subsystem
  (CS) or within an RFSS. Has no direct RF — it controls the CFSS
  over CFSI.
- **CFSS** is the RF hardware — one or more fixed station (base
  radio) units in a site.

One CFSH may control multiple CFSSes (across a site, a region, or
agency-wide). A single CFSS typically serves one CFSH at a time.

---

## 2. AFSI — Analog Fixed Station Interface

Per BAHA-A §4.1. Legacy path; not on modern deployments but still
in the standard for backward compatibility.

### 2.1 Physical layer

- **4-wire audio** path between CFSS and CFSH.
- **E&M signaling**:
  - **E-pair** — PTT from host (CFSH → CFSS).
  - **M-pair** — COR (Carrier Operated Relay, station-heard-a-signal) from CFSS to CFSH.
- **Audio range**: −30 to +10 dBm.

### 2.2 Tone Remote Control (TRC)

For single-4-wire deployments where E&M signaling isn't available, TRC
uses in-band audio tones:

- **HLGT (High Level Guard Tone)**: 2100 / 2175 / 2325 Hz, 120 ms.
- Followed by one of **15 function tones** (650–2050 Hz), 40 ms at
  −10 dB relative to HLGT.
- **LLGT (Low Level Guard Tone)**: −30 dB, mixed with speech.

HLGT signals the start of a control command; the function tone
selects the specific command; LLGT is continuous during keyup.

**Decoder consequence.** An AFSI decoder needs an audio-domain HLGT
/ LLGT detector. Frequency-shift tones would pass through a
straight PCM capture but need narrow-band FIRs to detect cleanly.

---

## 3. DFSI — Digital Fixed Station Interface

Per BAHA-A §4.2. IP/UDP transport. Three parallel logical services:

| Service | Transport | DSCP | Purpose |
|---------|-----------|------|---------|
| **Control Service** | UDP, port **50006** | `100010` (AF41) | Connect / disconnect / station control / voter control |
| **Voice Conveyance Service** | RTP / UDP | `101110` (Expedited Forwarding) | Voice audio (IMBE or PCM), stream framing, receiver reports |
| **Data Conveyance Service** | UDP, **PV2 only** | — | CAI Packet Data (Confirmed and Unconfirmed) |

**Protocol Versions:**

- **PV1** — original; Control + Voice Conveyance services.
- **PV2** — revision A; adds Data Conveyance, new control messages
  (receiver / transmitter / scan / data control), decoded block types
  (LC, ES, LSD, Header word).

PV is negotiated at `FSC_CONNECT`; a conformant implementation
advertises its supported PV and selects the minimum of both sides'.

---

## 4. DFSI Control Service

Per BAHA-A §5. Full catalog: **`annex_tables/baha_dfsi_control_messages.csv`**.

### 4.1 Base header (3 octets)

Every FSC message shares:

```
Octet 0: Message ID
Octet 1: Message Version
Octet 2: Message Correlation Tag
Octet 3+: message-specific body
```

### 4.2 17 FSC messages

**10 common to PV1 and PV2:**

| Message | ID | Purpose |
|---------|-----|---------|
| `FSC_CONNECT` | `$00` | Establish connection |
| `FSC_HEARTBEAT` | `$01` | Liveness (every 30 s; 2 missed = session lost) |
| `FSC_ACK` | `$02` | Success (CONTROL_ACK) or one of 11 NAKs |
| `FSC_SBC` | `$03` | Site Busy Control |
| `FSC_MAN_EXT` | `$04` | Manufacturer Extension |
| `FSC_SEL_CHAN` | `$05` | Select Channel |
| `FSC_SEL_RPT` | `$06` | Select Repeater |
| `FSC_SEL_SQUELCH` | `$07` | Select Squelch mode |
| `FSC_REPORT_SEL` | `$08` | Report current selection |
| `FSC_DISCONNECT` | `$09` | Teardown |

**7 PV2-only:**

| Message | ID | Purpose |
|---------|-----|---------|
| `FSC_SCAN` | `$0A` | Scan control |
| `FSC_RCV_CNTRL` | `$0B` | Receiver: REPORT / NORMAL / SELECT / DISABLE / ENABLE (receiver `0` = all) |
| `FSC_TX_CNTRL` | `$0C` | Transmitter (analogous) |
| `FSC_MBC` | `$0D` | Multicast/Broadcast Control |
| `FSC_START_DATA` / `FSC_STOP_DATA` | `$0E` / `$0F` | Arbitrate CAI between voice and data |
| `FSC_CAI_BUSY_BITS` | `$10` | CAI busy status |

### 4.3 Connection types (FSC_CONNECT body)

FSC_CONNECT specifies one of three connection types:

| Type | Value | Opens |
|------|-------|-------|
| Control | `$10` | Control Service |
| Voice Conveyance | `$20` | Voice Conveyance Service |
| Data Conveyance (PV2) | `$40` | Data Conveyance Service |

Also negotiates `Max_CAI_Data_Blocks` (for Data Conveyance).

### 4.4 FSC_ACK response codes

One success code (`CONTROL_ACK`) plus **11 NAK codes** covering:
already-connected, unsupported version, bad parameters, busy,
connection failure, and other conditions. Full list in BAHA-A §5.7.

### 4.5 Retry protocol

- **`ControlRetryTimer`** — default **500 ms**.
- **`ControlAttemptLimit`** — default **3**.
- After N failed retries, declare connection failed.

### 4.6 State machine (§5.21)

```
CS Not Connected → CS Connecting → CS Connected → CS Disconnecting → CS Not Connected
```

- **Connecting:** CFSH has sent `FSC_CONNECT`, awaiting `FSC_ACK`.
- **Connected:** ACK received; heartbeats in progress.
- **Disconnecting:** `FSC_DISCONNECT` sent, awaiting teardown.

---

## 5. DFSI Voice Conveyance Service

Per BAHA-A §6. Full catalog: **`annex_tables/baha_dfsi_block_types.csv`**.

### 5.1 RTP parameters

- **PT (RTP Payload Type)**: **100** (dynamic).
- **Marker**: 0.
- **Timestamp clock**: 8 kHz.
- **CC** (CSRC count): 0.
- **SSRC**: assigned by the CFSH.

### 5.2 RTP payload structure

```
┌─────────────────────┐
│ Control Octet       │   1 byte; selects compact vs verbose block headers
├─────────────────────┤
│ Block Headers       │   compact (1 byte each) or verbose (4 bytes each)
├─────────────────────┤
│ Block Payloads      │   variable; one per block header
└─────────────────────┘
```

**Compact block header:**
```
Bit 7       0
┌─┬─────────┐
│E│ Block PT│
└─┴─────────┘
```
- `E` (end of block list): `1` = this is the last header; more block payloads follow.
- **Block PT**: 6-bit Payload Type selecting the block shape.

### 5.3 18 Block Payload Types (PT = 0..127)

Consolidated list (see CSV for full detail):

| PT | Name | PV1 | PV2 | Purpose |
|----|------|-----|-----|---------|
| 0 | CAI Voice (IMBE) or PCM | ✓ | ✓ | Voice audio (§7) |
| 2 | Decoded Link Control Word | — | ✓ | Post-decoding LCW |
| 3 | Decoded Encryption Sync | — | ✓ | Post-decoding ESS |
| 4 | Decoded Low Speed Data | — | ✓ | Post-decoding LSD |
| 5 | Decoded Header Word | — | ✓ | Post-decoding HDU |
| 6, 7 | Voice Header Part 1 / Part 2 | ✓ | ✓ | First / last 18 Golay code words |
| 8 | Analog Start of Stream | — | ✓ | Analog stream with CTCSS / CDCSS |
| 9 | Start of Stream with NID | ✓ | ✓ | Reliable NID delivery; repeated until PT=14 Ack |
| 10 | End of Stream | ✓ | ✓ | Termination; 4 copies spaced 100 ms |
| 11 | PTT Control Word | ✓ | ✓ | — |
| 12 | Receiver Report | ✓ | ✓ | Per-receiver status (NO_SIGNAL / SELECTED / …) |
| 13 | Voter Control | ✓ | — | PV1; superseded by FSC_RCV_CNTRL |
| 14 | Stream Acknowledge | ✓ | ✓ | ACK for PT=9 |
| 20 | Transmitter Report | — | ✓ | Per-TX status (PV2+ optional) |
| 63–127 | Manufacturer Specific | ✓ | ✓ | Vendor-proprietary |

### 5.4 PV2 decoded blocks (PT 2–5) — why they exist

PV1 passed encoded IMBE frames and decoded the LC / ESS / LSD on the
CFSH side. PV2 optionally **pre-decodes** them at the CFSS and
transmits the decoded fields as structured blocks. This offloads
decoding work and enables CFSS-side filtering.

**Decoder implication for CFSH tooling.** PV2 CFSH code needs both
paths: accept the IMBE stream (PT=0) and also accept the pre-decoded
LC / ESS / LSD / Header blocks (PT=2..5) if the CFSS advertises
decoded mode.

---

## 6. Stream Framing

Per BAHA-A §6 and §7.

### 6.1 Start of Stream (PT=9 with NID; PT=8 analog)

- Carries **16-bit NID (NAC + DUID)** + 4-bit error count.
- **Sent repeatedly** until a `PT=14 Stream Acknowledge` is received.
  This ensures reliable NID delivery — if a single SoS packet is
  lost, the stream still begins correctly on the next copy.
- CFSH sends an SoS with every LDU1 during active transmission.

### 6.2 Stream Acknowledge (PT=14)

CFSS responds with Stream Ack once it's processed the SoS. After
that, the CFSH can stop repeating SoS (or continue at LDU1 cadence).

### 6.3 End of Stream (PT=10)

- **Both ends send 4 copies** of End of Stream, spaced **100 ms apart**.
- CFSS de-keys its transmitter on EoS receipt, or after a **~4-second
  timeout** with no incoming voice packets — whichever comes first.

---

## 7. Voice Transport Details

### 7.1 IMBE (PT=0, 14 octets)

Each CAI Voice block is a **fixed 14-octet** IMBE frame carrying:

| Field | Encoding |
|-------|----------|
| U0–U6 | Golay / Hamming decoded and error-corrected from the CAI |
| U7 | Unprotected |
| Et, Er, E4, E1 | Error counts |
| M | Mute flag |
| L | Lost-frame flag |
| SF | 2-bit super frame counter |
| B | CAI busy status bit |

### 7.2 18 IMBE frame types (PID/FT $62–$73)

The **Frame Type** in octet 0 identifies the superframe position
and the supplemental data layout:

| FT | Frame # | Supplemental |
|----|---------|--------------|
| `$62` / `$63` | 1 / 2 | none (only IMBE) |
| `$64`–`$69` | 3–8 | **Link Control** — 4 of 24 RS-encoded code words |
| `$6A` | 9 | **Low Speed Data** — LSD0 / LSD1 |
| `$6B` / `$6C` | 10 / 11 | none (only IMBE) |
| `$6D`–`$72` | 12–17 | **Encryption Sync** — 4 of 24 RS-encoded code words |
| `$73` | 18 | **Low Speed Data** — LSD2 / LSD3 |

Each variant appends its extra data + a `STATUS` field encoding
Hamming decode error counts for each code word.

**Cross-reference:** the same superframe structure appears in BACE
Block Type 0 IMBE Voice. The two specs intentionally share this
framing — BACE Block Type 5 "ISSI Header Word" is a direct copy from
BACA; BAHA's Frame Types match BAAA-B's §5.4–§5.6 LDU layout.

### 7.3 PCM transparent analog (PT=0, 160 octets)

G.711 μ-law, 160 samples assembled every 20 ms. Block Type 0 with
`E=0` (PCMU per RFC 3551).

### 7.4 Voice Header split (PT=6 / PT=7)

Voice headers (HDU) contain 36 Golay code words; BAHA-A splits them
across two payload types:

- **PT=6 Voice Header Part 1**: first 18 code words (G0–G17).
- **PT=7 Voice Header Part 2**: last 18 code words (G18–G35).

Rationale: keeping each header packet small reduces peak latency
when sending the full header plus voice blocks in a single RTP
packet.

---

## 8. Receiver Reports and Voter Control

### 8.1 Receiver Report (PT=12)

Per BAHA-A §6.16.

Multiple receivers may be reported in a single RTP packet. Status
codes:

- `NO_SIGNAL` / `SELECTED` / `GOOD_P25` / `GOOD_FM` / `BAD_P25` /
  `BAD_FM` / `NOT_EQUIPPED` / `FAILED`

**Cadence:** on status change AND periodically at the **Voter
Reporting Period** (default **10 s**).

### 8.2 Voter Control — PV1 (PT=13) vs PV2 (FSC_RCV_CNTRL)

- **PV1** used in-band Block Type 13 (Voter Control) for
  REPORT / NORMAL / SELECT / DISABLE commands.
- **PV2** replaces this with the out-of-band control message
  `FSC_RCV_CNTRL` (§4.2). Block Type 13 is **not used** in PV2.

### 8.3 Transmitter Report (PT=20) — PV2+

Optional. Per-transmitter status with codes analogous to receiver
report. Sent on-change and periodically.

---

## 9. DFSI Data Conveyance Service (PV2 only)

Per BAHA-A §8.

### 9.1 Connection

Opened via `FSC_CONNECT` with connection type `$40` **after the
Control connection is active**. Separate UDP port from Control.

### 9.2 Data Conveyance message format

```
┌─────────────────────┐
│ DC Header           │   Message ID=$40, Version=$01, Correlation Tag, DC Flags
├─────────────────────┤
│ DC Payload Header   │   block count, block type octets, CAI error count octets
├─────────────────────┤
│ DC Block Payloads   │   one or more CAI Packet Data blocks
└─────────────────────┘
```

### 9.3 DC Flags — reliable delivery handshake

| Flag | Purpose |
|------|---------|
| **FSI Transport Request / Ack** | Confirms FSI delivery (layer 3 reliability) |
| **CAI Bundle Sent Request / Ack** | Confirms actual CAI RF transmission (layer 1 reliability) |
| **NID flag** | CFSS can request CFSH supply a NID via a SoS block |

### 9.4 DC Block Types (Table 19)

| Block Type | Name |
|------------|------|
| 9 | Start of Stream with NID |
| 10 | End of Stream |
| 12 | Receiver Report |
| 20 | Transmitter Report |
| 31 | Confirmed Data Header |
| 32 | Confirmed Data Block |
| 33 | Confirmed Data Last Block |
| 34 | Confirmed Response |
| 35 | Unconfirmed Data Header |
| 36 | Unconfirmed Data Block |
| 37 | Unconfirmed Data Last Block |

Confirmed / Unconfirmed payloads follow **CAI Packet Data formats
from BAAA-B** — this is just the wire transport; the PDU framing
is unchanged.

### 9.5 Max blocks per DC message

Negotiated via `FSC_CONNECT` / `FSC_ACK` — CFSH and CFSS each
specify inbound / outbound maximums; the **effective limit is the
minimum of the two**. A single packet data message may span **up to
128 CAI data blocks** across multiple DC messages.

### 9.6 CFSS NID management modes

Three configurable modes:
1. **Self-manage only** — CFSS tracks NID; ignores any CFSH-supplied NID.
2. **Use CFSH NID if available** — CFSH can push NID via SoS; fall
   back to self-managed otherwise.
3. **Use CFSH NID only** — CFSS requires CFSH-supplied NID (via the
   NID request flag); no self-management.

---

## 10. PV1 vs PV2 Summary

| Feature | PV1 | PV2 |
|---------|-----|-----|
| Control Service | ✓ | ✓ (+ 7 new messages) |
| Voice Conveyance Service | ✓ | ✓ (+ decoded blocks PT=2–5) |
| Data Conveyance Service | — | ✓ |
| Voter Control | in-band PT=13 | out-of-band FSC_RCV_CNTRL |
| Transmitter Reports | — | optional PT=20 |
| Analog Start of Stream | — | PT=8 |

**Interop rule:** an implementation advertises supported PV in
`FSC_CONNECT`. Mixed-PV deployments use the minimum.

---

## 11. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify:

- CFSI architecture (CFSS / CFSH) — TIA-102.BAHA-A §4.
- AFSI (4-wire + E&M + TRC) — §4.1.
- DFSI services and transport — §4.2.
- DFSI Control Service network params — §5.1.
- Control message set (Table 5) — §5.3.
- Control Message Base Header — §5.4.
- FSC_CONNECT — §5.5.
- FSC_HEARTBEAT — §5.6.
- FSC_ACK response codes — §5.7.
- PV2 control messages (FSC_SCAN / RCV_CNTRL / TX_CNTRL / MBC /
  START_DATA / STOP_DATA / CAI_BUSY_BITS) — §5.15–§5.20.
- Control Service state machine — §5.21.
- Voice Conveyance Service network params — §6.1.
- RTP payload structure — §6.2.
- Block Payload Types (Table 13) — §6.3.
- CAI Voice (PT=0, IMBE) — §6.4.
- CAI Frame Types (Table 14) — §6.5.
- PV2 decoded blocks (PT=2..5) — §6.6–§6.9.
- Voice Header (PT=6 / PT=7) — §6.10 / §6.11.
- Analog Start of Stream (PT=8) — §6.12.
- Start of Stream with NID (PT=9) — §6.13.
- End of Stream (PT=10) — §6.14.
- Receiver Report (PT=12) — §6.16.
- PV1 Voter Control (PT=13) — §6.17.
- Transmitter Report (PT=20) — §6.19.
- Voice Conveyance procedures (connection through termination) — §§7.1–7.8.
- Data Conveyance (PV2 only) — §8, Figure 72 (DC message format).
- Data Conveyance Block Types (Table 19) — §8.3.

---

## 12. Cross-References

**Upstream (this doc depends on):**
- `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md`
  — CAI physical / MAC; IMBE frame structure; LC / ES / LSD code-word layouts.
- `standards/TIA-102.BABA-A/P25_Vocoder_Implementation_Spec.md` —
  IMBE vocoder specification.
- `standards/TIA-102.AABF-D/P25_Link_Control_Word_Implementation_Spec.md`
  — LCO values for PT=2 Decoded Link Control Word.
- `standards/TIA-102.AAAD-B/P25_Block_Encryption_Protocol_Implementation_Spec.md`
  — MI / ALGID / KID for PT=3 Decoded Encryption Sync and PT=5 Decoded Header.

**Companion specs (adjacent interfaces):**
- `standards/TIA-102.BACE/P25_ISSI_Conventional_Implementation_Spec.md`
  — Conventional ISSI (Ec interface); reuses the same RTP payload
  framework with additional Block Types 16–30 for console-specific
  controls.
- **TIA-102.BACA-B** — Trunking ISSI; defines Block Types 1, 11, 15
  that BACE + BAHA share.

**External references:**
- **RFC 3550** (RTP), **RFC 3551** (RTP A/V Profile).
- **ITU-T G.711** (μ-law PCM).
- **TIA/EIA-464** (analog Tone Remote Control standards reference).

**Supporting annex tables:**
- `annex_tables/baha_dfsi_control_messages.csv` — 17-row FSC catalog.
- `annex_tables/baha_dfsi_block_types.csv` — 18-row RTP block-type catalog.
