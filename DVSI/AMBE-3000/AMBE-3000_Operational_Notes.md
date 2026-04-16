# AMBE-3000 Operational Notes

**Sources:**
- AMBE-3000F™ Vocoder Chip Users Manual v4.0 §3.6, §10 (reset behaviour, errata)
- AMBE-3000™ Vocoder Chip Designer's Notes v1.1 (April 2017)
- Columbia libambe `api.cc` — cross-reference for soft-reset convention

**Purpose:** Implementation guidance beyond the wire format — reset timing,
initialization ordering, firmware errata, and hardware integration gotchas that
affect a correct host implementation.

---

## 1. Reset Sequence

### 1.1 Hardware reset

Hardware reset is asserted via the chip's `RESETn` pin. After the host releases
`RESETn`:

| Event | Time |
|-------|------|
| `RESETn` released → `PKT_READY` emitted | 20 ms max, 17 ms typical |
| `TX_RDY` readings valid | ~1 ms after reset release |

The host should not read `TX_RDY` for roughly 1 ms post-reset — the pin may be
transiently driven high during chip initialization.

### 1.2 Soft reset

Soft reset is triggered by sending the `PKT_RESET` (0x33) or `PKT_RESETSOFTCFG`
(0x34) control packets:

| Event | Time |
|-------|------|
| Soft reset packet received → `PKT_READY` emitted | ~7 ms |

**Side effect**: when the chip receives a soft-reset packet, it drives its own
`RESETn` pin LOW for approximately 20 µs. The `RESETn` pin is bidirectional on the
AMBE-3000. Do not share the chip's `RESETn` line with other components that cannot
tolerate this brief pulse.

### 1.3 Cascading reset recommendation

Recommended power-up sequence:

```
Host → AMBE-3000 RESETn (chip comes out of reset)
  AMBE-3000 → CODEC_RESETn (codec comes out of reset)
```

Let each device finish its own reset before the next releases. Prevents data loss
during bring-up.

### 1.4 Soft-reset robustness (from libambe)

Columbia's `libambe` prefaces every soft reset by sending **~35,000 zero bytes**
(3,500 writes of 10 bytes) to the chip, to flush any partial in-flight packet state.
The reset packet is always sent with parity enabled, regardless of the chip's
current parity setting, so that the chip accepts it regardless of state. This
convention is attributed to DVSI's official Linux client software.

Implementation sketch (C-style pseudocode):

```c
/* Flush any partial packet state before attempting reset */
uint8_t zeros[10] = {0};
for (int i = 0; i < 3500; i++) uart_write(zeros, 10);

/* Soft reset packet with parity enabled: 61 00 02 33 2F XX
 * where XX = XOR of 00 02 33 2F */
uint8_t reset_pkt[] = {0x61, 0x00, 0x02, 0x33, 0x2F, 0x00};
reset_pkt[5] = reset_pkt[1] ^ reset_pkt[2] ^ reset_pkt[3] ^ reset_pkt[4];
uart_write(reset_pkt, 6);

/* Wait for PKT_READY — parity not checked on this response */
wait_for_ready_packet();
```

---

## 2. Initialization Sequence (P25 full-rate)

Canonical order for bringing up the chip in P25 IMBE full-rate mode. Order matters
(see Section 4 on the pre-Release-007 `PKT_INIT` gotcha).

```
1. Power-up / hardware reset
   └─ wait for PKT_READY (≤ 20 ms)

2. Send configuration in one or more control packets:
   PKT_CHANNEL0            (select channel 0)
   PKT_LOWPOWER    0x00    (normal power)
   PKT_COMPAND     0x00    (linear PCM)
   PKT_RATEP       0x0558 0x086B 0x1030 0x0000 0x0000 0x0190   (P25 full-rate w/ FEC)
   PKT_INIT        0x03    (initialize encoder + decoder)
   PKT_ECMODE      0x1040  (NS_ENABLE + TD_ENABLE; echo canceller not used in packet mode)
   PKT_DCMODE      0x0000
   PKT_PARITYMODE  0x01    (parity on — default, but make it explicit)

3. Wait for response OK for each field.

4. Begin sending speech / channel packets.
```

