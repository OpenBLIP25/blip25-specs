# AMBE-3000 USB-3000™ Packet Protocol Specification

**Sources:**
- DVSI AMBE-3000F™ Vocoder Chip Users Manual v4.0 (October 2021) — canonical chip reference
- DVSI USB-3000™ Manual — host-side USB adapter
- DVSI Designer's Notes v1.1 (April 2017) — timing and integration guidance
- DVSI Soft Decision Error Decoding (2022) — 4-bit soft decision encoding
- Columbia University libambe (2019-2020, GPLv3) — reference open-source implementation

**Purpose:** Complete packet protocol reference for communicating with the AMBE-3000
vocoder chip, whether directly or via the USB-3000 USB adapter. The same packet format
is also used by the AMBE-4020 (2014+) and AMBE-3003/AMBE-3012.

**Scope:** This document describes the wire format. For operational sequencing (reset
timing, init ordering, errata workarounds), see `AMBE-3000_Operational_Notes.md`.

---

## 1. Physical Interfaces

The AMBE-3000 chip supports three mutually-exclusive physical interfaces selected at
boot via configuration pins:

| Interface | Use case |
|-----------|----------|
| UART | Low-pin-count, serial MCU connection (most common for USB-3000) |
| Parallel Port (PPT) | Higher throughput, more pins |
| McBSP serial port | Synchronous serial with DSPs |

USB adapter variants wrap a UART/FTDI interface on top:

| Adapter | Channels | Baud rate | USB chip |
|---------|----------|-----------|----------|
| USB-3000 | 1 | 460,800 | FTDI FT232R |
| USB-3003 | 3 | 921,600 | FTDI FT232R |
| USB-3012 | 12 | 921,600 | FTDI FT4232H |

All USB variants use RTS/CTS hardware flow control.

The packet format is identical regardless of physical interface.

---

## 2. Packet Format

Every packet begins with a 4-byte header. Parity is optional and **enabled by default
after reset**.

### 2.1 Packet without parity

```
┌────────────┬──────────┬──────────┬──────────────────┐
│ START_BYTE │  LENGTH  │   TYPE   │      FIELDS      │
│  1 byte    │ 2 bytes  │  1 byte  │  L bytes         │
│  0x61      │ big-endian│          │                  │
└────────────┴──────────┴──────────┴──────────────────┘
```

### 2.2 Packet with parity

```
┌────────────┬──────────┬──────────┬───────────┬───────────┬──────────┐
│ START_BYTE │  LENGTH  │   TYPE   │  FIELDS   │  0x2F     │  XOR     │
│  1 byte    │ 2 bytes  │  1 byte  │  L bytes  │  1 byte   │  1 byte  │
└────────────┴──────────┴──────────┴───────────┴───────────┴──────────┘
```

### 2.3 Field definitions

| Field | Bytes | Notes |
|-------|-------|-------|
| START_BYTE | 1 | Always `0x61` (ASCII 'a') |
| LENGTH | 2 | Big-endian. Count of bytes **after** the header — includes all FIELDS bytes plus the 2-byte parity trailer (when present). Excludes the 4-byte header itself. |
| TYPE | 1 | `0x00` control, `0x01` channel, `0x02` speech |
| FIELDS | variable | One or more TLV fields (Section 3) |
| Parity (optional) | 2 | Identifier `0x2F` + XOR byte. XOR is computed over every byte in the packet **except** the START_BYTE and the XOR byte itself. |

### 2.4 Verified wire-level examples

PKT_PRODID request (parity disabled):

```
61 00 01 00 30
│  └─┬─┘│   │
│    │  │   └─ Field ID: PKT_PRODID
│    │  └───── TYPE: control
│    └──────── LENGTH: 1
└───────────── START_BYTE
```

PKT_PRODID response:

```
61 00 0E 00 30 41 4D 42 45 33 30 30 30 53 41 54 46 00
         │  └─┬─┘ └──────────── string ─────────────┘
         │    │   "AMBE3000SATF\0" = 14 bytes
         │    └─ Field ID: PKT_PRODID
         └───── LENGTH: 14
```

PKT_INIT (init encoder + decoder):

```
61 00 02 0B 03
         │  │
         │  └─ 0x03 = encoder | decoder bits (see §3.1.4)
         └──── Field ID: PKT_INIT
```

