# P25 Packet Data LLC (Logical Link Control) -- Implementation Specification

**Source:** TIA-102.BAED-A (April 2019), "Project 25 Packet Data Logical Link Control Procedures"
**Classification:** PROTOCOL + DATA
**Phase:** 3 -- Implementation-ready
**Extracted:** 2026-04-12
**Purpose:** Complete implementation guide for the LLC layer that carries packet data over the
P25 air interface. The LLC sits between SNDCP/upper-layer SAPs above and the MAC/PHY
(FDMA PDU frames, TDMA data bursts) below. Covers confirmed (ARQ) and unconfirmed
(fire-and-forget) data conveyance, fragmentation/reassembly, block sequencing, CRC
validation, and encryption integration.

**Cross-references:**
- TIA-102.BAAA-B -- FDMA Common Air Interface (PDU frame format, trellis coding, CRC definitions)
- TIA-102.BBAC-A -- TDMA MAC Layer (data channel burst structure)
- TIA-102.BAAC-D -- Reserved Values (SAP identifiers, MFID values, LLID ranges)
- TIA-102.AABC-E -- Trunking Messages (SNDCP Data Channel Grant, data page)
- TIA-102.AAAD-B -- Block Encryption Protocol (auxiliary headers, data PDU encryption)
- TIA-102.BAEA-C -- Data Overview and Specification (four data configurations, SAP/SNDCP/LLC model)
- TIA-102.BAEJ-A -- CMS Specification for Packet Data (encryption failure notification)

---

## 1. LLC Layer Architecture

### 1.1 Position in the P25 Protocol Stack

```
 ┌─────────────────────────────────────────────────────────────┐
 │                    IP / Application Data                     │
 ├─────────────────────────────────────────────────────────────┤
 │                  SNDCP (TIA-102.BAEA-C)                     │
 │        Subnetwork Dependent Convergence Protocol            │
 │   - IP packet encapsulation / decapsulation                 │
 │   - SNDCP activation/deactivation                           │
 │   - Compression (optional)                                  │
 ├─────────────────────────────────────────────────────────────┤
 │             LLC (TIA-102.BAED-A) ◄── THIS SPEC             │
 │   - Confirmed data: stop-and-wait ARQ                       │
 │   - Unconfirmed data: fire-and-forget                       │
 │   - Fragmentation / reassembly (confirmed only)             │
 │   - Sequence numbering and duplicate detection              │
 │   - CRC validation (header, per-block, packet)              │
 │   - Encryption integration (auxiliary headers)              │
 ├─────────────────────────────────────────────────────────────┤
 │              MAC / PHY Layer                                │
 │  FDMA (TIA-102.BAAA-B)    │  TDMA (TIA-102.BBAC-A)        │
 │  - PDU frame (DUID 0xC)   │  - DCH data burst              │
 │  - Trellis coding          │  - Per-slot FEC                │
 │  - Frame sync + NID        │  - ISCH + sync pattern         │
 │  - Status symbols          │  - 2-slot TDMA structure       │
 └─────────────────────────────────────────────────────────────┘
```

### 1.2 Functional Summary

The LLC layer provides two services:

1. **Confirmed data conveyance** -- stop-and-wait ARQ protocol with ACK/NACK/SACK
   responses, selective retransmission, sequence numbering (mod 8), duplicate
   detection, and fragmentation of logical messages across multiple data packets.

2. **Unconfirmed data conveyance** -- single-shot delivery with no acknowledgment,
   no sequence numbering, no fragmentation. Receiver verifies CRC_PACKET only.

### 1.3 Four Data Configurations

All four configurations use identical LLC procedures. The only difference is addressing
mode (symmetric vs asymmetric).

| Configuration | Addressing | Participants | LLID Usage |
|--------------|------------|--------------|------------|
| Direct Data | Symmetric | SU-to-SU, no infrastructure | Source + Destination LLID in every packet |
| Repeated Data | Symmetric | SU-to-SU via FS_R | Source + Destination LLID in every packet |
| Conventional FNE Data | Asymmetric | SU-to-FNE | Single LLID (source on inbound, dest on outbound) |
| Trunked FNE Data | Asymmetric | SU-to-FNE | Single LLID (source on inbound, dest on outbound) |

### 1.4 Data Flow -- Receive Path

```
  RF Signal
     │
     ▼
  Demodulate + Frame Sync (48 bits)
     │
     ▼
  NID Decode (BCH) ──► NAC + DUID
     │                     │
     │              DUID = 0xC (PDU)?
     │                     │
     ▼                     ▼ yes
  [other DUIDs]    Trellis Decode Header Block
                          │
                          ▼
                   CRC_HEADER check ──► fail: discard
                          │ pass
                          ▼
                   Receiver Preprocessing (3.1)
                   ├─ NAC Qualification
                   ├─ MFID Qualification
                   └─ Address Qualification
                          │ all pass
                          ▼
                   Dispatch on packet type:
                   ├─ Confirmed  ──► Section 5 (ARQ receiver)
                   └─ Unconfirmed ──► Section 6 (simple receiver)
                          │
                          ▼
                   Extract logical message
                          │
                          ▼
                   Decrypt if encrypted (AAAD-B)
                          │
                          ▼
                   Route to SAP (Section 3)
```

---

## 2. PDU Data Header Formats

### 2.1 FDMA PDU Frame Structure (from TIA-102.BAAA-B)

An FDMA PDU (DUID 0xC) on the air:

```
 ┌──────────┬──────────┬─────────────────┬─────────────────┬─────┬──────────────┐
 │ Frame    │ NID      │  Header Block   │  Data Block(s)  │ ... │  Null fill   │
 │ Sync     │ (64 bits)│  (196 bits      │  (196 bits each │     │  + Status    │
 │ (48 bits)│ NAC+DUID │   trellis-coded)│   trellis-coded)│     │  Symbols     │
 └──────────┴──────────┴─────────────────┴─────────────────┴─────┴──────────────┘
```

Before trellis coding, the header block is 12 octets (96 bits). After rate-1/2 trellis
coding: 98 dibits = 196 bits.

### 2.2 Header Block Layout -- Unconfirmed Data Packet (Asymmetric)

12 octets (96 bits) before trellis coding:

```
 Byte 0               Byte 1               Byte 2
 ┌─┬─┬─┬─┬─┬─┬─┬─┐  ┌─┬─┬─┬─┬─┬─┬─┬─┐  ┌─┬─┬─┬─┬─┬─┬─┬─┐
 │A│R│ SAP ID  │MFID│  │    MFID (cont)   │  │     LLID       │
 │C│ │ (6 bits)│    │  │    (8 bits)      │  │   (24 bits,    │
 └─┴─┴─┴─┴─┴─┴─┴─┘  └─┴─┴─┴─┴─┴─┴─┴─┘  │    bytes 2-4)  │
                                            └─┴─┴─┴─┴─┴─┴─┴─┘
 Bytes 2-4: LLID (24 bits)
 Byte 5 [7:4]: Blocks To Follow (BTF) [high nibble of full BTF field]
 Byte 5 [3:0]: Pad Octet Count [high nibble]
 Byte 6: Pad Octet Count [low] + Data Header Offset
 Bytes 7-8: Remaining header fields
 Bytes 9: Reserved / format-specific
 Bytes 10-11: CRC_HEADER (16 bits, CRC-CCITT)
```

**Field descriptions:**

| Field | Bits | Description |
|-------|------|-------------|
| AC | 1 | Access Control: 0=outbound (FNE→SU), 1=inbound (SU→FNE) |
| R | 1 | Reserved (set to 0) |
| SAP ID | 6 | Service Access Point -- routes to upper-layer protocol (see Section 3) |
| MFID | 8 | Manufacturer ID ($00=standard, $01=Motorola, etc.) |
| LLID | 24 | Logical Link Identifier (source or destination depending on direction) |
| BTF | 7 | Blocks To Follow -- number of data blocks after header |
| Pad Octet Count | 5 | Number of pad octets in last data block |
| Data Header Offset | 6 | Offset to start of user data within data blocks |
| FMF | 1 | Full Message Flag: 1=complete packet, 0=selective retry |
| CRC_HEADER | 16 | CRC-CCITT over header (excluding CRC field itself) |

