# P25 Block Encryption Protocol -- Implementation Specification

**Source:** TIA-102.AAAD-B (December 2015)
**Classification:** ALGORITHM
**Phase:** 3 -- Implementation-ready
**Extracted:** 2026-04-12
**Purpose:** Defines how block cipher algorithms (DES, 3DES, AES-256) are applied to protect
P25 voice and data transmissions. This spec focuses on what a receiver implementation needs
to detect encrypted traffic, parse clear fields, and display encryption metadata -- without
performing actual decryption.

---

## 1. Encryption Architecture -- What Is Encrypted vs Clear

The single most important architectural fact: **P25 encryption protects payload only.
All framing, signaling, and control fields remain in the clear.**

### 1.1 Fields That Are ALWAYS Clear (Never Encrypted)

```
FDMA (Phase 1):
  - NID (Network ID: NAC + DUID) -- always clear
  - HDU (Header Data Unit): ALGID, KID, MI, TGID -- always clear
  - LDU2 ES fields: ALGID, KID, MI -- always clear
  - Link Control Word octet 0 (LCF byte: P bit + SF + LCO) -- always clear
  - MFID byte (octet 1 of LC) -- always clear
  - TDU and TDULC -- always clear
  - All trunking signaling (TSBK, PDU headers) -- always clear
  - Data packet header blocks -- always clear
  - CRC fields -- always clear (computed over ciphertext)

TDMA (Phase 2):
  - ISCH (Interslot Signaling Channel) -- always clear
  - DUID -- always clear
  - SACCH (Slow Associated Control Channel) -- always clear
  - FACCH (Fast Associated Control Channel) -- always clear
  - ESS fields (ALGID, KID, MI via ESS-A/ESS-B) -- always clear
  - MAC PTT PDU -- always clear
  - All MAC signaling messages -- always clear
```

### 1.2 Fields That ARE Encrypted

```
FDMA (Phase 1):
  - IMBE voice codewords: all 88 bits per frame (11 octets x 18 frames = 198 octets)
  - Link Control information: rightmost 64 bits (octets L1-L8, 8 octets)
  - Low Speed Data: 4 octets (LSD1-LSD4)
  - Reserved octets: 3
  Total per superframe: 213 octets

TDMA (Phase 2):
  - AMBE+2 half-rate voice codewords: 49 bits per frame (7 octets x 18 frames = 126 octets)
  Total per 360ms interval: 126 octets

Data mode:
  - Everything after the ES Auxiliary Header (except CRC and 2ndary SAP byte)
  - Pad octets may or may not be encrypted (implementation-specific)
```

### 1.3 Architecture Diagram

```
  TRANSMITTER SIDE                              RECEIVER SIDE

  [Voice/Data Payload]                          [Display: "ENCRYPTED"]
         |                                             ^
    [IMBE/AMBE encode]                          [Cannot decode -- no key]
         |                                             |
    [XOR with keystream] ----ciphertext----->   [Ciphertext bits -- skip]
         |                                             |
    [Insert into LDU/burst]                     [Parse clear fields]
         |                                             |
    [Add NID, HDU, LC, ESS] ----- clear -----> [Read NID, HDU, LC, ESS]
         |                                             |
    [Transmit on RF]                            [Display ALGID, KID, MI]
```

**Key insight for receiver implementation:** A receiver without keys can still identify
who is talking (Source Address from LC octet 0 + unencrypted portions), what group
they are on (TGID from HDU/LC), what algorithm they use (ALGID), which key slot (KID),
and the current MI. The only thing lost is the voice/data payload content.

---

## 2. ESS (Encryption Sync Signal) Structure

The ESS carries three fields that together enable a receiver to identify and synchronize
with an encrypted transmission.

### 2.1 ESS Field Layout

```
Total ESS information: 96 bits

Bits [95:88]  ALGID    8 bits   Algorithm Identifier
Bits [87:72]  KID     16 bits   Key Identifier
Bits [71:0]   MI      72 bits   Message Indicator

MI breakdown:
  MI[71:8]  = 64 bits of LFSR state (the actual cryptographic seed)
  MI[7:0]   = 8 reserved bits, always $00
```

### 2.2 ESS in FDMA (Phase 1) -- LDU2 Frames

In FDMA, the ESS is carried in the LDU2 portion of each voice superframe (360 ms).

```
FDMA Voice Superframe (360 ms):
  [HDU] -> [LDU1] -> [LDU2] -> [LDU1] -> [LDU2] -> ...
                        ^
                        |
                  Contains ESS:
                    ALGID (8 bits)
                    KID   (16 bits)
                    MI    (72 bits)

LDU2 internal structure (per BAAA-B):
  The 96-bit ESS is distributed across the LDU2 frame in the
  Encryption Sync (ES) field positions between voice frames 10-18.

First transmission: IV is sent in the HDU MI field.
Subsequent superframes: Next MI (from LFSR) is sent in LDU2 ES fields.
```

Cross-reference: **BAAA-B** Section 7 defines the exact bit positions of the ES
fields within LDU2. The LDU2 DUID = $0A.

### 2.3 ESS in TDMA (Phase 2) -- ESS-A and ESS-B

In TDMA, the 96-bit ESS information is RS-encoded into a (44,16,29) Reed-Solomon
codeword (264 bits) and distributed across five voice bursts per superframe.