---

## 3. Control Packet Fields (TYPE = 0x00)

A control packet contains one or more **control fields**. The chip responds to every
control packet with a control response packet containing corresponding response fields.
For most fields the response echoes the field ID followed by a 1-byte status code
(`0x00` = OK, anything else = error).

### 3.1 Field Identifier Table

Official names and sizes per the canonical AMBE-3000F v4.0 manual (Table 32):

| Field ID | Name | Ctrl data | Resp data | Dir | Description |
|----------|------|-----------|-----------|-----|-------------|
| `0x03` | PKT_SAMPLES | 2 | — | I/O | Number of output samples per decoded frame (156–164, default 160) |
| `0x05` | PKT_ECMODE | 2 | — | I/O | Set ECMODE_IN flags (16-bit) for current channel |
| `0x06` | PKT_DCMODE | 2 | — | I/O | Set DCMODE_IN flags (16-bit) for current channel |
| `0x09` | PKT_RATET | 1 | — | I/O | Select built-in rate from table (see `annex_tables/rate_index_table.csv`) |
| `0x0A` | PKT_RATEP | 12 | — | I/O | Select custom rate: 6 × 16-bit rate control words (big-endian) |
| `0x0B` | PKT_INIT | 1 | — | I/O | Initialize encoder/decoder/echo-canceller (see §3.1.4) |
| `0x10` | PKT_LOWPOWER | 1 | — | I/O | Enable (`0x01`) or disable (`0x00`) low-power mode |
| `0x15` | PKT_CHANFMT | 2 | — | I/O | Format of output channel packet (controls ECMODE_OUT inclusion) |
| `0x16` | PKT_SPCHFMT | 2 | — | I/O | Format of output speech packet (controls DCMODE_OUT inclusion) |
| `0x2A` | PKT_CODECSTART | 1 | — | I/O | Switch to codec mode (sends codec configuration via I²C) |
| `0x2B` | PKT_CODECSTOP | 0 | — | I/O | Switch to packet mode |
| `0x2F` | (parity) | — | — | — | Reserved as parity trailer identifier (not a control field) |
| `0x30` | PKT_PRODID | 0 | varies | I/O | Query product identifier string |
| `0x31` | PKT_VERSTRING | 0 | 48 | I/O | Query firmware version string (48 bytes + NUL) |
| `0x32` | PKT_COMPAND | 1 | — | I/O | Companding mode (see §3.1.6) |
| `0x33` | PKT_RESET | 0 | — | I | Reset using hardware configuration pins |
| `0x34` | PKT_RESETSOFTCFG | 6 | — | I | Reset with software-specified configuration |
| `0x35` | PKT_HALT | 0 | — | I | Enter lowest-power halt state (AMBE-3003) |
| `0x36` | PKT_GETCFG | 0 | 3 | I/O | Query configuration-pin state at last power-up/reset |
| `0x37` | PKT_READCFG | 0 | 3 | I/O | Query current state of configuration pins |
| `0x38` | PKT_CODECCFG | varies | — | I/O | Codec register configuration data (sent to external codec on PKT_CODECSTART) |
| `0x39` | PKT_READY | 0 | — | O | (Chip → host) Device is ready to receive packets |
| `0x3F` | PKT_PARITYMODE | 1 | — | I/O | Enable (default `1`) / disable (`0`) parity trailers |
| `0x40` | PKT_CHANNEL0 | 0 | — | I/O | Subsequent fields apply to channel 0 |
| `0x41` | PKT_CHANNEL1 | 0 | — | I/O | Subsequent fields apply to channel 1 (USB-3003/3012) |
| `0x42` | PKT_CHANNEL2 | 0 | — | I/O | Subsequent fields apply to channel 2 (USB-3003/3012) |
| `0x44` | PKT_WRITE_I2C | varies | — | I/O | Write to external I²C device (used to drive external codec) |
| `0x46` | PKT_CLRCODECRESET | 0 | — | I/O | Drive codec reset signal low |
| `0x47` | PKT_SETCODECRESET | 0 | — | I/O | Drive codec reset signal high |
| `0x48` | PKT_DISCARDCODEC | 2 | — | I/O | Discard N codec samples (integer, big-endian) |
| `0x49` | PKT_DELAYNUS | 2 | — | I/O | Delay next control-field processing by N microseconds |
| `0x4A` | PKT_DELAYNNS | 2 | — | I/O | Delay next control-field processing by N nanoseconds |
| `0x4B` | PKT_GAIN | 2 | — | I/O | Input/output gain in 0.5 dB steps, range ±90 dB |
| `0x4E` | PKT_RTSTHRESH | 5 | — | I/O | Flow-control (RTS) threshold settings |
| `0x50` | PKT_TONEXMT | — | — | I/O | Tone transmit control (see AMBE-3000F manual §6.11.2) |
| `0x51` | PKT_TONEGEN | — | — | I/O | Tone generator parameters (tone index, amplitude, duration) |
| `0x52` | PKT_TONEDET | — | — | I/O | Tone detection enable |