### 2.3 Header Block Layout -- Confirmed Data Packet (Asymmetric)

Same 12-octet structure with additional confirmed-mode fields:

| Field | Bits | Description |
|-------|------|-------------|
| AC | 1 | Access Control |
| R | 1 | Reserved |
| SAP ID | 6 | Service Access Point |
| MFID | 8 | Manufacturer ID |
| LLID | 24 | Logical Link Identifier |
| BTF | 7 | Blocks To Follow |
| FMF | 1 | Full Message Flag (1=full, 0=selective retry) |
| Pad Octet Count | 5 | Pad octets in last data block |
| Data Header Offset | 6 | Offset to user data |
| SYN | 1 | Resynchronize: resets V(S) and V(R) to 0 |
| N(S) | 3 | Packet Sequence Number Sent (mod 8) |
| FSNF | 4 | Fragment Sequence Number Field (LIC:1 + FSN:3) |
| CRC_HEADER | 16 | CRC-CCITT |

### 2.4 Header Block Layout -- Enhanced Addressing (Symmetric)

When symmetric addressing is used (Direct Data, Repeated Data), the header block SAP
field is set to Extended Address ($1F). The actual SAP is carried in a Second Header
block chained after the header block within the data blocks. The Second Header also
contains the second LLID (source LLID).

### 2.5 Response Packet Structure

Sent by receiver back to sender (confirmed mode only):

| Field | Bits | Description |
|-------|------|-------------|
| Class | 2 | Response class (see Table in Section 5.3) |
| Type | 3 | Response type |
| Status | 3 | Contains N(R) = N(S) of the packet being acknowledged |
| LLID | 24 | Matches the LLID from the data packet |
| Selective Retry Flags | variable | For SACK: bit flags indicating which blocks need retransmission |
| BTF | 7 | For SACK: number of retry flag entries |

### 2.6 Data Block Structure -- Unconfirmed

Each unconfirmed data block is 12 octets (96 bits), trellis-coded at rate 1/2 to 196 bits:

```
 ┌────────────────────────────────────────────────────────────────────┐
 │              Data payload (12 octets = 96 bits)                    │
 │  No serial number, no per-block CRC                               │
 │  CRC_PACKET (32 bits) covers ALL data blocks collectively         │
 └────────────────────────────────────────────────────────────────────┘
```

The 32-bit CRC_PACKET is appended as the last 4 octets across the data block payload area.

### 2.7 Data Block Structure -- Confirmed

Each confirmed data block is 18 octets before trellis coding:
- 16 octets data payload
- 9-bit Serial Number (block index within packet)
- 9-bit CRC_BLOCK

Trellis-coded at rate 3/4 to 196 bits:

```
 ┌────────────────────────────────────────────────┬──────────┬───────────┐
 │        Data payload (16 octets = 128 bits)      │ Serial # │ CRC_BLOCK │
 │                                                 │ (9 bits) │ (9 bits)  │
 └────────────────────────────────────────────────┴──────────┴───────────┘
 Total: 128 + 9 + 9 = 146 bits → padded to 144 bits input → rate 3/4 → 196 bits
```

**Important:** 18 octets = 144 bits input for rate-3/4 trellis (48 tribits + 1 flush).
The CRC_PACKET (32 bits) is embedded within the data payload area across blocks.

---

## 3. SAP Identifiers (Cross-Reference: TIA-102.BAAC-D)

The 6-bit SAP field in the PDU header determines which upper-layer entity receives the
logical message after LLC processing. This is the primary demultiplexing field.

### 3.1 SAP Values Relevant to Packet Data LLC

| SAP (hex) | Decimal | Name | LLC Routing |
|-----------|---------|------|-------------|
| $00 | 0 | Unencrypted User Data | Direct to application/CMS |
| $01 | 1 | Encrypted User Data | Decrypt via AAAD-B, then to application/CMS |
| $04 | 4 | Packet Data | SNDCP for IP-over-P25 |
| $05 | 5 | ARP (Address Resolution Protocol) | SNDCP ARP handler |
| $06 | 6 | SNDCP Packet Data Control | SNDCP control plane |
| $0F | 15 | Packet Data Scan Preamble | If SNDCP active -> SNDCP; else -> CMS |
| $1D | 29 | Packet Data Encryption Support | Encryption key management |
| $1F | 31 | Extended Address | Indicates symmetric addressing; real SAP is in Second Header |
| $20 | 32 | Registration and Authorization | CMS registration handler |
| $28 | 40 | Unencrypted Key Management Message | OTAR (Over-the-Air Rekeying) |
| $29 | 41 | Encrypted Key Management Message | OTAR (encrypted) |
| $3D | 61 | Trunking Control | TSBK messages (not LLC data) |
| $3F | 63 | Protected Trunking Control | Encrypted TSBK messages |

### 3.2 SAP Routing Logic

```
FUNCTION route_to_sap(sap: u8, sndcp_active: bool, logical_message: &[u8]):
    MATCH sap:
        0x00 => deliver_to_application(logical_message)  // unencrypted user data
        0x01 => deliver_to_application(logical_message)  // post-decryption user data
        0x04 => deliver_to_sndcp(logical_message)        // IP packet data
        0x05 => deliver_to_sndcp_arp(logical_message)    // ARP
        0x06 => deliver_to_sndcp_control(logical_message) // SNDCP signaling
        0x0F => {                                         // Preamble / scan
            IF sndcp_active THEN deliver_to_sndcp(logical_message)
            ELSE deliver_to_cms(logical_message)
        }
        0x1D => deliver_to_encryption_support(logical_message)
        0x1F => {
            // Extended addressing: extract real SAP from Second Header,
            // then re-route based on that SAP
            real_sap = extract_second_header_sap(logical_message)
            route_to_sap(real_sap, sndcp_active, strip_second_header(logical_message))
        }
        0x20 => deliver_to_cms_registration(logical_message)
        0x28 | 0x29 => deliver_to_otar(logical_message)
        _    => log_unknown_sap(sap); discard(logical_message)
```

---

## 4. Fragmentation and Reassembly

Fragmentation applies to **confirmed data only**. Unconfirmed data does not support
fragmentation -- each unconfirmed packet is a complete logical message.

### 4.1 Fragment Sequence Number Field (FSNF) -- 4 bits

The FSNF in the confirmed data header encodes both the chain position and fragment number:

```
  Bit 3 (MSB)    Bits 2:0
  ┌─────┐        ┌─────────┐
  │ LIC │        │  FSN    │
  │     │        │ (0-7)   │
  └─────┘        └─────────┘
```

| LIC | FSN | Meaning | Action |
|-----|-----|---------|--------|
| 1 | 0 | Only-in-chain | Complete message in single packet; no reassembly needed |
| 0 | 0 | First-in-chain | Start of multi-packet message; begin accumulating fragments |
| 0 | 1-7 | Middle-in-chain | Continue accumulating; FSN cycles 1,2,3,4,5,6,7,1,2,... |
| 1 | 1-7 | Last-in-chain | Final fragment; reassemble and deliver to SAP |

### 4.2 Fragmentation Procedure (Sender)

