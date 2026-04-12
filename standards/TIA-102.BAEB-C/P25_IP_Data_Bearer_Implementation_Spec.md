# P25 IP Data Bearer Service (SNDCP/SCEP) -- Implementation Specification

**Source:** TIA-102.BAEB-C (December 2019), "Project 25 IP Data Bearer Service Specification"
**Classification:** PROTOCOL + DATA
**Phase:** 3 -- Implementation-ready
**Extracted:** 2026-04-12
**Purpose:** Complete implementation guide for the IP convergence layer that carries IPv4
datagrams over the P25 air interface. Defines two protocols: SCEP (Simple CAI Encapsulation
Protocol) for direct/repeated/conventional data, and SNDCP (Subnetwork Dependent Convergence
Protocol) for conventional and trunked FNE data. SNDCP provides context management, IP address
binding, header compression, authentication, segmentation via the LLC layer, and multiplexing
of up to 14 concurrent data connections per subscriber unit.

**Cross-references:**
- TIA-102.BAED-A -- Packet Data LLC (layer below SNDCP; fragmentation, ARQ, data blocks)
- TIA-102.BAAA-B -- FDMA Common Air Interface (PDU frame format, DUID 0xC, trellis coding)
- TIA-102.BBAC-A -- TDMA MAC Layer (data channel burst structure for Phase 2)
- TIA-102.BAAC-D -- Reserved Values (SAP identifiers, LLID broadcast address, ARP SAP)
- TIA-102.AABC-E -- Trunking Control Channel Messages (SN-DATA_CHN_REQ/GNT, SN-DATA_PAGE)
- TIA-102.AABF-D -- Link Control Words (LC_CALL_TRM_CAN for TDS teardown)
- TIA-102.BAEA-C -- Data Overview and Specification (four configurations, endpoint models)
- TIA-102.BAEJ-A -- CMS Specification for Packet Data (scan mode, conventional management)
- SDRTrunk / OP25 -- Open-source implementations with partial SNDCP data service parsing

---

## 1. Protocol Stack -- Where SNDCP Sits

### 1.1 Full Protocol Stack

```
 ┌──────────────────────────────────────────────────────────────────────┐
 │                     Application Layer                                │
 │              (TCP, UDP, ICMP over IPv4)                              │
 ├──────────────────────────────────────────────────────────────────────┤
 │                     Network Layer (IPv4)                             │
 │          RFC 791 datagrams / RFC 792 ICMP messages                  │
 ├─────────────────────────┬────────────────────────────────────────────┤
 │  SCEP                   │  SNDCP ◄── THIS SPEC                     │
 │  (Simple CAI            │  (Subnetwork Dependent Convergence)       │
 │   Encapsulation)        │  - Context management (activate/deact)    │
 │  - No PDU wrapper       │  - 2-byte data header (PCOMP+DCOMP)      │
 │  - Raw IP = logical msg │  - Header compression (VJ/RFC2507)       │
 │  - ARP for addr binding │  - Up to 14 NSAPIs per SU                │
 │  - Direct/Repeated/Conv │  - Confirmed + Unconfirmed delivery      │
 │                         │  - Conventional + Trunked FNE             │
 ├─────────────────────────┴────────────────────────────────────────────┤
 │              LLC Layer (TIA-102.BAED-A)                             │
 │  - Confirmed data: stop-and-wait ARQ with SACK                     │
 │  - Unconfirmed data: fire-and-forget                                │
 │  - Fragmentation / reassembly of logical messages                   │
 │  - Block sequencing, CRC validation, encryption integration         │
 ├──────────────────────────────────────────────────────────────────────┤
 │              MAC / PHY Layer                                        │
 │  FDMA (TIA-102.BAAA-B)         │  TDMA (TIA-102.BBAC-A)           │
 │  - PDU frame (DUID 0xC)        │  - DCH data burst                 │
 │  - Trellis coding, NID, sync   │  - Per-slot FEC, ISCH             │
 └──────────────────────────────────────────────────────────────────────┘
```

### 1.2 Protocol Selection by Configuration

| Data Configuration | SCEP | SNDCP | Notes |
|-------------------|------|-------|-------|
| Direct Data (SU-to-SU) | YES | NO | No infrastructure; SCEP only |
| Repeated Data (SU-to-SU via repeater) | YES | NO | SCEP only |
| Conventional FNE Data (SU-FNE) | YES | YES | Either protocol; SNDCP for managed service |
| Trunked FNE Data (SU-FNE) | NO | YES | SNDCP mandatory for trunking |

### 1.3 SAP Values and Data Header Offset

| Protocol | SAP Value | Data Header Offset | Notes |
|----------|-----------|-------------------|-------|
| SCEP IP datagram | Packet Data (from BAAC-D) | `%000000` (0) | Raw IP is the logical message |
| SCEP ARP message | ARP (from BAAC-D) | `%000000` (0) | Unconfirmed delivery only |
| SNDCP context mgmt PDU | SNDCP Packet Data Control | `%000000` (0) | Confirmed delivery required |
| SNDCP data PDU (SN-Data/SN-UData) | Unencrypted User Data | `%000010` (2) | 2-byte SNDCP header precedes IP payload |

---

## 2. SNDCP PDU Formats

### 2.1 PDU Type Field (4 bits)

| Value | PDU Name | Direction | Purpose |
|-------|----------|-----------|---------|
| 0 | SN-Context Activation Request | SU -> FNE | Initiate context; negotiate parameters |
| 1 | SN-Context Activation Accept | FNE -> SU | Confirm context; return negotiated params |
| 2 | SN-Context Activation Reject | FNE -> SU | Deny context; include reject code |
| 3 | SN-Context Deactivation Request | SU <-> FNE | Tear down context(s) |
| 4 | SN-Data | SU <-> FNE | Confirmed IP datagram conveyance |
| 5 | SN-UData | SU <-> FNE | Unconfirmed IP datagram conveyance |
| 6 | SN-Context Activation Command | FNE -> SU | FNE-initiated activation (V2+) |
| 7-15 | Reserved | -- | -- |

### 2.2 SNDCP Version Field (4 bits)

| Value | Version | Key Additions |
|-------|---------|---------------|
| 1 | V1 | Base: context mgmt, VJ TCP/IP compression, static/dynamic IPv4 |
| 2 | V2 | + CHAP auth (PCO), APN for data host selection, FNE-initiated activation |
| 3 | V3 | + RFC 2507 compression, expanded HCOMP, MSO, Version Supported, DAC updates |
| 15 | WAI V1 | WAI (wideband) variant; distinct protocol space |

### 2.3 Data Payload PDU Formats

#### 2.3.1 SN-Data PDU (Confirmed, PDU Type = 4)

```
 Byte 0                 Byte 1                 Byte 2..N
 ┌─┬─┬─┬─┬─┬─┬─┬─┐   ┌─┬─┬─┬─┬─┬─┬─┬─┐   ┌──────────────────┐
 │PDU Type│  NSAPI │   │ PCOMP │  DCOMP │   │  Network PDU     │
 │  0100  │ (4b)   │   │ (4b)  │  (4b)  │   │  (IP datagram,   │
 └─┴─┴─┴─┴─┴─┴─┴─┘   └─┴─┴─┴─┴─┴─┴─┴─┘   │   variable len)  │
                                             └──────────────────┘
```

- **LLC delivery:** Confirmed Data Packet Delivery (ARQ, retransmission)
- **SAP:** Unencrypted User Data SAP
- **Data Header Offset:** 2 (the 2-byte SNDCP header)
- **Max size:** Up to negotiated MTU (296, 510, 1020, or 1500 bytes)

#### 2.3.2 SN-UData PDU (Unconfirmed, PDU Type = 5)

```
 V1/V2:
 Byte 0                 Byte 1                 Byte 2..N
 ┌─┬─┬─┬─┬─┬─┬─┬─┐   ┌─┬─┬─┬─┬─┬─┬─┬─┐   ┌──────────────────┐
 │PDU Type│  NSAPI │   │Reserved│  DCOMP │   │  Network PDU     │
 │  0101  │ (4b)   │   │ 0000   │  (4b)  │   │  (IP datagram)   │
 └─┴─┴─┴─┴─┴─┴─┴─┘   └─┴─┴─┴─┴─┴─┴─┴─┘   └──────────────────┘

 V3:
 Byte 0                 Byte 1                 Byte 2..N
 ┌─┬─┬─┬─┬─┬─┬─┬─┐   ┌─┬─┬─┬─┬─┬─┬─┬─┐   ┌──────────────────┐
 │PDU Type│  NSAPI │   │ PCOMP │  DCOMP │   │  Network PDU     │
 │  0101  │ (4b)   │   │ (4b)  │  (4b)  │   │  (IP datagram)   │
 └─┴─┴─┴─┴─┴─┴─┴─┘   └─┴─┴─┴─┴─┴─┴─┴─┘   └──────────────────┘
```

