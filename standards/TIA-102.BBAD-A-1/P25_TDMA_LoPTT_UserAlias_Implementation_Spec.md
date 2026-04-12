# P25 Phase 2 TDMA Location on PTT & User Alias -- Implementation Specification

**Sources:** TIA-102.BBAD-A-1 (MAC message formats), TIA-102.BBAE-1 (MAC procedures)  
**Extracted:** 2026-04-12 from full-text addenda  
**Purpose:** Self-contained spec for implementing LoPTT and User Alias MAC message
parsing/generation and the associated SACCH procedures on FNE and SU sides.
No reference to the original PDFs required.

---

## 1. Overview

Two companion addenda (both March 2025, approved January 2025) define:

- **TIA-102.BBAD-A-1** -- Three new VCH SACCH MAC messages: User Alias, and two
  manufacturer-specific Location on PTT variants.
- **TIA-102.BBAE-1** -- Procedures governing when and how those messages are transmitted
  and processed during active TDMA voice calls.

All three messages are 17 octets, carried on the Slow Associated Control Channel (SACCH)
within active TDMA voice channel timeslots. They use manufacturer-specific MAC opcodes
(MFID != 0x00).

| Message Name | Short | Direction | MFID | MCO | Use |
|---|---|---|---|---|---|
| MAC_MFIDA4_UA | User Alias | Outbound (FNE to SU) | 0xA4 | 0b101000 | Display talker alias |
| MAC_MFIDA4_LOP | MFIDA4 Location on PTT | Inbound (SU to FNE) | 0xA4 | 0b101010 | L3Harris GPS location |
| MAC_MOT_PTT_LOC | MFID90 Location on PTT | Inbound (SU to FNE) | 0x90 | 0b100001 | Motorola GPS location |

Manufacturer IDs: 0xA4 = L3Harris, 0x90 = Motorola Solutions.

---

## 2. MAC Message Header Convention

All three messages share the standard TDMA MAC header layout:

| Octet | Bits 7-6 | Bits 5-0 |
|---|---|---|
| 1 | B1, B2 | MCO (6 bits) |
| 2 | MFID (8 bits) | |
| 3 | (varies) | Length or RES+Length |

For all three messages: **B1 = 1, B2 = 0** (binary `10`).

Octet 1 byte value = `(1 << 7) | (0 << 6) | MCO` = `0x80 | MCO`.

---

## 3. Message 1: MAC_MFIDA4_UA (User Alias)

### 3.1 Summary

Outbound-only. Sent by the FNE on the SACCH during an active voice call to convey a
14-character ASCII alias of the transmitting subscriber to receiving SUs.

### 3.2 Constants

```
B1          = 0b1
B2          = 0b0
MCO         = 0b101000     // 0x28
MFID        = 0xA4
LENGTH      = 0x11         // 17 decimal
OCTET_1     = 0xA8         // 0x80 | 0x28
ALIAS_LEN   = 14           // characters, one per octet
TOTAL_LEN   = 17           // octets
```

### 3.3 Byte-Level Format

| Octet | Field | Bits | Value / Encoding |
|---|---|---|---|
| 1 | B1(1) B2(0) MCO | [7:0] | 0xA8 fixed |
| 2 | MFID | [7:0] | 0xA4 fixed |
| 3 | RES(3:3) Length(4:0) | [7:0] | Upper 3 bits reserved; lower 5 bits = 0x11 (17) |
| 4 | ASCII Character 0 | [7:0] | First character of alias |
| 5 | ASCII Character 1 | [7:0] | |
| 6 | ASCII Character 2 | [7:0] | |
| 7 | ASCII Character 3 | [7:0] | |
| 8 | ASCII Character 4 | [7:0] | |
| 9 | ASCII Character 5 | [7:0] | |
| 10 | ASCII Character 6 | [7:0] | |
| 11 | ASCII Character 7 | [7:0] | |
| 12 | ASCII Character 8 | [7:0] | |
| 13 | ASCII Character 9 | [7:0] | |
| 14 | ASCII Character 10 | [7:0] | |
| 15 | ASCII Character 11 | [7:0] | |
| 16 | ASCII Character 12 | [7:0] | |
| 17 | ASCII Character 13 | [7:0] | Last character of alias |

### 3.4 Detection

```
octet[0] == 0xA8  &&  octet[1] == 0xA4
```

### 3.5 Notes

- Each character is 7-bit ASCII transmitted in a full 8-bit octet (MSB usage not specified;
  treat as unsigned byte, mask to 0x7F if strict ASCII is desired).
- Aliases shorter than 14 characters: the standard does not specify padding. Implementations
  should expect space (0x20) or NUL (0x00) padding in unused trailing positions.
- The standard does not specify endianness for the alias string; octets 4-17 are sequential
  left-to-right (character 0 first).

---

## 4. Message 2: MAC_MFIDA4_LOP (MFIDA4 Location on PTT)

### 4.1 Summary

Inbound-only. Sent by the SU on the SACCH during a PTT event, directly following each
Voice Channel User MAC message. Carries full GPS coordinates with high-resolution time,
course, and speed.

### 4.2 Constants