```
FUNCTION fragment_logical_message(message: &[u8], max_fragment_size: usize) -> Vec<Fragment>:
    fragments = []
    offset = 0
    fsn = 0

    WHILE offset < message.len():
        chunk_end = min(offset + max_fragment_size, message.len())
        chunk = message[offset..chunk_end]
        is_first = (offset == 0)
        is_last  = (chunk_end == message.len())

        IF is_first AND is_last:
            fsnf = 0b1_000   // only-in-chain: LIC=1, FSN=0
        ELSE IF is_first:
            fsnf = 0b0_000   // first-in-chain: LIC=0, FSN=0
        ELSE IF is_last:
            fsnf = (1 << 3) | fsn   // last-in-chain: LIC=1, FSN=current
        ELSE:
            fsnf = (0 << 3) | fsn   // middle-in-chain: LIC=0, FSN=current

        fragments.push(Fragment { fsnf, data: chunk })
        offset = chunk_end

        IF NOT is_first:
            fsn = if fsn == 7 { 1 } else { fsn + 1 }  // wraps 7 -> 1 (skips 0)
        ELSE:
            fsn = 1  // next fragment after first starts at FSN=1

    RETURN fragments
```

### 4.3 Reassembly Procedure (Receiver)

```
FUNCTION reassemble_fragment(state: &mut ReassemblyState, fsnf: u8, data: &[u8]) -> Option<Vec<u8>>:
    lic = (fsnf >> 3) & 1
    fsn = fsnf & 0x07

    MATCH (lic, fsn):
        (1, 0):  // only-in-chain
            state.reset()
            RETURN Some(data.to_vec())

        (0, 0):  // first-in-chain
            state.reset()
            state.fragments.push(data.to_vec())
            state.expected_fsn = 1
            RETURN None

        (0, fsn):  // middle-in-chain
            IF state.fragments.is_empty():
                RETURN None   // missed first fragment, discard
            IF fsn != state.expected_fsn:
                state.reset()  // gap detected, discard partial
                RETURN None
            state.fragments.push(data.to_vec())
            state.expected_fsn = if fsn == 7 { 1 } else { fsn + 1 }
            RETURN None

        (1, fsn):  // last-in-chain
            IF state.fragments.is_empty():
                RETURN None   // missed earlier fragments
            IF fsn != state.expected_fsn:
                state.reset()
                RETURN None
            state.fragments.push(data.to_vec())
            assembled = state.fragments.concat()
            state.reset()
            RETURN Some(assembled)
```

### 4.4 Maximum Fragment Size

Each confirmed data packet carries up to BTF data blocks x 16 octets per block, minus
4 octets for CRC_PACKET, minus pad octets. The maximum logical message fragment size
per packet:

```
max_fragment_bytes = BTF * 16 - 4 - pad_octets
```

Where BTF is computed as: `BTF = floor((L + 4 + P_MAXC) / 16)` with P_MAXC = 15.

For unconfirmed: `BTF = floor((L + 4 + P_MAXU) / 12)` with P_MAXU = 11, block size = 12.

---

## 5. Confirmed Data Procedures

### 5.1 Stop-and-Wait ARQ Overview

The confirmed conveyance implements a stop-and-wait protocol: one data packet is in-flight
at a time per session. The sender transmits a packet and waits for a response before
sending the next.

```
  Sender                              Receiver
    │                                    │
    │──── Data Packet (N(S)=1) ─────────>│
    │                                    │── CRC checks
    │<──── ACK (N(R)=1) ────────────────│
    │                                    │
    │──── Data Packet (N(S)=2) ─────────>│
    │                                    │── some blocks fail CRC_BLOCK
    │<──── SACK (N(R)=2, flags) ────────│
    │                                    │
    │──── Selective Retry (FMF=0) ──────>│
    │                                    │── all blocks now OK, but CRC_PACKET fails
    │<──── NACK (N(R)=2) ──────────────│
    │                                    │
    │──── Full Retry (FMF=1) ──────────>│
    │                                    │── all OK
    │<──── ACK (N(R)=2) ────────────────│
    │                                    │
```

### 5.2 Sequence Numbering

Both V(S) (sender) and V(R) (receiver) are 3-bit values, modulo 8.

**Sender rules:**
- V(S) incremented before each NEW data packet (not retransmissions)
- N(S) in the packet = V(S)
- On resynchronization (SYN=1): V(S) = V(R) = 0

**Receiver rules (if duplicate detection supported):**
- On receiving N(S), compare to V(R):

| Condition | Meaning | Action |
|-----------|---------|--------|
| N(S) = V(R) + 1 (mod 8) | Normal, in-sequence | Accept; V(R) = N(S) |
| N(S) = V(R) | Possible duplicate | Compare CRC_PACKET to CRC_LAST; if match, discard as duplicate; V(R) unchanged |
| N(S) = anything else | Packets were lost | Accept; V(R) = N(S) (resync for duplicate detection) |

### 5.3 Response Types

| Response | Class | Type | Status | Meaning |
|----------|-------|------|--------|---------|
| ACK | %00 | %001 | N(R) | All blocks received correctly |
| NACK | %01 | %001 | N(R) | CRC_PACKET failure; full retransmission requested |
| SACK | %10 | %000 | N(R) | Selective retry; flags identify bad blocks |

### 5.4 Retry Logic

| Parameter | Suggested Value | Description |
|-----------|----------------|-------------|
| TX_MAX | 4 | Maximum transmissions per data packet |
| T_RETRY | 2 seconds | Timeout waiting for response |
| P_MAXC | 15 octets | Maximum pad octets (confirmed) |
| P_MAXU | 11 octets | Maximum pad octets (unconfirmed) |

**Retry state machine:**

```
FUNCTION send_confirmed_packet(state: &mut SenderState, fragment: &[u8]):
    packet = construct_full_data_packet(state, fragment)
    tx_num = 0

    LOOP:
        IF tx_num >= TX_MAX:
            // Abort delivery of this fragment
            // V(S) was already incremented, so next packet will show a gap to receiver
            RETURN Err(DeliveryAborted)

        transmit(packet)
        tx_num += 1
        start_timer(T_RETRY)

        MATCH wait_for_response(T_RETRY):
            Timeout:
                // Retransmit same packet
                CONTINUE

            Response(ACK, n_r) IF n_r == state.v_s:
                RETURN Ok(())

            Response(NACK, n_r) IF n_r == state.v_s:
                // Full retry: reconstruct with FMF=1
                packet = reconstruct_full_packet(state, fragment, fmf=1)
                CONTINUE

            Response(SACK, n_r, flags) IF n_r == state.v_s:
                // Selective retry: only retransmit flagged blocks
                packet = construct_selective_packet(state, fragment, flags)
                // FMF=0, BTF = number of blocks being retried
                CONTINUE

            Response(_, n_r) IF n_r != state.v_s:
                // Stale or mismatched response, ignore
                CONTINUE
```

### 5.5 Selective Retry (SACK) Details

On SACK, the response packet contains selective retry flags -- one bit per data block,
indicating which blocks need retransmission:

```
  Flag bit 0 → Data block with serial number 0
  Flag bit 1 → Data block with serial number 1
  ...
  Flag bit N → Data block with serial number N
  1 = needs retransmission, 0 = received OK
```

The sender constructs a selective retry packet with:
- FMF = 0 (not a full message)
- BTF = number of blocks being retransmitted
- Only the flagged data blocks, each retaining its original serial number
- Updated CRC_HEADER

The receiver stores previously-received good blocks by serial number and fills in gaps
from selective retries.

### 5.6 Resynchronization (SYN Bit)

The SYN bit in the header resets both endpoints' sequence counters:

| Event | Action |
|-------|--------|
| SU power-up/restart | Sender sets SYN=1 on first data packet |
| Channel change (with static packet data registration per BAEJ-A) | Sender sets SYN=1 |
| Repeated identical payload | Sender sets SYN=1 to prevent false duplicate detection |
| SYN received | Receiver sets V(S)=V(R)=0, marks CRC_LAST invalid |

---

## 6. Unconfirmed Data Conveyance

### 6.1 Properties

Unconfirmed data is the simplest LLC mode:

- **No sequence numbering** -- no V(S), V(R), N(S), N(R)
- **No fragmentation** -- each packet is a complete logical message
- **No responses** -- no ACK, NACK, or SACK
- **No retransmission** -- transmit once and move on
- **CRC_PACKET only** -- 32-bit CRC over all data blocks; if it fails, silently discard

### 6.2 Sender Procedure