**Columns:**
- *Ctrl data* = bytes of data **after** the field ID byte in a request packet.
- *Resp data* = bytes of data after the field ID in the chip's response. `—` means the response payload is just a 1-byte OK/error status.
- *Dir* = I (host→chip only), O (chip→host only), or I/O.

### 3.1.1 Known divergence from older documents

- Some older references (including earlier drafts of this spec) named `0x15` as
  `PKT_ENCOUT_FORMAT` and `0x16` as `PKT_DECOUT_FORMAT`. The canonical manual uses
  `PKT_CHANFMT` and `PKT_SPCHFMT`.
- `PKT_INIT` control data is **1 byte**, not 2. Older references showing "value
  0x0007" should be read as `0x07`.
- `PKT_GAIN` control data is **2 bytes**, not 4 (one signed byte for input gain,
  one for output gain, each in 0.5 dB steps).
- The parity trailer is a **2-byte field** (`0x2F` identifier + XOR byte), not a
  bare XOR byte.

### 3.1.2 PKT_RATET — built-in rate selection

One byte selecting a rate from the 62-entry rate table (indices 0–61):
- 0–15: AMBE-1000 rates (AMBE®)
- 16–31: AMBE-2000 rates (AMBE+™)
- 32: AMBE-2000 compatible but outside its standard table
- 33–61: AMBE+2™ rates; index 33 interoperates with P25 Half Rate and DMR

Full table: `annex_tables/rate_index_table.csv`.

### 3.1.3 PKT_RATEP — custom rate selection

Six big-endian 16-bit rate control words (RCWs), total 12 bytes of data.

P25-relevant RCW sets (not in the built-in rate table — require PKT_RATEP):

| Mode | RCW0 | RCW1 | RCW2 | RCW3 | RCW4 | RCW5 | Total bps |
|------|------|------|------|------|------|------|-----------|
| P25 full-rate with FEC (IMBE) | `0x0558` | `0x086B` | `0x1030` | `0x0000` | `0x0000` | `0x0190` | 7200 |
| P25 full-rate without FEC (IMBE) | `0x0558` | `0x086B` | `0x0000` | `0x0000` | `0x0000` | `0x0158` | 4400 |
| D-STAR | `0x0130` | `0x0763` | `0x4000` | `0x0000` | `0x0000` | `0x0048` | 3600 |

Note: P25 full-rate invokes the IMBE algorithm from TIA-102.BABA-A, not an AMBE+2
rate. The RCWs above are **not** in the canonical rate table (Table 120 of the
AMBE-3000F manual) and are documented separately by DVSI.

Full RCW table for built-in rates: `annex_tables/rate_control_words.csv`.

### 3.1.4 PKT_INIT — initialization

One byte of control data with these bit meanings:

| Value | Meaning |
|-------|---------|
| `0x01` | Encoder initialized |
| `0x02` | Decoder initialized |
| `0x04` | Echo canceller initialized |
| `0x03` | Encoder + decoder |
| `0x07` | Encoder + decoder + echo canceller |

**Side effects** when the encoder-init bit is set:
- `TONE_DET_ENABLE` bit in ECMODE_IN is set to 1
- `NS_ENABLE`, `ES_ENABLE`, `CP_ENABLE`, `CP_SELECT`, `DTX_ENABLE`, `EC_ENABLE`
  are reset from the corresponding configuration pins
- Echo canceller state is governed by the configuration pin *or* the echo-canceller
  init bit (bit 2)
