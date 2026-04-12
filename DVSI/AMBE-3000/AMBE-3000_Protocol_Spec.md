# AMBE-3000 USB-3000™ Packet Protocol Specification

**Source:** DVSI USB-3000™ Manual, USB-3000 host software (usb3k_client)  
**Date:** 2026-04-13  
**Purpose:** Complete protocol reference for communicating with the AMBE-3000
vocoder chip via the USB-3000 USB interface. Required for black-box testing
of a software AMBE-3000 implementation against the hardware.

---

## 1. Physical Interface

| Parameter | USB-3000 | USB-3003 | USB-3012 |
|-----------|----------|----------|----------|
| Channels | 1 | 3 | 12 |
| Baud rate | 460,800 | 921,600 | 921,600 |
| USB chip | FTDI FT232R | FTDI FT232R | FTDI FT4232H |
| Interface | USB 2.0 | USB 2.0 | USB 2.0 |
| Flow control | RTS/CTS hardware handshaking | RTS/CTS | RTS/CTS |

On Windows, communication occurs via named pipes through the `dvsiserver`
process. On Linux, the `usb3klinux` client communicates directly with the
FTDI chip via libftdi or the kernel ftdi_sio driver.

---

## 2. Packet Structure

All communication uses a uniform packet format:

```
┌──────────┬────────────┬──────────┬──────────────────┬────────┐
│  Header  │   Length   │   Type   │     Payload      │ Parity │
│  1 byte  │  2 bytes   │  1 byte  │  variable        │ 1 byte │
│  0x61    │  big-endian│          │                  │  XOR   │
└──────────┴────────────┴──────────┴──────────────────┴────────┘
```

| Field | Bytes | Description |
|-------|-------|-------------|
| Header | 1 | Always `0x61` (ASCII 'a') |
| Length | 2 | Payload length (big-endian), excludes header byte and length field itself |
| Type | 1 | Packet type: `0x00` control, `0x01` channel, `0x02` speech |
| Payload | variable | Type-specific fields (see sections below) |
| Parity | 1 | XOR of all bytes in the packet except the header byte |

The length field includes the type byte, all payload fields, and the
parity byte. So: `length = 1 (type) + payload_size + 1 (parity)`.

---

## 3. Packet Types

### 3.1 Control Packets (Type 0x00)

Control packets configure the vocoder and query device status. Each
control field is a tag-length-value structure within the payload. Multiple
fields can be packed into a single control packet.

The device responds to each control packet with a response packet
containing `PKT_RESPONSE_OK` (0x00) or `PKT_RESPONSE_ERROR` (0x01) for
each field.

#### Configuration Fields

| Field ID | Name | Size | Description |
|----------|------|------|-------------|
| `0x0b` | PKT_INIT | 2 bytes | Initialize subsystems. Bit 0: encoder, Bit 1: decoder, Bit 2: echo canceller. Value `0x0007` initializes all three. Flushes state. |
| `0x09` | PKT_RATET | 1 byte | Rate table index (0–63). Sets both encoder and decoder rate. |
| `0x0a` | PKT_RATEP | 12 bytes | Rate parameters: 6 × 16-bit words (big-endian). Used for custom rates or P25 mode. |
| `0x32` | PKT_COMPAND | 1 byte | Companding mode. `0x00` = linear PCM, `0x01` = µ-law, `0x03` = A-law. |
| `0x10` | PKT_LOWPOWER | 1 byte | Low power mode. `0x00` = normal, `0x01` = low power. |
| `0x4b` | PKT_GAIN | 4 bytes | Input gain (2 bytes) and output gain (2 bytes). |

#### Encoder/Decoder Mode Fields

| Field ID | Name | Size | Description |
|----------|------|------|-------------|
| `0x05` | PKT_ECMODE | 2 bytes | Encoder control mode (see CMODE flags in Section 6). |
| `0x06` | PKT_DCMODE | 2 bytes | Decoder control mode flags. |
| `0x15` | PKT_ENCOUT_FORMAT | 2 bytes | Encoder output format flags. |
| `0x16` | PKT_DECOUT_FORMAT | 2 bytes | Decoder output format flags. |
| `0x0c` | PKT_SDBITS | 1 byte | Soft decision bits per symbol: `1` (hard), `2`, or `4`. |
| `0x19` | PKT_VADLEVEL | 2 bytes | VAD threshold in dBm0 (signed, range -90 to +3, default -25). |
| `0x1a` | PKT_ERRTHRESH | varies | Error detection thresholds for frame validity. |

#### Rate Conversion / Repeater Fields