```
B1          = 0b1
B2          = 0b0
MCO         = 0b101010     // 0x2A
MFID        = 0xA4
LENGTH      = 17
OCTET_1     = 0xAA         // 0x80 | 0x2A
TOTAL_LEN   = 17           // octets
```

### 4.3 Byte-Level Format

| Octet | Field | Bits | Encoding |
|---|---|---|---|
| 1 | B1(1) B2(0) MCO | [7:0] | 0xAA fixed |
| 2 | MFID | [7:0] | 0xA4 fixed |
| 3 | Length | [7:0] | 17 (0x11) |
| 4 | Lat Min (decimal) high | [7:0] | UINT16 high byte -- ten-thousandths of minutes |
| 5 | Lat Min (decimal) low | [7:0] | UINT16 low byte |
| 6 | Lat Hemisphere + Lat Min (integer) | [7]: hemisphere, [6:0]: UINT7 integer minutes | [7]=0: North, [7]=1: South |
| 7 | Lat Degrees | [7:0] | UINT8, LSB = 1 degree |
| 8 | Lon Min (decimal) high | [7:0] | UINT16 high byte -- ten-thousandths of minutes |
| 9 | Lon Min (decimal) low | [7:0] | UINT16 low byte |
| 10 | Lon Hemisphere + Lon Min (integer) | [7]: hemisphere, [6:0]: UINT7 integer minutes | [7]=0: East, [7]=1: West |
| 11 | Lon Degrees | [7:0] | UINT8, LSB = 1 degree |
| 12 | Time high | [7:0] | Lower 16 bits of 17-bit seconds-since-midnight (high byte) |
| 13 | Time low | [7:0] | Lower 16 bits of 17-bit seconds-since-midnight (low byte) |
| 14 | Course low | [7:0] | Course: lower 8 bits (UINT8), tenths of degrees |
| 15 | Speed low | [7:0] | Speed: lower 8 bits (UINT8), tenths of knots |
| 16 | Speed[11:8] : Course[11:8] | [7:4]: upper 4 bits of speed, [3:0]: upper 4 bits of course | Combined nibbles |
| 17 | T : Status | [7]: T (17th time bit), [6]: Emergency, [5:4]: GPS quality, [3:0]: Sat count | See below |

**Note on octet numbering:** The source PDF labels the Course/Speed octets as 14-16 and
the combined nibble byte as octet 18 in the field descriptions, but the message is 17 octets
total with octets 1-17. The field description text references "Octet 16" for course low,
"Octet 17" for speed low, and "Octet 18" for the combined nibble byte. This appears to be
an off-by-one in the original standard's field descriptions relative to the figure. The figure
shows Course and Speed spanning octets 15-16 with Status in octet 17. Implementers should
follow the figure layout (17 octets total) and treat the field description octet numbers
as having a +2 offset from the figure.

### 4.4 Coordinate Decoding (MFIDA4)

```
// Latitude
lat_min_decimal = u16::from_be_bytes([octet[3], octet[4]]);  // octets 4-5 (0-indexed: 3-4)
lat_hemisphere  = (octet[5] >> 7) & 1;   // 0=North, 1=South
lat_min_integer = octet[5] & 0x7F;       // UINT7
lat_degrees     = octet[6];              // UINT8

lat_minutes = (lat_min_integer as f64) + (lat_min_decimal as f64) / 10_000.0;
latitude_dd = (lat_degrees as f64) + (lat_minutes / 60.0);
if lat_hemisphere == 1 { latitude_dd = -latitude_dd; }  // South is negative

// Longitude
lon_min_decimal = u16::from_be_bytes([octet[7], octet[8]]);
lon_hemisphere  = (octet[9] >> 7) & 1;   // 0=East, 1=West
lon_min_integer = octet[9] & 0x7F;
lon_degrees     = octet[10];

lon_minutes = (lon_min_integer as f64) + (lon_min_decimal as f64) / 10_000.0;
longitude_dd = (lon_degrees as f64) + (lon_minutes / 60.0);
if lon_hemisphere == 1 { longitude_dd = -longitude_dd; }  // West is negative
```

### 4.5 Time Decoding

```
time_lower16 = u16::from_be_bytes([octet[11], octet[12]]);
t_bit        = (octet[16] >> 7) & 1;
seconds_since_midnight: u32 = ((t_bit as u32) << 16) | (time_lower16 as u32);

// 0x1FFFF (131071) or 0x00000 (0) = time unknown
// Valid range: 0..86399 (seconds in a day)
```

### 4.6 Course and Speed Decoding

```
course_low8   = octet[13];               // lower 8 bits
speed_low8    = octet[14];               // lower 8 bits
course_hi4    = octet[15] & 0x0F;        // upper 4 bits of course
speed_hi4     = (octet[15] >> 4) & 0x0F; // upper 4 bits of speed

course_raw: u16 = ((course_hi4 as u16) << 8) | (course_low8 as u16);  // tenths of degrees
speed_raw:  u16 = ((speed_hi4 as u16) << 8) | (speed_low8 as u16);    // tenths of knots

course_degrees = course_raw as f64 / 10.0;  // 0.0 = North
speed_knots    = speed_raw as f64 / 10.0;
```