```
TDMA Voice Superframe (360 ms = 12 timeslots, 5 voice bursts per LCH):

  4V burst 1:  ESS-B  =  4 hexbits  =  24 bits  -> ALGID[7:0] + KID[15:0]
  4V burst 2:  ESS-B  =  4 hexbits  =  24 bits  -> MI[71:48]
  4V burst 3:  ESS-B  =  4 hexbits  =  24 bits  -> MI[47:24]
  4V burst 4:  ESS-B  =  4 hexbits  =  24 bits  -> MI[23:0]
  2V burst:    ESS-A  = 28 hexbits  = 168 bits   -> RS parity + info
               Total: 44 hexbits = 264 bits (RS codeword)
```

Cross-reference: **BBAC-A** Section 7 defines the ESS structure and distribution.
**BBAC-1** Annex E provides the exact burst bit allocations, with ESS-B bits at
symbol positions 84-95 in 4V bursts.

### 2.4 Progressive ESS Decode Strategy (TDMA)

```
The RS(44,16,29) code has minimum distance 29, so it can correct up to 14 erasures.
Since ESS-B segments arrive before ESS-A:

  ESS-A only (28 hexbits known, 16 erased):     correctable
  ESS-A + 1 ESS-B (32 known, 12 erased):        better margin
  ESS-A + all 4 ESS-B (44 known, 0 erased):     maximum correction

A late-entry receiver can begin decoding ESS from partial segments.
```

---

## 3. ALGID Values -- Complete Table

From **BAAC-D** Table 6 (Algorithm ID assignments):

### 3.1 Standard ALGID Values

| ALGID | Hex | Name | Block Size (n) | Key Size (k) | Status |
|-------|-----|------|----------------|---------------|--------|
| $00 | 0x00 | ACCORDION 1.3 | -- | -- | Type 1 (classified) |
| $01 | 0x01 | BATON (Auto Even) | -- | -- | Type 1 (classified) |
| $02 | 0x02 | FIREFLY Type 1 | -- | -- | Type 1 (classified) |
| $03 | 0x03 | MAYFLY Type 1 | -- | -- | Type 1 (classified) |
| $04 | 0x04 | SAVILLE | -- | -- | Type 1 (classified) |
| $41 | 0x41 | BATON (Auto Odd) | -- | -- | Type 1 (classified) |
| $80 | 0x80 | **Unencrypted** | -- | -- | **Indicates clear traffic** |
| $81 | 0x81 | DES-OFB | 64 | 64 | Deprecated (NIST withdrew 2005) |
| $83 | 0x83 | Triple DES (3-key TDEA) | 64 | 192 | Deprecated (NIST 2019) |
| $84 | 0x84 | AES-256-OFB | 128 | 256 | **Current recommended** |
| $85 | 0x85 | AES-128 | 128 | 128 | Defined in BAAC-D |

### 3.2 Vendor/Proprietary ALGID Values (from SDRTrunk/field observation)

| ALGID | Name | Notes |
|-------|------|-------|
| $88 | AES-CBC | Seen in some newer deployments |
| $9F | Motorola DES-XL | Proprietary Motorola |
| $A0 | DVI-XL (DVP+XL) | Motorola legacy |
| $A1 | DVP/PADSTONE | Motorola proprietary |
| $AA | ADP/RC4 | Motorola Advanced Data Protection (RC4-based, weak) |

Values between $04 and $41, and between $41 and $80, are governed by the external
ALGID Value Assignment Guide maintained by TIA TR-8.15.

### 3.3 ALGID Interpretation Rules

```
if algid == 0x80 {
    // Unencrypted -- voice/data payload is cleartext
    // MI will be all zeros, KID will be $0000
} else if algid == 0x00 {
    // Could be uninitialized/invalid -- treat as unknown
    // Some implementations treat $00 as ACCORDION 1.3
} else {
    // Encrypted -- voice/data payload is ciphertext
    // Cannot decode voice without the corresponding key
}
```

---

## 4. Keystream Generation

### 4.1 LFSR (MI Sequence Generator)

The MI is derived from a 64-bit Linear Feedback Shift Register:

```
Polynomial: C(x) = 1 + x^15 + x^27 + x^38 + x^46 + x^62 + x^64
Taps:       stages 15, 27, 38, 46, 62, 64
Period:     2^64 - 1 (maximal length sequence)
Advance:    64 shifts per superframe (produces next MI)

Register layout:
  Stage 64 (MSB) = MI(71)
  Stage  1 (LSB) = MI(8)
  MI(7:0) = reserved = $00

Feedback = XOR of stages 15, 27, 38, 46, 62, 64
New bit enters stage 1 after each shift.
After 64 shifts: entire register has advanced to the next MI.
```

### 4.2 OFB (Output Feedback) Mode

All supported algorithms operate in OFB mode. The block cipher is always used in
**encrypt** direction for both encryption and decryption.