```
FUNCTION send_unconfirmed(sap: u8, message: &[u8]):
    // 1. Optionally encrypt
    payload = if encryption_required { encrypt(message) } else { message }

    // 2. Chain headers (auxiliary header if encrypted, second header if symmetric)
    payload = chain_headers(payload, encryption_required, symmetric_addressing)

    // 3. Compute BTF and padding
    L = payload.len()
    btf = (L + 4 + P_MAXU) / 12    // integer division
    pad_count = btf * 12 - L - 4

    // 4. Format header block (12 octets)
    header = format_unconfirmed_header(sap, mfid, llid, btf, pad_count, data_header_offset)
    set_crc_header(&mut header)

    // 5. Segment into 12-octet data blocks
    padded_payload = payload + [0u8; pad_count] + crc_packet_bytes(payload, pad_count)
    blocks = padded_payload.chunks(12)

    // 6. Trellis encode and transmit
    transmit_pdu(header, blocks)
```

### 6.3 Receiver Procedure

```
FUNCTION receive_unconfirmed(header: &HeaderBlock, data_blocks: &[DataBlock]):
    // 1. Reassemble payload from data blocks
    payload = concatenate(data_blocks)

    // 2. Verify CRC_PACKET (last 4 octets)
    IF NOT verify_crc_packet(payload):
        DISCARD; RETURN

    // 3. Strip padding and CRC
    user_data = payload[0 .. payload.len() - 4 - header.pad_count]

    // 4. Remove chained headers if present
    IF header.sap == 0x1F:
        (real_sap, user_data) = extract_second_header(user_data)
    IF encrypted:
        (aux_header, user_data) = extract_auxiliary_header(user_data)
        user_data = decrypt(user_data, aux_header)
        IF decrypt_failed:
            notify_cms(); DISCARD; RETURN

    // 5. Route to SAP
    route_to_sap(header.sap, sndcp_active, user_data)
```

---

## 7. Block Sequencing and Numbering

### 7.1 Confirmed Data Blocks

Each confirmed data block carries a **serial number** (9 bits) that identifies its
position within the data packet:

```
  Serial Number 0  →  First data block (immediately after header)
  Serial Number 1  →  Second data block
  ...
  Serial Number BTF-1  →  Last data block
```

Serial numbers are critical for:
1. **SACK selective retry** -- receiver identifies missing blocks by serial number
2. **Out-of-order reassembly** -- receiver stores blocks indexed by serial number
3. **Selective retry construction** -- sender retransmits only specific serial numbers

### 7.2 Unconfirmed Data Blocks

Unconfirmed data blocks have **no serial numbers** and **no per-block CRC**. They are
simply sequential 12-octet chunks. Block ordering is implicit from transmission order.

### 7.3 Block Count (BTF) Computation

```
// Confirmed: 16-octet blocks
btf_confirmed = floor((L + 4 + P_MAXC) / 16)
// where L = logical message fragment length in octets
//       4 = CRC_PACKET size (32 bits)
//       P_MAXC = 15 (maximum pad octets)

// Unconfirmed: 12-octet blocks
btf_unconfirmed = floor((L + 4 + P_MAXU) / 12)
// where P_MAXU = 11 (maximum pad octets)

// Pad octets fill the last block to its full size
pad_confirmed   = btf_confirmed * 16 - L - 4
pad_unconfirmed = btf_unconfirmed * 12 - L - 4
```

---

## 8. CRC Definitions for Data Blocks

Three CRC types protect different scopes of the data packet.

### 8.1 CRC_HEADER -- 16-bit CRC-CCITT

Protects the header block (first 10 octets of the 12-octet header).

```
Polynomial: G(x) = x^16 + x^12 + x^5 + 1
Hex:        0x1021
Init:       0xFFFF (all ones)
Scope:      Header block octets 0-9 (80 bits)
Location:   Header block octets 10-11 (last 16 bits)
```

### 8.2 CRC_BLOCK -- 9-bit CRC-9 (Confirmed Only)

Protects each individual confirmed data block (16 data octets).

```
Polynomial: G(x) = x^9 + x^6 + x^4 + x^3 + 1
Hex:        0x059
Init:       0x1FF (all ones in 9 bits)
Scope:      16 data octets of one confirmed data block (128 bits)
Location:   Last 9 bits of the 18-octet trellis input block
```

**Purpose:** Enables selective retry. If CRC_BLOCK fails for a block, the receiver
requests retransmission of only that block via SACK. Blocks passing CRC_BLOCK are stored
and do not need retransmission.

### 8.3 CRC_PACKET -- 32-bit CRC-32

Protects the complete reassembled logical message fragment plus pad octets.

```
Polynomial: G(x) = x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10
                  + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1
Hex:        0x04C11DB7
Init:       0xFFFFFFFF
Scope:      All data octets across all data blocks (logical message + pad octets)
Location:   Last 4 octets of the data block payload area
```

**CRITICAL:** CRC_PACKET is computed over the logical message fragment AND pad octets
(but not the CRC_PACKET itself). The 4-byte CRC is placed at the end of the data
area, spanning the last bytes of the final data block(s).

### 8.4 CRC Verification Order

For confirmed data:
```
1. CRC_HEADER  →  If fail: discard entire packet (receiver preprocessing)
2. CRC_BLOCK   →  Per-block check; failing blocks flagged for SACK
3. CRC_PACKET  →  After all blocks received; if fail: NACK (full retry)
```

For unconfirmed data:
```
1. CRC_HEADER  →  If fail: discard entire packet
2. CRC_PACKET  →  If fail: silently discard
   (No CRC_BLOCK -- unconfirmed blocks have no per-block CRC)
```

### 8.5 CRC_LAST (Duplicate Detection)

The receiver stores CRC_PACKET of the last successfully received confirmed data packet
as CRC_LAST. When N(S) = V(R), the receiver compares CRC_PACKET against CRC_LAST:
- Match: discard as duplicate
- Mismatch: accept (this is a new packet that happens to have the same sequence number
  because 7 intermediate packets were lost)

---

## 9. Encryption of Data PDUs (Cross-Reference: TIA-102.AAAD-B)

### 9.1 What Is Encrypted

In data PDUs, encryption protects the logical message payload. The following remain clear:

| Always Clear | Always Encrypted (when encryption active) |
|-------------|-------------------------------------------|
| Frame Sync (48 bits) | Logical message content |
| NID (NAC + DUID) | |
| Header block (all fields) | |
| Serial numbers (confirmed) | |
| CRC_BLOCK (confirmed) | |
| CRC_PACKET (computed over ciphertext) | |
| SAP field | |
| LLID(s) | |

**Key insight:** CRC_PACKET is computed AFTER encryption. The receiver must pass CRC
checks on the ciphertext, then decrypt. Decryption failure after CRC pass means the
key is wrong, not that the data was corrupted on the air.

### 9.2 Auxiliary Header

When a data PDU is encrypted, one or more Auxiliary Headers are chained into the logical
message fragment BEFORE the header block is formatted:

```
  [Header Block] → [Data Blocks containing: [Aux Header(s)] [Encrypted Payload] [Pad] [CRC_PACKET]]
```

The Auxiliary Header carries:
- **ALGID** (8 bits) -- Algorithm ID (e.g., $80=AES-256, $81=DES-OFB, $84=3DES)
- **KID** (16 bits) -- Key ID
- **MI** (72 bits or variable) -- Message Indicator (IV/nonce)

The SAP field in the header block is set according to the first chained header (per
AAAD-B rules). The actual user-data SAP is saved in the last header in the chain.

### 9.3 Encryption Processing Order

**Sender:**
```
1. Start with cleartext logical message fragment
2. Encrypt using AAAD-B block encryption protocol
3. Prepend Auxiliary Header(s)
4. If symmetric addressing: prepend Second Header
5. Format header block (SAP reflects chained headers)
6. Segment into data blocks, compute CRCs
7. Trellis encode and transmit
```