- **LLC delivery:** Unconfirmed Data Packet Delivery (no ACK)
- **Max size:** min(510, negotiated MTU) octets -- hard limit on unconfirmed
- **V1/V2:** PCOMP field is reserved (0); no header compression on unconfirmed
- **V3:** PCOMP is active; header compression permitted on unconfirmed datagrams

### 2.4 Context Management PDU Formats

All context management PDUs use:
- Confirmed Data Packet Delivery
- SNDCP Packet Data Control SAP
- Data Header Offset = 0

#### 2.4.1 SN-Context Activation Request (PDU Type = 0)

**V1 format (10 octets):**

| Octet | Bits 7-4 | Bits 3-0 |
|-------|----------|----------|
| 0 | PDU Type (0000) | SNDCP Version (0001) |
| 1 | NSAPI (1-14) | NAT |
| 2-5 | IPv4 Address (32 bits) | |
| 6 | DSUT | UDPC |
| 7 | IPHC (8 bits) | |
| 8 | TCPSS | UDPSS |
| 9 | Reserved | MDPCO |

**V2 format (10+ octets):** Same as V1 plus:
- APN Length (1 octet) + APN Value (variable)
- PCO Length (1 octet) + PCO Value (variable)

**V3 format (variable):** Major changes:
- IPv4 Address is CONDITIONAL (present only when NAT = Static)
- IPHC replaced by HCOMP (8 bits) -- adds RFC 2507 bit
- TCPSS/UDPSS replaced by VJ_TCP_CSS (8b), TCP_CSS (8b), NON_TCP_CSS (16b)
- Added: MAX_INT_FHD (8b), MAX_TME_FHD (8b), MAX_HDR_SZE (8b)
- APN, PCO, MSO fields at end

#### 2.4.2 SN-Context Activation Accept (PDU Type = 1)

**V1 format (13 octets):**

| Octet | Bits 7-4 | Bits 3-0 |
|-------|----------|----------|
| 0 | PDU Type (0001) | NSAPI |
| 1 | PDUPM (priority 0-15) | Ready Timer code |
| 2 | Standby Timer code | NAT |
| 3-6 | IPv4 Address (32 bits) | |
| 7 | IPHC (8 bits) | |
| 8 | TCPSS | UDPSS |
| 9 | MTU code | UDPC |
| 10 | Reserved | MDPCO |
| 11-12 | DAC (16 bits) | |

**V3 format (variable):** IPv4 Address conditional (present only when NAT = Dynamic).
Expanded compression negotiation fields. DAC updatable via unsolicited Accept.

#### 2.4.3 SN-Context Activation Reject (PDU Type = 2)

| Octet | Content |
|-------|---------|
| 0 | PDU Type (0010) + NSAPI |
| 1 | Reject Code (8 bits) |
| 2-3 | Version Supported (16 bits) -- V3 only, conditional on Reject Code = 23 |

#### 2.4.4 SN-Context Deactivation Request (PDU Type = 3)

| Octet | Content |
|-------|---------|
| 0 | PDU Type (0011) + NSAPI |
| 1 | Deactivation Type: 0 = all NSAPIs, 1 = this NSAPI only |

#### 2.4.5 SN-Context Activation Command (PDU Type = 6, V2+)

| Octet | Content |
|-------|---------|
| 0 | PDU Type (0110) + SNDCP Version |
| 1 | NSAPI + NAT |
| 2 | APN Length |
| 3-X | APN Value (conditional) |
| Y-Z | IPv4 Address (conditional) |

---

## 3. IP Packet Encapsulation -- End-to-End Data Flow

### 3.1 SCEP: IP Datagram Encapsulation (Direct/Repeated/Conventional)

SCEP performs zero-overhead encapsulation -- the IP datagram IS the logical message:

```
  Application generates IPv4 datagram
       │
       ▼
  IP datagram (raw bytes)
       │
       ▼
  Set SAP = Packet Data, Data Header Offset = 0
       │
       ▼
  LLC layer (BAED-A): fragment into data blocks
  ├── Confirmed: ARQ with ACK/NACK/SACK
  └── Unconfirmed: fire-and-forget single shot
       │
       ▼
  MAC/PHY: trellis-code header + data blocks
       │
       ▼
  Transmit FDMA PDU frame (DUID 0xC) or TDMA data burst
```

### 3.2 SNDCP: IP Datagram Encapsulation (Conventional FNE / Trunked FNE)

```
  Application generates IPv4 datagram
       │
       ▼
  SNDCP context lookup: find active NSAPI for this flow
       │
       ▼
  [Optional] Header compression:
  ├── PCOMP=0: no compression, pass through
  ├── PCOMP=1: Van Jacobson compressed TCP/IP (RFC 1144)
  ├── PCOMP=2: Van Jacobson uncompressed TCP/IP (RFC 1144)
  ├── PCOMP=3: FULL_HEADER (RFC 2507) -- V3 only
  ├── PCOMP=4: COMPRESSED_TCP (RFC 2507) -- V3 only
  ├── PCOMP=5: COMPRESSED_TCP_NODELTA (RFC 2507) -- V3 only
  └── PCOMP=6: COMPRESSED_NON_TCP (RFC 2507) -- V3 only
       │
       ▼
  Prepend 2-byte SNDCP header:
  ┌──────────────────┬──────────────────┐
  │ PDU Type | NSAPI │ PCOMP  |  DCOMP │
  │  (4b)    | (4b)  │  (4b)  |  (4b)  │
  └──────────────────┴──────────────────┘
       │
       ▼
  Set SAP = Unencrypted User Data, Data Header Offset = 2
       │
       ▼
  LLC layer (BAED-A): fragment SNDCP PDU into data blocks
  ├── SN-Data (type 4): Confirmed delivery, up to MTU bytes
  └── SN-UData (type 5): Unconfirmed delivery, max 510 bytes
       │
       ▼
  MAC/PHY: trellis-code header + data blocks
       │
       ▼
  Transmit on PDCH (packet data channel)
```

### 3.3 Concrete Byte-Level Example

Sending a 100-byte IP datagram via SNDCP confirmed, NSAPI=3, no compression:

```
  Original IP datagram: 100 bytes
  SNDCP header: [0x43, 0x00]  (PDU Type=4, NSAPI=3, PCOMP=0, DCOMP=0)
  Logical message: 102 bytes total

  LLC header (asymmetric, confirmed):
    AC=1 (inbound), SAP=User Data, MFID=$00
    LLID=subscriber's 24-bit ID
    BTF = ceil(102/12) data blocks  [depends on encoding; see BAED-A]
    Data Header Offset = 2
    N(S) = sequence number (mod 8)
    FMF = 1 (full message)

  Data blocks: each 12 bytes raw (196 bits trellis-coded)
    Block 0: bytes 0-11 of logical message (includes SNDCP header)
    Block 1: bytes 12-23
    ...
    Last block: remaining bytes + pad octets
```

---

## 4. Data Channel Setup -- SNDCP Channel Request/Grant Flow

### 4.1 TDS PDCH Access (Trunked Data Service)

Two access modes exist for obtaining a packet data channel in trunked mode:

#### 4.1.1 Requested Access (RA)

```
  SU (Control Channel)              FNE (Control Channel)
       │                                  │
       │── SN-DATA_CHN_REQ ──────────────>│  (AABC-E ISP message)
       │                                  │  FNE evaluates:
       │                                  │  - SU authorized?
       │                                  │  - PDCH available?
       │                                  │
       │<── SN-DATA_CHN_GNT ─────────────│  (AABC-E OSP: channel + freq)
       │    OR                            │
       │<── QUE_RSP ─────────────────────│  (queued, try later)
       │    OR                            │
       │<── DENY_RSP ────────────────────│  (denied, reason code)
       │                                  │
       │  [SU tunes to granted PDCH]      │
       │                                  │
  SU (PDCH)                          FNE (PDCH)
       │── SN-Context Activation Req ────>│  (first context)
       │    OR                            │
       │── SN-Data / SN-UData ───────────>│  (subsequent data)
```

#### 4.1.2 Autonomous Access (AA)