- **All other bits in ECMODE_IN are cleared to 0**

When the decoder-init bit is set:
- `CP_ENABLE`, `CP_SELECT` reset from configuration pins
- **All other bits in DCMODE_IN are cleared to 0**

### 3.1.5 PKT_PARITYMODE — parity control

- `0x00` = disable parity trailer; response-and-subsequent-request packets omit the `0x2F`+XOR trailer
- `0x01` = enable parity trailer (default)

The new mode takes effect **starting with the response packet** to the PKT_PARITYMODE
command (i.e., the response already reflects the new setting).

### 3.1.6 PKT_COMPAND — companding mode

One byte:

| Value | Mode |
|-------|------|
| `0x00` | Companding disabled (linear 16-bit PCM) |
| `0x01` | µ-law companding (8-bit samples) |
| `0x03` | A-law companding (8-bit samples) |

Bit 0 = enable, Bit 1 = select (0=µ-law, 1=A-law). Value `0x02` is reserved.

### 3.1.7 PKT_CHANFMT and PKT_SPCHFMT — output packet formats

Two bytes controlling which optional fields are included in outgoing channel and
speech packets respectively. Common use: enable embedding `PKT_CMODE` in outgoing
packets so the host receives per-frame ECMODE_OUT/DCMODE_OUT status flags.

See AMBE-3000F v4.0 manual Tables 62, 63, 65 (pages 69–70) for bit definitions.

---

## 4. Speech Packet Fields (TYPE = 0x02)

### 4.1 PKT_SPCHD (0x00) — speech data

```
┌────────┬───────┬──────────────┐
│  0x00  │ Count │   Samples    │
│ 1 byte │ 1 byte│  N × 1 or 2  │
└────────┴───────┴──────────────┘
```

- `Count` — number of samples (nominal 160; 156–164 allowed when skew control is active)
- Samples:
  - Linear: 16-bit signed, **big-endian on the wire** (2 bytes/sample)
  - µ-law / A-law: 8-bit per sample (1 byte/sample)

### 4.2 PKT_CMODE (0x02) — control mode flags

Two-byte field embedded in speech/channel packets to convey ECMODE_IN (in host-to-chip
direction) or ECMODE_OUT / DCMODE_OUT (chip-to-host direction). See Section 6.

### 4.3 Typical packet structure

**Host → chip (encoding):**

```
61 LLLL 02 | 40 (channel0) | 00 [count] [samples...] | 02 [cmode] | [parity]
```

**Chip → host (decoded output):**

```
61 LLLL 02 | 40 (channel0) | 00 [count] [samples...] | 02 [cmode_out] | [parity]
```

When speech-output-format does not include CMODE, the CMODE field is omitted.

---

## 5. Channel Packet Fields (TYPE = 0x01)

### 5.1 PKT_CHAND (0x01) — hard-decision channel data

```
┌────────┬─────────┬─────────────────┐
│  0x01  │ # bits  │  Channel data   │
│ 1 byte │ 1 byte  │  ceil(N/8) bytes│
└────────┴─────────┴─────────────────┘
```

Packing rule: **MSB first**. The most-significant bit of the first byte is the most
significant (and most error-sensitive) bit of the codec frame. For bit rates that
are not a multiple of 400 bps (i.e., frame sizes not divisible by 8), the MSBs of
the last byte carry the channel data; the LSBs of the last byte are zero-padded.

Example: a 49-bit P25 half-rate-no-FEC frame occupies 7 bytes with the last 7 bits
of byte 6 set to 0.

### 5.2 PKT_CHAND4 (0x17) — 4-bit soft-decision channel data

Same layout as `PKT_CHAND` but each codec bit is represented by 4 soft-decision
bits, so `# bits × 4` raw bits total, packed 2 soft-decisions per byte.

4-bit soft-decision encoding (sign-magnitude confidence):

| Value | Interpretation |
|-------|----------------|
| `0000` | Most confident 0 |
| `0001`–`0111` | 0, decreasing confidence |
| `0111` | Least confident 0 |
| `1000` | Least confident 1 |
| `1001`–`1110` | 1, increasing confidence |
| `1111` | Most confident 1 |