**Receiver:**
```
1. Trellis decode, CRC_HEADER check
2. For confirmed: CRC_BLOCK per block, collect all blocks, CRC_PACKET check
   For unconfirmed: CRC_PACKET check
3. Extract logical message fragment from data blocks
4. Remove Second Header if present (symmetric addressing)
5. Remove Auxiliary Header(s) -- extract ALGID, KID, MI
6. Decrypt using AAAD-B with extracted parameters
7. If decryption fails: notify CMS (per BAEJ-A), discard
8. If decryption succeeds: route to SAP
```

---

## 10. Parser Pseudocode

### 10.1 FDMA PDU Data Extraction

```
FUNCTION parse_fdma_data_pdu(raw_dibits: &[u8]) -> Result<DataPacket>:
    // Step 1: Frame sync and NID already decoded by top-level dispatcher
    // We receive dibits starting at the header block

    // Step 2: Decode header block (196 bits = 98 dibits)
    header_dibits = raw_dibits[0..98]
    header_bytes = trellis_decode_rate_half(header_dibits)  // -> 12 bytes
    IF header_bytes.is_err():
        RETURN Err(TrellisDecodeFailed)

    // Step 3: Parse header fields
    header = parse_header_block(header_bytes)

    // Step 4: Verify CRC_HEADER
    IF NOT verify_crc_ccitt(header_bytes[0..10], header_bytes[10..12]):
        RETURN Err(HeaderCrcFailed)

    // Step 5: Determine packet type and block parameters
    is_confirmed = header.format_is_confirmed()
    btf = header.blocks_to_follow
    IF btf == 0:
        RETURN Err(NoDataBlocks)

    // Step 6: Decode data blocks
    offset = 98  // dibits consumed by header
    data_blocks = []

    FOR i IN 0..btf:
        block_dibits = raw_dibits[offset .. offset + 98]
        IF is_confirmed:
            block_bytes = trellis_decode_rate_three_quarter(block_dibits)  // -> 18 bytes
            serial_num = extract_serial_number(block_bytes)  // 9 bits
            crc_block = extract_crc_block(block_bytes)       // 9 bits
            payload = block_bytes[0..16]                      // 16 data octets
            crc_ok = verify_crc9(payload, crc_block)
            data_blocks.push(ConfirmedBlock { serial_num, payload, crc_ok })
        ELSE:
            block_bytes = trellis_decode_rate_half(block_dibits)  // -> 12 bytes
            data_blocks.push(UnconfirmedBlock { payload: block_bytes })
        offset += 98

    // Step 7: Reassemble payload
    IF is_confirmed:
        // Check if all blocks passed CRC_BLOCK
        bad_blocks = data_blocks.iter().filter(|b| !b.crc_ok).collect()
        IF NOT bad_blocks.is_empty():
            RETURN NeedRetry(SackFlags(bad_blocks))

        // Sort by serial number and concatenate
        data_blocks.sort_by_key(|b| b.serial_num)
        raw_payload = data_blocks.iter().flat_map(|b| b.payload).collect()
    ELSE:
        raw_payload = data_blocks.iter().flat_map(|b| b.payload).collect()

    // Step 8: Verify CRC_PACKET
    (message_data, crc_bytes) = split_at(raw_payload, raw_payload.len() - 4)
    IF NOT verify_crc32(message_data, crc_bytes):
        IF is_confirmed:
            RETURN NeedRetry(Nack)
        ELSE:
            RETURN Err(PacketCrcFailed)

    // Step 9: Strip padding
    user_data = message_data[0 .. message_data.len() - header.pad_count]

    RETURN Ok(DataPacket {
        header,
        user_data,
        is_confirmed,
    })
```

### 10.2 TDMA Data Burst Extraction

TDMA data is carried on a dedicated data channel (DCH) assigned by the trunking
system via SNDCP Data Channel Grant (AABC-E opcode 0x14). The MAC/PHY layer
delivers decoded data bursts to the LLC layer.

```
FUNCTION parse_tdma_data_burst(burst_payload: &[u8], burst_type: BurstType) -> Result<DataPacket>:
    // TDMA data channel bursts deliver the same logical content as FDMA PDUs,
    // but with TDMA-specific FEC and burst framing (per TIA-102.BBAC-A).
    //
    // By the time data reaches the LLC layer, the TDMA MAC has already:
    //   1. Extracted the burst from the time slot
    //   2. Performed TDMA-specific FEC decoding
    //   3. Delivered header + data block octets
    //
    // The LLC processing is IDENTICAL to FDMA from this point:

    header = parse_header_block(burst_payload[0..12])
    IF NOT verify_crc_ccitt(header):
        RETURN Err(HeaderCrcFailed)

    // Remaining processing identical to FDMA (Section 10.1 Steps 5-9)
    RETURN parse_data_blocks(header, burst_payload[12..])
```

### 10.3 Confirmed Data Receiver State Machine

```
ENUM ConfirmedRxState:
    WaitForNew
    ReceiveInProgress { p_r: u8, btf: u8, pad_count: u8, blocks: HashMap<u8, [u8; 16]> }
    RetryWait { p_r: u8, btf: u8, pad_count: u8, blocks: HashMap<u8, [u8; 16]> }

FUNCTION confirmed_rx_step(state: &mut ConfirmedRxState, packet: DataPacket) -> Option<Action>:
    MATCH state:
        WaitForNew:
            IF packet.header.fmf == 0:
                // Selective retry with no reception in progress -- ACK and discard
                send_ack(packet.header.n_s)
                RETURN None
            // FMF=1: new full data packet
            GOTO receive_new(state, packet)

        ReceiveInProgress { p_r, btf, pad_count, blocks }:
            UNREACHABLE  // blocks are received synchronously within a single PDU

        RetryWait { p_r, btf, pad_count, blocks }:
            IF packet.header.fmf == 1:
                // New full packet or full retry -- restart
                GOTO receive_new(state, packet)
            IF packet.header.n_s != *p_r:
                // Retry for different packet -- ignore
                RETURN None
            // Expected selective retry: merge new blocks
            FOR block IN packet.data_blocks:
                IF block.crc_ok:
                    blocks.insert(block.serial_num, block.payload)
            GOTO check_complete(state, p_r, btf, pad_count, blocks)

FUNCTION receive_new(state: &mut ConfirmedRxState, packet: DataPacket):
    p_r = packet.header.n_s
    btf = packet.header.btf
    pad_count = packet.header.pad_count
    blocks = HashMap::new()

    IF packet.header.syn:
        v_s = 0; v_r = 0; crc_last = None

    FOR block IN packet.data_blocks:
        IF block.crc_ok:
            blocks.insert(block.serial_num, block.payload)

    check_complete(state, p_r, btf, pad_count, blocks)

FUNCTION check_complete(state, p_r, btf, pad_count, blocks):
    IF blocks.len() < btf:
        // Missing blocks -- send SACK
        flags = compute_sack_flags(btf, &blocks)
        send_sack(p_r, flags)
        *state = RetryWait { p_r, btf, pad_count, blocks }
        RETURN None

    // All blocks received -- verify CRC_PACKET
    payload = reassemble_by_serial_number(&blocks, btf)
    IF NOT verify_crc_packet(payload):
        send_nack(p_r)
        blocks.clear()  // discard -- likely undetected CRC_BLOCK error
        *state = RetryWait { p_r, btf, pad_count, blocks: HashMap::new() }
        RETURN None

    // Success -- ACK and deliver
    send_ack(p_r)
    user_data = strip_padding_and_crc(payload, pad_count)
    *state = WaitForNew
    RETURN Some(Action::Deliver(user_data))
```

### 10.4 Receiver Preprocessing