| Field ID | Name | Size | Description |
|----------|------|------|-------------|
| `0x56` | PKT_RPT_MODE | 1 byte | Repeater mode. `0x00` = off, `0x01` = rate conversion enabled. |
| `0x57` | PKT_RE_RATET | 1 byte | Repeater encoder rate table index. |
| `0x58` | PKT_RE_RATEP | 12 bytes | Repeater encoder rate parameters (6 × 16-bit words). |
| `0x59` | PKT_RD_RATET | 1 byte | Repeater decoder rate table index. |
| `0x5a` | PKT_RD_RATEP | 12 bytes | Repeater decoder rate parameters (6 × 16-bit words). |
| `0x5b` | PKT_LOSTFRAME | — | Signal a lost frame to the repeater decoder. |

#### Tone Fields

| Field ID | Name | Description |
|----------|------|-------------|
| `0x08` | PKT_TONE | Tone generation control |
| `0x50` | PKT_TONEXMT | Tone transmit |
| `0x51` | PKT_TONEGEN | Tone generator parameters |
| `0x52` | PKT_TONEDET | Tone detection enable |
| `0x53` | PKT_TONERCV | Tone receive notification |
| `0x54` | PKT_TONEMODE | Tone mode configuration |

#### Device Management Fields

| Field ID | Name | Description |
|----------|------|-------------|
| `0x33` | PKT_RESET | Soft reset. Device responds with PKT_READY when complete. |
| `0x39` | PKT_READY | Device → host: reset complete, ready to receive packets. |
| `0x30` | PKT_PRODID | Query product ID string. |
| `0x31` | PKT_VERSTRING | Query firmware version string. |
| `0x2f` | PKT_PARITYBYTE | Enable/disable parity byte. |
| `0x3F` | PKT_PARITYMODE | Parity mode configuration. |
| `0x2a` | PKT_STARTCODEC | Switch to codec mode, start codec hardware. |
| `0x2b` | PKT_STOPCODEC | Switch to packet mode, stop codec hardware. |

#### Channel Selection (USB-3003/3012 only)

| Field ID | Name | Description |
|----------|------|-------------|
| `0x40` | PKT_CHANNEL0 | Select channel 0 |
| `0x41` | PKT_CHANNEL1 | Select channel 1 |
| `0x42` | PKT_CHANNEL2 | Select channel 2 |

#### Timing / Diagnostics

| Field ID | Name | Description |
|----------|------|-------------|
| `0x1b` | PKT_ETIME | Encoder processing time |
| `0x1c` | PKT_DTIME | Decoder processing time |
| `0x1d` | PKT_EWS | Encoder word size |
| `0x1e` | PKT_DWS | Decoder word size |
| `0x4c` | PKT_RESETMAXTIME | Reset max encoder/decoder timing counters |
| `0x4d` | PKT_GETMAXTIME | Retrieve max encoder/decoder times |
| `0x4e` | PKT_RTSTHRESH | Set RTS flow control threshold |

### 3.2 Speech Packets (Type 0x02)

Speech packets carry PCM audio samples between the host and vocoder.

**Host → Device:** PCM input for encoding  
**Device → Host:** Decoded PCM output

```
┌──────────┬────────┬───────────┬──────────────┬──────────┬───────────┬────────┐
│ PKT_SPEECHD │ Count │  Samples  │  PKT_CMODE  │  Mode    │  Parity  │
│   0x00   │ 1 byte │ N×2 bytes │    0x02     │  2 bytes │  1 byte  │
└──────────┴────────┴───────────┴──────────────┴──────────┴───────────┴────────┘
```

| Field | Value | Description |
|-------|-------|-------------|
| PKT_SPEECHD | `0x00` | Speech data field marker |
| Count | 1 byte | Number of samples (typically 160 for 20 ms at 8 kHz) |
| Samples | Count × 2 bytes | 16-bit signed PCM, big-endian. Or Count × 1 byte if companding enabled. |
| PKT_CMODE | `0x02` | Control mode field marker |
| Mode | 2 bytes | CMODE flags (see Section 6) |

**Audio format:**
- Sample rate: 8,000 Hz
- Frame duration: 20 ms
- Samples per frame: 160 (nominal; 156–164 for skew testing)
- Linear PCM: 16-bit signed, big-endian (320 bytes per frame)
- µ-law/A-law: 8-bit compressed (160 bytes per frame)

### 3.3 Channel Packets (Type 0x01)

Channel packets carry encoded vocoder frames (MBE codec bits).

**Host → Device:** Encoded frames for decoding  
**Device → Host:** Encoder output

```
┌────────────────┬────────┬──────────────┬────────┬──────────┬──────────┬──────────┬────────┐
│ PKT_CHAND      │ Bits   │ Channel Data │ PKT_WS │ WS Value │ PKT_CMODE│  Mode    │ Parity │
│ 0x01 or 0x17   │ 1 byte │  variable    │  0x03  │  2 bytes │   0x02   │ 2 bytes  │ 1 byte │
└────────────────┴────────┴──────────────┴────────┴──────────┴──────────┴──────────┴────────┘
```