### 4.7 Status Byte (Octet 17)

| Bit | Field | Values |
|---|---|---|
| 7 | T | 17th bit of seconds-since-midnight |
| 6 | Emergency | 0 = not in emergency, 1 = emergency |
| 5:4 | GPS Quality | 0b00 = No Fix, 0b01 = GPS Fix, 0b10 = DGPS Fix, 0b11 = No GPS HW / disabled |
| 3:0 | Satellite Count | UINT4 (0-15) |

### 4.8 Detection

```
octet[0] == 0xAA  &&  octet[1] == 0xA4
```

---

## 5. Message 3: MAC_MOT_PTT_LOC (MFID90 Location on PTT)

### 5.1 Summary

Inbound-only. Sent by the SU during a group call or MFID90 supergroup call. Uses
LRRP-derived coordinate encoding with coarser time/speed resolution but includes
group address and source address fields.

### 5.2 Constants

```
B1          = 0b1
B2          = 0b0
MCO         = 0b100001     // 0x21
MFID        = 0x90
LENGTH      = 0b010001     // 17
OCTET_1     = 0xA1         // 0x80 | 0x21
TOTAL_LEN   = 17           // octets
```

### 5.3 Byte-Level Format

| Octet | Field | Bits | Encoding |
|---|---|---|---|
| 1 | B1(1) B2(0) MCO | [7:0] | 0xA1 fixed |
| 2 | MFID | [7:0] | 0x90 fixed |
| 3 | Reserved[7:5] Length[4:0] | [7:0] | Upper 3 bits reserved; lower 5 = 0x11 (17) |
| 4 | Type[7:6] DV[5] 000[4:2] E[1] P[0] | [7:0] | Flags byte (see below) |
| 5 | Group/Supergroup Address high | [7:0] | 24-bit WGID, big-endian |
| 6 | Group/Supergroup Address mid | [7:0] | |
| 7 | Group/Supergroup Address low | [7:0] | |
| 8 | Source Address high | [7:0] | 24-bit WUID, big-endian |
| 9 | Source Address mid | [7:0] | |
| 10 | Source Address low | [7:0] | |
| 11 | Latitude high | [7:0] | 3-byte LRRP latitude (see 5.5) |
| 12 | Latitude mid | [7:0] | |
| 13 | Latitude low | [7:0] | |
| 14 | Longitude high | [7:0] | 3-byte LRRP longitude (see 5.5) |
| 15 | Longitude mid | [7:0] | |
| 16 | Minute[7:2] Second[1:0] | [7:0] | Time fields |
| 17 | Direction[7:5] Speed[4:0] | [7:0] | Direction and speed |

### 5.4 Flags Byte (Octet 4)

| Bits | Field | Values |
|---|---|---|
| 7:6 | Type | 0b00 = Point-2D (lat/lon, no altitude). Others reserved. |
| 5 | DV (Direction Valid) | 0 = direction invalid, 1 = direction valid |
| 4:2 | Reserved | 0b000 |
| 1 | E (Emergency) | 0 = non-emergency, 1 = emergency |
| 0 | P (Encryption) | 0 = clear, 1 = encrypted |

### 5.5 Coordinate Decoding (MFID90 / LRRP-derived)

The 3-byte values are truncated from 4-byte LRRP encodings with rounding.

**Latitude** (3 bytes = 24 bits):

```
raw24 = u32::from_be_bytes([0x00, octet[10], octet[11], octet[12]]);

// Sign bit is bit 23 (MSB of octet 11)
sign = (raw24 >> 23) & 1;
magnitude = raw24 & 0x7FFFFF;  // 23-bit magnitude

// Reconstruct approximate 4-byte LRRP value (shift left 8, lost precision)
lrrp32 = (raw24 as u32) << 8;

// LRRP latitude encoding: encoded_latitude = (L * 2^31) / 90
// So: L = (magnitude * 90.0) / 2^31  (using 23-bit magnitude << 8 = 31-bit)
// Simplified for 3-byte truncated form:
latitude_dd = (magnitude as f64) * 90.0 / (2u64.pow(23) as f64);
if sign == 1 { latitude_dd = -latitude_dd; }

// Note: L=90 encodes as 2^31 - 1 (special case, maps to 0x7FFFFF at 3 bytes)
```

**Longitude** (3 bytes = 24 bits):

```
raw24 = u32::from_be_bytes([0x00, octet[13], octet[14], octet[15]]);

// Sign bit is bit 23 (MSB of octet 14)
sign = (raw24 >> 23) & 1;
magnitude = raw24 & 0x7FFFFF;

// LRRP longitude encoding: encoded_longitude = (L * 2^32) / 360
// For 3-byte truncated: L = (magnitude * 360.0) / 2^24  ... but using sign+magnitude:
// Actually the 4-byte LRRP uses 2's complement for longitude.
// The 3-byte form: MSB=sign, remaining 23 bits = absolute value encoding.
// L = (magnitude * 360.0) / 2^24  -- but with the shift this is:
// L_approx = (raw24_unsigned_abs * 360.0) / 2^24
// See LRRP spec (TIA-102.BAJC) for exact reconstruction.

longitude_dd = (magnitude as f64) * 360.0 / (2u64.pow(24) as f64);
if sign == 1 { longitude_dd = -longitude_dd; }
```