```
  SU (monitors PDCH announcements)
       │
       │  SN-DATA_CHN_ANN_EXP seen on control channel (AABC-E OSP)
       │  DAC bits in context Accept permit AA for this SU
       │
       │  [SU tunes to announced PDCH directly -- no REQ/GNT]
       │
  SU (PDCH)                          FNE (PDCH)
       │── SN-Data / SN-UData ───────────>│
```

- AA is controlled by the 16-bit DAC (Data Access Control) field
- In V3+, DAC can be updated dynamically via unsolicited SN-Context Activation Accept
- AA is not permitted for CDS (conventional); only RA applies

#### 4.1.3 FNE-Initiated Data Delivery (Outbound)

```
  FNE (Control Channel)              SU (Control Channel)
       │                                  │
       │── SN-DATA_PAGE_REQ ─────────────>│  (AABC-E OSP: page the SU)
       │                                  │
       │<── SN-DATA_PAGE_RES ────────────│  (AABC-E ISP)
       │    OR                            │
       │<── SN-DATA_CHN_REQ ─────────────│  (SU requests PDCH)
       │                                  │
       │── SN-DATA_CHN_GNT ─────────────>│
       │                                  │
  FNE (PDCH)                          SU (PDCH)
       │── SN-Data / SN-UData ───────────>│
```

### 4.2 CDS PDCH Access (Conventional Data Service)

Conventional mode has no separate control channel. The SU transmits directly on the
conventional PDCH -- no SN-DATA_CHN_REQ/GNT exchange needed:

```
  SU (Conventional PDCH)              FNE (Conventional PDCH)
       │                                    │
       │── SN-Context Activation Req ──────>│
       │<── SN-Context Activation Accept ──│
       │                                    │
       │── SN-Data / SN-UData ────────────>│  (data transfer)
```

### 4.3 Cross-Reference: Control Channel Messages (AABC-E)

| Alias | Direction | Type | Purpose |
|-------|-----------|------|---------|
| SN-DATA_CHN_REQ | SU -> FNE | ISP | Request PDCH assignment |
| SN-DATA_CHN_GNT | FNE -> SU | OSP | Grant PDCH (channel + freq) |
| SN-DATA_PAGE_REQ | FNE -> SU | OSP | Page SU for outbound data |
| SN-DATA_PAGE_RES | SU -> FNE | ISP | Respond to page |
| SN-REC_REQ | SU -> FNE | ISP | Reconnect after roaming |
| SN-DATA_CHN_ANN_EXP | FNE -> SU | OSP | Announce available PDCH for AA |
| QUE_RSP | FNE -> SU | OSP | Request queued |
| DENY_RSP | FNE -> SU | OSP | Request denied |
| CAN_SRV_REQ | SU -> FNE | ISP | Cancel pending service request |

---

## 5. Address Resolution -- IP to P25 Subscriber ID Mapping

### 5.1 SCEP Address Resolution (ARP)

SCEP uses a P25 variant of Ethernet ARP (RFC 826) with Hardware Address Type = 33
(`ARP-TYPE_CAI`). This is used only in Direct Data and Repeated Data configurations.

```
  ARP message format (22 octets):
  ┌──────────────────────────────────────────────────────┐
  │ Hardware Address Type     (16b) = 0x0021 (33)       │
  │ Protocol Address Type     (16b) = Packet Data SAP   │
  │ Hardware Address Length   (8b)  = 3 (LLID = 3 bytes)│
  │ Protocol Address Length   (8b)  = 4 (IPv4 = 4 bytes)│
  │ Opcode                   (16b) = 1 (Req) / 2 (Reply)│
  │ Sender Hardware Address  (24b) = Source LLID         │
  │ Sender Protocol Address  (32b) = Source IPv4         │
  │ Target Hardware Address  (24b) = Target LLID         │
  │ Target Protocol Address  (32b) = Target IPv4         │
  └──────────────────────────────────────────────────────┘
```

**ARP Request:** Destination LLID = broadcast ("Designates Everyone" from BAAC-D).
Target Hardware Address = don't care (unknown).

**ARP Reply:** Destination LLID = requester's LLID. Sender Hardware Address = responder's LLID.

**Delivery:** ARP messages use unconfirmed data packet delivery, symmetric addressing,
SAP = ARP (from BAAC-D), Data Header Offset = 0.

### 5.2 SNDCP Address Resolution

SNDCP does NOT use ARP. IP-to-LLID binding is established during context activation:

| Method | NAT Value | Flow |
|--------|-----------|------|
| Static IPv4 | NAT = 0 | SU includes pre-provisioned IPv4 in Activation Request; FNE validates |
| Dynamic IPv4 | NAT = 1 | SU requests; FNE allocates IP and returns it in Activation Accept |
| No Address | NAT = 15 | Control signaling only (NSAPI 0) |

- FNE LLID is implicit (no IP needed for FNE in conventional/trunked FNE configs)
- FNE maintains IP-to-LLID bindings for all active contexts
- SU only needs its own IP binding
- Bindings are discarded on context deactivation

### 5.3 Implementation: Address Binding Table

```
  Per-SU binding record:
  ┌──────────────────────────────────────────┐
  │ LLID          (24 bits)                  │  P25 subscriber logical link ID
  │ NSAPI         (4 bits, 1-14)             │  Context identifier
  │ IPv4 Address  (32 bits)                  │  Bound IP address
  │ NAT           (static/dynamic)           │  Assignment method
  │ SNDCP Version (1/2/3)                    │  Negotiated version
  │ PCOMP config  (compression params)       │  Header compression state
  │ MTU           (296/510/1020/1500)        │  Negotiated MTU
  │ Ready Timer   (encoded value)            │  Timeout to Standby
  │ Standby Timer (encoded value)            │  Timeout to Idle
  │ DAC           (16 bits)                  │  Data Access Control (TDS only)
  │ PDUPM         (4 bits)                   │  Priority max
  └──────────────────────────────────────────┘
```

---

## 6. Header Compression

### 6.1 Van Jacobson TCP/IP Header Compression (RFC 1144)

Available in all SNDCP versions (V1+). Negotiated during context activation via the
IPHC/HCOMP field (bit 0) and compression state slots (TCPSS / VJ_TCP_CSS).

**PCOMP values for VJ compression:**

| PCOMP | Meaning | Description |
|-------|---------|-------------|
| 0 | No compression | Full IP+TCP header transmitted |
| 1 | Compressed TCP/IP | VJ-compressed header (delta encoding) |
| 2 | Uncompressed TCP/IP | Full header but with connection ID added |

**Negotiation:**
- SU requests VJ compression in Activation Request: IPHC bit 0 = 1, TCPSS = N (slots)
- FNE may accept (echo settings) or deny (clear IPHC bit 0, TCPSS = 0)
- Both sides maintain N+1 compression state slots (TCPSS field encodes 0-15 = 1-16 slots)

**V1/V2 slots:** TCPSS field is 4 bits, range 1-16 slots (encoded as 0-15).
**V3 slots:** VJ_TCP_CSS field is 8 bits, range 0-255 slots.

### 6.2 RFC 2507 Header Compression (V3 Only)

Available in SNDCP V3+. Negotiated via HCOMP field (bit 1) and extended fields.

**Additional PCOMP values for RFC 2507:**

| PCOMP | RFC 2507 Packet Type | Description |
|-------|---------------------|-------------|
| 3 | FULL_HEADER | Uncompressed; establishes/refreshes context |
| 4 | COMPRESSED_TCP | Delta-compressed TCP header |
| 5 | COMPRESSED_TCP_NODELTA | TCP with no delta (after loss/reset) |
| 6 | COMPRESSED_NON_TCP | Compressed non-TCP (UDP, ICMP, etc.) |

**V3 compression negotiation fields:**