```
Initialization:
  1. Load MI[71:8] (64 bits) into LFSR
  2. If n > 64: expand MI using LFSR (shift n-64 times in n-stage register)
     For AES-256 (n=128): produces 128-bit input from 64-bit MI
  3. Load (possibly expanded) MI into block cipher input register

Keystream generation per superframe:
  Iteration 1: bEnc(MI_expanded) -> Out1  [DISCARDED -- not used for XOR]
               Feed Out1 back to input register
  Iteration 2: bEnc(Out1) -> Out2 = Keystream block 1
               Feed Out2 back to input register
  Iteration 3: bEnc(Out2) -> Out3 = Keystream block 2
  ...
  Iteration B: bEnc(Out_{B-1}) -> OutB = Keystream block B-1

Each iteration produces m = floor(n/8) keystream octets.
Rightmost r = n mod 8 bits are discarded (but still fed back).
```

### 4.3 OFB Parameters by Algorithm

| Algorithm | n | m | r | B (FDMA) | B (TDMA) |
|-----------|---|---|---|----------|----------|
| DES ($81) | 64 | 8 | 0 | 28 | 17 |
| 3DES ($83) | 64 | 8 | 0 | 28 | 17 |
| AES-256 ($84) | 128 | 16 | 0 | 15 | 9 |

```
B = 1 + ceil(encryptable_octets / m)
FDMA: encryptable_octets = 213
TDMA: encryptable_octets = 126
```

### 4.4 MI Expansion for AES-256 (n=128)

When n > 64, the 64-bit MI must be expanded to fill the 128-bit input register.

```
Algorithm:
  1. Load MI[71:8] (64 bits) into an n-stage register (leftmost 64 bits)
  2. Apply the LFSR polynomial taps to the rightmost 64 bits
  3. Shift left (n - 64) = 64 times
  4. Result: 128-bit value = [original MI(71:8)] ++ [next LFSR state]

Example (from Table 4-1):
  MI = 1234 5678 90AB CDEF
  Expanded (n=128): 1234 5678 90AB CDEF 6FE2 802A A403 828B
                    |<-- original MI -->||<-- LFSR extension -->|
```

---

## 5. Voice Encryption

### 5.1 FDMA Voice Encryption (Phase 1)

Each FDMA superframe (360 ms) contains 213 encryptable octets:

```
213 octets total:
   3 reserved octets
   8 LC information octets (rightmost 64 bits of Link Control Word)
  18 IMBE frames x 11 octets = 198 octets
   4 LSD octets (Low Speed Data)

IMBE frame packing (u0-u7 into w0-w10):
  w0  = u0[11:4]
  w1  = u0[3:0] | u1[11:8]
  w2  = u1[7:0]
  w3  = u2[11:4]
  w4  = u2[3:0] | u3[11:8]
  w5  = u3[7:0]
  w6  = u4[10:3]
  w7  = u4[2:0] | u5[10:6]
  w8  = u5[5:0] | u6[10:9]
  w9  = u6[8:1]
  w10 = u6[0] | u7[6:0]
```

### 5.2 Encryption Schedule Summary (FDMA)

```
LDU1 (octets 1-112):
  Octets  1-3:   Reserved (encrypted)
  Octets  4-11:  LC L1-L8 (encrypted)
  Octets 12-22:  Voice frame 1 (w0-w10)
  Octets 23-33:  Voice frame 2
  ...
  Octets 100-101: LSD1, LSD2
  Octets 102-112: Voice frame 9

LDU2 (octets 113-213):
  Octets 113-123: Voice frame 10
  ...
  Octets 201-202: LSD3, LSD4
  Octets 203-213: Voice frame 18

For DES/3DES (m=8): keystream blocks B#2 through B#28
  Octet # maps to block B# = 1 + ceil(#/8), position m# = (#-1) mod 8

For AES-256 (m=16): keystream blocks B#2 through B#15
  Octet # maps to block B# = 1 + ceil(#/16), position m# = (#-1) mod 16
```

### 5.3 TDMA Voice Encryption (Phase 2)

Each 360 ms interval contains 18 half-rate AMBE+2 voice frames.

```
126 octets total:
  18 voice frames x 7 octets = 126 octets

Half-rate vocoder packing (u0-u3 into W0-W6):
  W0 = u0[11:4]
  W1 = u0[3:0] | u1[11:8]
  W2 = u1[7:0]
  W3 = u2[10:3]
  W4 = u2[2:0] | u3[13:9]
  W5 = u3[8:1]
  W6 = u3[0] | 7 reserved bits (not transmitted)

Note: The 7 LSBs of W6 are not transmitted and need not be decrypted.
```

### 5.4 What Happens When Encrypted Voice Is Received Without Keys

The receiver sees valid NID, valid frame sync, and valid LDU structure. However:

```
1. IMBE/AMBE codewords are XORed with unknown keystream
2. Feeding ciphertext into the vocoder produces random noise / silence
3. The vocoder may detect invalid codewords and mute output
4. SDRTrunk behavior: displays "ENCRYPTED" and shows ALGID/KID/MI
5. OP25 behavior: logs encryption params, mutes audio output
```

---

## 6. Data Encryption

### 6.1 ES Auxiliary Header Structure

Data packets carry encryption sync in an ES Auxiliary Header prepended to each fragment:

```
Octet  0:  MI octet 0  = MI[71:64]
Octet  1:  MI octet 1  = MI[63:56]
Octet  2:  MI octet 2  = MI[55:48]
Octet  3:  MI octet 3  = MI[47:40]
Octet  4:  MI octet 4  = MI[39:32]
Octet  5:  MI octet 5  = MI[31:24]
Octet  6:  MI octet 6  = MI[23:16]
Octet  7:  MI octet 7  = MI[15:8]
Octet  8:  Reserved     = $00
Octet  9:  ALGID
Octet 10:  KID high byte
Octet 11:  KID low byte
Octet 12:  [1|1|1| 2ndary SAP (5 bits)]   <-- NOT encrypted
```