**Worked examples from the standard:**

- Latitude 12.345345 degrees: 4-byte LRRP = 0x118ECD8D. Truncate to 3 bytes = 0x118ECD
  (0x8D < 0x80 would not round, but 0x8D >= 0x80 so round up to 0x118ECE).
- Longitude 24.668866 degrees: 4-byte LRRP = 0x118AD47B. Truncate to 3 bytes = 0x118AD4
  (0x7B < 0x80, no rounding).

### 5.6 Time Fields (Octet 16)

| Bits | Field | Encoding |
|---|---|---|
| 7:2 | Minute | UINT6: 0-59 |
| 1:0 | Second | 0b00=0s, 0b01=15s, 0b10=30s, 0b11=45s |

```
minute = (octet[15] >> 2) & 0x3F;
second_code = octet[15] & 0x03;
second = second_code * 15;  // 0, 15, 30, or 45
```

Note: Only minute and coarse second are provided. The RFSS is responsible for adding
the year, day, and hour when forwarding to the mapping application.

### 5.7 Direction and Speed (Octet 17)

| Bits | Field | Encoding |
|---|---|---|
| 7:5 | Direction | 3-bit compass (45-degree resolution) |
| 4:0 | Speed | 5-bit, 2 m/s bins |

**Direction values:**

| Code | Compass | Degree Range |
|---|---|---|
| 0b000 | N | 337.5 to < 22.5 |
| 0b001 | NE | 22.5 to < 67.5 |
| 0b010 | E | 67.5 to < 112.5 |
| 0b011 | SE | 112.5 to < 157.5 |
| 0b100 | S | 157.5 to < 202.5 |
| 0b101 | SW | 202.5 to < 247.5 |
| 0b110 | W | 247.5 to < 292.5 |
| 0b111 | NW | 292.5 to < 337.5 |

**Speed encoding:** Each increment = 2 m/s. 0 = stationary, 1 = >0 to <2 m/s,
30 = 58 to <60 m/s, 31 = >=60 m/s (~134 mph). Max representable = 60 m/s.

```
direction_code = (octet[16] >> 5) & 0x07;
speed_code     = octet[16] & 0x1F;
speed_mps_approx = speed_code as f64 * 2.0;  // approximate center of bin
```

### 5.8 Detection

```
octet[0] == 0xA1  &&  octet[1] == 0x90
```

---

## 6. Procedures: When Messages Are Transmitted

### 6.1 User Alias (Outbound, FNE-originated)

1. A trunking SU requests a voice call (any type: group, announcement, U2U, PSTN, regroup).
2. The FNE looks up the associated user alias for the calling SU.
3. If an alias exists, the FNE embeds one MAC_MFIDA4_UA message per Voice Channel User
   message in the outbound SACCH.
4. Receiving SUs decode and display the 14-character alias.
5. The transmitting SU takes no special action and need not know what alias is used.
6. Works on dynamically regrouped calls (alias is in the voice stream, not call setup).

**Precondition:** FNE has a user alias registered for the calling SU.

### 6.2 MFIDA4 LoPTT (Inbound, SU-originated)

1. SU requests a group call (GRP_V_REQ) and receives a channel grant.
2. On the working channel, the SU sends MAC_MFIDA4_LOP in the SACCH directly after
   each Voice Channel User MAC message.
3. SU continuously updates GPS coordinates as the call progresses.
4. If GPS fix is lost mid-call, SU sends last known position and sets GPS Quality
   bits to indicate stale/no-fix data.
5. SU does not send location if no valid GPS reading is available at call start.

**Preconditions (all must be met):**
- SU has NOT decoded a MOT_STS_BCST message (i.e., not on a Motorola-flagged site)
- MFIDA4 LoPTT is enabled in the radio
- GPS receiver is equipped and enabled

**Supported call types:** All voice call types (group, announcement, U2U, PSTN,
explicit encryption group regroup, etc.).

**Roaming:** Supported -- works when SU is not on its home system.

### 6.3 MFID90 LoPTT (Inbound, SU-originated)

1. SU requests a call and receives a channel grant.
2. After tuning to the 2-slot TDMA voice channel, the SU sends MAC_MOT_PTT_LOC
   in the inbound SACCH of the **first 3 frames of the ultraframe**.
3. The same GPS reading is sent for the entire duration of a single PTT.
4. SU continuously sends the location MAC for the duration of the PTT.

**Preconditions (all must be met):**
- SU has decoded a MOT_STS_BCST indicating RFSS supports LoPTT
- SU has completed context activation for trunked data service (per TIA-102.BAEB-C)
- SU has completed Tier-2 location registration (per TIA-102.BAJC-B)
- LoPTT is enabled in the SU codeplug
- GPS receiver is equipped and user-enabled
- GPS reading freshness does not exceed configurable threshold

**Supported call types:** Talkgroup, Emergency, Announcement Group, individually
regrouped subscriber, Remote Unit Monitor-initiated, MFID90 Supergroup.