| Field | Value | Description |
|-------|-------|-------------|
| PKT_CHAND | `0x01` | Channel data, 1-bit hard decision or 2-bit soft decision |
| PKT_CHAND4 | `0x17` | Channel data, 4-bit soft decision |
| Bits | 1 byte | Number of codec bits in the frame |
| Channel Data | ceil(Bits×bps/8) bytes | Packed bit stream, MSB first |
| PKT_WS | `0x03` | Word size field marker |
| WS Value | 2 bytes | Decoder output samples per frame (typically 160) |
| PKT_CMODE | `0x02` | Control mode field marker |
| Mode | 2 bytes | CMODE flags |

**Bit packing:**
- Hard decision (1 bit/symbol): 8 symbols per byte, MSB first
- 2-bit soft decision: 4 symbols per byte, MSB first
- 4-bit soft decision: 2 symbols per byte, MSB first
- Last byte zero-padded to byte boundary

**P25 channel bit counts:**
- Full-rate with FEC: 144 bits per frame (18 bytes)
- Full-rate without FEC: 88 bits per frame (11 bytes)
- Half-rate with FEC: 72 bits per frame (9 bytes)
- Half-rate without FEC: 49 bits per frame (7 bytes, with 1 bit padding)

---

## 4. Rate Configuration

### 4.1 Rate Table Indices (PKT_RATET)

The AMBE-3000 supports 64 predefined rate configurations indexed 0–63:

| Index Range | Codec Generation | Notes |
|-------------|-----------------|-------|
| 0–15 | AMBE-1000 (AMBE) | Legacy rates |
| 16–31 | AMBE-2000 (AMBE+) | Intermediate generation |
| 32–63 | AMBE-3000 (AMBE+2) | Current generation |

### 4.2 Rate Parameters (PKT_RATEP)

For modes not covered by the rate table (including P25), 6 × 16-bit
rate control words (RCW) specify the codec configuration:

```
PKT_RATEP: [RCW0] [RCW1] [RCW2] [RCW3] [RCW4] [RCW5]
            2 bytes each, big-endian
```

#### Known P25 Rate Control Words

| Mode | RCW0 | RCW1 | RCW2 | RCW3 | RCW4 | RCW5 | Bits/Frame |
|------|------|------|------|------|------|------|------------|
| P25 full-rate with FEC | `0x0558` | `0x086b` | `0x1030` | `0x0000` | `0x0000` | `0x0190` | 144 |
| P25 full-rate no FEC | `0x0558` | `0x086b` | `0x0000` | `0x0000` | `0x0000` | `0x0158` | 88 |

#### Known D-STAR Rate Control Words

| Mode | RCW0 | RCW1 | RCW2 | RCW3 | RCW4 | RCW5 |
|------|------|------|------|------|------|------|
| D-STAR | `0x0130` | `0x0763` | `0x4000` | `0x0000` | `0x0000` | `0x0048` |

### 4.3 Codec Modes (usb3k_client -c flag)

The client software recognizes these codec mode strings:

| Mode String | Description |
|-------------|-------------|
| `STD` | Standard AMBE codec. Uses rate table index (`-r N`). |
| `P25` | P25 IMBE full-rate. Uses rate parameters (`-r RCW0..RCW5`). |
| `P25A` | P25 enhanced variant. Same rate words, different codec path. |
| `P25X` | P25 extended variant. Same rate words, different codec path. |
| `RC` | Rate conversion mode. Uses `-rd` (decoder rate) and `-re` (encoder rate). |

The difference between P25, P25A, and P25X is the codec algorithm
variant — they use the same rate control words but may produce different
encoded output. All three are tested in the standard test vectors.

---

## 5. Operating Modes

### 5.1 Encoder Mode (PCM → Channel Bits)

```
Host                              AMBE-3000
  │                                   │
  │──── Speech Packet (PCM) ─────────>│
  │                                   │ encode
  │<─── Channel Packet (bits) ────────│
  │                                   │
```

CLI: `usb3k_client -c P25 -enc -r 0x0558 0x086b 0x1030 0x0000 0x0000 0x0190 input.pcm`

### 5.2 Decoder Mode (Channel Bits → PCM)

```
Host                              AMBE-3000
  │                                   │
  │──── Channel Packet (bits) ───────>│
  │                                   │ decode
  │<─── Speech Packet (PCM) ─────────│
  │                                   │
```

CLI: `usb3k_client -c P25 -dec -r 0x0558 0x086b 0x1030 0x0000 0x0000 0x0190 input.bit`

### 5.3 Encode-Decode Mode (PCM → Channel Bits → PCM)