### 6.2 Encrypted Data Packet Layout

```
Unconfirmed packet:
  [Header Block (12 octets)]     NOT encrypted
  [ES Aux Header (13 octets)]    NOT encrypted (MI, ALGID, KID, 2ndary SAP)
  [Data octets]                  ENCRYPTED
  [Pad octets]                   may or may not be encrypted
  [CRC (4 octets)]               NOT encrypted (computed over ciphertext)

Confirmed packet:
  [Header Block (12 octets)]     NOT encrypted
  [Serial# + CRC9 (2 octets)]   NOT encrypted
  [ES Aux Header (13 octets)]    NOT encrypted
  [Data octets]                  ENCRYPTED
  [Pad + CRC]                    CRC not encrypted
```

### 6.3 Data OFB Operation

```
Same OFB procedure as voice:
  1. Load MI into input register (expand if n > 64)
  2. First bEnc iteration: result discarded, fed back
  3. Subsequent iterations: XOR output with data octets

Data octet 0 XORs with output register octet 0
Data octet 1 XORs with output register octet 1
...
Every m octets, iterate OFB again.

For 512 data octets with DES (m=8): ceil(512/8) + 1 = 65 iterations
For 512 data octets with AES-256 (m=16): ceil(512/16) + 1 = 33 iterations
```

---

## 7. MI Management

### 7.1 MI as Initialization Vector

```
The MI serves as the IV (Initialization Vector) for each superframe.
- First MI = IV, generated at transmission start (method manufacturer-specific)
- All-zero MI ($000000000000000000) = INVALID / reserved for unencrypted
- If IV generation produces all-zero, must retry

The MI is NOT a nonce in the modern sense -- it is an LFSR-derived sequence.
Given any MI, all subsequent MIs are deterministic via the LFSR polynomial.
```

### 7.2 LFSR Advancement

```
Per-superframe LFSR advancement:
  1. Current LFSR state = current MI[71:8]
  2. Shift LFSR 64 times using C(x) = 1 + x^15 + x^27 + x^38 + x^46 + x^62 + x^64
  3. New LFSR state = next MI[71:8]
  4. Append MI[7:0] = $00

Both FDMA and TDMA advance MI once per 360 ms.
```

### 7.3 MI Transmission Points

```
FDMA:
  - Header (HDU): carries the IV (first MI)
  - Every LDU2: carries the next MI for the following superframe
  - A receiver joining mid-call can sync from the next LDU2

TDMA:
  - MAC_PTT PDU: carries the IV (first MI)
  - Every 360 ms: MI distributed across ESS-A/ESS-B in 5 voice bursts
  - Progressive ESS decode allows mid-call sync
```

### 7.4 MI Verification

```
A receiver can verify it is still synchronized by:
  1. Predict next MI from current MI using LFSR
  2. Compare predicted MI with received MI in next LDU2/ESS
  3. If mismatch: resync from the received MI value
  4. If match: encryption stream is still synchronized

This is the receiver's only mechanism for detecting desync.
The receiver cannot verify correctness of decrypted audio -- it can only
check MI continuity.
```

---

## 8. Detecting Encrypted Traffic

### 8.1 Primary Indicators

A receiver identifies encrypted vs clear traffic through multiple redundant indicators:

```
Method 1: ALGID in ESS
  if algid != 0x80 -> encrypted
  if algid == 0x80 -> unencrypted

Method 2: Protected bit (P) in Link Control Word
  LCW octet 0, bit 7 (MSB):
    P = 0 -> clear
    P = 1 -> encrypted (rightmost 64 bits of LC are ciphertext)

Method 3: LCF byte value (combines P + SF + LCO)
  Group Voice Channel User:
    $00 = clear          $80 = encrypted
  Unit-to-Unit Voice Channel User:
    $03 = clear          $83 = encrypted
  Group Voice Channel Update:
    $42 = clear          $C2 = encrypted
  General rule: encrypted LCF = unencrypted LCF + $80

Method 4: MI value
  MI = $000000000000000000 -> unencrypted
  MI = anything else       -> encrypted (in theory)

Method 5: ALGID in HDU (FDMA only)
  HDU carries ALGID in the clear, available at call start.

Method 6: Protected bit in MAC header (TDMA)
  MAC PDU header flags indicate encrypted payload.
```

### 8.2 Reliable Detection Logic

```rust
/// Returns true if the traffic is encrypted
fn is_encrypted(algid: u8, mi: &[u8; 9], lcf: u8) -> bool {
    // Primary check: ALGID
    if algid != 0x80 {
        return true;
    }

    // Secondary check: LCF Protected bit
    if lcf & 0x80 != 0 {
        return true;  // P bit set
    }

    // Tertiary check: MI is non-zero
    if mi.iter().any(|&b| b != 0) {
        return true;  // Non-zero MI with ALGID=$80 is anomalous
    }

    false
}
```

### 8.3 Unencrypted Default Values

Per AAAD-B Table 5-1 and BAAC-D:

| Field | Unencrypted Value |
|-------|-------------------|
| ALGID | $80 |
| KID | $0000 |
| MI | $000000000000000000 (72 bits, all zero) |

---

## 9. Parsing Encrypted Packets Gracefully

### 9.1 Fields Readable on Encrypted Voice (FDMA)

```
Always readable (clear):
  - NAC (12 bits from NID)
  - DUID (4 bits from NID)
  - ALGID, KID, MI (from HDU and LDU2 ES)
  - LCF byte (octet 0 of LC) -- tells you call type
  - MFID (octet 1 of LC, if P=0 in the SF=0 Explicit format)
  - Talkgroup ID (from HDU, if present)
  - Source ID (from HDU, if present)

Readable only when P=0 (unencrypted LC):
  - Service Options (LC octet 2)
  - Group Address / Source Address (LC octets 3-8)

NOT readable when P=1 (encrypted LC):
  - LC octets 1-8 are ciphertext (64 bits encrypted)
  - But: LCF byte (octet 0) is ALWAYS clear, so call type is known
  - Source/Group addresses must come from HDU or trunking channel grant
```

### 9.2 Fields Readable on Encrypted Voice (TDMA)

```
Always readable (clear):
  - ISCH (sync/channel info)
  - SACCH signaling (MAC messages)
  - ALGID, KID, MI (from ESS-A/ESS-B)
  - Any FACCH MAC messages

NOT readable:
  - Voice codewords (ciphertext in VTCH bursts)
```

### 9.3 Fields Readable on Encrypted Data

```
Always readable (clear):
  - Data Header Block (12 octets) -- SAP, format, length info
  - ES Auxiliary Header: MI, ALGID, KID, Secondary SAP
  - CRC (computed over ciphertext, still verifiable)
  - Serial#/CRC9 in confirmed packets

NOT readable:
  - Data payload octets after the ES header
```

### 9.4 Display Strategy for Encrypted Traffic

```
When encrypted traffic is detected, display:

1. Call metadata (from clear fields):
   "ENCRYPTED VOICE CALL"
   "  Talkgroup: 12345"       (from HDU or trunking grant)
   "  Source: 1234567"         (from HDU or trunking grant)

2. Encryption parameters:
   "  Algorithm: AES-256"      (ALGID $84)
   "  Key ID: 0x1A2B"          (KID)
   "  MI: 1234567890ABCDEF00"  (72-bit MI in hex)

3. Status:
   "  Audio: MUTED (encrypted)"

Do NOT attempt to feed ciphertext to the vocoder -- it produces
random noise that could be very loud.
```

### 9.5 Handling Algorithm Transitions

```
A call can transition between encrypted and unencrypted mid-stream
(though this is rare in practice):

  Monitor ALGID in every LDU2 (FDMA) or ESS (TDMA).
  If ALGID changes from encrypted to $80: resume clear audio.
  If ALGID changes from $80 to encrypted: mute and display params.
```

---

## 10. Rust Types

### 10.1 Algorithm ID Enum

```rust
/// P25 Algorithm Identifier (ALGID) per BAAC-D Table 6
/// and the ALGID Value Assignment Guide.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[repr(u8)]
pub enum AlgorithmId {
    /// Type 1 classified algorithms (NSA)
    Accordion1_3   = 0x00,
    BatonAutoEven  = 0x01,
    Firefly        = 0x02,
    Mayfly         = 0x03,
    Saville        = 0x04,
    BatonAutoOdd   = 0x41,

    /// Unencrypted (clear traffic)
    Unencrypted    = 0x80,

    /// Standard Type 3/4 algorithms
    DesOfb         = 0x81,
    TripleDesOfb   = 0x83,
    Aes256Ofb      = 0x84,
    Aes128         = 0x85,

    /// Vendor/proprietary (commonly seen in the field)
    AesCbc         = 0x88,
    MotorolaDesXl  = 0x9F,
    DviXl          = 0xA0,
    DvpPadstone    = 0xA1,
    AdpRc4         = 0xAA,
}

impl AlgorithmId {
    pub fn from_byte(b: u8) -> Self {
        match b {
            0x00 => Self::Accordion1_3,
            0x01 => Self::BatonAutoEven,
            0x02 => Self::Firefly,
            0x03 => Self::Mayfly,
            0x04 => Self::Saville,
            0x41 => Self::BatonAutoOdd,
            0x80 => Self::Unencrypted,
            0x81 => Self::DesOfb,
            0x83 => Self::TripleDesOfb,
            0x84 => Self::Aes256Ofb,
            0x85 => Self::Aes128,
            0x88 => Self::AesCbc,
            0x9F => Self::MotorolaDesXl,
            0xA0 => Self::DviXl,
            0xA1 => Self::DvpPadstone,
            0xAA => Self::AdpRc4,
            other => {
                // Unknown ALGID -- treat as encrypted with unknown algorithm
                // Log or store the raw value
                Self::Unencrypted // fallback; real impl should handle Unknown(u8)
            }
        }
    }

    /// Returns true if this ALGID indicates encrypted traffic.
    pub fn is_encrypted(&self) -> bool {
        *self != Self::Unencrypted
    }

    /// Human-readable name for display.
    pub fn name(&self) -> &'static str {
        match self {
            Self::Accordion1_3  => "ACCORDION 1.3",
            Self::BatonAutoEven => "BATON (Auto Even)",
            Self::Firefly       => "FIREFLY",
            Self::Mayfly        => "MAYFLY",
            Self::Saville       => "SAVILLE",
            Self::BatonAutoOdd  => "BATON (Auto Odd)",
            Self::Unencrypted   => "Unencrypted",
            Self::DesOfb        => "DES-OFB",
            Self::TripleDesOfb  => "3DES-OFB (TDEA)",
            Self::Aes256Ofb     => "AES-256-OFB",
            Self::Aes128        => "AES-128",
            Self::AesCbc        => "AES-CBC",
            Self::MotorolaDesXl => "Motorola DES-XL",
            Self::DviXl         => "DVI-XL",
            Self::DvpPadstone   => "DVP/PADSTONE",
            Self::AdpRc4        => "ADP (RC4)",
        }
    }

    /// Block size in bits (n). Returns None for classified/unknown algorithms.
    pub fn block_size_bits(&self) -> Option<u32> {
        match self {
            Self::DesOfb | Self::TripleDesOfb => Some(64),
            Self::Aes256Ofb | Self::Aes128 | Self::AesCbc => Some(128),
            _ => None,
        }
    }

    /// OFB keystream octets per iteration (m = n/8).
    pub fn keystream_octets_per_block(&self) -> Option<u32> {
        self.block_size_bits().map(|n| n / 8)
    }

    /// OFB iterations needed per FDMA superframe (B).
    pub fn ofb_iterations_fdma(&self) -> Option<u32> {
        self.keystream_octets_per_block().map(|m| 1 + (213 + m - 1) / m)
    }

    /// OFB iterations needed per TDMA 360ms interval (B).
    pub fn ofb_iterations_tdma(&self) -> Option<u32> {
        self.keystream_octets_per_block().map(|m| 1 + (126 + m - 1) / m)
    }
}
```