**Critical ordering (pre-Release 007 chips):** send `PKT_RATET` or `PKT_RATEP`
*after* `PKT_INIT`, not before. See Section 4.1.

### 2.1 Clearing vocoder state between unrelated audio streams

To reset the chip's internal state (without resetting the whole chip) when switching
to an unrelated audio stream:

```
PKT_CODECSTOP            61 00 01 2B              (exit codec mode)
PKT_INIT        0x03     61 00 02 0B 03           (reinit encoder + decoder)
PKT_CODECSTART  0x??     61 00 02 00 2A XX        (re-enter codec mode with chosen config byte)
```

This clears any stale MBE model-parameter history that would otherwise bleed from
the previous stream into the new one.

---

## 3. Hardware Pin Requirements

### 3.1 UART_TX pin at boot

**Must be held HIGH at boot.** The chip samples this pin at startup to determine
UART configuration. If the UART_TX pin is allowed to float or is driven low during
boot, the chip will boot into the wrong configuration.

If the UART interface is not used in the design, the pin can be left unconnected
— it has an internal pull-up.

Pin location: LQFP pin 111, BGA pin C7.

### 3.2 RESETn pin as I/O

The chip's `RESETn` pin (LQFP pin 113, BGA pin D6) is not input-only. The chip
drives it low for ~20 µs on soft reset (see §1.2). Do not tie it to a shared
board-wide reset rail.

---

## 4. Firmware Errata

Check the chip's firmware release number via `PKT_VERSTRING` — the value after the
"R" in the response string indicates the release. Example version string:
`V120.E100.XXXX.C106.G514.R007.A0030608.C0020208` → Release 007.

### 4.1 Release 007 (2009): PKT_INIT resets FEC thresholds

**Pre-Release 007**: `PKT_INIT` resets the FEC error-mitigation thresholds to zero.
Therefore `PKT_RATET` / `PKT_RATEP` **must** be sent *after* `PKT_INIT`, not before,
or FEC error mitigation will not work correctly.

**Release 007 and later**: the thresholds are set correctly regardless of field
order.

For portability across firmware versions, always send the rate after init.

### 4.2 Release 014 (2012): DCMODE LOST_FRAME bit

**Pre-Release 014**: the `LOST_FRAME` bit (bit 2 of `DCMODE_IN`) was ignored by the
decoder. The only way to force a frame-repeat on those chips was to omit the
channel packet for the frame interval entirely.

**Release 014 and later**: `LOST_FRAME` works as documented.

### 4.3 Release 3.6 (April 2016): Echo canceller and suppressor

**Echo canceller (`EC_ENABLE`, bit 13 of ECMODE_IN) and echo suppressor
(`ES_ENABLE`, bit 9) are not supported in packet mode** — only in codec mode. In
packet mode these bits have no effect. Plan ECMODE_IN values accordingly when using
the USB-3000 (which runs the chip in packet mode).

### 4.4 Other release notes of interest

- **Release 003 (2008)**: Added `PKT_WRITE_I2C`, `PKT_SETCODECRESET`,
  `PKT_CLRCODECRESET`, `PKT_DISCARDCODEC`, `PKT_DELAYNUS`, `PKT_DELAYNNS`. These
  fields are present on all current chips.
- **Release 004 (2008)**: Improved FEC decoder performance for Golay codes.
- **Release 005 (2009)**: Added RTS flow control, `PKT_RTSTHRESH`, `PKT_GAIN`.
  Also: packets should only be sent when RTSn is low; packets not fitting in the
  receive buffer are silently discarded (oldest-first).

---

## 5. Input / Output Buffer Sizes

| Buffer | AMBE-3000F / AMBE-3000R |
|--------|-------------------------|
| TXBUFF (chip → host) | 700 bytes |
| RXBUFF (host → chip) | 760 bytes |

Large enough for: one speech packet + one channel packet, two speech packets, or
several channel packets. Pending-packet pipelining is supported — the host can
queue multiple packets without waiting for each response.