CLI: `usb3k_client -c P25 -encdec -r 0x0558 0x086b 0x1030 0x0000 0x0000 0x0190 input.pcm`

Round-trip encode then decode. Used for quality testing.

### 5.4 Rate Conversion Mode (Channel Bits → Channel Bits)

```
Host                              AMBE-3000
  │                                   │
  │──── Channel Packet (rate A) ─────>│
  │                                   │ FEC decode → extract MBE params
  │                                   │ re-quantize → FEC encode
  │<─── Channel Packet (rate B) ──────│
  │                                   │
```

Configuration:
```
PKT_RPT_MODE  = 0x01 (enable)
PKT_RD_RATET/PKT_RD_RATEP = decoder (input) rate
PKT_RE_RATET/PKT_RE_RATEP = encoder (output) rate
```

CLI: `usb3k_client -c RC -rc -rd <decoder_rate> -re <encoder_rate> input.bit`

Example — P25 full-rate to D-STAR:
```
usb3k_client -c RC -rc \
  -rd 0x0558 0x086b 0x1030 0x0000 0x0000 0x0190 \
  -re 0x0130 0x0763 0x4000 0x0000 0x0000 0x0048 \
  p25/input.bit
```

### 5.5 Repeater Mode (Channel Bits → Channel Bits, same rate)

Same as rate conversion but with identical input/output rates. Used for
FEC error correction and re-transmission without rate change.

---

## 6. CMODE Flags

The 16-bit CMODE word accompanies every speech and channel packet,
carrying per-frame control and status information:

| Bit | Flag | Description |
|-----|------|-------------|
| 15 | DTMF_FRAME_FLAG | 1 = DTMF detected (encoder) or received (decoder) |
| 14 | DTMF_SEND_FLAG | 1 = force tone transmission/synthesis |
| 13 | ECHO_CANCEL_ENABLE_FLAG | 1 = echo canceller enabled |
| 12 | TONE_DET_ENABLE_FLAG | 1 = tone detection enabled in encoder |
| 11 | DTX_ENABLE_FLAG | 1 = discontinuous transmission (silence frames) |
| 10 | VAD_FLAG | 1 = voice activity detected |
| 9 | ECHO_SUPPRESS_ENABLE_FLAG | 1 = echo suppressor enabled |
| 8 | COMPAND_ENABLE_FLAG | 1 = companding enabled |
| 7 | COMPAND_SELECT_FLAG | 0 = µ-law, 1 = A-law |
| 6 | NS_ENABLE_FLAG | 1 = noise suppression enabled |
| 5 | DATA_INVALID_FLAG | 1 = data invalid |
| 4 | (reserved) | |
| 3 | CNI_FRAME_FLAG | Comfort Noise Indication |
| 2 | LOST_FRAME_FLAG | 1 = frame lost / error detected |
| 1 | VOICE_ACTIVE_FLAG | 1 = voice active |
| 0 | (reserved) | |

**Typical ECMODE for P25 encoding:** `0x1040` = tone detection (bit 12) +
noise suppression (bit 6).

---

## 7. Initialization Sequence

Standard startup sequence for P25 full-rate operation:

```
1. Host sends PKT_RESET
2. Wait for PKT_READY from device
3. Send control packet:
     PKT_CHANNEL0         (select channel 0)
     PKT_LOWPOWER  0x00   (normal power)
     PKT_INIT      0x0007 (init encoder + decoder + echo)
     PKT_COMPAND   0x00   (linear PCM)
     PKT_RATEP     0x0558 0x086b 0x1030 0x0000 0x0000 0x0190
     PKT_ECMODE    0x1040 (tone det + noise suppression)
     PKT_DCMODE    0x0000
     PKT_SDBITS    0x01   (hard decision)
4. Wait for control response (PKT_RESPONSE_OK for each field)
5. Begin sending speech or channel packets
```

**Packet buffering:** The device buffers up to 2 speech packets and 2
channel packets. The "pending packets" technique allows sending up to
3 control fields before waiting for responses.

---

## 8. File Formats

### 8.1 PCM Files (.pcm)

- 16-bit signed integer, little-endian byte order
- 8,000 Hz sample rate
- 160 samples per 20 ms frame
- No header — raw sample data
- File size = num_frames × 160 × 2 bytes

### 8.2 Bit Files (.bit)

- Hard-decision encoded vocoder frames
- Each byte is a single bit value: `0x00` or `0x01`
- One frame = N bytes where N is the channel bits per frame
  (e.g., 144 bytes for P25 full-rate with FEC)
- No header — raw bit-per-byte data
- File size = num_frames × bits_per_frame bytes

### 8.3 Companded Files (.pcmu, .pcma)

- µ-law (.pcmu) or A-law (.pcma) compressed audio
- 1 byte per sample, 8,000 Hz
- 160 bytes per 20 ms frame