Semantics: the MSB is the hard decision; the lower 3 bits are distance from the
decision threshold. Soft-decision is **decoder-only** — encoders always output hard
bits. The host must supply the 4-bit confidence values from its own demodulator.

Field ID selection controls per-packet mode:
- `PKT_CHAND` (`0x01`) for hard-decision input
- `PKT_CHAND4` (`0x17`) for 4-bit soft-decision input

A corresponding `PKT_SDBITS` (`0x0C`) control field (1 byte = `1`, `2`, or `4`) can
globally select soft-decision precision.

### 5.3 P25 frame sizes

| Mode | Frame size | Bytes per frame |
|------|-----------|------------------|
| P25 full-rate with FEC | 144 bits | 18 |
| P25 full-rate without FEC | 88 bits | 11 |
| P25 half-rate with FEC | 72 bits | 9 |
| P25 half-rate without FEC | 49 bits | 7 (1 bit padding) |

---

## 6. CMODE Flags — Four Separate 16-bit Words

The chip exposes four distinct 16-bit control/status words. Earlier documentation
often conflates these into a single "CMODE" table; this is a common source of
confusion. Each word has a different bit layout and direction.

### 6.1 ECMODE_IN — encoder control (host → chip)

Carried by `PKT_ECMODE` (Field ID `0x05`) or embedded as `PKT_CMODE` in incoming
speech packets. Retains its value until changed.

| Bit | Name | Meaning | Default after reset |
|-----|------|---------|---------------------|
| 0–5 | reserved | — | 0 |
| 6 | NS_ENABLE | Noise suppressor enable | From NS_ENABLE pin |
| 7 | CP_SELECT | 0 = µ-law, 1 = A-law (ignored if CP_ENABLE = 0) | From CP_SELECT pin |
| 8 | CP_ENABLE | Companding enable | From CP_ENABLE pin |
| 9 | ES_ENABLE | Echo suppressor enable (codec mode only) | From ES_ENABLE pin |
| 10 | reserved | — | 0 |
| 11 | DTX_ENABLE | Discontinuous transmit | From DTX_ENABLE pin |
| 12 | TD_ENABLE | Tone detect enable | **1 (on)** at reset |
| 13 | EC_ENABLE | Echo canceller enable (codec mode only) | From EC_ENABLE pin |
| 14 | TS_ENABLE | Tone send enable | 0 |
| 15 | reserved | — | 0 |

### 6.2 ECMODE_OUT — encoder status (chip → host)

Carried per-frame in outgoing channel packets when enabled via `PKT_CHANFMT`.

| Bit | Name | Meaning |
|-----|------|---------|
| 0 | reserved | — |
| 1 | VOICE_ACTIVE | 1 if DTX is enabled and this frame should be transmitted (voice detected). When DTX is off, the encoder still emits channel data but VOICE_ACTIVE may be 0. |
| 2–14 | reserved | — |
| 15 | TONE_FRAME | Output frame contains a single-frequency tone, DTMF, KNOX tone, or call-progress tone |

### 6.3 DCMODE_IN — decoder control (host → chip)

Carried by `PKT_DCMODE` (Field ID `0x06`) or embedded as `PKT_CMODE` in incoming
channel packets. Retains its value until changed.

| Bit | Name | Meaning | Default after reset |
|-----|------|---------|---------------------|
| 0–1 | reserved | — | 0 |
| 2 | LOST_FRAME | If 1, decoder ignores the incoming channel data and performs a frame repeat (see errata: broken before Release 014) | 0 |
| 3 | CNI_FRAME | Comfort-noise insertion enable — decoder ignores channel data and emits comfort noise based on last silence frame | 0 |
| 4–6 | reserved | — | 0 |
| 7 | CP_SELECT | 0 = µ-law, 1 = A-law | From CP_SELECT pin |
| 8 | CP_ENABLE | Companding enable | From CP_ENABLE pin |
| 9–13 | reserved | — | 0 |
| 14 | TS_ENABLE | Tone synthesis enable — decoder synthesizes specified tone instead of channel data | 0 |
| 15 | reserved | — | 0 |

### 6.4 DCMODE_OUT — decoder status (chip → host)

Carried per-frame in outgoing speech packets when enabled via `PKT_SPCHFMT`.