**Roaming:** NOT supported -- assumes subscriber is on its home system.

### 6.4 MFIDA4 vs MFID90: Mutual Exclusion

The two LoPTT variants are mutually exclusive at a given site. The SU determines which
to use based on whether it has decoded a MOT_STS_BCST message:
- If MOT_STS_BCST decoded: use MFID90 (if other preconditions met)
- If MOT_STS_BCST NOT decoded: use MFIDA4 (if enabled)

---

## 7. FNE / RFSS Behavior

### 7.1 MFIDA4 LoPTT Processing

1. RFSS receives inbound MAC_MFIDA4_LOP from the working channel.
2. If successfully decoded, RFSS routes the location data to the network for conveyance
   to dispatch consoles and other interested network devices.
3. If the RFSS cannot decode the message (legacy site), it ignores the message silently --
   voice is not disrupted.
4. If a newer location update arrives before the previous one has been forwarded, the
   RFSS replaces the stale update with the newer one in its outbound queue.
5. The RFSS does NOT forward location data it cannot successfully decode.
6. Conveyance mechanism to the network is implementation-specific (not standardized).

### 7.2 MFID90 LoPTT Processing

1. RFSS receives inbound MAC_MOT_PTT_LOC during a group call.
2. RFSS creates a data message destined for the SU location service / mapping application:
   - Fills in destination IP address and port from internal RFSS configuration
   - Adds the SU's IP address (from context activation)
   - Adds year, day, and hour to complete the time fields
3. Sends the completed IP message to the location service / mapping application.
4. On the repeat path, RFSS converts the location MAC messages into standard
   MAC_GRP_V_CH_USR messages -- this prevents location data from being broadcast to
   other subscribers while still identifying the talker.
5. For MFID90 supergroup calls, RFSS converts location MACs into
   MAC_MOT_GRG_V_CH_USR messages instead.

### 7.3 User Alias Processing

1. RFSS/FNE receives a standard voice call request.
2. FNE determines the user alias for the transmitting SU from its database.
3. FNE embeds one MAC_MFIDA4_UA per Voice Channel User message in the outbound SACCH.

### 7.4 ISSI / CSSI

Location information is NOT conveyed over ISSI or CSSI for either LoPTT variant.
Location data does not traverse between different RFSS systems.

### 7.5 Encryption

Location information is explicitly NOT end-to-end encrypted, even when voice is encrypted.
The MFID90 P-bit indicates whether the associated voice call is encrypted, but the location
data itself travels in the clear MAC signaling.

---

## 8. Rust Struct Definitions