See USB-3000 Manual §4.8 ("Using Pending Packets to Improve Throughput") for the
pipelining protocol.

---

## 6. Timing Reference

### 6.1 Algorithmic and processing delays (AMBE-3000F §7.1)

Total algorithmic delay (encoder + decoder) = **62 ms**.

Breakdown (approximate):

| Stage | Delay |
|-------|-------|
| Encoder algorithmic delay (PCM collection) | 52 ms |
| Encoder processing delay | 6 ms |
| Channel transmission | user-defined |
| Decoder scheduling | 0–20 ms |
| Decoder algorithmic delay | 10 ms |
| Decoder processing delay | 5 ms |

### 6.2 Codec-mode TX_RDY timing

In codec mode, `TX_RDY` goes high once every 20 ms to indicate a channel packet
is ready. When skew control is enabled:

- `TX_RDY` rising edge ≈ 5.625 ms after `TX_RQST` 0→1 transition
- Packet Uncertainty Zone: last ~1 ms of the 20 ms frame interval

**Packet Uncertainty Zone rules:**
- Packets arriving during this zone have indeterminate frame-interval assignment.
- If no packet is received in a frame interval, the decoder performs a **frame repeat**
  (synthesizes output from the prior frame's model parameters).
- If two packets are received in a frame interval, the **older** is discarded.

Infrequent erasure frames and frame repeats are acceptable and are one way to
tolerate small clock-drift differences between TX and RX.

---

## 7. Parity Mode Caveats

- Parity trailers are **enabled by default** after hardware reset.
- `PKT_PARITYMODE 0x00` disables parity. The new setting takes effect starting
  with the **response to the PKT_PARITYMODE packet itself** — i.e., the response
  already reflects the new mode.
- Only values `0x00` and `0x01` are defined; other values are reserved.
- Implementations that dynamically toggle parity must update their own parity
  state atomically with the request, since the next packet in either direction
  uses the new setting.
- `PKT_RESETSOFTCFG` carries a 6-byte configuration payload that can override
  parity mode as part of the reset.

---

## 8. Codec-Mode Helpers

Present for systems that use codec mode (where the chip talks directly to an
external I²S codec like TLV320AIC14 or PCM3500) rather than packet mode:

| Field | Purpose |
|-------|---------|
| `PKT_CODECCFG` (0x38) | Codec register program: `[count R][reg# regdata]×count`. count ≤ 10. Example default (TLV320AIC14): `38 05 01 41 02 A0 04 83 05 BB 06 04` |
| `PKT_CODECSTART` (0x2A) | Exit packet mode, enter codec mode, program codec via `PKT_CODECCFG` |
| `PKT_CODECSTOP` (0x2B) | Exit codec mode, return to packet mode |
| `PKT_WRITE_I2C` (0x44) | Arbitrary I²C write to external codec |
| `PKT_CLRCODECRESET`/`PKT_SETCODECRESET` (0x46/0x47) | Drive codec's reset line |
| `PKT_DISCARDCODEC` (0x48) | Discard N codec samples before resuming |

When using the USB-3000 adapter, codec mode is not user-visible — the chip always
runs in packet mode from the host's perspective.

---

## 9. PKT_PRODID / PKT_VERSTRING — Connectivity Sanity Check

The canonical "is the chip alive" probe (Designer's Notes §7):

```
Host → Chip:  61 00 01 00 30          (PKT_PRODID query)
Chip → Host:  61 00 0E 00 30 41 4D 42 45 33 30 30 30 53 41 54 46 00
                             └────────── "AMBE3000SATF\0" ──────────┘
```

Response is the product identifier as a NUL-terminated ASCII string following the
0x30 field ID. Exact string content varies by chip variant (AMBE-3000F,
AMBE-3000R, AMBE-3000SATF, AMBE-3003, etc.).

`PKT_VERSTRING` (`0x31`) returns a 48-byte version string of the form
`V<n>.E<n>.XXXX.C<n>.G<n>.R<release>.A<date>.C<date>\0`. The `R` field is the most
useful — it identifies the firmware release for matching against errata in
Section 4.