### 10.2 Encryption Sync Signal Struct

```rust
/// Encryption Synchronization Signal -- carried in LDU2 (FDMA) or ESS (TDMA).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct EncryptionSyncSignal {
    /// Algorithm Identifier (8 bits)
    pub algid: u8,
    /// Key Identifier (16 bits)
    pub kid: u16,
    /// Message Indicator (72 bits: 64 LFSR bits + 8 reserved zero bits)
    /// Stored as 9 bytes, MI[71:64] in mi[0], MI[7:0] in mi[8].
    pub mi: [u8; 9],
}

impl EncryptionSyncSignal {
    pub const UNENCRYPTED: Self = Self {
        algid: 0x80,
        kid: 0x0000,
        mi: [0u8; 9],
    };

    /// Parse from 96 raw information bits (ALGID || KID || MI).
    pub fn from_bits(bits: &[u8; 96]) -> Self {
        let algid = bits_to_u8(&bits[0..8]);
        let kid = bits_to_u16(&bits[8..24]);
        let mut mi = [0u8; 9];
        for i in 0..9 {
            mi[i] = bits_to_u8(&bits[24 + i * 8..24 + (i + 1) * 8]);
        }
        Self { algid, kid, mi }
    }

    /// Parse from the 12-byte ES Auxiliary Header in data packets
    /// (octets 0-7 = MI[71:8], octet 8 = reserved, octet 9 = ALGID,
    ///  octets 10-11 = KID).
    pub fn from_es_aux_header(bytes: &[u8; 12]) -> Self {
        let mut mi = [0u8; 9];
        mi[..8].copy_from_slice(&bytes[0..8]);
        mi[8] = bytes[8]; // reserved, should be $00
        Self {
            algid: bytes[9],
            kid: u16::from_be_bytes([bytes[10], bytes[11]]),
            mi,
        }
    }

    pub fn is_encrypted(&self) -> bool {
        self.algid != 0x80
    }

    pub fn algorithm(&self) -> AlgorithmId {
        AlgorithmId::from_byte(self.algid)
    }

    /// Extract the 64-bit LFSR state from the MI (bits 71:8).
    pub fn lfsr_state(&self) -> u64 {
        u64::from_be_bytes([
            self.mi[0], self.mi[1], self.mi[2], self.mi[3],
            self.mi[4], self.mi[5], self.mi[6], self.mi[7],
        ])
    }

    /// Format MI as hex string for display.
    pub fn mi_hex(&self) -> String {
        self.mi.iter().map(|b| format!("{:02X}", b)).collect::<Vec<_>>().join("")
    }

    /// Format a human-readable summary.
    pub fn display_summary(&self) -> String {
        if self.is_encrypted() {
            format!(
                "ENCRYPTED [{}] KID=0x{:04X} MI={}",
                self.algorithm().name(),
                self.kid,
                self.mi_hex()
            )
        } else {
            "UNENCRYPTED".to_string()
        }
    }
}
```

### 10.3 LFSR MI Generator