```rust
/// MAC_MFIDA4_UA -- User Alias (Section 3.99)
/// Outbound: FNE -> SU, SACCH, 17 octets
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MacMfidA4UserAlias {
    /// 14-character ASCII alias, padded with spaces or NUL if shorter.
    pub alias: [u8; 14],
}

impl MacMfidA4UserAlias {
    pub const OCTET_1: u8 = 0xA8; // B1=1, B2=0, MCO=0b101000
    pub const MFID: u8 = 0xA4;
    pub const LENGTH: u8 = 17;
    pub const TOTAL_OCTETS: usize = 17;

    /// Parse from a 17-byte SACCH MAC message.
    pub fn parse(data: &[u8; 17]) -> Option<Self> {
        if data[0] != Self::OCTET_1 || data[1] != Self::MFID {
            return None;
        }
        let mut alias = [0u8; 14];
        alias.copy_from_slice(&data[3..17]);
        Some(Self { alias })
    }

    /// Encode to a 17-byte SACCH MAC message.
    pub fn encode(&self) -> [u8; 17] {
        let mut buf = [0u8; 17];
        buf[0] = Self::OCTET_1;
        buf[1] = Self::MFID;
        buf[2] = Self::LENGTH & 0x1F; // RES bits = 0
        buf[3..17].copy_from_slice(&self.alias);
        buf
    }

    /// Return the alias as a UTF-8 string, trimming NUL and trailing spaces.
    pub fn alias_str(&self) -> &str {
        let s = core::str::from_utf8(&self.alias).unwrap_or("");
        s.trim_end_matches(|c: char| c == '\0' || c == ' ')
    }
}

/// GPS quality indicator for MFIDA4 LoPTT.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum GpsQuality {
    NoFix = 0b00,
    GpsFix = 0b01,
    DgpsFix = 0b10,
    NoGpsHardware = 0b11,
}

/// MAC_MFIDA4_LOP -- MFIDA4 Location on PTT (Section 3.100)
/// Inbound: SU -> FNE, SACCH, 17 octets
#[derive(Debug, Clone, PartialEq)]
pub struct MacMfidA4LocationOnPtt {
    /// Latitude degrees (integer, 0-90)
    pub lat_degrees: u8,
    /// Latitude minutes integer part (0-59)
    pub lat_min_integer: u8,
    /// Latitude minutes decimal part (ten-thousandths, 0-9999)
    pub lat_min_decimal: u16,
    /// true = South, false = North
    pub lat_south: bool,

    /// Longitude degrees (integer, 0-180)
    pub lon_degrees: u8,
    /// Longitude minutes integer part (0-59)
    pub lon_min_integer: u8,
    /// Longitude minutes decimal part (ten-thousandths, 0-9999)
    pub lon_min_decimal: u16,
    /// true = West, false = East
    pub lon_west: bool,

    /// Seconds since midnight (17-bit, 0-131071). 0x1FFFF or 0 = unknown.
    pub seconds_since_midnight: u32,

    /// Course in tenths of degrees (12-bit, 0-3599). 0 = North.
    pub course_tenths: u16,
    /// Speed in tenths of knots (12-bit).
    pub speed_tenths: u16,

    /// Emergency status
    pub emergency: bool,
    /// GPS quality
    pub gps_quality: GpsQuality,
    /// Number of satellites (0-15)
    pub satellite_count: u8,
}

impl MacMfidA4LocationOnPtt {
    pub const OCTET_1: u8 = 0xAA; // B1=1, B2=0, MCO=0b101010
    pub const MFID: u8 = 0xA4;
    pub const LENGTH: u8 = 17;
    pub const TOTAL_OCTETS: usize = 17;

    /// Parse from a 17-byte SACCH MAC message.
    pub fn parse(data: &[u8; 17]) -> Option<Self> {
        if data[0] != Self::OCTET_1 || data[1] != Self::MFID {
            return None;
        }
        let lat_min_decimal = u16::from_be_bytes([data[3], data[4]]);
        let lat_south = (data[5] & 0x80) != 0;
        let lat_min_integer = data[5] & 0x7F;
        let lat_degrees = data[6];

        let lon_min_decimal = u16::from_be_bytes([data[7], data[8]]);
        let lon_west = (data[9] & 0x80) != 0;
        let lon_min_integer = data[9] & 0x7F;
        let lon_degrees = data[10];

        let time_lower16 = u16::from_be_bytes([data[11], data[12]]);
        let t_bit = (data[16] >> 7) & 1;
        let seconds_since_midnight = ((t_bit as u32) << 16) | (time_lower16 as u32);

        let course_low8 = data[13] as u16;
        let speed_low8 = data[14] as u16;
        let course_hi4 = (data[15] & 0x0F) as u16;
        let speed_hi4 = ((data[15] >> 4) & 0x0F) as u16;
        let course_tenths = (course_hi4 << 8) | course_low8;
        let speed_tenths = (speed_hi4 << 8) | speed_low8;

        let emergency = (data[16] & 0x40) != 0;
        let gps_quality_raw = (data[16] >> 4) & 0x03;
        let gps_quality = match gps_quality_raw {
            0b00 => GpsQuality::NoFix,
            0b01 => GpsQuality::GpsFix,
            0b10 => GpsQuality::DgpsFix,
            _ => GpsQuality::NoGpsHardware,
        };
        let satellite_count = data[16] & 0x0F;

        Some(Self {
            lat_degrees,
            lat_min_integer,
            lat_min_decimal,
            lat_south,
            lon_degrees,
            lon_min_integer,
            lon_min_decimal,
            lon_west,
            seconds_since_midnight,
            course_tenths,
            speed_tenths,
            emergency,
            gps_quality,
            satellite_count,
        })
    }

    /// Convert latitude to decimal degrees (positive = North, negative = South).
    pub fn latitude_dd(&self) -> f64 {
        let minutes = (self.lat_min_integer as f64)
            + (self.lat_min_decimal as f64) / 10_000.0;
        let dd = (self.lat_degrees as f64) + (minutes / 60.0);
        if self.lat_south { -dd } else { dd }
    }

    /// Convert longitude to decimal degrees (positive = East, negative = West).
    pub fn longitude_dd(&self) -> f64 {
        let minutes = (self.lon_min_integer as f64)
            + (self.lon_min_decimal as f64) / 10_000.0;
        let dd = (self.lon_degrees as f64) + (minutes / 60.0);
        if self.lon_west { -dd } else { dd }
    }

    /// Returns true if time is unknown (0x1FFFF or 0x00000).
    pub fn time_unknown(&self) -> bool {
        self.seconds_since_midnight == 0x1FFFF || self.seconds_since_midnight == 0
    }

    /// Course in degrees (f64).
    pub fn course_degrees(&self) -> f64 {
        self.course_tenths as f64 / 10.0
    }

    /// Speed in knots (f64).
    pub fn speed_knots(&self) -> f64 {
        self.speed_tenths as f64 / 10.0
    }
}

/// Compass direction (45-degree resolution) for MFID90 LoPTT.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum CompassDirection {
    N  = 0b000,
    NE = 0b001,
    E  = 0b010,
    SE = 0b011,
    S  = 0b100,
    SW = 0b101,
    W  = 0b110,
    NW = 0b111,
}

/// MAC_MOT_PTT_LOC -- MFID90 Location on PTT (Section 3.101)
/// Inbound: SU -> FNE, SACCH, 17 octets
#[derive(Debug, Clone, PartialEq)]
pub struct MacMfid90PttLocation {
    /// GPS reading type. Currently only 0 = Point-2D.
    pub gps_type: u8,
    /// Direction valid flag
    pub direction_valid: bool,
    /// Emergency transmission
    pub emergency: bool,
    /// Encrypted call (P-bit)
    pub encrypted: bool,

    /// Group or Supergroup Address (24-bit WGID)
    pub group_address: u32,
    /// Source Address (24-bit WUID)
    pub source_address: u32,

    /// 3-byte LRRP-truncated latitude (raw 24-bit value, sign+magnitude)
    pub latitude_raw: u32,
    /// 3-byte LRRP-truncated longitude (raw 24-bit value, sign+magnitude)
    pub longitude_raw: u32,

    /// GPS sample minute (0-59)
    pub minute: u8,
    /// GPS sample second code (0=0s, 1=15s, 2=30s, 3=45s)
    pub second_code: u8,

    /// Direction (3-bit compass code)
    pub direction: CompassDirection,
    /// Speed code (5-bit, 2 m/s bins)
    pub speed_code: u8,
}

impl MacMfid90PttLocation {
    pub const OCTET_1: u8 = 0xA1; // B1=1, B2=0, MCO=0b100001
    pub const MFID: u8 = 0x90;
    pub const LENGTH: u8 = 17;
    pub const TOTAL_OCTETS: usize = 17;

    /// Parse from a 17-byte SACCH MAC message.
    pub fn parse(data: &[u8; 17]) -> Option<Self> {
        if data[0] != Self::OCTET_1 || data[1] != Self::MFID {
            return None;
        }

        let flags = data[3];
        let gps_type = (flags >> 6) & 0x03;
        let direction_valid = (flags & 0x20) != 0;
        let emergency = (flags & 0x02) != 0;
        let encrypted = (flags & 0x01) != 0;

        let group_address =
            ((data[4] as u32) << 16) | ((data[5] as u32) << 8) | (data[6] as u32);
        let source_address =
            ((data[7] as u32) << 16) | ((data[8] as u32) << 8) | (data[9] as u32);

        let latitude_raw =
            ((data[10] as u32) << 16) | ((data[11] as u32) << 8) | (data[12] as u32);
        let longitude_raw =
            ((data[13] as u32) << 16) | ((data[14] as u32) << 8) | (data[15] as u32);

        let minute = (data[15] >> 2) & 0x3F;
        let second_code = data[15] & 0x03;

        let direction_code = (data[16] >> 5) & 0x07;
        let direction = match direction_code {
            0 => CompassDirection::N,
            1 => CompassDirection::NE,
            2 => CompassDirection::E,
            3 => CompassDirection::SE,
            4 => CompassDirection::S,
            5 => CompassDirection::SW,
            6 => CompassDirection::W,
            _ => CompassDirection::NW,
        };
        let speed_code = data[16] & 0x1F;

        Some(Self {
            gps_type,
            direction_valid,
            emergency,
            encrypted,
            group_address,
            source_address,
            latitude_raw,
            longitude_raw,
            minute,
            second_code,
            direction,
            speed_code,
        })
    }

    /// Approximate latitude in decimal degrees (positive = North, negative = South).
    /// Uses 3-byte truncated LRRP encoding: sign bit + 23-bit magnitude.
    /// Precision is reduced compared to full 4-byte LRRP.
    pub fn latitude_dd(&self) -> f64 {
        let sign = (self.latitude_raw >> 23) & 1;
        let magnitude = self.latitude_raw & 0x7F_FFFF;
        // Reconstruct: the 3-byte value is the upper 3 bytes of a 4-byte encoding
        // where full encoding = (L * 2^31) / 90.
        // So: magnitude_24bit << 8 approximates the 32-bit sign+magnitude form.
        // L = (magnitude_shifted * 90) / 2^31
        let magnitude_shifted = (magnitude as u64) << 8;
        let dd = (magnitude_shifted as f64) * 90.0 / (2u64.pow(31) as f64);
        if sign == 1 { -dd } else { dd }
    }

    /// Approximate longitude in decimal degrees (positive = East, negative = West).
    /// Uses 3-byte truncated LRRP encoding.
    pub fn longitude_dd(&self) -> f64 {
        let sign = (self.longitude_raw >> 23) & 1;
        let magnitude = self.longitude_raw & 0x7F_FFFF;
        // Full LRRP: encoded = (L * 2^32) / 360 in 2's complement.
        // 3-byte truncation: magnitude << 8 approximates the unsigned absolute value.
        let magnitude_shifted = (magnitude as u64) << 8;
        let dd = (magnitude_shifted as f64) * 360.0 / (2u64.pow(32) as f64);
        if sign == 1 { -dd } else { dd }
    }

    /// GPS sample second (0, 15, 30, or 45).
    pub fn second(&self) -> u8 {
        self.second_code * 15
    }

    /// Speed in m/s (approximate, center of 2 m/s bin).
    /// Code 0 = 0 m/s, code 31 = >=60 m/s.
    pub fn speed_mps_approx(&self) -> f64 {
        self.speed_code as f64 * 2.0
    }
}

/// Unified LoPTT location extracted from either manufacturer variant.
/// Useful for FNE code that processes both message types identically after parsing.
#[derive(Debug, Clone, PartialEq)]
pub struct LoPttLocation {
    pub latitude_dd: f64,
    pub longitude_dd: f64,
    pub emergency: bool,
    /// Source address (WUID). Only present in MFID90 messages.
    pub source_address: Option<u32>,
    /// Group address (WGID). Only present in MFID90 messages.
    pub group_address: Option<u32>,
}

impl LoPttLocation {
    pub fn from_mfida4(msg: &MacMfidA4LocationOnPtt) -> Self {
        Self {
            latitude_dd: msg.latitude_dd(),
            longitude_dd: msg.longitude_dd(),
            emergency: msg.emergency,
            source_address: None,
            group_address: None,
        }
    }

    pub fn from_mfid90(msg: &MacMfid90PttLocation) -> Self {
        Self {
            latitude_dd: msg.latitude_dd(),
            longitude_dd: msg.longitude_dd(),
            emergency: msg.emergency,
            source_address: Some(msg.source_address),
            group_address: Some(msg.group_address),
        }
    }
}
```