```
FUNCTION receiver_preprocess(nac: u16, header: &HeaderBlock, config: &RxConfig) -> bool:
    // 1. NAC Qualification
    IF NOT config.is_nac_allowed(nac):
        RETURN false

    // 2. MFID Qualification (skip for FS_R)
    IF NOT config.is_fs_r:
        IF header.mfid != 0x00 AND NOT config.is_mfid_allowed(header.mfid):
            RETURN false

    // 3. Address Qualification
    MATCH config.role:
        Role::SubscriberUnit:
            IF header.dest_llid != config.my_llid AND header.dest_llid != LLID_EVERYONE:
                RETURN false
        Role::FNE:
            IF header.addressing != Asymmetric:
                RETURN false
        Role::FS_R:
            IF header.addressing != Symmetric:
                RETURN false

    RETURN true
```

---

## 11. Rust Struct Definitions

### 11.1 Header Block Structures

```rust
/// PDU header block format identifier.
/// Source: TIA-102.BAAA-B, derived from the header block AC/format bits.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PduFormat {
    /// Unconfirmed data packet -- fire-and-forget, no ARQ.
    UnconfirmedData,
    /// Confirmed data packet -- stop-and-wait ARQ with ACK/NACK/SACK.
    ConfirmedData,
    /// Response packet (ACK, NACK, SACK) to a confirmed data packet.
    ResponsePacket,
    /// Alternate MBT (Multi-Block Trunking) -- used by TSBK continuation.
    AlternateMbt,
    /// Unconfirmed Multi-Block Trunking.
    UnconfirmedMbt,
}

/// Addressing mode used by the LLC layer.
/// Source: TIA-102.BAED-A Section 2.1
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AddressingMode {
    /// Asymmetric: single LLID (Conventional FNE Data, Trunked FNE Data).
    Asymmetric,
    /// Symmetric: source + destination LLID via Enhanced Addressing (Direct, Repeated).
    Symmetric,
}

/// Parsed PDU header block -- common fields across all formats.
/// Source: TIA-102.BAAA-B header block layout, TIA-102.BAED-A Section 2
///
/// The header block is always 12 octets (96 bits) before trellis coding.
/// After rate-1/2 trellis: 98 dibits = 196 bits.
#[derive(Debug, Clone)]
pub struct PduHeader {
    /// Access Control: false=outbound (FNE->SU), true=inbound (SU->FNE).
    pub ac: bool,

    /// PDU format (confirmed, unconfirmed, response, MBT).
    pub format: PduFormat,

    /// Service Access Point -- 6-bit routing identifier.
    /// Cross-ref: TIA-102.BAAC-D Section 2.9
    pub sap: u8,

    /// Manufacturer ID -- 0x00 = standard P25.
    /// Cross-ref: TIA-102.BAAC-D Section 2.4
    pub mfid: u8,

    /// Logical Link Identifier (24-bit).
    /// In asymmetric mode: source (inbound) or destination (outbound).
    /// In symmetric mode: destination; source is in Second Header.
    pub llid: u32,

    /// Blocks To Follow -- number of data blocks after this header.
    pub blocks_to_follow: u8,

    /// Pad Octet Count -- padding in last data block.
    pub pad_count: u8,

    /// Data Header Offset -- byte offset to user data within data blocks.
    pub data_header_offset: u8,

    /// Full Message Flag (confirmed only): true = complete packet, false = selective retry.
    pub fmf: bool,

    /// Resynchronize flag (confirmed only): resets V(S)/V(R) to 0.
    pub syn: bool,

    /// Packet Sequence Number Sent (confirmed only): 3-bit, mod 8.
    pub n_s: u8,

    /// Fragment Sequence Number Field (confirmed only): 4-bit (LIC:1 + FSN:3).
    pub fsnf: u8,

    /// True if CRC_HEADER verified OK.
    pub header_crc_ok: bool,

    /// Raw 12-byte header for CRC recalculation.
    pub raw_bytes: [u8; 12],
}

/// Response packet fields -- parsed from a response PDU.
/// Source: TIA-102.BAED-A Section 3.2.4, Table 4
#[derive(Debug, Clone)]
pub struct ResponsePacket {
    /// Response class (2 bits).
    pub class: u8,

    /// Response type (3 bits).
    pub response_type: u8,

    /// Status field carrying N(R) (3 bits).
    pub n_r: u8,

    /// LLID matching the original data packet.
    pub llid: u32,

    /// Selective retry flags (SACK only): one bit per block.
    /// Bit set = block needs retransmission.
    pub sack_flags: Vec<bool>,

    /// BTF in response (SACK only): number of flag entries.
    pub btf: u8,
}

/// Decoded response meaning.
/// Source: TIA-102.BAED-A Table 4
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ResponseMeaning {
    /// ACK -- all blocks successfully received.
    Ack,
    /// NACK -- CRC_PACKET failure, full retransmission required.
    Nack,
    /// SACK -- selective retry, some blocks need retransmission.
    Sack,
    /// Unknown or manufacturer-specific response.
    Unknown { class: u8, response_type: u8 },
}

impl ResponsePacket {
    /// Decode the response meaning from class and type fields.
    /// Source: TIA-102.BAED-A Table 4
    pub fn meaning(&self) -> ResponseMeaning {
        match (self.class, self.response_type) {
            (0b00, 0b001) => ResponseMeaning::Ack,
            (0b01, 0b001) => ResponseMeaning::Nack,
            (0b10, 0b000) => ResponseMeaning::Sack,
            _ => ResponseMeaning::Unknown {
                class: self.class,
                response_type: self.response_type,
            },
        }
    }
}
```

### 11.2 Data Block Structures

```rust
/// A single confirmed data block (16 data octets + 9-bit serial + 9-bit CRC).
/// Source: TIA-102.BAAA-B confirmed data block format
///
/// Before trellis coding: 18 octets (144 bits).
/// After rate-3/4 trellis coding: 98 dibits = 196 bits.
#[derive(Debug, Clone)]
pub struct ConfirmedDataBlock {
    /// Serial number (9 bits, 0 to BTF-1) -- position within the data packet.
    pub serial_number: u16,

    /// 16 octets of data payload.
    pub payload: [u8; 16],

    /// CRC_BLOCK (9-bit CRC-9) verification result.
    pub crc_ok: bool,
}

/// A single unconfirmed data block (12 octets, no serial number, no per-block CRC).
/// Source: TIA-102.BAAA-B unconfirmed data block format
///
/// Before trellis coding: 12 octets (96 bits).
/// After rate-1/2 trellis coding: 98 dibits = 196 bits.
#[derive(Debug, Clone)]
pub struct UnconfirmedDataBlock {
    /// 12 octets of data payload.
    pub payload: [u8; 12],
}

/// A complete data packet after LLC processing.
#[derive(Debug, Clone)]
pub struct DataPacket {
    /// Parsed header block.
    pub header: PduHeader,

    /// Confirmed data blocks (empty if unconfirmed).
    pub confirmed_blocks: Vec<ConfirmedDataBlock>,

    /// Unconfirmed data blocks (empty if confirmed).
    pub unconfirmed_blocks: Vec<UnconfirmedDataBlock>,

    /// Extracted logical message (after CRC_PACKET verification, padding removal).
    /// None if CRC_PACKET failed or blocks are still missing.
    pub logical_message: Option<Vec<u8>>,

    /// CRC_PACKET verification result.
    pub packet_crc_ok: bool,
}
```

### 11.3 Fragment Reassembly State