```rust
/// P25 LFSR MI generator.
/// Polynomial: C(x) = 1 + x^15 + x^27 + x^38 + x^46 + x^62 + x^64
pub struct MiLfsr {
    state: u64,
}

impl MiLfsr {
    /// Initialize from MI[71:8] (the 64 LFSR bits).
    pub fn new(mi_64: u64) -> Self {
        Self { state: mi_64 }
    }

    /// Initialize from the full 9-byte MI (ignores MI[7:0]).
    pub fn from_mi_bytes(mi: &[u8; 9]) -> Self {
        let state = u64::from_be_bytes([
            mi[0], mi[1], mi[2], mi[3],
            mi[4], mi[5], mi[6], mi[7],
        ]);
        Self { state }
    }

    /// Advance the LFSR by one clock cycle.
    fn clock_once(&mut self) {
        // Taps at positions 15, 27, 38, 46, 62, 64
        // In 0-indexed: bits 14, 26, 37, 45, 61, 63
        let feedback = ((self.state >> 63) // bit 64 (MSB)
            ^ (self.state >> 61)           // bit 62
            ^ (self.state >> 45)           // bit 46
            ^ (self.state >> 37)           // bit 38
            ^ (self.state >> 26)           // bit 27
            ^ (self.state >> 14))          // bit 15
            & 1;
        self.state = (self.state << 1) | feedback;
    }

    /// Advance 64 clocks to produce the next MI.
    pub fn next_mi(&mut self) -> [u8; 9] {
        for _ in 0..64 {
            self.clock_once();
        }
        let mut mi = [0u8; 9];
        let bytes = self.state.to_be_bytes();
        mi[..8].copy_from_slice(&bytes);
        mi[8] = 0x00; // reserved
        mi
    }

    /// Get current state as 64-bit value.
    pub fn state(&self) -> u64 {
        self.state
    }

    /// Expand MI to n bits for algorithms with n > 64 (e.g., AES-256, n=128).
    /// Returns the expanded value as a byte array.
    pub fn expand_mi(mi_64: u64, n: usize) -> Vec<u8> {
        assert!(n >= 64);
        if n == 64 {
            return mi_64.to_be_bytes().to_vec();
        }

        // Use an n-stage register with LFSR taps on rightmost 64 bits
        // Load MI into leftmost 64 bits, shift left (n-64) times
        let extra_bits = n - 64;
        let mut lfsr = Self::new(mi_64);

        // The expansion shifts (n-64) times, growing the register.
        // For n=128: shift 64 times. The original 64 bits remain at left,
        // new 64 bits at right = next LFSR state.
        for _ in 0..extra_bits {
            lfsr.clock_once();
        }

        let mut result = Vec::with_capacity(n / 8);
        result.extend_from_slice(&mi_64.to_be_bytes());
        result.extend_from_slice(&lfsr.state.to_be_bytes());

        // Trim to exactly n/8 bytes
        result.truncate(n / 8);
        result
    }
}
```

### 10.4 Encrypted Traffic Detector

```rust
/// Consolidated encrypted traffic detection from all available indicators.
#[derive(Debug, Clone)]
pub struct EncryptionStatus {
    pub is_encrypted: bool,
    pub algid: AlgorithmId,
    pub kid: u16,
    pub mi: [u8; 9],
}

impl EncryptionStatus {
    /// Detect encryption from ESS (LDU2 or TDMA ESS).
    pub fn from_ess(ess: &EncryptionSyncSignal) -> Self {
        Self {
            is_encrypted: ess.is_encrypted(),
            algid: ess.algorithm(),
            kid: ess.kid,
            mi: ess.mi,
        }
    }

    /// Detect encryption from LCF byte alone (quick check before ESS available).
    pub fn from_lcf(lcf_byte: u8) -> bool {
        // Protected bit = bit 7 of LCF byte
        lcf_byte & 0x80 != 0
    }

    /// Detect encryption from the FDMA HDU fields.
    pub fn from_hdu(algid: u8, kid: u16, mi: [u8; 9]) -> Self {
        let alg = AlgorithmId::from_byte(algid);
        Self {
            is_encrypted: alg.is_encrypted(),
            algid: alg,
            kid,
            mi,
        }
    }

    /// Check if this matches the unencrypted default values.
    pub fn is_clear_defaults(&self) -> bool {
        self.algid == AlgorithmId::Unencrypted
            && self.kid == 0x0000
            && self.mi == [0u8; 9]
    }
}
```

### 10.5 LCF Encryption Mapping

```rust
/// Map between clear and encrypted LCF values.
/// Encrypted LCF = Clear LCF | 0x80 (P bit set).

/// Common LCF values for reference:
pub const LCF_GROUP_VOICE_CLEAR: u8 = 0x00;
pub const LCF_GROUP_VOICE_ENCRYPTED: u8 = 0x80;
pub const LCF_UNIT_TO_UNIT_CLEAR: u8 = 0x03;
pub const LCF_UNIT_TO_UNIT_ENCRYPTED: u8 = 0x83;
pub const LCF_GROUP_UPDATE_CLEAR: u8 = 0x42;
pub const LCF_GROUP_UPDATE_ENCRYPTED: u8 = 0xC2;

/// Extract encryption status from LCF byte.
pub fn lcf_is_encrypted(lcf: u8) -> bool {
    lcf & 0x80 != 0
}

/// Get the base (unencrypted) LCO from any LCF byte.
pub fn lcf_to_lco(lcf: u8) -> u8 {
    lcf & 0x3F  // Lower 6 bits = LCO
}
```

---

## Cross-References

### To BAAA-B (FDMA CAI)

- **LDU2 ES field positions:** BAAA-B defines the exact bit locations within LDU2
  where ALGID (8 bits), KID (16 bits), and MI (72 bits) are carried.