---

## 9. Message Dispatch (Detection)

When parsing a 17-octet MAC message from the SACCH, use octet[0] and octet[1] to identify
the message type:

```rust
pub fn identify_loptt_ua_message(data: &[u8; 17]) -> Option<&'static str> {
    match (data[0], data[1]) {
        (0xA8, 0xA4) => Some("MAC_MFIDA4_UA"),      // User Alias
        (0xAA, 0xA4) => Some("MAC_MFIDA4_LOP"),     // MFIDA4 Location on PTT
        (0xA1, 0x90) => Some("MAC_MOT_PTT_LOC"),    // MFID90 Location on PTT
        _ => None,
    }
}
```

---

## 10. Precision Comparison

| Property | MFIDA4 (0xA4) | MFID90 (0x90) |
|---|---|---|
| Coordinate encoding | Degrees + minutes (int + decimal/10000) | LRRP binary, 3-byte truncated |
| Lat precision | ~0.0001 arcmin (~0.19 m) | ~11 m (23-bit, 90/2^23 degrees) |
| Lon precision | ~0.0001 arcmin (~0.19 m at equator) | ~21 m (23-bit, 360/2^24 degrees) |
| Time resolution | 1 second (17-bit seconds since midnight) | 15 seconds (6-bit min + 2-bit sec) |
| Course resolution | 0.1 degree (12-bit) | 45 degrees (3-bit compass) |
| Speed resolution | 0.1 knot (12-bit) | 2 m/s bins (5-bit) |
| Speed max | 409.5 knots (~758 km/h) | 60 m/s (~216 km/h) |
| Includes addresses | No | Yes (WGID + WUID, 24-bit each) |
| Includes encryption state | No | Yes (P-bit) |
| Includes sat count | Yes (4-bit) | No |
| Includes GPS quality | Yes (2-bit) | No |