```rust
/// Fragment Sequence Number Field (FSNF) decoded values.
/// Source: TIA-102.BAED-A Table 3
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FragmentPosition {
    /// LIC=1, FSN=0: complete message in a single packet.
    OnlyInChain,
    /// LIC=0, FSN=0: first fragment of a multi-packet message.
    FirstInChain,
    /// LIC=0, FSN=1-7: middle fragment.
    MiddleInChain { fsn: u8 },
    /// LIC=1, FSN=1-7: final fragment.
    LastInChain { fsn: u8 },
}

impl FragmentPosition {
    /// Decode FSNF field (4 bits) into fragment position.
    pub fn from_fsnf(fsnf: u8) -> Self {
        let lic = (fsnf >> 3) & 1;
        let fsn = fsnf & 0x07;
        match (lic, fsn) {
            (1, 0) => FragmentPosition::OnlyInChain,
            (0, 0) => FragmentPosition::FirstInChain,
            (0, n) => FragmentPosition::MiddleInChain { fsn: n },
            (1, n) => FragmentPosition::LastInChain { fsn: n },
            _ => unreachable!(),
        }
    }

    /// Encode fragment position back to 4-bit FSNF.
    pub fn to_fsnf(&self) -> u8 {
        match self {
            FragmentPosition::OnlyInChain => 0b1_000,
            FragmentPosition::FirstInChain => 0b0_000,
            FragmentPosition::MiddleInChain { fsn } => *fsn & 0x07,
            FragmentPosition::LastInChain { fsn } => 0b1_000 | (*fsn & 0x07),
        }
    }
}

/// State machine for reassembling fragmented logical messages.
/// Source: TIA-102.BAED-A Section 3.2.3 and receiver procedure steps 12-13
pub struct FragmentReassemblyState {
    /// Accumulated fragments (each is a logical message fragment).
    fragments: Vec<Vec<u8>>,

    /// Expected next FSN (1-7, cycling).
    expected_fsn: u8,

    /// Whether we are currently accumulating a multi-fragment message.
    in_progress: bool,
}

impl FragmentReassemblyState {
    pub fn new() -> Self {
        Self {
            fragments: Vec::new(),
            expected_fsn: 0,
            in_progress: false,
        }
    }

    /// Reset state, discarding any partial message.
    pub fn reset(&mut self) {
        self.fragments.clear();
        self.expected_fsn = 0;
        self.in_progress = false;
    }

    /// Process a received fragment. Returns the complete logical message
    /// when the last fragment is received, or None if still accumulating.
    pub fn process_fragment(
        &mut self,
        fsnf: u8,
        data: Vec<u8>,
    ) -> Option<Vec<u8>> {
        let pos = FragmentPosition::from_fsnf(fsnf);
        match pos {
            FragmentPosition::OnlyInChain => {
                self.reset();
                Some(data)
            }
            FragmentPosition::FirstInChain => {
                self.reset();
                self.fragments.push(data);
                self.expected_fsn = 1;
                self.in_progress = true;
                None
            }
            FragmentPosition::MiddleInChain { fsn } => {
                if !self.in_progress || fsn != self.expected_fsn {
                    self.reset();
                    return None;
                }
                self.fragments.push(data);
                self.expected_fsn = if fsn == 7 { 1 } else { fsn + 1 };
                None
            }
            FragmentPosition::LastInChain { fsn } => {
                if !self.in_progress || fsn != self.expected_fsn {
                    self.reset();
                    return None;
                }
                self.fragments.push(data);
                let assembled = self.fragments.concat();
                self.reset();
                Some(assembled)
            }
        }
    }
}
```

### 11.4 LLC Session State (Confirmed Data)

```rust
/// Suggested protocol constants.
/// Source: TIA-102.BAED-A Table 2
pub mod llc_constants {
    /// Maximum pad octets for confirmed data packets.
    pub const P_MAX_CONFIRMED: usize = 15;

    /// Maximum pad octets for unconfirmed data packets.
    pub const P_MAX_UNCONFIRMED: usize = 11;

    /// Maximum transmissions per confirmed data packet.
    pub const TX_MAX: u8 = 4;

    /// Maximum time (ms) to wait before retransmission.
    pub const T_RETRY_MS: u64 = 2000;

    /// Confirmed data block payload size in octets.
    pub const CONFIRMED_BLOCK_SIZE: usize = 16;

    /// Unconfirmed data block payload size in octets.
    pub const UNCONFIRMED_BLOCK_SIZE: usize = 12;

    /// CRC_PACKET size in octets.
    pub const CRC_PACKET_SIZE: usize = 4;

    /// Sequence number modulus (3-bit counter).
    pub const SEQ_MODULUS: u8 = 8;
}

/// LLC sender state for one confirmed data session.
/// Source: TIA-102.BAED-A Section 3.2.1 and 3.2.5
pub struct LlcSenderState {
    /// V(S): sequence number of last data packet sent (3-bit, mod 8).
    pub v_s: u8,

    /// V(R): sequence number of last received packet (used for SYN reset).
    pub v_r: u8,

    /// Whether resynchronization is needed on next packet.
    pub need_syn: bool,

    /// Fragment reassembly state for the current logical message being sent.
    pub fragment_state: FragmentReassemblyState,
}

impl LlcSenderState {
    pub fn new() -> Self {
        Self {
            v_s: 0,
            v_r: 0,
            need_syn: true, // SYN on first packet after startup
            fragment_state: FragmentReassemblyState::new(),
        }
    }

    /// Increment V(S) modulo 8 for a new data packet.
    pub fn next_sequence(&mut self) -> u8 {
        self.v_s = (self.v_s + 1) % llc_constants::SEQ_MODULUS;
        self.v_s
    }

    /// Resynchronize: set V(S) and V(R) to 0.
    pub fn resynchronize(&mut self) {
        self.v_s = 0;
        self.v_r = 0;
        self.need_syn = true;
    }
}

/// LLC receiver state for one confirmed data session.
/// Source: TIA-102.BAED-A Section 3.2.1, 3.2.6
pub struct LlcReceiverState {
    /// V(R): sequence number of last successfully received data packet (3-bit, mod 8).
    pub v_r: u8,

    /// V(S): sender's sequence variable (reset on SYN).
    pub v_s: u8,

    /// CRC_LAST: CRC_PACKET of last successfully received data packet.
    /// Used for duplicate detection when N(S) == V(R).
    pub crc_last: Option<u32>,

    /// P(R): N(S) of a partially received data packet (during retry wait).
    pub p_r: Option<u8>,

    /// Stored data blocks during confirmed reception (indexed by serial number).
    pub block_store: std::collections::HashMap<u16, [u8; 16]>,

    /// BTF and pad_count from the current packet header (during active reception).
    pub current_btf: u8,
    pub current_pad_count: u8,

    /// Fragment reassembly for multi-packet logical messages.
    pub fragment_state: FragmentReassemblyState,

    /// Current receiver state machine position.
    pub rx_state: ConfirmedRxPhase,
}

/// Confirmed receiver state machine phases.
/// Source: TIA-102.BAED-A Figure 3
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ConfirmedRxPhase {
    /// Waiting for a new confirmed data packet.
    WaitForNew,
    /// Waiting for retry data (after SACK or NACK sent).
    RetryWait,
}

impl LlcReceiverState {
    pub fn new() -> Self {
        Self {
            v_r: 0,
            v_s: 0,
            crc_last: None,
            p_r: None,
            block_store: std::collections::HashMap::new(),
            current_btf: 0,
            current_pad_count: 0,
            fragment_state: FragmentReassemblyState::new(),
            rx_state: ConfirmedRxPhase::WaitForNew,
        }
    }

    /// Check if a data packet is a duplicate.
    /// Source: TIA-102.BAED-A Section 3.2.1, V(R) handling
    pub fn is_duplicate(&self, n_s: u8, crc_packet: u32) -> bool {
        if n_s == self.v_r {
            if let Some(crc_last) = self.crc_last {
                return crc_packet == crc_last;
            }
        }
        false
    }

    /// Update V(R) based on received N(S).
    /// Source: TIA-102.BAED-A Section 3.2.1
    pub fn update_v_r(&mut self, n_s: u8, crc_packet: u32) {
        let expected = (self.v_r + 1) % llc_constants::SEQ_MODULUS;
        if n_s == expected {
            // Normal case: in-sequence
            self.v_r = n_s;
        } else if n_s == self.v_r {
            // Possible duplicate: V(R) unchanged
            // (caller should check is_duplicate before calling this)
        } else {
            // Packets lost: resync V(R) to N(S)
            self.v_r = n_s;
        }
        self.crc_last = Some(crc_packet);
    }

    /// Handle SYN bit: reset sequence state.
    pub fn handle_syn(&mut self) {
        self.v_s = 0;
        self.v_r = 0;
        self.crc_last = None;
    }
}
```