| Bit | Name | Meaning |
|-----|------|---------|
| 0 | reserved | — |
| 1 | VOICE_ACTIVE | 1 if decoder synthesized a voice or tone frame; 0 if comfort-noise frame |
| 2–4 | reserved | — |
| 5 | DATA_INVALID | Set when decoder performed a frame repeat or injected comfort noise due to channel errors / missing frames. Cleared when a valid voice/silence/tone frame was received. |
| 6–14 | reserved | — |
| 15 | TONE_FRAME | Decoder decoded a tone frame |

### 6.5 P25-typical ECMODE_IN values

For P25 encode in packet mode (echo canceller is not supported in packet mode, so
bits 9 and 13 stay cleared):

- `0x1040` — NS_ENABLE + TD_ENABLE (baseline)
- `0x1840` — NS_ENABLE + DTX_ENABLE + TD_ENABLE (with silence suppression)

---

## 7. Rate Conversion (Repeater Mode)

The AMBE-3000 supports parametric rate conversion between any two rates without a
PCM round-trip. Control fields:

| Field | Purpose |
|-------|---------|
| `PKT_RPT_MODE` (0x56) | Enable / disable repeater mode |
| `PKT_RE_RATET` (0x57) | Repeater output encoder rate (table index) |
| `PKT_RE_RATEP` (0x58) | Repeater output encoder rate (custom RCWs) |
| `PKT_RD_RATET` (0x59) | Repeater input decoder rate (table index) |
| `PKT_RD_RATEP` (0x5A) | Repeater input decoder rate (custom RCWs) |
| `PKT_LOSTFRAME` (0x5B) | Signal a lost frame to the repeater decoder |

Note: these fields appear in the USB-3000 Manual but not the AMBE-3000F chip manual —
repeater mode is implemented in the USB-3000 firmware layer on top of the
vocoder chip's normal packet interface.

---

## 8. File Formats (test vector convention)

Used by DVSI test vectors and `usb3k_client`.

### 8.1 .pcm — raw PCM

- 16-bit signed integer, **little-endian on disk**
- 8,000 Hz, 160 samples / 20 ms frame = 320 bytes/frame
- No header
- Note: wire format is big-endian (Section 4.1); tools must byte-swap when reading/writing these files.

### 8.2 .bit — codec bits

- **One byte per bit**, value `0x00` or `0x01` (not packed binary)
- One frame = N bytes where N is the codec-frame bit count

### 8.3 .pcmu / .pcma — companded

- µ-law (.pcmu) or A-law (.pcma), 1 byte/sample
- 8,000 Hz, 160 bytes/frame

---

## 9. Packet Type / Field Cross-Reference

### 9.1 Control packet (type 0x00)

Carries one or more control fields from Section 3.1.

### 9.2 Speech packet (type 0x02)

Typical field order:
1. `PKT_CHANNEL0/1/2` (0x40/41/42) — optional, selects channel
2. `PKT_SPCHD` (0x00) — sample data
3. `PKT_CMODE` (0x02) — optional ECMODE_IN flags for the frame
4. `PKT_TONEXMT` (0x50) — optional tone-transmit override
5. Parity trailer (if enabled)

### 9.3 Channel packet (type 0x01)

Typical field order:
1. `PKT_CHANNEL0/1/2` (0x40/41/42) — optional, selects channel
2. `PKT_CHAND` (0x01) or `PKT_CHAND4` (0x17) — bit data
3. `PKT_SAMPLES` (0x03) — optional override of output sample count (156–164)
4. `PKT_CMODE` (0x02) — optional DCMODE_IN flags for the frame
5. `PKT_TONEGEN` (0x51) — optional tone-generation override
6. Parity trailer (if enabled)

---

## 10. References

- AMBE-3000F™ Vocoder Chip Users Manual v4.0, Sections 4.5, 5, 6, 7, 10
  (October 2021) — canonical for field IDs, CMODE bits, rate table, errata
- USB-3000™ Manual — for USB adapter framing, rate conversion fields,
  pending-packet buffering
- Designer's Notes v1.1 (April 2017) — reset timing, UART_TX boot requirement,
  concrete packet examples
- Soft Decision Error Decoding with the AMBE+2™ Vocoder Chips (2022) —
  §1 for 4-bit SD encoding
- Columbia libambe `packet.h`, `api.cc` — open-source implementation cross-check