---

## 11. Known Gaps and Ambiguities

1. **MFIDA4 LOP octet numbering:** The standard's field descriptions for Course/Speed
   reference octets 16/17/18, but the message is only 17 octets and the figure shows
   these fields in octets 15/16/17. The Rust structs above follow the figure layout.
   Implementers should validate against OTA captures.

2. **User Alias padding:** The standard does not specify how aliases shorter than 14
   characters are padded. Space (0x20) and NUL (0x00) are both reasonable assumptions.

3. **MFID90 longitude sign convention:** The standard describes a sign+magnitude approach
   for the 3-byte truncated form but the underlying LRRP uses 2's complement for 4 bytes.
   The truncation and sign-copy procedure described should produce a sign+magnitude 3-byte
   value, but edge cases near +/-180 degrees should be tested against known LRRP outputs.

4. **MFID90 latitude special case:** L=90 degrees encodes as 2^31 - 1 per the standard.
   The 3-byte truncated form would be 0x7FFFFF. This is also the maximum positive magnitude,
   so no special handling is needed in the 3-byte form.

5. **Octet 3 format consistency:** MAC_MFIDA4_UA and MAC_MOT_PTT_LOC both show
   RES[7:5] + Length[4:0] in octet 3, while MAC_MFIDA4_LOP shows Length as a full
   8-bit field. The functional difference is minimal since Length=17=0x11 fits in 5 bits.

6. **MFIDA4 LOP octet 3:** The figure shows "Length = 17" without a RES field, but the
   summary table shows the same B1/B2/MCO pattern as the others. The Length field encoding
   (full byte vs. 5-bit) should be verified against OTA data.

---

## 12. Cross-References

| Standard | Content |
|---|---|
| TIA-102.BBAD-A | Base TDMA MAC messages (parent of BBAD-A-1) |
| TIA-102.BBAE | Base TDMA MAC procedures (parent of BBAE-1) |
| TIA-102.AABC-E-1 | Trunking control channel messages for LoPTT (MOT_STS_BCST) |
| TIA-102.AABD-B | Trunking procedures (call setup) |
| TIA-102.BAEB-C | IP data bearer service (context activation for MFID90) |
| TIA-102.BAJC-B | Tier 2 Location Services / LRRP encoding details |

---

*Extracted from TIA-102.BBAD-A-1 and TIA-102.BBAE-1 full-text markdown.*