- **HDU structure:** BAAA-B defines the Header Data Unit containing ALGID, KID, MI,
  and TGID -- all in the clear.
- **Superframe structure:** 2 LDUs per superframe, 360 ms, 18 IMBE frames.
- **LSD field positions:** 4 octets of Low Speed Data within the superframe.

### To BBAC-A / BBAC-1 (TDMA MAC)

- **ESS-A/ESS-B distribution:** BBAC-A Section 7 defines the 96-bit ESS encoded as
  RS(44,16,29) and distributed across 4 ESS-B segments (24 bits each in 4V bursts)
  and 1 ESS-A segment (168 bits in 2V burst).
- **ESS-B content mapping:** Burst 1 = ALGID+KID, Bursts 2-4 = MI (24 bits each).
- **Burst bit positions:** BBAC-1 Annex E tables show ESS-B at symbols 84-95 in
  4V bursts (both inbound and outbound).

### To BAAC-D (Reserved Values)

- **ALGID assignments:** BAAC-D Table 6 is the authoritative source for Algorithm ID
  values. Values $00-$04 and $41 are Type 1 (NSA classified). Values $81, $83, $84,
  $85 are standard Type 3/4. Value $80 = unencrypted.
- **KID $0000:** Reserved as default for single-key systems.
- **Additional ALGIDs** beyond the eleven listed are governed by the ALGID Value
  Assignment Guide (TIA TR-8.15, April 2015).

### To AABF-D (Link Control Words)

- **Protected bit (P):** Bit 7 of LCW octet 0. When P=1, octets 1-8 of the LC
  are encrypted. The LCF byte (octet 0) always remains clear.
- **Encrypted LCF values:** $80 = Group Voice (encrypted), $83 = Unit-to-Unit
  (encrypted). General rule: encrypted LCF = unencrypted LCF + $80.
- **When P=1:** Source Address and Group Address in the LC are ciphertext.
  These identifiers must be obtained from the HDU or trunking signaling instead.

---

## SDRTrunk Cross-Reference

SDRTrunk (Java) handles encryption detection at:

- **`Encryption.java`** (`io.github.dsheirer.module.decode.p25.reference`):
  Enumerates all known ALGID values including standard and proprietary.
  This is the primary ALGID dispatch table and closely mirrors Section 3 above.

- **Encrypted call handling:** SDRTrunk identifies encrypted calls, displays
  "ENCRYPTED" status, and shows ALGID name, KID hex, and MI hex. It does not
  attempt decryption. This is the reference behavior for a non-decrypting receiver.

- **LCW parsing:** `LinkControlWordFactory.java` dispatches on the full LCF byte,
  with $80/$83/$C2 etc. routed to encrypted LC message types that extract what
  fields remain clear (the LCF byte itself).

---

## OP25 Cross-Reference

OP25 (Python/C++, GNU Radio) handles encryption at:

- **DES-OFB decryption:** Implements the full MI -> LFSR -> OFB chain from
  Sections 3 and 4 of AAAD-B. Supports DES-OFB and RC4/ADP decryption with
  user-supplied keys.

- **Key configuration:** Keys provided as JSON config with keyid/algid/key triples.

- **Voice frame assembly:** The FDMA voice octet ordering (Tables 5-5 and 5-6)
  is directly reflected in the frame assembly logic -- IMBE words u0-u7 packed
  into w0-w10, 11 octets per frame, 18 frames per superframe.

- **MI extraction:** Extracts MI from LDU2 ES fields (FDMA) and ESS-A/ESS-B
  (TDMA) for encryption sync.

---

## DSD-FME Cross-Reference

DSD-FME (C) handles encryption at:

- **MI extraction and OFB mode:** Implements MI extraction from LDU2 and the
  OFB mode for voice decryption with user-supplied DES keys.

- **Encryption schedule:** Uses Tables 5-5 and 5-6 ordering for IMBE voice
  octet XOR with keystream.

- Reference: https://deepwiki.com/lwvmobile/dsd-fme/4.4-p25-encryption-and-key-management

---

## Test Vectors (from AAAD-B Annexes A, B, C)

Canonical test MI: `1234 5678 90AB CDEF 00`

| Algorithm | Key | B#2 Keystream (first usable block) |
|-----------|-----|------------------------------------|
| DES ($81) | `0123 4567 89AB CDEF` | `5D97 6A50 4786 581F` |
| 3DES ($83) | K1=`0123...CDEF` K2=`2345...EF01` K3=`4567...0123` | `F2EF 4174 6B0B EE27` |
| AES-256 ($84) | K0-127=`0123...EF01` K128-255=`4567...2345` | `A4A4 5220 6523 E33B AD35 8AF5 71CB 029A` |

LFSR next-MI verification:
```
MI #0 (Header): 1234 5678 90AB CDEF 00
MI #1:          6FE2 802A A403 828B 00
MI #2:          FF35 B190 9449 B835 00
MI #3:          7EAD FBE0 D23E CDDF 00
```

AES-256 MI expansion (n=128):
```
MI = 1234 5678 90AB CDEF
Expanded input register = 1234 5678 90AB CDEF 6FE2 802A A403 828B
```

These test vectors can validate an LFSR implementation and OFB keystream
generator independently of live RF traffic.