### 11.5 CRC Implementation

```rust
/// CRC functions for P25 data packets.
/// Source: TIA-102.BAAA-B CRC definitions, TIA-102.BAED-A Section 3
pub mod data_crc {
    /// CRC-CCITT (16-bit) for PDU header blocks.
    /// Polynomial: x^16 + x^12 + x^5 + 1 = 0x1021
    /// Init: 0xFFFF
    /// Source: TIA-102.BAAA-B
    pub fn crc_ccitt(data: &[u8]) -> u16 {
        let mut crc: u16 = 0xFFFF;
        for &byte in data {
            crc ^= (byte as u16) << 8;
            for _ in 0..8 {
                if crc & 0x8000 != 0 {
                    crc = (crc << 1) ^ 0x1021;
                } else {
                    crc <<= 1;
                }
            }
        }
        crc & 0xFFFF
    }

    /// CRC-9 for confirmed data blocks.
    /// Polynomial: x^9 + x^6 + x^4 + x^3 + 1 = 0x059
    /// Init: 0x1FF
    /// Source: TIA-102.BAAA-B
    pub fn crc9(data: &[u8]) -> u16 {
        let mut crc: u16 = 0x1FF;
        for &byte in data {
            crc ^= (byte as u16) << 1;
            for _ in 0..8 {
                if crc & 0x100 != 0 {
                    crc = (crc << 1) ^ 0x059;
                } else {
                    crc <<= 1;
                }
                crc &= 0x1FF;
            }
        }
        crc & 0x1FF
    }

    /// CRC-32 for packet integrity (CRC_PACKET).
    /// Polynomial: 0x04C11DB7
    /// Init: 0xFFFFFFFF
    /// Source: TIA-102.BAAA-B
    pub fn crc32_packet(data: &[u8]) -> u32 {
        let mut crc: u32 = 0xFFFF_FFFF;
        for &byte in data {
            crc ^= (byte as u32) << 24;
            for _ in 0..8 {
                if crc & 0x8000_0000 != 0 {
                    crc = (crc << 1) ^ 0x04C1_1DB7;
                } else {
                    crc <<= 1;
                }
            }
        }
        crc
    }

    /// Verify CRC_HEADER: compute CRC-CCITT over first 10 bytes,
    /// compare to bytes 10-11 of the header block.
    pub fn verify_header_crc(header: &[u8; 12]) -> bool {
        let computed = crc_ccitt(&header[0..10]);
        let stored = u16::from_be_bytes([header[10], header[11]]);
        computed == stored
    }

    /// Verify CRC_BLOCK: compute CRC-9 over 16 data bytes,
    /// compare to the 9-bit CRC field.
    pub fn verify_block_crc(payload: &[u8; 16], crc_field: u16) -> bool {
        let computed = crc9(payload);
        computed == (crc_field & 0x1FF)
    }

    /// Verify CRC_PACKET: compute CRC-32 over all data bytes (including pad),
    /// compare to the last 4 bytes of the assembled payload.
    pub fn verify_packet_crc(data_with_crc: &[u8]) -> bool {
        if data_with_crc.len() < 4 {
            return false;
        }
        let (data, crc_bytes) = data_with_crc.split_at(data_with_crc.len() - 4);
        let computed = crc32_packet(data);
        let stored = u32::from_be_bytes([crc_bytes[0], crc_bytes[1], crc_bytes[2], crc_bytes[3]]);
        computed == stored
    }
}
```

### 11.6 BTF Computation Helpers

```rust
/// Compute Blocks To Follow and pad count for confirmed data.
/// Source: TIA-102.BAED-A Section 3.2.5 Step 2.6
pub fn btf_confirmed(message_length: usize) -> (u8, u8) {
    let l = message_length;
    let btf = (l + 4 + llc_constants::P_MAX_CONFIRMED) / llc_constants::CONFIRMED_BLOCK_SIZE;
    let pad = btf * llc_constants::CONFIRMED_BLOCK_SIZE - l - 4;
    (btf as u8, pad as u8)
}

/// Compute Blocks To Follow and pad count for unconfirmed data.
/// Source: TIA-102.BAED-A Section 3.3.4 Step 2.5
pub fn btf_unconfirmed(message_length: usize) -> (u8, u8) {
    let l = message_length;
    let btf = (l + 4 + llc_constants::P_MAX_UNCONFIRMED) / llc_constants::UNCONFIRMED_BLOCK_SIZE;
    let pad = btf * llc_constants::UNCONFIRMED_BLOCK_SIZE - l - 4;
    (btf as u8, pad as u8)
}
```

---

## 12. Cross-Reference Summary

### 12.1 Specification Dependencies

| This Spec Section | Depends On | For |
|-------------------|-----------|-----|
| Section 1 (Architecture) | TIA-102.BAEA-C | Data overview, four configurations, LLC/SNDCP layering |
| Section 2 (Header Formats) | TIA-102.BAAA-B | Header block bit layout, trellis coding, DUID 0xC |
| Section 3 (SAP) | TIA-102.BAAC-D | SAP value assignments and meanings |
| Section 4 (Fragmentation) | TIA-102.BAED-A Section 3.2.3 | FSNF encoding, FSN cycling |
| Section 5 (Confirmed) | TIA-102.BAED-A Sections 3.2.1-3.2.6 | Full ARQ procedure |
| Section 6 (Unconfirmed) | TIA-102.BAED-A Sections 3.3.1-3.3.5 | Fire-and-forget procedure |
| Section 8 (CRC) | TIA-102.BAAA-B Sections on CRC | Polynomial definitions |
| Section 9 (Encryption) | TIA-102.AAAD-B | Block encryption, auxiliary headers |
| Section 10 (Parser) | TIA-102.BAAA-B | FDMA frame structure, trellis decoding |
| Section 10 (Parser) | TIA-102.BBAC-A | TDMA data burst extraction |

### 12.2 Trunking Integration (TIA-102.AABC-E)

Data channel allocation for LLC packet data is managed by trunking control:

| TSBK Opcode | Direction | Message | Relevance |
|-------------|-----------|---------|-----------|
| 0x14 (outbound) | FNE->SU | SNDCP Data Channel Grant | Assigns SU to a data channel for LLC operation |
| 0x15 (outbound) | FNE->SU | SNDCP Data Page Request | Pages SU to set up data session |
| 0x16 (outbound) | FNE->SU | SNDCP Data Channel Announcement Explicit | Announces active data channel |
| 0x12 (inbound) | SU->FNE | SNDCP Data Channel Request | SU requests data channel |
| 0x13 (inbound) | SU->FNE | SNDCP Data Page Response | SU responds to data page |
| 0x14 (inbound) | SU->FNE | SNDCP Reconnect Request | SU requests reconnection |

After receiving an SNDCP Data Channel Grant, the SU tunes to the assigned channel and
begins LLC-layer packet data exchange (confirmed or unconfirmed) on that channel.

### 12.3 SDRTrunk / OP25 Code References

For open-source implementation reference:

**SDRTrunk (Java):**
- `PDUMessage.java` -- PDU header parsing, field extraction
- `PacketMessage.java` -- data block assembly and CRC verification
- `ConfirmedDataBlock.java` -- confirmed block with serial number and CRC-9
- `UnconfirmedDataBlock.java` -- unconfirmed block handling
- `TrellisCodec.java` -- rate 1/2 and 3/4 trellis decode (Viterbi)

**OP25 (C++):**
- `p25p2_tdma.cc` -- TDMA data channel handling
- `p25_frame_assembler.cc` -- PDU frame assembly
- `rs.cc` / `bch.cc` -- FEC codecs used before LLC layer

These implementations confirm the header field positions, CRC algorithms, and trellis
decoding approach described in this specification.

---

*End of implementation specification -- TIA-102.BAED-A Packet Data LLC*