| Field | Size | Purpose |
|-------|------|---------|
| HCOMP | 8 bits | Bitmask: bit 0 = VJ (RFC 1144), bit 1 = RFC 2507 |
| VJ_TCP_CSS | 8 bits | VJ TCP compression state slots (0-255) |
| TCP_CSS | 8 bits | RFC 2507 TCP compression state slots (0-255) |
| NON_TCP_CSS | 16 bits | RFC 2507 non-TCP compression state slots (0-65535) |
| MAX_INT_FHD | 8 bits | Max interval between full headers: 0=disabled, else 16 x value |
| MAX_TME_FHD | 8 bits | Max time between full headers: 0=disabled, else 5sec x value |
| MAX_HDR_SZE | 8 bits | Max header size in octets (60-255 valid, 0=don't care) |

**Restrictions:**
- Maximum Context Identifier for TCP and non-TCP defaults to 15 (not negotiated)
- Reordering of streams option (RFC 2507) is explicitly NOT supported
- UDP/IP compression was never defined for V1/V2; RFC 2507 in V3 subsumes it

### 6.3 Data Compression (DCOMP)

No payload compression algorithms are defined. DCOMP field is always 0. Future SNDCP
versions may define algorithms; the 4-bit DCOMP field reserves 15 possible values.

---

## 7. Confirmed vs Unconfirmed Data Service Selection

### 7.1 Selection Criteria

The choice between confirmed (SN-Data) and unconfirmed (SN-UData) delivery is based on
local policy. Key differences:

| Aspect | Confirmed (SN-Data, Type 4) | Unconfirmed (SN-UData, Type 5) |
|--------|---------------------------|-------------------------------|
| LLC service | Stop-and-wait ARQ | Fire-and-forget |
| Acknowledgment | ACK/NACK/SACK from receiver | None |
| Retransmission | Yes (selective retry via SACK) | No |
| Fragmentation | Yes (LLC handles multi-block) | No (single logical message) |
| Max IP datagram | Negotiated MTU (up to 1500) | min(510, MTU) octets |
| Header compression V1/V2 | PCOMP active | PCOMP reserved (=0) |
| Header compression V3 | PCOMP active | PCOMP active |
| Typical use | Large transfers, critical data | Small datagrams, status updates |

### 7.2 PDU Type and LLC Mapping

```
  SN-Data (PDU Type = 4):
    → LLC confirmed data packet
    → Header: AC + SAP + MFID + LLID + BTF + N(S) + FMF + CRC_HEADER
    → Data blocks: trellis-coded, CRC per block
    → Response: ACK/NACK/SACK on response channel

  SN-UData (PDU Type = 5):
    → LLC unconfirmed data packet
    → Header: AC + SAP + MFID + LLID + BTF + CRC_HEADER
    → Data blocks: trellis-coded, CRC_PACKET at end
    → No response expected
```

### 7.3 Implementation Decision Logic

```
  fn select_delivery_mode(datagram_size: usize, context: &SndcpContext) -> DeliveryMode {
      let max_unconfirmed = std::cmp::min(510, context.mtu);
      if datagram_size > max_unconfirmed {
          // Must use confirmed -- exceeds unconfirmed limit
          return DeliveryMode::Confirmed;
      }
      // Within unconfirmed limit -- choose based on policy
      match context.delivery_policy {
          Policy::AlwaysConfirmed => DeliveryMode::Confirmed,
          Policy::AlwaysUnconfirmed => DeliveryMode::Unconfirmed,
          Policy::SizeThreshold(t) => {
              if datagram_size > t { DeliveryMode::Confirmed }
              else { DeliveryMode::Unconfirmed }
          }
          Policy::ProtocolBased => {
              // TCP = confirmed, UDP/ICMP = unconfirmed
              // (inspect IP protocol field)
              DeliveryMode::from_ip_protocol(datagram)
          }
      }
  }
```

---

## 8. Multi-Block Data Transfer

### 8.1 How Large IP Packets Span Multiple P25 Data Blocks

When an IP datagram (plus SNDCP header) exceeds a single data block, the LLC layer
fragments it across multiple data blocks within one PDU frame. This is distinct from
IP fragmentation -- the LLC operates on the full SNDCP PDU as a "logical message."

```
  IP datagram (e.g., 1500 bytes)
       │
       ▼
  SNDCP encapsulation: 2-byte header + 1500 bytes = 1502-byte logical message
       │
       ▼
  LLC confirmed data packet:
  ┌──────────────────────────────────────────────────────────────────┐
  │ Header Block (12 octets raw → 196 bits trellis-coded)          │
  │  - BTF = number of data blocks to follow                       │
  │  - Pad Octet Count = padding in last block                     │
  │  - Data Header Offset = 2 (SNDCP header)                      │
  │  - N(S) = sequence number                                       │
  │  - FMF = 1 (full message, first transmission)                  │
  ├──────────────────────────────────────────────────────────────────┤
  │ Data Block 0 (12 octets raw → 196 bits trellis-coded)          │
  │  Bytes: [SNDCP hdr (2)] [IP bytes 0..9]                       │
  ├──────────────────────────────────────────────────────────────────┤
  │ Data Block 1 (12 octets raw → 196 bits trellis-coded)          │
  │  Bytes: [IP bytes 10..21]                                      │
  ├──────────────────────────────────────────────────────────────────┤
  │ ...                                                             │
  ├──────────────────────────────────────────────────────────────────┤
  │ Data Block N (12 octets + pad → 196 bits trellis-coded)        │
  │  Bytes: [IP bytes ...end] [pad octets]                         │
  │  4 octets: CRC_PACKET (CRC-32 over entire logical message)    │
  └──────────────────────────────────────────────────────────────────┘
```

### 8.2 Block Count Calculation

```
  payload_size = sndcp_header_size + ip_datagram_size  // 2 + datagram_len
  
  // For FDMA (BAAA-B): each data block carries 12 octets of payload
  // The last 4 octets of the last block are CRC_PACKET
  total_payload_with_crc = payload_size + 4  // CRC_PACKET = 4 octets
  blocks_to_follow = ceil(total_payload_with_crc / 12)
  pad_octets = (blocks_to_follow * 12) - total_payload_with_crc
```

Note: Exact data block payload sizes depend on the PHY layer and confirmed/unconfirmed
mode. Consult BAED-A and BAAA-B for precise values. The 12-octet figure is typical for
FDMA; TDMA (BBAC-A) uses different burst sizes.

### 8.3 Confirmed Multi-Block: ARQ and Selective Retransmission

When using confirmed delivery for multi-block transfers:

1. Sender transmits all data blocks with sequence number N(S)
2. Receiver validates each block CRC
3. Receiver sends response:
   - **ACK:** All blocks received correctly
   - **NACK:** One or more blocks failed; request full retransmission
   - **SACK:** Selective ACK -- bitmap indicates which blocks failed;
     sender retransmits only failed blocks (FMF=0 for selective retry)
4. Process repeats until all blocks ACKed or retry limit reached

### 8.4 MTU Values and Their Implications

| MTU Code | Bytes | Data Blocks (approx FDMA) | Use Case |
|----------|-------|--------------------------|----------|
| 1 | 296 | ~26 blocks | Minimal; fits IP minimum reassembly buffer |
| 2 | 510 | ~44 blocks | Default; max for unconfirmed |
| 3 | 1020 | ~86 blocks | Intermediate |
| 4 | 1500 | ~126 blocks | Full Ethernet MTU; long air time |

Large MTU values mean long channel occupancy. A 1500-byte datagram requires 100+
data blocks and several seconds of continuous PDCH transmission at 9600 bps.

---

## 9. Registration for Data Services

### 9.1 TDS Registration Flow

Before SNDCP can operate in trunked mode, the SU must complete standard P25
trunking registration (per AABD-B). The SNDCP state machine begins in Closed state
and transitions to Idle when an LLID is allocated during registration:

```
  SU Power On
       │
       ▼
  Trunking Registration (AABD-B procedures)
  ├── SU scans for control channel
  ├── SU sends Registration Request
  ├── FNE validates, assigns LLID
  └── SU receives Registration Accept
       │
       ▼
  SNDCP State: Closed → Idle (LLID allocated)
       │
       ▼
  First Context Activation:
  ├── SU sends SN-DATA_CHN_REQ on control channel
  ├── FNE grants PDCH via SN-DATA_CHN_GNT
  ├── SU tunes to PDCH, enters Ready* state
  ├── SU sends SN-Context Activation Request
  ├── FNE accepts: SN-Context Activation Accept
  └── Both enter Ready state
       │
       ▼
  Data transfer permitted (SN-Data / SN-UData)
```

### 9.2 CDS Registration

For conventional data, registration is handled by the Conventional Management Service
(CMS, per BAEJ-A). The SU uses Packet Data Registration to establish its LLID and
IP binding before SNDCP context activation.

### 9.3 Ready Roaming (TDS Only)

When a trunked SU roams to a different site while in Ready state:

```
  Ready State (Site A)
       │
       ▼
  SU detects site change → Ready Roaming State
  ├── Confirmed delivery paused
  ├── SU registers with new site (Site B)
  ├── SU sends SN-REC_REQ on new control channel
  ├── FNE grants PDCH via SN-DATA_CHN_GNT
  └── SU resumes confirmed delivery on new PDCH
       │
       ▼
  Ready State (Site B)
```

### 9.4 State Machine Summary

#### TDS States (6):

```
  ┌────────┐  LLID allocated   ┌──────┐  Activate   ┌────────┐
  │ Closed ├──────────────────>│ Idle ├────────────>│ Ready* │
  └────────┘                   └──┬───┘             └───┬────┘
       ▲                          │                     │
       │ LLID revoked             │ All deactivated     │ Accept
       │ (from any state)         │                     ▼
       │                          │               ┌─────────┐
       │                          └───────────────│  Ready  │
       │                                          └──┬──┬───┘
       │                              Ready Timer    │  │ Roaming
       │                              expires        │  │ detected
       │                                    ┌────────┘  └──────┐
       │                                    ▼                   ▼
       │                              ┌──────────┐    ┌─────────────┐
       │                              │ Standby  │    │Ready Roaming│
       │                              └──────────┘    │  (TDS only) │
       │                                              └─────────────┘
```

#### CDS States (5): Same as TDS minus Ready Roaming.

### 9.5 Timer Values

**Ready Timer (4-bit code):**

| Code | Duration | Code | Duration |
|------|----------|------|----------|
| 0 | Not Allowed | 8 | 20 sec |
| 1 | 1 sec | 9 | 25 sec |
| 2 | 2 sec | 10 | 30 sec |
| 3 | 4 sec | 11 | 60 sec |
| 4 | 6 sec | 12 | 120 sec |
| 5 | 8 sec | 13 | 180 sec |
| 6 | 10 sec | 14 | 300 sec |
| 7 | 15 sec | 15 | Always Ready |

**Standby Timer (4-bit code):**

| Code | Duration | Code | Duration |
|------|----------|------|----------|
| 0 | Not Allowed | 8 | 2 hours |
| 1 | 10 sec | 9 | 4 hours |
| 2 | 30 sec | 10 | 8 hours |
| 3 | 1 min | 11 | 12 hours |
| 4 | 5 min | 12 | 24 hours |
| 5 | 10 min | 13 | 48 hours |
| 6 | 30 min | 14 | 72 hours |
| 7 | 1 hour | 15 | Always Standby |

---

## 10. Parser Pseudocode

### 10.1 SNDCP PDU Extraction from LLC Logical Message

```
fn parse_sndcp_pdu(logical_message: &[u8], sap: u8, data_header_offset: u8)
    -> Result<SndcpPdu, ParseError>
{
    // Step 1: Determine if this is a context management PDU or data PDU
    // Context mgmt: SAP = SNDCP Packet Data Control, DHO = 0
    // Data payload: SAP = Unencrypted User Data, DHO = 2

    if logical_message.is_empty() {
        return Err(ParseError::EmptyMessage);
    }

    let byte0 = logical_message[0];
    let pdu_type = (byte0 >> 4) & 0x0F;

    match pdu_type {
        0 => parse_context_activation_request(logical_message),
        1 => parse_context_activation_accept(logical_message),
        2 => parse_context_activation_reject(logical_message),
        3 => parse_context_deactivation_request(logical_message),
        4 => parse_sn_data(logical_message),     // confirmed data
        5 => parse_sn_udata(logical_message),    // unconfirmed data
        6 => parse_context_activation_command(logical_message),
        _ => Err(ParseError::UnknownPduType(pdu_type)),
    }
}

fn parse_sn_data(data: &[u8]) -> Result<SndcpPdu, ParseError> {
    // Minimum 3 bytes: 2-byte SNDCP header + at least 1 byte payload
    if data.len() < 3 {
        return Err(ParseError::TooShort);
    }

    let pdu_type = (data[0] >> 4) & 0x0F;  // should be 4
    let nsapi    = data[0] & 0x0F;
    let pcomp    = (data[1] >> 4) & 0x0F;
    let dcomp    = data[1] & 0x0F;
    let network_pdu = &data[2..];

    if nsapi == 0 || nsapi == 15 {
        return Err(ParseError::InvalidNsapi(nsapi));
    }

    Ok(SndcpPdu::Data {
        nsapi,
        pcomp,
        dcomp,
        network_pdu: network_pdu.to_vec(),
    })
}

fn parse_sn_udata(data: &[u8]) -> Result<SndcpPdu, ParseError> {
    if data.len() < 3 {
        return Err(ParseError::TooShort);
    }

    let nsapi = data[0] & 0x0F;
    let pcomp = (data[1] >> 4) & 0x0F;  // Reserved (0) in V1/V2; active in V3
    let dcomp = data[1] & 0x0F;
    let network_pdu = &data[2..];

    Ok(SndcpPdu::UData {
        nsapi,
        pcomp,
        dcomp,
        network_pdu: network_pdu.to_vec(),
    })
}

fn parse_context_activation_request(data: &[u8]) -> Result<SndcpPdu, ParseError> {
    if data.len() < 2 {
        return Err(ParseError::TooShort);
    }

    let version = data[0] & 0x0F;
    let nsapi   = (data[1] >> 4) & 0x0F;
    let nat     = data[1] & 0x0F;

    match version {
        1 => {
            // V1: fixed 10 octets
            if data.len() < 10 { return Err(ParseError::TooShort); }
            let ipv4 = u32::from_be_bytes([data[2], data[3], data[4], data[5]]);
            let dsut = (data[6] >> 4) & 0x0F;
            let udpc = data[6] & 0x0F;
            let iphc = data[7];
            let tcpss = (data[8] >> 4) & 0x0F;
            let udpss = data[8] & 0x0F;
            let mdpco = data[9] & 0x0F;

            Ok(SndcpPdu::ContextActivationRequest {
                version, nsapi, nat,
                ipv4_address: Some(ipv4),
                dsut, udpc, mdpco,
                header_compression: HeaderCompNegotiation::V1V2 {
                    iphc, tcpss, udpss,
                },
                apn: None, pco: None, mso: None,
            })
        }
        2 => {
            // V2: 10 octets fixed + APN + PCO (variable)
            if data.len() < 10 { return Err(ParseError::TooShort); }
            let ipv4 = u32::from_be_bytes([data[2], data[3], data[4], data[5]]);
            let dsut = (data[6] >> 4) & 0x0F;
            let udpc = data[6] & 0x0F;
            let iphc = data[7];
            let tcpss = (data[8] >> 4) & 0x0F;
            let udpss = data[8] & 0x0F;
            let mdpco = data[9] & 0x0F;

            let mut offset = 10;
            let apn = parse_apn_field(data, &mut offset)?;
            let pco = parse_pco_field(data, &mut offset)?;

            Ok(SndcpPdu::ContextActivationRequest {
                version, nsapi, nat,
                ipv4_address: Some(ipv4),
                dsut, udpc, mdpco,
                header_compression: HeaderCompNegotiation::V1V2 {
                    iphc, tcpss, udpss,
                },
                apn, pco, mso: None,
            })
        }
        3 => {
            // V3: IPv4 conditional, expanded compression fields
            if data.len() < 2 { return Err(ParseError::TooShort); }
            let mut offset = 2;

            let ipv4_address = if nat == 0 {
                // NAT = Static: IPv4 present
                if data.len() < offset + 4 { return Err(ParseError::TooShort); }
                let ip = u32::from_be_bytes([
                    data[offset], data[offset+1], data[offset+2], data[offset+3]
                ]);
                offset += 4;
                Some(ip)
            } else {
                None
            };

            if data.len() < offset + 10 { return Err(ParseError::TooShort); }
            let dsut = (data[offset] >> 4) & 0x0F;
            let udpc = data[offset] & 0x0F;
            offset += 1;
            let hcomp = data[offset]; offset += 1;
            let vj_tcp_css = data[offset]; offset += 1;
            let tcp_css = data[offset]; offset += 1;
            let non_tcp_css = u16::from_be_bytes([data[offset], data[offset+1]]);
            offset += 2;
            let max_int_fhd = data[offset]; offset += 1;
            let max_tme_fhd = data[offset]; offset += 1;
            let max_hdr_sze = data[offset]; offset += 1;
            let mdpco = data[offset] & 0x0F; offset += 1;

            let apn = parse_apn_field(data, &mut offset)?;
            let pco = parse_pco_field(data, &mut offset)?;
            let mso = parse_mso_field(data, &mut offset)?;

            Ok(SndcpPdu::ContextActivationRequest {
                version, nsapi, nat,
                ipv4_address,
                dsut, udpc, mdpco,
                header_compression: HeaderCompNegotiation::V3 {
                    hcomp, vj_tcp_css, tcp_css, non_tcp_css,
                    max_int_fhd, max_tme_fhd, max_hdr_sze,
                },
                apn, pco, mso,
            })
        }
        _ => Err(ParseError::UnsupportedVersion(version)),
    }
}

// Helper: parse variable-length APN field
fn parse_apn_field(data: &[u8], offset: &mut usize) -> Result<Option<Apn>, ParseError> {
    if *offset >= data.len() { return Ok(None); }
    let apn_len = data[*offset] as usize;
    *offset += 1;
    if apn_len == 0 { return Ok(None); }
    if *offset + apn_len > data.len() { return Err(ParseError::TooShort); }
    let apn_data = data[*offset .. *offset + apn_len].to_vec();
    *offset += apn_len;
    if apn_len == 1 {
        Ok(Some(Apn::Index(apn_data[0])))
    } else {
        Ok(Some(Apn::String(String::from_utf8_lossy(&apn_data).to_string())))
    }
}

// Helper: parse variable-length PCO field
fn parse_pco_field(data: &[u8], offset: &mut usize) -> Result<Option<Vec<u8>>, ParseError> {
    if *offset >= data.len() { return Ok(None); }
    let pco_len = data[*offset] as usize;
    *offset += 1;
    if pco_len == 0 { return Ok(None); }
    if *offset + pco_len > data.len() { return Err(ParseError::TooShort); }
    let pco_data = data[*offset .. *offset + pco_len].to_vec();
    *offset += pco_len;
    Ok(Some(pco_data))
}

// Helper: parse variable-length MSO field (V3 only)
fn parse_mso_field(data: &[u8], offset: &mut usize) -> Result<Option<Mso>, ParseError> {
    if *offset >= data.len() { return Ok(None); }
    let mso_len = data[*offset] as usize;
    *offset += 1;
    if mso_len == 0 { return Ok(None); }
    if *offset + mso_len > data.len() { return Err(ParseError::TooShort); }
    let mfid = data[*offset];
    let mso_data = data[*offset + 1 .. *offset + mso_len].to_vec();
    *offset += mso_len;
    Ok(Some(Mso { mfid, data: mso_data }))
}
```

### 10.2 IP Packet Reassembly from SNDCP PDU

```
fn reassemble_ip_packet(
    sndcp_pdu: &SndcpPdu,
    context: &SndcpContext,
) -> Result<Vec<u8>, ReassemblyError>
{
    let (pcomp, dcomp, network_pdu) = match sndcp_pdu {
        SndcpPdu::Data { pcomp, dcomp, network_pdu, .. } => (*pcomp, *dcomp, network_pdu),
        SndcpPdu::UData { pcomp, dcomp, network_pdu, .. } => (*pcomp, *dcomp, network_pdu),
        _ => return Err(ReassemblyError::NotDataPdu),
    };

    // Step 1: Data decompression (DCOMP)
    // Currently no algorithms defined; DCOMP should always be 0
    if dcomp != 0 {
        return Err(ReassemblyError::UnsupportedDataCompression(dcomp));
    }
    let decompressed_payload = network_pdu;  // pass-through

    // Step 2: Header decompression (PCOMP)
    let ip_datagram = match pcomp {
        0 => {
            // No header compression -- payload IS the IP datagram
            decompressed_payload.clone()
        }
        1 => {
            // Van Jacobson compressed TCP/IP (RFC 1144)
            // Requires maintaining compression state per NSAPI
            vj_decompress_tcp(decompressed_payload, &mut context.vj_state)?
        }
        2 => {
            // Van Jacobson uncompressed TCP/IP (RFC 1144)
            // Update connection state from full header
            vj_decompress_uncompressed(decompressed_payload, &mut context.vj_state)?
        }
        3 => {
            // RFC 2507 FULL_HEADER (V3 only)
            rfc2507_full_header(decompressed_payload, &mut context.rfc2507_state)?
        }
        4 => {
            // RFC 2507 COMPRESSED_TCP (V3 only)
            rfc2507_compressed_tcp(decompressed_payload, &mut context.rfc2507_state)?
        }
        5 => {
            // RFC 2507 COMPRESSED_TCP_NODELTA (V3 only)
            rfc2507_compressed_tcp_nodelta(decompressed_payload, &mut context.rfc2507_state)?
        }
        6 => {
            // RFC 2507 COMPRESSED_NON_TCP (V3 only)
            rfc2507_compressed_non_tcp(decompressed_payload, &mut context.rfc2507_state)?
        }
        _ => {
            return Err(ReassemblyError::UnsupportedHeaderCompression(pcomp));
        }
    };

    // Step 3: Validate IP header
    if ip_datagram.len() < 20 {
        return Err(ReassemblyError::InvalidIpHeader);
    }
    let ip_version = (ip_datagram[0] >> 4) & 0x0F;
    if ip_version != 4 {
        return Err(ReassemblyError::NotIpv4(ip_version));
    }
    let total_length = u16::from_be_bytes([ip_datagram[2], ip_datagram[3]]) as usize;
    if total_length > ip_datagram.len() {
        return Err(ReassemblyError::TruncatedDatagram);
    }

    Ok(ip_datagram[..total_length].to_vec())
}
```

### 10.3 SCEP ARP Parser

```
fn parse_scep_arp(data: &[u8]) -> Result<ScepArp, ParseError> {
    if data.len() < 22 {
        return Err(ParseError::TooShort);
    }

    let hw_type = u16::from_be_bytes([data[0], data[1]]);
    if hw_type != 33 {  // ARP-TYPE_CAI
        return Err(ParseError::InvalidArpHwType(hw_type));
    }

    let proto_type = u16::from_be_bytes([data[2], data[3]]);
    let hw_len = data[4];   // should be 3 (LLID)
    let proto_len = data[5]; // should be 4 (IPv4)
    let opcode = u16::from_be_bytes([data[6], data[7]]);

    if hw_len != 3 || proto_len != 4 {
        return Err(ParseError::InvalidArpLengths);
    }

    let sender_llid = u32::from_be_bytes([0, data[8], data[9], data[10]]) & 0x00FFFFFF;
    let sender_ip = u32::from_be_bytes([data[11], data[12], data[13], data[14]]);
    let target_llid = u32::from_be_bytes([0, data[15], data[16], data[17]]) & 0x00FFFFFF;
    let target_ip = u32::from_be_bytes([data[18], data[19], data[20], data[21]]);

    Ok(ScepArp {
        opcode: match opcode {
            1 => ArpOpcode::Request,
            2 => ArpOpcode::Reply,
            _ => return Err(ParseError::InvalidArpOpcode(opcode)),
        },
        sender_llid,
        sender_ip: Ipv4Addr::from(sender_ip),
        target_llid,
        target_ip: Ipv4Addr::from(target_ip),
    })
}
```

### 10.4 Full Receive Path: RF to IP Datagram

```
fn process_received_pdu_frame(
    frame: &PduFrame,          // decoded from FDMA/TDMA PHY
    contexts: &mut ContextTable,
    arp_table: &mut ArpTable,
) -> Result<ReceivedPacket, ProcessError>
{
    // Step 1: LLC layer has already:
    //   - Validated CRC_HEADER
    //   - Qualified NAC, MFID, address
    //   - Reassembled data blocks into logical_message
    //   - Verified CRC_PACKET
    //   - Handled ARQ (if confirmed)
    let logical_message = &frame.logical_message;
    let sap = frame.header.sap;
    let dho = frame.header.data_header_offset;
    let llid = frame.header.llid;

    // Step 2: Route based on SAP
    match sap {
        SAP_ARP => {
            // SCEP ARP message
            let arp = parse_scep_arp(logical_message)?;
            arp_table.process(arp, llid);
            Ok(ReceivedPacket::ArpProcessed)
        }
        SAP_PACKET_DATA if dho == 0 => {
            // Either SCEP IP datagram (raw) or SNDCP context management
            if is_sndcp_context_sap(sap) {
                let pdu = parse_sndcp_pdu(logical_message, sap, dho)?;
                process_context_management(pdu, llid, contexts)?;
                Ok(ReceivedPacket::ContextManagement)
            } else {
                // SCEP: raw IP datagram
                Ok(ReceivedPacket::IpDatagram(logical_message.to_vec()))
            }
        }
        SAP_SNDCP_CONTROL if dho == 0 => {
            // SNDCP context management PDU
            let pdu = parse_sndcp_pdu(logical_message, sap, dho)?;
            process_context_management(pdu, llid, contexts)?;
            Ok(ReceivedPacket::ContextManagement)
        }
        SAP_USER_DATA if dho == 2 => {
            // SNDCP data PDU (SN-Data or SN-UData)
            let pdu = parse_sndcp_pdu(logical_message, sap, dho)?;
            let nsapi = pdu.nsapi();
            let context = contexts.lookup(llid, nsapi)?;
            let ip_datagram = reassemble_ip_packet(&pdu, context)?;
            Ok(ReceivedPacket::IpDatagram(ip_datagram))
        }
        _ => Err(ProcessError::UnknownSap(sap)),
    }
}
```

---

## 11. Rust Struct Definitions

```rust
use std::net::Ipv4Addr;

// ─── SNDCP PDU Types ───────────────────────────────────────────────

/// SNDCP PDU Type codes (4 bits)
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SndcpPduType {
    ContextActivationRequest = 0,
    ContextActivationAccept  = 1,
    ContextActivationReject  = 2,
    ContextDeactivationRequest = 3,
    Data  = 4,
    UData = 5,
    ContextActivationCommand = 6,
}

/// SNDCP Version (4 bits)
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SndcpVersion {
    V1 = 1,
    V2 = 2,
    V3 = 3,
    WaiV1 = 15,
}

// ─── Header Compression ────────────────────────────────────────────

/// PCOMP values: header compression type applied to data PDU payload
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Pcomp {
    None                    = 0,
    VjCompressedTcp         = 1,  // RFC 1144 compressed
    VjUncompressedTcp       = 2,  // RFC 1144 uncompressed
    Rfc2507FullHeader       = 3,  // V3 only
    Rfc2507CompressedTcp    = 4,  // V3 only
    Rfc2507CompTcpNodelta   = 5,  // V3 only
    Rfc2507CompressedNonTcp = 6,  // V3 only
}

/// Header compression negotiation parameters (version-dependent)
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum HeaderCompNegotiation {
    /// V1/V2: IPHC bitmask + VJ slots + (unused) UDP slots
    V1V2 {
        iphc: u8,       // bit 0: VJ TCP/IP enabled
        tcpss: u8,      // 0-15 → 1-16 compression state slots
        udpss: u8,      // reserved, always 0
    },
    /// V3: expanded HCOMP bitmask + per-algorithm slots + timing
    V3 {
        hcomp: u8,          // bit 0: VJ, bit 1: RFC 2507
        vj_tcp_css: u8,     // VJ TCP compression state slots (0-255)
        tcp_css: u8,        // RFC 2507 TCP slots (0-255)
        non_tcp_css: u16,   // RFC 2507 non-TCP slots (0-65535)
        max_int_fhd: u8,    // 0=disabled, else 16*value = max interval
        max_tme_fhd: u8,    // 0=disabled, else 5sec*value = max time
        max_hdr_sze: u8,    // 60-255 = max header size in octets
    },
}

// ─── Context Management Fields ─────────────────────────────────────

/// Network Address Type (4 bits)
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum NetworkAddressType {
    Ipv4Static  = 0,
    Ipv4Dynamic = 1,
    NoAddress   = 15,
}

/// Data Subscriber Unit Type (4 bits)
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DataSubscriberUnitType {
    TrunkedDataOnly            = 0,
    AlternatingTrunkedVoiceData = 1,
    ConventionalDataOnly       = 2,
    AlternatingConvVoiceData   = 3,
    TrunkedAndConvDataOnly     = 4,
    AlternatingTrunkedConvVD   = 5,
}

/// MTU negotiated values (4 bits)
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MtuCode {
    Bytes296  = 1,
    Bytes510  = 2,
    Bytes1020 = 3,
    Bytes1500 = 4,
}

impl MtuCode {
    pub fn to_bytes(self) -> usize {
        match self {
            MtuCode::Bytes296  => 296,
            MtuCode::Bytes510  => 510,
            MtuCode::Bytes1020 => 1020,
            MtuCode::Bytes1500 => 1500,
        }
    }
}

/// Access Point Name (variable)
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Apn {
    Index(u8),
    String(String),
}

/// Manufacturer Specific Options (V3)
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Mso {
    pub mfid: u8,
    pub data: Vec<u8>,
}

/// SNDCP Reject codes (8 bits)
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RejectCode {
    AnyReason                     = 0,
    SuNotProvisioned              = 1,
    DsutNotSupported              = 2,
    MaxContextsExceeded           = 3,
    VersionNotSupported           = 4,
    PdsNotSupportedSite           = 5,
    PdsNotSupportedSystem         = 6,
    StaticAddressNotCorrect       = 7,
    StaticAllocNotSupported       = 8,
    StaticAddressInUse            = 9,
    Ipv4NotSupported              = 10,
    DynamicPoolEmpty              = 11,
    DynamicAllocNotSupported      = 12,
    TemporaryRejection            = 14,  // V3
    AuthenticationFailed          = 20,  // V3
    RejectedByExternalNetwork     = 21,  // V3
    ApnIncorrect                  = 22,  // V3
    VersionNotSupportedWithField  = 23,  // V3: Version Supported included
}

/// Deactivation Type (8 bits)
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DeactivationType {
    DeactivateAll      = 0,
    DeactivateThisNsapi = 1,
}

// ─── SNDCP PDU Structures ──────────────────────────────────────────

/// Parsed SNDCP PDU (all types)
#[derive(Debug, Clone, PartialEq)]
pub enum SndcpPdu {
    ContextActivationRequest {
        version: u8,
        nsapi: u8,
        nat: u8,
        ipv4_address: Option<u32>,
        dsut: u8,
        udpc: u8,
        mdpco: u8,
        header_compression: HeaderCompNegotiation,
        apn: Option<Apn>,
        pco: Option<Vec<u8>>,
        mso: Option<Mso>,
    },
    ContextActivationAccept {
        nsapi: u8,
        pdupm: u8,
        ready_timer: u8,
        standby_timer: u8,
        nat: u8,
        ipv4_address: Option<u32>,
        header_compression: HeaderCompNegotiation,
        mtu: u8,
        udpc: u8,
        mdpco: u8,
        dac: u16,
        apn: Option<Apn>,
        pco: Option<Vec<u8>>,
        mso: Option<Mso>,
    },
    ContextActivationReject {
        nsapi: u8,
        reject_code: u8,
        version_supported: Option<u16>,  // V3 only, when reject_code == 23
    },
    ContextDeactivationRequest {
        nsapi: u8,
        deactivation_type: u8,
    },
    Data {
        nsapi: u8,
        pcomp: u8,
        dcomp: u8,
        network_pdu: Vec<u8>,
    },
    UData {
        nsapi: u8,
        pcomp: u8,
        dcomp: u8,
        network_pdu: Vec<u8>,
    },
    ContextActivationCommand {
        version: u8,
        nsapi: u8,
        nat: u8,
        apn: Option<Apn>,
        ipv4_address: Option<u32>,
    },
}

impl SndcpPdu {
    pub fn nsapi(&self) -> u8 {
        match self {
            SndcpPdu::ContextActivationRequest { nsapi, .. } => *nsapi,
            SndcpPdu::ContextActivationAccept { nsapi, .. } => *nsapi,
            SndcpPdu::ContextActivationReject { nsapi, .. } => *nsapi,
            SndcpPdu::ContextDeactivationRequest { nsapi, .. } => *nsapi,
            SndcpPdu::Data { nsapi, .. } => *nsapi,
            SndcpPdu::UData { nsapi, .. } => *nsapi,
            SndcpPdu::ContextActivationCommand { nsapi, .. } => *nsapi,
        }
    }

    pub fn pdu_type(&self) -> SndcpPduType {
        match self {
            SndcpPdu::ContextActivationRequest { .. } => SndcpPduType::ContextActivationRequest,
            SndcpPdu::ContextActivationAccept { .. } => SndcpPduType::ContextActivationAccept,
            SndcpPdu::ContextActivationReject { .. } => SndcpPduType::ContextActivationReject,
            SndcpPdu::ContextDeactivationRequest { .. } => SndcpPduType::ContextDeactivationRequest,
            SndcpPdu::Data { .. } => SndcpPduType::Data,
            SndcpPdu::UData { .. } => SndcpPduType::UData,
            SndcpPdu::ContextActivationCommand { .. } => SndcpPduType::ContextActivationCommand,
        }
    }
}

// ─── SCEP ARP ──────────────────────────────────────────────────────

/// ARP opcode for SCEP
#[repr(u16)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ArpOpcode {
    Request = 1,
    Reply   = 2,
}

/// Parsed SCEP ARP message
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ScepArp {
    pub opcode: ArpOpcode,
    pub sender_llid: u32,      // 24-bit LLID (stored in low 24 bits)
    pub sender_ip: Ipv4Addr,
    pub target_llid: u32,      // 24-bit LLID
    pub target_ip: Ipv4Addr,
}

/// ARP-TYPE_CAI hardware address type for P25
pub const ARP_TYPE_CAI: u16 = 33;

// ─── SNDCP Context State ───────────────────────────────────────────

/// SNDCP context: active logical association between SU and FNE
#[derive(Debug, Clone)]
pub struct SndcpContext {
    pub llid: u32,              // 24-bit subscriber LLID
    pub nsapi: u8,              // 1-14
    pub version: SndcpVersion,
    pub nat: NetworkAddressType,
    pub ipv4_address: Option<Ipv4Addr>,
    pub dsut: DataSubscriberUnitType,
    pub mtu: usize,             // negotiated MTU in bytes
    pub pdupm: u8,              // PDU priority maximum (0-15)
    pub dac: u16,               // Data Access Control (TDS only)

    // Header compression state
    pub hcomp_config: HeaderCompNegotiation,
    pub vj_state: Option<VjCompressionState>,
    pub rfc2507_state: Option<Rfc2507State>,

    // Timers
    pub ready_timer_code: u8,
    pub standby_timer_code: u8,

    // Optional features
    pub apn: Option<Apn>,
    pub scan_mode: bool,        // CDS only
    pub mdpco_interface: u8,    // SU interface info (3 bits)
}

/// Van Jacobson compression state (per-context)
#[derive(Debug, Clone)]
pub struct VjCompressionState {
    pub slots: Vec<VjSlot>,
    pub last_connection_id: u8,
}

/// Single VJ compression slot
#[derive(Debug, Clone, Default)]
pub struct VjSlot {
    pub active: bool,
    pub ip_header: [u8; 20],   // cached IP header
    pub tcp_header: [u8; 20],  // cached TCP header
}

/// RFC 2507 compression state (per-context, V3 only)
#[derive(Debug, Clone)]
pub struct Rfc2507State {
    pub tcp_slots: Vec<Rfc2507TcpSlot>,
    pub non_tcp_slots: Vec<Rfc2507NonTcpSlot>,
    pub max_context_id: u8,    // default 15
}

#[derive(Debug, Clone, Default)]
pub struct Rfc2507TcpSlot {
    pub active: bool,
    pub full_header: Vec<u8>,
}

#[derive(Debug, Clone, Default)]
pub struct Rfc2507NonTcpSlot {
    pub active: bool,
    pub full_header: Vec<u8>,
}

// ─── PDS State Machine ─────────────────────────────────────────────

/// Packet Data Service state (TDS or CDS)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PdsState {
    Closed,
    Idle,
    ReadyStar,      // Ready* (first context activation pending)
    Standby,
    Ready,
    ReadyRoaming,   // TDS only
}

/// Data service mode
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DataServiceMode {
    Tds,  // Trunked Data Service
    Cds,  // Conventional Data Service
}

/// PDCH access mode (TDS only)
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PdchAccessMode {
    RequestedAccess,   // RA: SN-DATA_CHN_REQ/GNT exchange
    AutonomousAccess,  // AA: SU occupies PDCH directly
}

/// Delivery mode selection
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DeliveryMode {
    Confirmed,     // SN-Data (PDU Type 4), LLC confirmed with ARQ
    Unconfirmed,   // SN-UData (PDU Type 5), LLC unconfirmed
}

// ─── Context Table ─────────────────────────────────────────────────

/// Table of active SNDCP contexts (up to 14 per SU)
#[derive(Debug, Clone)]
pub struct ContextTable {
    pub contexts: Vec<SndcpContext>,
    pub state: PdsState,
    pub mode: DataServiceMode,
}

impl ContextTable {
    pub fn new(mode: DataServiceMode) -> Self {
        Self {
            contexts: Vec::new(),
            state: PdsState::Closed,
            mode,
        }
    }

    /// Look up active context by LLID + NSAPI
    pub fn lookup(&self, llid: u32, nsapi: u8) -> Option<&SndcpContext> {
        self.contexts.iter().find(|c| c.llid == llid && c.nsapi == nsapi)
    }

    /// Look up active context by IPv4 address (for outbound routing)
    pub fn lookup_by_ip(&self, ip: Ipv4Addr) -> Option<&SndcpContext> {
        self.contexts.iter().find(|c| c.ipv4_address == Some(ip))
    }

    /// Number of active contexts
    pub fn active_count(&self) -> usize {
        self.contexts.len()
    }

    /// Maximum concurrent contexts per SU
    pub const MAX_CONTEXTS: usize = 14;
}

// ─── SCEP IP Address Binding Table ─────────────────────────────────

/// IP-to-LLID binding entry (SCEP ARP or provisioned)
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct IpBinding {
    pub ip_address: Ipv4Addr,
    pub llid: u32,  // 24-bit
}

/// ARP table for SCEP configurations
#[derive(Debug, Clone)]
pub struct ArpTable {
    pub bindings: Vec<IpBinding>,
}
```

---

## 12. Implementation Notes

### 12.1 SDRTrunk / OP25 Observations

Open-source P25 decoders (SDRTrunk, OP25) implement partial SNDCP parsing:

- **SDRTrunk** (Java): Decodes SNDCP context management PDUs for display; recognizes
  SN-DATA_CHN_GNT and SN-DATA_PAGE_REQ in trunking control channel messages. Data PDU
  payloads are typically displayed as hex but not fully reconstructed into IP datagrams.
  SDRTrunk maps LLID to subscriber aliases in its alias list, providing the IP-to-subscriber
  correlation.

- **OP25** (Python/C++): Implements PDU frame decoding and can extract logical messages
  from confirmed/unconfirmed data packets. SNDCP header parsing is present for the
  2-byte data PDU header. Full IP reassembly from multi-block confirmed transfers requires
  the LLC ARQ state machine which OP25 implements partially.

### 12.2 Key Implementation Pitfalls

1. **V3 conditional IPv4 field:** The IPv4 Address is NOT always present in V3 PDUs.
   For Activation Request, it is present only when NAT = Static (0). For Activation
   Accept, only when NAT = Dynamic (1). All subsequent field offsets shift by 4 bytes
   depending on presence. This is the most common parsing bug.

2. **Unconfirmed MTU limit:** Even if MTU is negotiated to 1500, unconfirmed datagrams
   are hard-limited to min(510, MTU). Do not attempt to send >510-byte SN-UData.

3. **PCOMP in SN-UData V1/V2:** The PCOMP nibble is RESERVED (must be 0) in SN-UData
   for V1 and V2. Only V3 enables header compression on unconfirmed datagrams.

4. **NSAPI 0 and 15:** NSAPI 0 is control signaling only (not a user context).
   NSAPI 15 is reserved. Valid user NSAPIs are 1-14 only.

5. **Ready Roaming (TDS only):** CDS has no roaming mechanism. Attempting to use
   SN-REC_REQ on a conventional channel is invalid.

6. **DAC updates:** In V3 TDS, the FNE can send an unsolicited SN-Context Activation
   Accept to update the DAC field. Receivers must handle Accept PDUs even when no
   Request was sent. Only Ready, Standby, and DAC fields may change (see Annex A).

7. **ARP only for Direct/Repeated:** SCEP ARP is used ONLY in Direct Data and
   Repeated Data configurations. SNDCP uses context activation for IP binding, NOT ARP.

### 12.3 Cross-Reference Index

| Topic | Primary Spec | Sections |
|-------|-------------|----------|
| PDU frame format (FDMA) | BAAA-B | DUID 0xC, trellis coding, NID |
| Data burst (TDMA) | BBAC-A | DCH burst structure, per-slot FEC |
| LLC fragmentation/ARQ | BAED-A | Confirmed/unconfirmed packets, CRC |
| SAP values, LLID ranges | BAAC-D | Packet Data SAP, ARP SAP, broadcast LLID |
| Trunking channel grant messages | AABC-E | SN-DATA_CHN_REQ/GNT, SN-DATA_PAGE |
| Link control (call termination) | AABF-D | LC_CALL_TRM_CAN |
| Data overview / configurations | BAEA-C | Four configs, endpoint models, SAP model |
| CMS scan mode | BAEJ-A | Scan preamble, packet data registration |
| Trunking procedures | AABD-B | Registration, LLID allocation |
| Block encryption | AAAD-B | Encrypted PDU auxiliary headers |
