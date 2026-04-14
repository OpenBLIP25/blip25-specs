# LCW Implementation Traps: Common Errors in P25 Link Control Word Parsing

**Source specs:** TIA-102.AABF-D (2015), cross-referenced against SDRTrunk and OP25
**Written:** 2026-04-14 (Phase 4 uplift)

This file documents three implementation traps that repeatedly trip up P25 decoders
and are not obvious from reading the spec in isolation.

---

## 1. Two Different "P" and "Protected" Bits

AABF-D uses the letter "P" for two completely different purposes, in two different
places in the LCW, with different semantics:

**LCF P flag (octet 0, bit 7):** Indicates whether the *LC payload* (octets 1-8 or
2-8) is encrypted. When this bit is 1, the payload cannot be parsed without
decryption keys. This is the "are we even allowed to read this?" gate.

**Service Options P bit (octet 2/payload_start, bit 6):** Indicates whether the
*voice call* uses encrypted (protected) resources. This bit is readable and present
in clear LCWs. It says "the voice audio on this channel is encrypted" — it does NOT
mean the LC word itself is encrypted.

```
Octet 0:  ┌───┬───┬───────────────────┐
          │ P │SF │    LCO[5:0]       │  ← LCF P = "payload encrypted"
          └───┴───┴───────────────────┘
               │
               ▼ When SF=0 (explicit MFID):
Octet 1:  MFID
Octet 2:  ┌───┬───┬───┬───┬───┬───────┐
          │ E │ P │ D │ M │ R │ Prio  │  ← Service Options P = "call encrypted"
          └───┴───┴───┴───┴───┴───────┘
```

**Consequence of confusion:** A decoder that checks `service_options.protected` to
decide whether to decrypt the LC payload will misparse *all* encrypted clear-channel
voice calls (unencrypted LC payload + encrypted voice). It will also fail to detect
the actual encryption state on encrypted calls that have Service Options P=0.

**Rule:** Always check the LCF P flag (octet 0 bit 7) to gate payload parsing. Use
Service Options P (payload bit 6) only as call metadata to surface to the user.

---

## 2. Source ID Extension (LCO $09) Is NOT Optional When S=1

When any LC word sets its S flag to 1, the *next* LC word in the stream SHALL be
LCO $09 (Source ID Extension). This is normative language in AABF-D §7.3.x for
each message that carries the S flag:

> "A '1' shall indicate that the next LC shall be the Source ID Extension LC."

Messages with an S flag: LCO $00, $05, $0A, $13, $16. Messages that always imply
Source ID Extension (no S flag, but always followed): $1A, $1B, $1C.

**Common mistake:** Treating the Source ID Extension as a separate, standalone LC
word to be parsed independently. SDRTrunk and OP25 handle this correctly by keeping
state across consecutive LDU1 frames and watching for LCO $09 after any word with
S=1. A stateless parser that just dispatches on LCO per frame will:
- Lose the Source ID Extension content entirely (no context to associate it with)
- Not decode it back into the right `wacn+sys_id+unit_id` tuple for the call

**Rule:** Parse LCWs with a two-slot state machine: `(previous_lcw, current_lcw)`.
If `previous_lcw` had S=1, treat `current_lcw` as the Source ID Extension that
completes it. Do not dispatch `current_lcw` through the normal dispatch table as an
independent message when in this state.

---

## 3. LCO 9 and 10 Are Trunked-Only — Not Conventional

A prior extraction of AABF-D Table 3 (page 39) incorrectly marked LCO 9 (Source ID
Extension) and LCO 10 (Unit-to-Unit VCU - Extended) as usable in conventional mode.
The actual PDF table leaves those conventional columns empty for both opcodes.

**Why this matters:** Conventional P25 systems do not carry cross-system identification
(the whole point of Source ID Extension is to identify subscribers from *other* WACN
networks, which only occurs in trunked inter-system roaming). A conventional-only
decoder should not expect to see LCO $09 or $0A and can safely log them as unexpected.
A trunked decoder must handle them.

**Practical implication for the S flag on conventional:** When a conventional call
carries LCO $00 with S=1, the S flag may be set by infrastructure that shares firmware
with trunked sites, but the Source ID Extension that should follow will not come. A
robust decoder should time out waiting for LCO $09 on conventional channels.

---

## 4. Channel Field Bit Layout: Channel Identifier Is in the High 4 Bits

The 16-bit Channel field used in LCO $02, $04, $18-$19, $21-$24, $26-$29 has the
Channel Identifier in bits [15:12] (the most significant nibble), not bits [3:0]:

```
Bit:  15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
     ┌───┬───┬───┬───┬───────────────────────────────────────────────┐
     │         Channel Identifier[3:0]  │   Channel Number[11:0]    │
     └───────────────────────────────────────────────────────────────┘
```

This field appears at the *octet level* as two consecutive octets: the high octet
carries the Channel Identifier in its upper nibble and bits [11:8] of Channel Number
in its lower nibble. The low octet carries Channel Number bits [7:0].

**Common mistake:** Treating the two-octet channel field as a simple 16-bit number
where the channel ID is masked from the low nibble. Correct extraction:

```c
uint16_t ch      = ((uint16_t)data[high_octet] << 8) | data[low_octet];
uint8_t  ch_id   = (uint8_t)(ch >> 12);       /* bits [15:12] */
uint16_t ch_num  = ch & 0x0FFFU;              /* bits [11:0]  */
```

The actual RF frequency is then: `base_freq + (ch_num * ch_spacing) ± tx_offset`,
where `base_freq`, `ch_spacing`, and `tx_offset` come from the Channel Identifier
Update (LCO $18/$19) that matches `ch_id`.
