# TIA-102.BBAD-A — Part 3: MAC Messages (A), Pages 37–80

---

Multiple MAC messages may be conveyed within any given MAC_IDLE, MAC_ACTIVE, or MAC_HANGTIME PDU as message size allows, with any unused space within the MAC PDU Contents being filled using the Null Information message.

Table 2 below lists all MAC Messages that may be conveyed on the VCH by category, indicates whether they are inbound or outbound signaling messages, and provides a reference to the subclause where the message is defined. For messages with multiple formats (e.g. abbreviated/extended), all formats are permitted unless a specific format is specified in Table 2 below.

## Table 2 – VCH MAC Messages

| Category | Name | Inbound | Outbound | Clause |
|---|---|---|---|---|
| Voice Service | Group Voice Service Request | X | | 3.25 |
| Voice Service | Unit to Unit Voice Service Request | X | | 3.5 |
| Voice Service | Group Voice Channel Grant | | X | 3.7 |
| Voice Service | Group Voice Channel Grant Update | | X | 3.8 |
| Voice Service | Unit to Unit Answer Request | | X | 3.10 |
| Voice Service | Unit to Unit Voice Service Channel Grant - Abbreviated | | X | 3.9 |
| Voice Service | Unit to Unit Voice Service Channel Grant - Extended VCH | | X | 3.9 |
| Voice Service | Telephone Interconnect Answer Request | | X | 3.12 |
| Voice Service | Unit to Unit Voice Channel Grant Update - Abbreviated | | X | 3.13 |
| Voice Service | Unit to Unit Voice Channel Grant Update - Extended VCH | | X | 3.13 |
| Data Service | SNDCP Data Channel Grant | | X | 3.15 |
| Data Service | SNDCP Data Page Request | | X | 3.16 |
| Data Service | SNDCP Data Channel Announcement - Explicit | | X | 3.17 |
| Control and Status Service | Acknowledge Response FNE - Abbreviated | | X | 3.14 |
| Control and Status Service | Adjacent Status Broadcast - Implicit | | X | 3.18 |
| Control and Status Service | Adjacent Status Broadcast - Explicit | | X | 3.18 |
| Control and Status Service | Call Alert - Abbreviated | | X | 3.19 |
| Control and Status Service | Call Alert - Extended VCH | | X | 3.19 |
| Control and Status Service | Deny Response | | X | 3.30 |
| Control and Status Service | Extended Function Command - Abbreviated | | X | 3.20 |
| Control and Status Service | Extended Function Command - Extended VCH | | X | 3.20 |
| Control and Status Service | Group Affiliation Query | | X | 3.21 |
| Control and Status Service | Identifier Update | | X | 3.22 |
| Control and Status Service | Message Update - Abbreviated | | X | 3.42 |
| Control and Status Service | Message Update - Extended VCH | | X | 3.42 |
| Control and Status Service | Network Status Broadcast | | X | 3.24 |
| Control and Status Service | Queued Response | | X | 3.29 |
| Control and Status Service | RFSS Status Broadcast | | X | 3.26 |
| Control and Status Service | Secondary Control Channel Broadcast | | X | 3.27 |
| Control and Status Service | Status Query - Abbreviated | | X | 3.28 |
| Control and Status Service | Status Query - Extended VCH | | X | 3.28 |
| Control and Status Service | Status Update - Abbreviated | | X | 3.41 |
| Control and Status Service | Status Update - Extended VCH | | X | 3.41 |
| Control and Status Service | System Service Broadcast | | X | 3.31 |
| Control and Status Service | Unit Registration Command | | X | 3.32 |
| Control and Status Service | Radio Unit Monitor Command - Abbreviated | | X | 3.43 |
| Control and Status Service | Radio Unit Monitor Command - Extended VCH | | X | 3.43 |
| Control and Status Service | Time and Date Announcement | | X | 3.23 |
| Control and Status Service | Identifier Update for VHF/UHF Bands | | X | 3.34 |
| Control and Status Service | Identifier Update for TDMA - Abbreviated | | X | 3.35 |
| Control and Status Service | Radio Unit Monitor Enhanced Command - Abbreviated | | X | 3.11 |
| MAC Specific | Null Information | X | X | 3.1 |
| MAC Specific | Manufacturer Specific | X | X | 3.36 |
| MAC Specific | Group Voice Channel User | X | X | 3.2 |
| MAC Specific | Unit to Unit Voice Channel User | X | X | 3.3 |
| MAC Specific | Telephone Interconnect Voice Channel User | X | X | 3.4 |
| MAC Specific | Group Voice Channel Grant Update Multiple | | X | 3.6 |
| MAC Specific | Individual Paging with Priority | | X | 3.37 |
| MAC Specific | Indirect Group Paging without Priority | | X | 3.38 |
| MAC Specific | Power Control Signal Quality | | X | 3.39 |
| MAC Specific | MAC_Release | | X | 3.40 |
| Dynamic Regrouping | MFID90 Group Regroup Voice Channel User | X | X | 3.86 |
| Dynamic Regrouping | MFID90 Group Regroup Voice Channel Update | | X | 3.87 |
| Dynamic Regrouping | MFID90 Extended Function Command | | X | 3.88 |

---

## 3. MAC Messages

The clauses that follow specify the format of all MAC messages. All fields specified in these messages are defined in section 4 below.

In general, the MCO structure was constructed to re-use the ISP/OSP opcodes defined in [2] for the abbreviated messages while maintaining an ability to extend this set for new functionality. This was done by partitioning the opcode space into four different sets, defined by the B1 and B2 fields. This implies that the 6-bit MCO may be re-used within the different partitions and that the B1 and B2 fields are examined in conjunction with the MCO to determine the actual message. The following list describes the four partitions:

- One group contains messages that were created specifically for the TDMA CAI that have no counterpart in the FDMA CAI OSP/ISP messages.
- One group contains messages derived from abbreviated format of the FDMA CAI OSP/ISP messages.
- One group contains any manufacturer specific signaling messages.
- One group contains messages derived from extended or explicit format of the FDMA CAI OSP/ISP messages.

This is summarized in Table 3 below:

### Table 3 – MCO Partitioning

| B1 | B2 | Partition Description |
|---|---|---|
| 0 | 0 | Unique TDMA CAI message |
| 0 | 1 | Derived from FDMA CAI OSP/ISP abbreviated format |
| 1 | 0 | Manufacturer specific message |
| 1 | 1 | Derived from FDMA CAI OSP/ISP extended or explicit format |

MAC Message lengths are determined in one of four ways:

- The message length is inherent based on the MCO (see Table 4 below)
- The message length is variable and the length is determined by parsing out additional fields in the message such as in the following messages:
  - Individual Paging with Priority
  - Indirect Group Paging without Priority
- The message length is variable and it fills out the remaining space in the MAC PDU such as in the following message:
  - Null Information
- A specific length octet is included in the message that contains the message length. The message length is always contained in the least significant 6 bits of the 2nd octet in the message, that is, the octet that immediately follows the B1/B2/MCO octet.

In order to avoid future compatibility problems for older SUs with new messages, all new messages added after the initial publication of [5] shall contain a length octet as discussed in the fourth major bullet point above. This allows older SUs to skip over messages in the same outbound MAC PDU that may be defined in future releases of the TDMA CAI.

There are a few MAC messages that are too large to be conveyed in a single PDU. These are referred to as multi-fragment messages and are specified when necessary in the MAC message details that follow. Multi-fragment messages are only available on the LCCH. The fragments of a multi-fragment message shall be transmitted in consecutive bursts if a dual LCCH is employed and in consecutive logical channel (LCH 0 or LCH 1) bursts if a single LCCH is employed. The MAC fields associated with multi-fragment messages are defined in section 4 below.

The Null Information message or Null Avoid Zero Bias Information message, if included, shall always be the last message within the MAC PDU. Any Manufacturer Specific messages shall be inserted after the standard defined messages and before the Null Information message or the Null Avoid Zero Bias Information message.

### Table 4 – MAC Message Lengths

| B1 | B2 | MCO | Length | Name | Clause |
|---|---|---|---|---|---|
| 0 | 0 | %000000 | Variable | Null Information | 3.1 |
| 0 | 0 | %000001 | 7 | Group Voice Channel User - Abbreviated | 3.2 |
| 0 | 0 | %100001 | 14 | Group Voice Channel User - Extended | 3.2 |
| 0 | 0 | %000010 | 8 | Unit to Unit Voice Channel User - Abbreviated | 3.3 |
| 0 | 0 | %100010 | 15 | Unit to Unit Voice Channel User - Extended | 3.3 |
| 0 | 0 | %000011 | 7 | Telephone Interconnect Voice Channel User | 3.4 |
| 0 | 1 | %000100 | 8 | Unit to Unit Voice Service Request - Abbreviated | 3.5 |
| 1 | 1 | %000100 | 12 | Unit to Unit Voice Service Request - Extended | 3.5 |
| 0 | 0 | %000101 | 16 | Group Voice Channel Grant Update Multiple - Implicit | 3.6 |
| 0 | 0 | %100101 | 15 | Group Voice Channel Grant Update Multiple - Explicit | 3.6 |
| 0 | 1 | %000000 | 9 | Group Voice Channel Grant - Implicit | 3.7 |
| 1 | 1 | %000000 | 11 | Group Voice Channel Grant - Explicit | 3.7 |
| 0 | 1 | %000010 | 9 | Group Voice Channel Grant Update - Implicit | 3.8 |
| 1 | 1 | %000011 | 8 | Group Voice Channel Grant Update - Explicit | 3.8 |
| 0 | 1 | %000100 | 9 | Unit to Unit Voice Service Channel Grant - Abbreviated | 3.9 |
| 1 | 1 | %000100 | 15 | Unit to Unit Voice Service Channel Grant - Extended VCH | 3.9 |
| 1 | 1 | %001111 | 32 | Unit to Unit Voice Service Channel Grant - Extended LCCH | 3.9 |
| 0 | 1 | %000101 | 8 | Unit to Unit Answer Request - Abbreviated | 3.10 |
| 1 | 1 | %000101 | 12 | Unit to Unit Answer Request - Extended | 3.10 |
| 0 | 1 | %011110 | 14 | Radio Unit Monitor Enhanced Command - Abbreviated | 3.11 |
| 1 | 1 | %011110 | 40 | Radio Unit Monitor Enhanced Command - Extended | 3.11 |
| 0 | 1 | %001010 | 9 | Telephone Interconnect Answer Request | 3.12 |
| 0 | 1 | %000110 | 9 | Unit to Unit Voice Channel Grant Update - Abbreviated | 3.13 |
| 1 | 1 | %000110 | 15 | Unit to Unit Voice Channel Grant Update - Extended VCH | 3.13 |
| 1 | 1 | %000111 | 32 | Unit to Unit Voice Channel Grant Update - Extended LCCH | 3.13 |
| 0 | 1 | %100000 | 9 | Acknowledge Response FNE - Abbreviated | 3.14 |
| 1 | 1 | %100000 | 28 | Acknowledge Response FNE - Extended | 3.14 |
| 0 | 1 | %010100 | 9 | SNDCP Data Channel Grant | 3.15 |
| 0 | 1 | %010101 | 7 | SNDCP Data Page Request | 3.16 |
| 1 | 1 | %010110 | 9 | SNDCP Data Channel Announcement - Explicit | 3.17 |
| 0 | 1 | %111100 | 9 | Adjacent Status Broadcast - Implicit | 3.18 |
| 1 | 1 | %111100 | 11 | Adjacent Status Broadcast - Explicit | 3.18 |
| 1 | 1 | %111110 | 15 | Adjacent Status Broadcast - Extended - Explicit | 3.18 |
| 0 | 1 | %011111 | 7 | Call Alert - Abbreviated | 3.19 |
| 1 | 1 | %011111 | 11 | Call Alert - Extended VCH | 3.19 |
| 1 | 1 | %001011 | 27 | Call Alert - Extended LCCH | 3.19 |
| 0 | 1 | %100100 | 9 | Extended Function Command - Abbreviated | 3.20 |
| 1 | 1 | %100100 | 17 | Extended Function Command - Extended VCH | 3.20 |
| 1 | 1 | %100101 | 14 | Extended Function Command - Extended LCCH | 3.20 |
| 0 | 1 | %101010 | 7 | Group Affiliation Query - Abbreviated | 3.21 |
| 1 | 1 | %101010 | 11 | Group Affiliation Query - Extended | 3.21 |
| 0 | 1 | %111101 | 9 | Identifier Update | 3.22 |
| 0 | 1 | %110101 | 9 | Time and Date Announcement | 3.23 |
| 0 | 1 | %111011 | 11 | Network Status Broadcast - Implicit | 3.24 |
| 1 | 1 | %111011 | 13 | Network Status Broadcast - Explicit | 3.24 |
| 0 | 1 | %000001 | 7 | Group Voice Service Request | 3.25 |
| 0 | 1 | %111010 | 9 | RFSS Status Broadcast - Implicit | 3.26 |
| 1 | 1 | %111010 | 11 | RFSS Status Broadcast - Explicit | 3.26 |
| 0 | 1 | %111001 | 9 | Secondary Control Channel Broadcast - Implicit | 3.27 |
| 1 | 1 | %101001 | 8 | Secondary Control Channel Broadcast - Explicit | 3.27 |
| 0 | 1 | %011010 | 7 | Status Query - Abbreviated | 3.28 |
| 1 | 1 | %011010 | 11 | Status Query - Extended VCH | 3.28 |
| 1 | 1 | %011011 | 27 | Status Query - Extended LCCH | 3.28 |
| 0 | 1 | %100001 | 9 | Queued Response | 3.29 |
| 0 | 1 | %100111 | 9 | Deny Response | 3.30 |
| 0 | 1 | %111000 | 9 | System Service Broadcast | 3.31 |
| 0 | 1 | %101101 | 7 | Unit Registration Command | 3.32 |
| 0 | 1 | %011101 | 8 | Radio Unit Monitor Command - Obsolete | 3.33 |
| 0 | 1 | %110100 | 9 | Identifier Update for VHF/UHF Bands | 3.34 |
| 0 | 1 | %110011 | 9 | Identifier Update for TDMA - Abbreviated | 3.35 |
| 1 | 1 | %110011 | 14 | Identifier Update for TDMA - Extended | 3.35 |
| 1 | 0 | Undefined | Variable | Manufacturer Specific | 3.36 |
| 0 | 0 | %010010 | Variable | Individual Paging with Priority | 3.37 |
| 0 | 0 | %010001 | Variable | Indirect Group Paging without Priority | 3.38 |
| 0 | 0 | %110000 | 5 | Power Control Signal Quality | 3.39 |
| 0 | 0 | %110001 | 7 | MAC_Release | 3.40 |
| 0 | 1 | %011000 | 10 | Status Update - Abbreviated | 3.41 |
| 1 | 1 | %011000 | 14 | Status Update - Extended VCH | 3.41 |
| 1 | 1 | %011001 | 29 | Status Update - Extended LCCH | 3.41 |
| 0 | 1 | %011100 | 10 | Message Update - Abbreviated | 3.42 |
| 1 | 1 | %011100 | 14 | Message Update - Extended VCH | 3.42 |
| 1 | 1 | %001110 | 29 | Message Update - Extended LCCH | 3.42 |
| 0 | 1 | %001100 | 10 | Radio Unit Monitor Command - Abbreviated | 3.43 |
| 1 | 1 | %001100 | 14 | Radio Unit Monitor Command - Extended VCH | 3.43 |
| 1 | 1 | %001101 | 29 | Radio Unit Monitor Command - Extended LCCH | 3.43 |
| 0 | 1 | %001000 | 10 | Telephone Interconnect Voice Channel Grant - Implicit | 3.44 |
| 1 | 1 | %001000 | 12 | Telephone Interconnect Voice Channel Grant - Explicit | 3.44 |
| 0 | 1 | %001001 | 10 | Telephone Interconnect Voice Channel Grant Update - Implicit | 3.45 |
| 1 | 1 | %001001 | 12 | Telephone Interconnect Voice Channel Grant Update - Explicit | 3.45 |
| 0 | 1 | %101000 | 10 | Group Affiliation Response - Abbreviated | 3.46 |
| 1 | 1 | %101000 | 16 | Group Affiliation Response - Extended | 3.46 |
| 0 | 1 | %101100 | 10 | Unit Registration Response - Abbreviated | 3.47 |
| 1 | 1 | %101100 | 13 | Unit Registration Response - Extended | 3.47 |
| 0 | 1 | %101111 | 9 | Unit Deregistration Acknowledge | 3.48 |
| 0 | 1 | %101011 | 10 | Location Registration Response | 3.49 |
| 0 | 1 | %110110 | 10 | Roaming Address Command | 3.50 |
| 0 | 1 | %110111 | 13 | Roaming Address Update | 3.51 |
| 0 | 1 | %110001 | 29 | Authentication Demand | 3.52 |
| 0 | 1 | %110010 | 9 | Authentication FNE Response - Abbreviated | 3.53 |
| 1 | 1 | %110010 | 16 | Authentication FNE Response - Extended | 3.53 |
| 0 | 1 | %110000 | 9 | Synchronization Broadcast | 3.54 |
| 0 | 1 | %000101 | 10 | Unit to Unit Voice Service Answer Response - Abbreviated | 3.55 |
| 1 | 1 | %000101 | 14 | Unit to Unit Voice Service Answer Response - Extended | 3.55 |
| 0 | 1 | %001000 | Variable | Telephone Interconnect Request - Explicit Dialing | 3.56 |
| 0 | 1 | %001001 | 7 | Telephone Interconnect Request - Implicit Dialing | 3.57 |
| 0 | 1 | %001010 | 7 | Telephone Interconnect Answer Response | 3.58 |
| 0 | 1 | %010010 | 8 | SNDCP Data Channel Request | 3.59 |
| 0 | 1 | %010011 | 9 | SNDCP Data Page Response | 3.60 |
| 0 | 1 | %010100 | 9 | SNDCP Reconnect Request | 3.61 |
| 0 | 1 | %100000 | 9 | Acknowledge Response Unit - Abbreviated | 3.62 |
| 1 | 1 | %100000 | 13 | Acknowledge Response Unit - Extended | 3.62 |
| 0 | 1 | %011111 | 8 | Call Alert Request - Abbreviated | 3.63 |
| 1 | 1 | %011111 | 12 | Call Alert Request - Extended | 3.63 |
| 0 | 1 | %100011 | 10 | Cancel Service Request - Abbreviated | 3.64 |
| 1 | 1 | %100011 | 14 | Cancel Service Request - Extended | 3.64 |
| 0 | 1 | %100111 | 9 | Emergency Alarm Request | 3.65 |
| 0 | 1 | %100100 | 9 | Extended Function Response - Abbreviated | 3.66 |
| 1 | 1 | %100100 | 14 | Extended Function Response - Extended | 3.66 |
| 0 | 1 | %101010 | 10 | Group Affiliation Query Response | 3.67 |
| 0 | 1 | %101000 | 9 | Group Affiliation Request - Abbreviated | 3.68 |
| 1 | 1 | %101000 | 12 | Group Affiliation Request - Extended | 3.68 |
| 0 | 1 | %110010 | 6 | Identifier Update Request | 3.69 |
| 0 | 1 | %011100 | 10 | Message Update Request - Abbreviated | 3.70 |
| 1 | 1 | %011100 | 14 | Message Update Request - Extended | 3.70 |
| 0 | 1 | %011010 | 8 | Status Query Request - Abbreviated | 3.71 |
| 1 | 1 | %011010 | 12 | Status Query Request - Extended | 3.71 |
| 0 | 1 | %011001 | 10 | Status Query Response - Abbreviated | 3.72 |
| 1 | 1 | %011001 | 14 | Status Query Response - Extended | 3.72 |
| 0 | 1 | %011000 | 10 | Status Update Request - Abbreviated | 3.73 |
| 1 | 1 | %011000 | 14 | Status Update Request - Extended | 3.73 |
| 0 | 1 | %101100 | 10 | Unit Registration Request | 3.74 |
| 0 | 1 | %101111 | 9 | Unit Deregistration Request | 3.75 |
| 0 | 1 | %101101 | 9 | Location Registration Request - Abbreviated | 3.76 |
| 1 | 1 | %101101 | 19 | Location Registration Request - Extended | 3.76 |
| 0 | 1 | %011101 | 10 | Radio Unit Monitor Request - Abbreviated | 3.77 |
| 1 | 1 | %011101 | 14 | Radio Unit Monitor Request - Extended | 3.77 |
| 0 | 1 | %110110 | 8 | Roaming Address Request - Abbreviated | 3.78 |
| 1 | 1 | %110110 | 12 | Roaming Address Request - Extended | 3.78 |
| 0 | 1 | %110111 | Variable | Roaming Address Response | 3.79 |
| 0 | 1 | %111010 | 6 | Authentication FNE Result - Abbreviated | 3.80 |
| 1 | 1 | %111010 | 13 | Authentication FNE Result - Extended | 3.80 |
| 0 | 1 | %111000 | 10 | Authentication Response - Abbreviated | 3.81 |
| 1 | 1 | %111000 | 22 | Authentication Response - Extended | 3.81 |
| 0 | 1 | %111001 | 27 | Authentication Response Mutual | 3.82 |
| 0 | 1 | %111011 | 5 | Authentication SU Demand - Abbreviated | 3.83 |
| 1 | 1 | %111011 | 12 | Authentication SU Demand - Extended | 3.83 |
| 0 | 1 | %011110 | 20 | Radio Unit Monitor Enhanced Request - Abbreviated | 3.84 |
| 1 | 1 | %011110 | 28 | Radio Unit Monitor Enhanced Request - Extended | 3.84 |
| 0 | 0 | %001000 | Variable | Null Avoid Zero Bias Information | 3.85 |
| 1 | 0 | %000000 | 8 | MFID90 Group Regroup Voice Channel User - Abbreviated | 3.86 |
| 1 | 0 | %100000 | 16 | MFID90 Group Regroup Voice Channel User - Extended | 3.86 |
| 1 | 0 | %000011 | 7 | MFID90 Group Regroup Voice Channel Update | 3.87 |
| 1 | 0 | %000100 | 11 | MFID90 Extended Function Command | 3.88 |
| 1 | 0 | %100001 | 9 | MFID90 Group Regroup Voice Request | 3.89 |
| 1 | 0 | %100010 | 11 | MFID90 Extended Function Response | 3.90 |
| 1 | 0 | %110000 | Variable | MFIDA4 Group Regroup Explicit Encryption Command | 3.91 |
| 1 | 0 | %000001 | Variable | MFID90 Group Regroup Add Command | 3.92 |
| 1 | 0 | %001001 | Variable | MFID90 Group Regroup Delete Command | 3.93 |
| 1 | 0 | %100011 | 11 | MFID90 Group Regroup Channel Grant - Implicit | 3.94 |
| 1 | 0 | %100100 | 13 | MFID90 Group Regroup Channel Grant - Explicit | 3.94 |
| 1 | 0 | %100101 | 11 | MFID90 Group Regroup Channel Update | 3.95 |
| 1 | 0 | %100110 | 11 | MFID90 Queued Response | 3.96 |
| 1 | 0 | %100111 | 11 | MFID90 Deny Response | 3.97 |
| 1 | 0 | %101000 | 10 | MFID90 Acknowledge Response | 3.98 |
| 0 | 0 | %010000 | Variable | Continuation Fragment | See Note |

**Note:** The continuation fragment(s) of multi-fragment messages are specified in the MAC message details that follow for MAC messages that are too large to be conveyed in a single MAC PDU.

---

### 3.1 Null Information

The Null Information message is used to fill any unused portion of a MAC PDU. The length is sufficient to fill the MAC PDU. The message format is shown in Figure 18 below.

#### Figure 18 – Null Information

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | | | | | |
| ⁞ | | | | Null field | | | | |
| … | | | | | | | | |

B1 = %0, B2 = %0, MCO = %000000

---

### 3.2 Group Voice Channel User

The Group Voice Channel User messages indicate the user of this channel for group voice traffic on both inbound and outbound messages. The abbreviated format is shown in Figure 19 below and the extended format is shown in Figure 20 below.

#### Figure 19 – Group Voice Channel User - Abbreviated

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Service Options | | | | | |
| 3 | | | | Group Address | | | | |
| 4 | | | | | | | | |
| 5 | | | | | | | | |
| 6 | | | | Source Address | | | | |
| 7 | | | | | | | | |

B1 = %0, B2 = %0, MCO = %000001

#### Figure 20 – Group Voice Channel User - Extended

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Service Options | | | | | |
| 3 | | | | Group Address | | | | |
| 4 | | | | | | | | |
| 5 | | | | | | | | |
| 6 | | | | Source Address | | | | |
| 7 | | | | | | | | |
| 8 | | | | | | | | |
| 9 | | | | Source SUID | | | | |
| ⁞ | | | | | | | | |
| 13 | | | | | | | | |
| 14 | | | | | | | | |

B1 = %0, B2 = %0, MCO = %100001

---

### 3.3 Unit to Unit Voice Channel User

The Unit to Unit Voice Channel User messages indicate the user of this channel for unit to unit voice traffic. This is used on both inbound and outbound messages. The abbreviated format is shown in Figure 21 below and the extended format is shown in Figure 22 below.

The extended format shall be used when the source or destination of the message is from a different system or network than the current Site. If the Target Address is unknown, then the Target Address shall be set to zero (0).

#### Figure 21 – Unit to Unit Voice Channel User - Abbreviated

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Service Options | | | | | |
| 3 | | | | | | | | |
| 4 | | | | Target Address | | | | |
| 5 | | | | | | | | |
| 6 | | | | | | | | |
| 7 | | | | Source Address | | | | |
| 8 | | | | | | | | |

B1 = %0, B2 = %0, MCO = %000010

#### Figure 22 – Unit to Unit Voice Channel User - Extended

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Service Options | | | | | |
| 3 | | | | | | | | |
| 4 | | | | Target Address | | | | |
| 5 | | | | | | | | |
| 6 | | | | | | | | |
| 7 | | | | Source Address | | | | |
| 8 | | | | | | | | |
| 9 | | | | | | | | |
| 10 | | | | Source SUID | | | | |
| ⁞ | | | | | | | | |
| 14 | | | | | | | | |
| 15 | | | | | | | | |

B1 = %0, B2 = %0, MCO = %100010

---

### 3.4 Telephone Interconnect Voice Channel User

The Telephone Interconnect Voice Channel User message indicates the user of this channel for telephone interconnect voice traffic. This is used on both inbound and outbound messages. The message format is shown in Figure 23 below.

#### Figure 23 – Telephone Interconnect Voice Channel User

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Service Options | | | | | |
| 3 | | | | Call Timer | | | | |
| 4 | | | | | | | | |
| 5 | | | | | | | | |
| 6 | | | | Source/Target Address | | | | |
| 7 | | | | | | | | |

B1 = %0, B2 = %0, MCO = %000011

---

### 3.5 Unit to Unit Voice Service Request

The Unit to Unit Voice Service Request messages indicate a request for a voice call between two specified SUs. The abbreviated format is shown in Figure 24 below and the extended format is shown in Figure 25 below. The abbreviated format is only applicable if the requesting SU is currently in its Home system and the target SU is a member of the same Home system. The extended format shall always be acceptable.

#### Figure 24 – Unit to Unit Voice Service Request - Abbreviated

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Service Options | | | | | |
| 3 | | | | | | | | |
| 4 | | | | Target ID | | | | |
| 5 | | | | | | | | |
| 6 | | | | | | | | |
| 7 | | | | Source Address | | | | |
| 8 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %000100

#### Figure 25 – Unit to Unit Voice Service Request - Extended

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Service Options | | | | | |
| 3 | | | | | | | | |
| 4 | | | | Source Address | | | | |
| 5 | | | | | | | | |
| 6 | | | | | | | | |
| 7 | | | | Target SUID | | | | |
| ⁞ | | | | | | | | |
| 11 | | | | | | | | |
| 12 | | | | | | | | |

B1 = %1, B2 = %1, MCO = %000100

---

### 3.6 Group Voice Channel Grant Update Multiple

The Group Voice Channel Grant Update Multiple messages indicate the updates of other group voice traffic on this system and may be used to move directly to the specified channel. The implicit channel format is shown in Figure 26 below and the explicit channel format is shown in Figure 27 below. The implicit channel format conveys up to three group call channel assignments. The explicit channel format conveys up to two group call channel assignments. These messages also convey the Service Options with each channel assignment.

#### Figure 26 – Group Voice Channel Update Multiple - Implicit

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Service Options 1 | | | | | |
| 3 | | | | Channel 1 | | | | |
| 4 | | | | | | | | |
| 5 | | | | Group Address 1 | | | | |
| 6 | | | | | | | | |
| 7 | | | Service Options 2 | | | | | |
| 8 | | | | Channel 2 | | | | |
| 9 | | | | | | | | |
| 10 | | | | Group Address 2 | | | | |
| 11 | | | | | | | | |
| 12 | | | Service Options 3 | | | | | |
| 13 | | | | Channel 3 | | | | |
| 14 | | | | | | | | |
| 15 | | | | Group Address 3 | | | | |
| 16 | | | | | | | | |

B1 = %0, B2 = %0, MCO = %000101

#### Figure 27 – Group Voice Channel Update Multiple - Explicit

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Service Options 1 | | | | | |
| 3 | | | | Channel (T) 1 | | | | |
| 4 | | | | | | | | |
| 5 | | | | Channel (R) 1 | | | | |
| 6 | | | | | | | | |
| 7 | | | | Group Address 1 | | | | |
| 8 | | | | | | | | |
| 9 | | | Service Options 2 | | | | | |
| 10 | | | | Channel (T) 2 | | | | |
| 11 | | | | | | | | |
| 12 | | | | Channel (R) 2 | | | | |
| 13 | | | | | | | | |
| 14 | | | | Group Address 2 | | | | |
| 15 | | | | | | | | |

B1 = %0, B2 = %0, MCO = %100101

---

### 3.7 Group Voice Channel Grant

The Group Voice Channel Grant messages indicate that voice service for a group audience is assigned to the specified channel resource on the system. The implicit channel format is shown in Figure 28 below and the explicit channel format is shown in Figure 29 below.

#### Figure 28 – Group Voice Channel Grant - Implicit

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Service Options | | | | | |
| 3 | | | | Channel | | | | |
| 4 | | | | | | | | |
| 5 | | | | Group Address | | | | |
| 6 | | | | | | | | |
| 7 | | | | | | | | |
| 8 | | | | Source Address | | | | |
| 9 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %000000

#### Figure 29 – Group Voice Channel Grant - Explicit

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Service Options | | | | | |
| 3 | | | | Channel (T) | | | | |
| 4 | | | | | | | | |
| 5 | | | | Channel (R) | | | | |
| 6 | | | | | | | | |
| 7 | | | | Group Address | | | | |
| 8 | | | | | | | | |
| 9 | | | | | | | | |
| 10 | | | | Source Address | | | | |
| 11 | | | | | | | | |

B1 = %1, B2 = %1, MCO = %000000

---

### 3.8 Group Voice Channel Grant Update

The Group Voice Channel Grant Update messages indicate the updates of other group voice traffic on the system and may be used to move directly to the specified channel. The implicit channel format is shown in Figure 30 below and the explicit channel format is shown in Figure 31 below. The implicit channel format conveys implicit channel assignments for up to two other group voice calls. The explicit channel format is used when both the transmit and receive frequencies need to be provided and conveys the Service Options and explicit channel assignment for one other group call.

#### Figure 30 – Group Voice Channel Grant Update - Implicit

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | Channel 1 | | | | |
| 3 | | | | | | | | |
| 4 | | | | Group Address 1 | | | | |
| 5 | | | | | | | | |
| 6 | | | | Channel 2 | | | | |
| 7 | | | | | | | | |
| 8 | | | | Group Address 2 | | | | |
| 9 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %000010

#### Figure 31 – Group Voice Channel Grant Update - Explicit

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Service Options | | | | | |
| 3 | | | | Channel (T) | | | | |
| 4 | | | | | | | | |
| 5 | | | | Channel (R) | | | | |
| 6 | | | | | | | | |
| 7 | | | | Group Address | | | | |
| 8 | | | | | | | | |

B1 = %1, B2 = %1, MCO = %000011

---

### 3.9 Unit to Unit Voice Service Channel Grant

The Unit to Unit Voice Service Channel Grant messages indicate the particular channel assignment for a requested voice call between individual units of the system. The abbreviated format is shown in Figure 32 below and the extended format for the VCH is shown in Figure 33 below. The extended format for the LCCH is a multi-fragment message. The format for the first fragment is shown in Figure 34 below and the format for the second fragment is shown in Figure 35 below.

The abbreviated format utilizes the implicit channel format and is used when the Source SU and the Target SU are members of the same Home system, with both SUs residing in the Home system. The extended format for the VCH utilizes the explicit channel format and conveys the fully qualified source address (SUID). The extended format for the LCCH utilizes the explicit channel format and conveys Service Options, fully qualified source and target addresses, and working source and target addresses. The extended format shall always be acceptable even in cases where the abbreviated format could be used.

#### Figure 32 – Unit to Unit Voice Service Channel Grant - Abbreviated

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | Channel | | | | |
| 3 | | | | | | | | |
| 4 | | | | | | | | |
| 5 | | | | Target Address | | | | |
| 6 | | | | | | | | |
| 7 | | | | | | | | |
| 8 | | | | Source Address | | | | |
| 9 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %000100

#### Figure 33 – Unit to Unit Voice Service Channel Grant - Extended VCH

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | Channel (T) | | | | |
| 3 | | | | | | | | |
| 4 | | | | Channel (R) | | | | |
| 5 | | | | | | | | |
| 6 | | | | | | | | |
| 7 | | | | Source SUID | | | | |
| ⁞ | | | | | | | | |
| 11 | | | | | | | | |
| 12 | | | | | | | | |
| 13 | | | | | | | | |
| 14 | | | | Target Address | | | | |
| 15 | | | | | | | | |

B1 = %1, B2 = %1, MCO = %000100

#### Figure 34 – Unit to Unit Voice Service Channel Grant - Extended LCCH (1 of 2)

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 18 | | | |
| 3 | | | Data Length = 28 | | | | | |
| 4 | | | Service Options | | | | | |
| 5 | | | | | | | | |
| 6 | | | | Source Address | | | | |
| 7 | | | | | | | | |
| 8 | | | | | | | | |
| 9 | | | | Source SUID | | | | |
| ⁞ | | | | | | | | |
| 13 | | | | | | | | |
| 14 | | | | | | | | |
| 15 | | | | Channel (T) | | | | |
| 16 | | | | | | | | |
| 17 | | | | Channel (R) | | | | |
| 18 | | | | | | | | |

B1 = %1, B2 = %1, MCO = %001111

#### Figure 35 – Unit to Unit Voice Service Channel Grant - Extended LCCH (2 of 2)

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 14 | | | |
| 3 | | | | | | | | |
| 4 | | | | Target Address | | | | |
| 5 | | | | | | | | |
| 6 | | | | | | | | |
| 7 | | | | Target SUID | | | | |
| ⁞ | | | | | | | | |
| 11 | | | | | | | | |
| 12 | | | | | | | | |
| 13 | | | | Multi-Fragment CRC-16 | | | | |
| 14 | | | | | | | | |

B1 = %0, B2 = %0, MCO = %010000

---

### 3.10 Unit to Unit Answer Request

The Unit to Unit Answer Request messages indicate to the target unit that a unit to unit call has been requested involving the target unit. The abbreviated format is shown in Figure 36 below and the extended format is shown in Figure 37 below.

#### Figure 36 – Unit to Unit Answer Request - Abbreviated

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Service Options | | | | | |
| 3 | | | | | | | | |
| 4 | | | | Target Address | | | | |
| 5 | | | | | | | | |
| 6 | | | | | | | | |
| 7 | | | | Source Address | | | | |
| 8 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %000101

#### Figure 37 – Unit to Unit Answer Request - Extended

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Service Options | | | | | |
| 3 | | | | | | | | |
| 4 | | | | Target Address | | | | |
| 5 | | | | | | | | |
| 6 | | | | | | | | |
| 7 | | | | Source SUID | | | | |
| ⁞ | | | | | | | | |
| 11 | | | | | | | | |
| 12 | | | | | | | | |

B1 = %1, B2 = %1, MCO = %000101

---

### 3.11 Radio Unit Monitor Enhanced Command

The Radio Unit Monitor Enhanced Command message is to be used to command a radio to execute an enhanced radio unit monitor operation. The target SU may initiate either a clear or encrypted unit-to-unit call, or a clear or encrypted group call. The abbreviated format is shown in Figure 38 below. The extended format is a multi-fragment message. The format for the first fragment is shown in Figure 39 below, the format for the second fragment is shown in Figure 40 below, and the format for the third fragment is shown in Figure 41 below.

The abbreviated format is only used on the outbound VCH when the CCH is using the corresponding form of the OSP. The extended format is used if the abbreviated format is not appropriate.

#### Figure 38 – Radio Unit Monitor Enhanced Command - Abbreviated

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | | | | | |
| 3 | | | | Target Address | | | | |
| 4 | | | | | | | | |
| 5 | | | | Group ID | | | | |
| 6 | | | | | | | | |
| 7 | | | | | | | | |
| 8 | | | | Source Address | | | | |
| 9 | | | | | | | | |
| 10 | SM | TG | | | Reserved | | | |
| 11 | | | | TX Time | | | | |
| 12 | | | | Key ID | | | | |
| 13 | | | | | | | | |
| 14 | | | | ALGID | | | | |

B1 = %0, B2 = %1, MCO = %011110

#### Figure 39 – Radio Unit Monitor Enhanced Command - Extended (1 of 3)

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 18 | | | |
| 3 | | | Data Length = 34 | | | | | |
| 4 | | | | | | | | |
| 5 | | | | Target Address | | | | |
| 6 | | | | | | | | |
| 7 | | | | | | | | |
| 8 | | | | Source SUID | | | | |
| ⁞ | | | | | | | | |
| 12 | | | | | | | | |
| 13 | | | | | | | | |
| 14 | SM | TG | | | Reserved | | | |
| 15 | | | | TX Time | | | | |
| 16 | | | | Key ID | | | | |
| 17 | | | | | | | | |
| 18 | | | | ALGID | | | | |

B1 = %1, B2 = %1, MCO = %011110

#### Figure 40 – Radio Unit Monitor Enhanced Command - Extended (2 of 3)

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 18 | | | |
| 3 | | | | | | | | |
| 4 | | | | Source Address | | | | |
| 5 | | | | | | | | |
| 6 | | | | | | | | |
| 7 | | | | Target SUID | | | | |
| ⁞ | | | | | | | | |
| 11 | | | | | | | | |
| 12 | | | | | | | | |
| 13 | | | | | | | | |
| 14 | | | | SGID | | | | |
| ⁞ | | | | | | | | |
| 17 | | | | | | | | |
| 18 | | | | | | | | |

B1 = %0, B2 = %0, MCO = %010000

#### Figure 41 – Radio Unit Monitor Enhanced Command - Extended (3 of 3)

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 4 | | | |
| 3 | | | | Multi-Fragment CRC-16 | | | | |
| 4 | | | | | | | | |

B1 = %0, B2 = %0, MCO = %010000

---

### 3.12 Telephone Interconnect Answer Request

The Telephone Interconnect Answer Request message informs the target unit of a pending PSTN call and solicits a response from the target unit. The message may indicate the calling party's 10-digit telephone number. The message format is shown in Figure 42 below.

#### Figure 42 – Telephone Interconnect Answer Request

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | Digit 1 | | | | Digit 2 | | |
| 3 | | Digit 3 | | | | Digit 4 | | |
| 4 | | Digit 5 | | | | Digit 6 | | |
| 5 | | Digit 7 | | | | Digit 8 | | |
| 6 | | Digit 9 | | | | Digit 10 | | |
| 7 | | | | | | | | |
| 8 | | | | Target Address | | | | |
| 9 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %001010

---

### 3.13 Unit to Unit Voice Channel Grant Update

The Unit to Unit Voice Channel Grant Update messages indicate updates of voice call traffic on this system and may be used to move directly to the specified channel. The abbreviated format is shown in Figure 43 below and the extended format for the VCH is shown in Figure 44 below. The extended format for the LCCH is a multi-fragment message. The format for the first fragment is shown in Figure 45 below and the format for the second fragment is shown in Figure 46 below.

The abbreviated format utilizes the implicit channel format and is used when the Source SU and the Target SU are members of the same Home system, with both SUs residing in the Home system. The extended format for the VCH utilizes the explicit channel format and conveys the fully qualified source address (SUID). The extended format for the LCCH utilizes the explicit channel format and conveys Service Options, fully qualified source and target addresses, and working source and target addresses. The extended format shall always be acceptable even in cases where the abbreviated format could be used.

#### Figure 43 – Unit to Unit Voice Channel Grant Update - Abbreviated

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | Channel | | | | |
| 3 | | | | | | | | |
| 4 | | | | | | | | |
| 5 | | | | Target Address | | | | |
| 6 | | | | | | | | |
| 7 | | | | | | | | |
| 8 | | | | Source Address | | | | |
| 9 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %000110

#### Figure 44 – Unit to Unit Voice Channel Grant Update - Extended VCH

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | Channel (T) | | | | |
| 3 | | | | | | | | |
| 4 | | | | Channel (R) | | | | |
| 5 | | | | | | | | |
| 6 | | | | | | | | |
| 7 | | | | Source SUID | | | | |
| ⁞ | | | | | | | | |
| 11 | | | | | | | | |
| 12 | | | | | | | | |
| 13 | | | | | | | | |
| 14 | | | | Target Address | | | | |
| 15 | | | | | | | | |

B1 = %1, B2 = %1, MCO = %000110

#### Figure 45 – Unit to Unit Voice Channel Grant Update - Extended LCCH (1 of 2)

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 18 | | | |
| 3 | | | Data Length = 28 | | | | | |
| 4 | | | Service Options | | | | | |
| 5 | | | | | | | | |
| 6 | | | | Source Address | | | | |
| 7 | | | | | | | | |
| 8 | | | | | | | | |
| 9 | | | | Source SUID | | | | |
| ⁞ | | | | | | | | |
| 13 | | | | | | | | |
| 14 | | | | | | | | |
| 15 | | | | Channel (T) | | | | |
| 16 | | | | | | | | |
| 17 | | | | Channel (R) | | | | |
| 18 | | | | | | | | |

B1 = %1, B2 = %1, MCO = %000111

#### Figure 46 – Unit to Unit Voice Channel Grant Update - Extended LCCH (2 of 2)

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 14 | | | |
| 3 | | | | | | | | |
| 4 | | | | Target Address | | | | |
| 5 | | | | | | | | |
| 6 | | | | | | | | |
| 7 | | | | Target SUID | | | | |
| ⁞ | | | | | | | | |
| 11 | | | | | | | | |
| 12 | | | | | | | | |
| 13 | | | | Multi-Fragment CRC-16 | | | | |
| 14 | | | | | | | | |

B1 = %0, B2 = %0, MCO = %010000

---

### 3.14 Acknowledge Response FNE

The Acknowledge Response FNE message is a generic response supplied to a unit to acknowledge an action when there is no other expected response. It is sent to a subscriber unit in response to an earlier action or service request. The abbreviated format is shown in Figure 47 below. This extended format is a multi-fragment message. The format of the first fragment is shown in Figure 48 below and the format of the second fragment is shown in Figure 49 below.

#### Figure 47 – Acknowledge Response FNE - Abbreviated

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | AIV | EX | | | Service Type | | | |
| 3 | | | | | | | | |
| 4 | | | | Additional Information | | | | |
| 5 | | | | | | | | |
| 6 | | | | | | | | |
| 7 | | | | | | | | |
| 8 | | | | Target Address / ID | | | | |
| 9 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %100000

#### Figure 48 – Acknowledge Response FNE - Extended (1 of 2)

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 18 | | | |
| 3 | | | Data Length = 24 | | | | | |
| 4 | Reserved | | | | Service Type | | | |
| 5 | | | | | | | | |
| 6 | | | | Source SUID | | | | |
| ⁞ | | | | | | | | |
| 10 | | | | | | | | |
| 11 | | | | | | | | |
| 12 | | | | | | | | |
| 13 | | | | Target SUID | | | | |
| ⁞ | | | | | | | | |
| 17 | | | | | | | | |
| 18 | | | | | | | | |

B1 = %1, B2 = %1, MCO = %100000

#### Figure 49 – Acknowledge Response FNE - Extended (2 of 2)

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 10 | | | |
| 3 | | | | | | | | |
| 4 | | | | Source Address | | | | |
| 5 | | | | | | | | |
| 6 | | | | | | | | |
| 7 | | | | Target Address | | | | |
| 8 | | | | | | | | |
| 9 | | | | Multi-Fragment CRC-16 | | | | |
| 10 | | | | | | | | |

B1 = %0, B2 = %0, MCO = %010000

---

### 3.15 SNDCP Data Channel Grant

The SNDCP Data Channel Grant message is a DCH grant (channel assignment) for an SNDCP trunked data service operation. This is the packet format utilized when the assigned working DCH requires a unique designation for transmit and receive frequency values. This is accommodated with an explicit channel field for the FNE transmit [Channel (T)], and FNE receive [Channel (R)] frequency component. The message format is shown in Figure 50 below.

#### Figure 50 – SNDCP Data Channel Grant

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Data Service Options | | | | | |
| 3 | | | | Channel (T) | | | | |
| 4 | | | | | | | | |
| 5 | | | | Channel (R) | | | | |
| 6 | | | | | | | | |
| 7 | | | | | | | | |
| 8 | | | | Target Address | | | | |
| 9 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %010100

---

### 3.16 SNDCP Data Page Request

The SNDCP Data Page Request message is used to indicate to the target that trunked data service has been requested. The message format is shown in Figure 51 below.

#### Figure 51 – SNDCP Data Page Request

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Data Service Options | | | | | |
| 3 | | | | Data Access Control | | | | |
| 4 | | | | | | | | |
| 5 | | | | | | | | |
| 6 | | | | Target Address | | | | |
| 7 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %010101

---

### 3.17 SNDCP Data Channel Announcement - Explicit

The SNDCP Data Channel Announcement - Explicit message indicates current trunked data service working channel assignments and access permissions for the indicated data access control group(s). The message format is shown in Figure 52 below.

#### Figure 52 – SNDCP Data Channel Announcement - Explicit

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Data Service Options | | | | | |
| 3 | AA | RA | | | Reserved | | | |
| 4 | | | | Channel (T) | | | | |
| 5 | | | | | | | | |
| 6 | | | | Channel (R) | | | | |
| 7 | | | | | | | | |
| 8 | | | | Data Access Control | | | | |
| 9 | | | | | | | | |

B1 = %1, B2 = %1, MCO = %010110

---

### 3.18 Adjacent Status Broadcast

The Adjacent Status Broadcast messages inform the subscriber unit of the presence of Sites adjacent to this particular Site.

The implicit format is shown in Figure 53 below and the explicit format is shown in Figure 54 below. These formats are used when the current system's channel identifiers match the adjacent system's channel identifiers and when the WACN ID of the adjacent Site is the same as the WACN ID of the transmitting Site. When these formats are used, the identifier field of the Channel is used to select a channel identifier with a matching identifier to provide the channel identifier information for the channel.

The extended-explicit format is shown in Figure 55 below and is used when the current system's channel identifiers do not match the adjacent system's channel identifiers or when the WACN ID of the adjacent Site is different than the WACN ID of the transmitting Site. When this format is used, the identifier field of the Channel, the System ID, and the WACN ID are used to select a channel identifier with a matching identifier, System ID, and WACN ID to provide the channel identifier information for the channel.

#### Figure 53 – Adjacent Status Broadcast - Implicit

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | LRA | | | | |
| 3 | C | F | V | A | | | | |
| 4 | | | | System ID | | | | |
| 5 | | | | RFSS ID | | | | |
| 6 | | | | Site ID | | | | |
| 7 | | | | Channel | | | | |
| 8 | | | | | | | | |
| 9 | | | | System Service Class | | | | |

B1 = %0, B2 = %1, MCO = %111100

#### Figure 54 – Adjacent Status Broadcast - Explicit

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | LRA | | | | |
| 3 | C | F | V | A | | | | |
| 4 | | | | System ID | | | | |
| 5 | | | | RFSS ID | | | | |
| 6 | | | | Site ID | | | | |
| 7 | | | | Channel (T) | | | | |
| 8 | | | | | | | | |
| 9 | | | | Channel (R) | | | | |
| 10 | | | | | | | | |
| 11 | | | | System Service Class | | | | |

B1 = %1, B2 = %1, MCO = %111100

#### Figure 55 – Adjacent Status Broadcast - Extended - Explicit

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 15 | | | |
| 3 | | | | LRA | | | | |
| 4 | C | F | V | A | | | | |
| 5 | | | | System ID | | | | |
| 6 | | | | RFSS ID | | | | |
| 7 | | | | Site ID | | | | |
| 8 | | | | Channel (T) | | | | |
| 9 | | | | | | | | |
| 10 | | | | Channel (R) | | | | |
| 11 | | | | | | | | |
| 12 | | | | System Service Class | | | | |
| 13 | | | | WACN ID | | | | |
| 14 | | | | | | | | |
| 15 | | | | Reserved | | | | |

B1 = %1, B2 = %1, MCO = %111110

---

### 3.19 Call Alert

The Call Alert messages request a target SU to call a source SU. The abbreviated format is shown in Figure 56 below and the extended format for the VCH is shown in Figure 57 below. This extended format for the LCCH is a multi-fragment message. The format of the first fragment is shown in Figure 58 below and the format of the second fragment is shown in Figure 59 below.

#### Figure 56 – Call Alert - Abbreviated

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | | | | | |
| 3 | | | | Target Address | | | | |
| 4 | | | | | | | | |
| 5 | | | | | | | | |
| 6 | | | | Source Address | | | | |
| 7 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %011111

#### Figure 57 – Call Alert - Extended VCH

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | | | | | |
| 3 | | | | Target Address | | | | |
| 4 | | | | | | | | |
| 5 | | | | | | | | |
| 6 | | | | Source SUID | | | | |
| ⁞ | | | | | | | | |
| 10 | | | | | | | | |
| 11 | | | | | | | | |

B1 = %1, B2 = %1, MCO = %011111

#### Figure 58 – Call Alert - Extended LCCH (1 of 2)

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 18 | | | |
| 3 | | | Data Length = 23 | | | | | |
| 4 | | | | | | | | |
| 5 | | | | Source Address | | | | |
| 6 | | | | | | | | |
| 7 | | | | | | | | |
| 8 | | | | Source SUID | | | | |
| ⁞ | | | | | | | | |
| 12 | | | | | | | | |
| 13 | | | | | | | | |
| 14 | | | | | | | | |
| 15 | | | | Target Address | | | | |
| 16 | | | | | | | | |
| 17 | | | | Target SUID | | | | |
| 18 | | | | (octets 1 – 2) | | | | |

B1 = %1, B2 = %1, MCO = %001011

#### Figure 59 – Call Alert - Extended LCCH (2 of 2)

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 9 | | | |
| 3 | | | | | | | | |
| 4 | | | | Target SUID | | | | |
| 5 | | | | (octets 3 – 7) | | | | |
| 6 | | | | | | | | |
| 7 | | | | | | | | |
| 8 | | | | Multi-Fragment CRC-16 | | | | |
| 9 | | | | | | | | |

B1 = %0, B2 = %0, MCO = %010000

---

### 3.20 Extended Function Command

The Extended Function Command messages are addressed to an SU for an extended function transaction. The abbreviated format is shown in Figure 60 below, the extended format for the VCH is shown in Figure 61 below, and the extended format for the LCCH is shown in Figure 62 below.

The abbreviated format is used when the target SU is active on its Home system and the source SU and Target SU have the same Home system. The extended formats are used when the abbreviated format is not appropriate.

#### Figure 60 – Extended Function Command - Abbreviated

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | | | | | |
| 3 | | | | Extended Function | | | | |
| 4 | | | | | | | | |
| 5 | | | | | | | | |
| 6 | | | | | | | | |
| 7 | | | | | | | | |
| 8 | | | | Target Address | | | | |
| 9 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %100100

#### Figure 61 – Extended Function Command - Extended VCH

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 17 | | | |
| 3 | | | | | | | | |
| 4 | | | | Extended Function | | | | |
| 5 | | | | | | | | |
| 6 | | | | | | | | |
| 7 | | | | | | | | |
| 8 | | | | | | | | |
| 9 | | | | Target Address | | | | |
| 10 | | | | | | | | |
| 11 | | | | | | | | |
| 12 | | | | Source SUID | | | | |
| ⁞ | | | | | | | | |
| 16 | | | | | | | | |
| 17 | | | | | | | | |

B1 = %1, B2 = %1, MCO = %100100

#### Figure 62 – Extended Function Command - Extended LCCH

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 14 | | | |
| 3 | | | | | | | | |
| 4 | | | | Extended Function | | | | |
| 5 | | | | | | | | |
| 6 | | | | | | | | |
| 7 | | | | | | | | |
| 8 | | | | | | | | |
| 9 | | | | Target Address | | | | |
| 10 | | | | | | | | |
| 11 | | | | Source WACN ID | | | | |
| 12 | | | | | | | | |
| 13 | | | | | | | | |
| 14 | | | | Source System ID | | | | |

B1 = %1, B2 = %1, MCO = %100101

---

### 3.21 Group Affiliation Query

The Group Affiliation Query messages are used to determine what a targeted subscriber unit maintains as the group affiliation data for the unit. The abbreviated format is shown in Figure 63 below and the extended format is shown in Figure 64 below.

#### Figure 63 – Group Affiliation Query - Abbreviated

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | | | | | |
| 3 | | | | Target Address | | | | |
| 4 | | | | | | | | |
| 5 | | | | | | | | |
| 6 | | | | Source Address | | | | |
| 7 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %101010

#### Figure 64 – Group Affiliation Query - Extended

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | | | | | |
| 3 | | | | Target Address | | | | |
| 4 | | | | | | | | |
| 5 | | | | | | | | |
| 6 | | | | Source SUID | | | | |
| ⁞ | | | | | | | | |
| 10 | | | | | | | | |
| 11 | | | | | | | | |

B1 = %1, B2 = %1, MCO = %101010

---

### 3.22 Identifier Update

The Identifier Update message should be used to inform the subscriber units of the channel parameters to associate with a specific channel identifier. This message is to be used for base frequencies outside the VHF and UHF bands. The message format is shown in Figure 65 below.

#### Figure 65 – Identifier Update

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Channel Identifier | | | | | |
| 3 | | | | BW | | | | |
| 4 | | | | Transmit Offset | | | | |
| 5 | | | | Channel Spacing | | | | |
| 6 | | | | | | | | |
| 7 | | | | Base Frequency | | | | |
| 8 | | | | | | | | |
| 9 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %111101

---

### 3.23 Time and Date Announcement

The Time and Date Announcement message is sent by the FNE to inform SUs of the current time and date. The message format is shown in Figure 66 below.

#### Figure 66 – Time and Date Announcement

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | VD | VT | VL | res | | | | |
| 3 | | | Local Time Offset | | | | | |
| 4 | | | | | | | | |
| 5 | | | | Date | | | | |
| 6 | | | | | | | | |
| 7 | | | | | | | | |
| 8 | | | | Time | | | | |
| 9 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %110101

---

### 3.24 Network Status Broadcast

The Network Status Broadcast messages provide the current WACN ID and System ID to the SUs monitoring the channel. The implicit format is shown in Figure 67 below and the explicit format is shown in Figure 68 below.

#### Figure 67 – Network Status Broadcast - Implicit

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | LRA | | | | |
| 3 | | | | WACN ID | | | | |
| 4 | | | | | | | | |
| 5 | | | | | | | | |
| 6 | | | | System ID | | | | |
| 7 | | | | Channel | | | | |
| 8 | | | | | | | | |
| 9 | | | | System Service Class | | | | |
| 10 | | | Reserved | | | | | |
| 11 | | | | Color Code | | | | |

B1 = %0, B2 = %1, MCO = %111011

#### Figure 68 – Network Status Broadcast - Explicit

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | LRA | | | | |
| 3 | | | | WACN ID | | | | |
| 4 | | | | | | | | |
| 5 | | | | | | | | |
| 6 | | | | System ID | | | | |
| 7 | | | | Channel (T) | | | | |
| 8 | | | | | | | | |
| 9 | | | | Channel (R) | | | | |
| 10 | | | | | | | | |
| 11 | | | | System Service Class | | | | |
| 12 | | | Reserved | | | | | |
| 13 | | | | Color Code | | | | |

B1 = %1, B2 = %1, MCO = %111011

---

### 3.25 Group Voice Service Request

The Group Voice Service Request message is used to request voice service targeting a group reference. The message format is shown in Figure 69 below.

#### Figure 69 – Group Voice Service Request

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Service Options | | | | | |
| 3 | | | | Group Address | | | | |
| 4 | | | | | | | | |
| 5 | | | | | | | | |
| 6 | | | | Source Address | | | | |
| 7 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %000001

---

### 3.26 RFSS Status Broadcast

The RFSS Status Broadcast messages provide the current RFSS and Site identities to the SUs monitoring this channel. The implicit format is shown in Figure 70 below and the explicit format is shown in Figure 71 below.

#### Figure 70 – RFSS Status Broadcast - Implicit

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | LRA | | | | |
| 3 | Reserved | | | R | A | | | |
| 4 | | | | System ID | | | | |
| 5 | | | | RFSS ID | | | | |
| 6 | | | | Site ID | | | | |
| 7 | | | | Channel | | | | |
| 8 | | | | | | | | |
| 9 | | | | System Service Class | | | | |

B1 = %0, B2 = %1, MCO = %111010

#### Figure 71 – RFSS Status Broadcast - Explicit

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | LRA | | | | |
| 3 | Reserved | | | R | A | | | |
| 4 | | | | System ID | | | | |
| 5 | | | | RFSS ID | | | | |
| 6 | | | | Site ID | | | | |
| 7 | | | | Channel (T) | | | | |
| 8 | | | | | | | | |
| 9 | | | | Channel (R) | | | | |
| 10 | | | | | | | | |
| 11 | | | | System Service Class | | | | |

B1 = %1, B2 = %1, MCO = %111010

---

### 3.27 Secondary Control Channel Broadcast

The Secondary Control Channel Broadcast messages indicate the current secondary control channel assignments for this Site. The implicit channel format is shown in Figure 72 below and the explicit channel format is shown in Figure 73 below. The implicit channel format conveys up to two secondary control channels. The explicit channel format conveys one secondary control channel. These messages also convey the System Service Class with each secondary control channel.

#### Figure 72 – Secondary Control Channel Broadcast - Implicit

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | RFSS ID | | | | |
| 3 | | | | Site ID | | | | |
| 4 | | | | Channel 1 | | | | |
| 5 | | | | | | | | |
| 6 | | | | System Service Class 1 | | | | |
| 7 | | | | Channel 2 | | | | |
| 8 | | | | | | | | |
| 9 | | | | System Service Class 2 | | | | |

B1 = %0, B2 = %1, MCO = %111001

#### Figure 73 – Secondary Control Channel Broadcast - Explicit

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | RFSS ID | | | | |
| 3 | | | | Site ID | | | | |
| 4 | | | | Channel (T) | | | | |
| 5 | | | | | | | | |
| 6 | | | | Channel (R) | | | | |
| 7 | | | | | | | | |
| 8 | | | | System Service Class | | | | |

B1 = %1, B2 = %1, MCO = %101001

---

### 3.28 Status Query

The Status Query messages may request the current status condition of another SU. The abbreviated format is shown in Figure 74 below and the extended format for the VCH is shown in Figure 75 below. The extended format for the LCCH is a multi-fragment message. The format for the first fragment is shown in Figure 76 below and the format for the second fragment is shown in Figure 77 below.

The abbreviated format is used when the message is destined for the Source SU, the Source SU is active on its Home system, and the Source SU and Target SU have the same Home system. The abbreviated format is also used when the message is destined for the Target SU, the Target SU is active on its Home system, and the Source SU and Target SU have the same Home system. The extended format is used when the abbreviated format is not appropriate.

#### Figure 74 – Status Query - Abbreviated

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | | | | | |
| 3 | | | | Target Address | | | | |
| 4 | | | | | | | | |
| 5 | | | | | | | | |
| 6 | | | | Source Address | | | | |
| 7 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %011010

#### Figure 75 – Status Query - Extended VCH

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | | | | | |
| 3 | | | | Target Address | | | | |
| 4 | | | | | | | | |
| 5 | | | | | | | | |
| 6 | | | | Source SUID | | | | |
| ⁞ | | | | | | | | |
| 10 | | | | | | | | |
| 11 | | | | | | | | |

B1 = %1, B2 = %1, MCO = %011010

#### Figure 76 – Status Query - Extended LCCH (1 of 2)

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 18 | | | |
| 3 | | | Data Length = 23 | | | | | |
| 4 | | | | | | | | |
| 5 | | | | Target Address | | | | |
| 6 | | | | | | | | |
| 7 | | | | | | | | |
| 8 | | | | Source SUID | | | | |
| ⁞ | | | | | | | | |
| 12 | | | | | | | | |
| 13 | | | | | | | | |
| 14 | | | | | | | | |
| 15 | | | | Source Address | | | | |
| 16 | | | | | | | | |
| 17 | | | | Target SUID | | | | |
| 18 | | | | (octets 1 – 2) | | | | |

B1 = %1, B2 = %1, MCO = %011011

#### Figure 77 – Status Query - Extended LCCH (2 of 2)

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 9 | | | |
| 3 | | | | | | | | |
| 4 | | | | Target SUID | | | | |
| 5 | | | | (octets 3 – 7) | | | | |
| 6 | | | | | | | | |
| 7 | | | | | | | | |
| 8 | | | | Multi-Fragment CRC-16 | | | | |
| 9 | | | | | | | | |

B1 = %0, B2 = %0, MCO = %010000

---

### 3.29 Queued Response

The Queued Response message indicates a requested service cannot be granted at this time. The message format is shown in Figure 78 below.

#### Figure 78 – Queued Response

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | AIV | 0 | | | Service Type | | | |
| 3 | | | | Reason Code | | | | |
| 4 | | | | | | | | |
| 5 | | | | Additional Information | | | | |
| 6 | | | | | | | | |
| 7 | | | | | | | | |
| 8 | | | | Target Address | | | | |
| 9 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %100001

---

### 3.30 Deny Response

The Deny Response message indicates a problem with the requested service. The message format is shown in Figure 79 below.

#### Figure 79 – Deny Response

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | AIV | 0 | | | Service Type | | | |
| 3 | | | | Reason Code | | | | |
| 4 | | | | | | | | |
| 5 | | | | Additional Information | | | | |
| 6 | | | | | | | | |
| 7 | | | | | | | | |
| 8 | | | | Target Address | | | | |
| 9 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %100111

---

### 3.31 System Service Broadcast

The System Service Broadcast message informs the SUs of the current system services supported and currently offered on this Site. The message format is shown in Figure 80 below.

#### Figure 80 – System Service Broadcast

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | Twuid Validity | | | | |
| 3 | | | | | | | | |
| 4 | | | | System Services Available | | | | |
| 5 | | | | | | | | |
| 6 | | | | | | | | |
| 7 | | | | System Services Supported | | | | |
| 8 | | | | | | | | |
| 9 | | | | Request Priority Level | | | | |

B1 = %0, B2 = %1, MCO = %111000

---

### 3.32 Unit Registration Command

The Unit Registration Command message is used to force an SU to initiate unit registration. The abbreviated format is shown in Figure 81 below.

#### Figure 81 – Unit Registration Command - Abbreviated

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | | | | | |
| 3 | | | | Target Address | | | | |
| 4 | | | | | | | | |
| 5 | | | | | | | | |
| 6 | | | | Source Address | | | | |
| 7 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %101101

---

### 3.33 Radio Unit Monitor Command - Obsolete

The Radio Unit Monitor Command message is used to command a radio to execute a radio unit monitor operation. The obsolete format is shown in Figure 82 below.

**Note:** This obsolete format was only specified for the VCH and was replaced by the format specified in 3.43 below when TIA-102.BBAC-1 was published in February 2013.

#### Figure 82 – Radio Unit Monitor Command - Obsolete

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Reserved | | | TX Mult | | |
| 3 | | | | | | | | |
| 4 | | | | Source Address | | | | |
| 5 | | | | | | | | |
| 6 | | | | | | | | |
| 7 | | | | Target Address | | | | |
| 8 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %011101

---

### 3.34 Identifier Update for VHF/UHF Bands

The Identifier Update for VHF/UHF Bands message should be used to inform the subscriber units of the channel parameters to associate with a specific channel identifier. This message is to be used for base frequencies within the VHF and UHF bands. The message format is shown in Figure 83 below.

#### Figure 83 – Identifier Update for VHF/UHF Bands

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | Channel Identifier | | | | BW | VU | |
| 3 | | | Transmit Offset VU | | | | | |
| 4 | | | | | | | | |
| 5 | | | | Channel Spacing | | | | |
| 6 | | | | | | | | |
| 7 | | | | Base Frequency | | | | |
| 8 | | | | | | | | |
| 9 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %110100

---

### 3.35 Identifier Update for TDMA

The Identifier Update for TDMA message should be used to inform the subscriber units of a TIA-102 TDMA system of the channel parameters to associate with a specific channel identifier. The abbreviated format is shown in Figure 84 below and the extended format is shown in Figure 85 below.

The extended format is used when the channel identifier information is provided for a system other than the current system (i.e. with a different WACN ID or System ID).

#### Figure 84 – Identifier Update for TDMA - Abbreviated

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | Channel Identifier | | | Channel Type | | | |
| 3 | | | Transmit Offset | | | TDMA | | |
| 4 | | | | | | | | |
| 5 | | | | Channel Spacing | | | | |
| 6 | | | | | | | | |
| 7 | | | | Base Frequency | | | | |
| 8 | | | | | | | | |
| 9 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %110011

#### Figure 85 – Identifier Update for TDMA - Extended

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 14 | | | |
| 3 | | Channel Identifier | | | Channel Type | | | |
| 4 | | | Transmit Offset | | | TDMA | | |
| 5 | | | | | | | | |
| 6 | | | | Channel Spacing | | | | |
| 7 | | | | | | | | |
| 8 | | | | Base Frequency | | | | |
| 9 | | | | | | | | |
| 10 | | | | | | | | |
| 11 | | | | WACN ID | | | | |
| 12 | | | | | | | | |
| 13 | | | | | | | | |
| 14 | | | | System ID | | | | |

B1 = %1, B2 = %1, MCO = %110011

---

### 3.36 Manufacturer Specific

The Manufacturer Specific message conveys information defined by the manufacturer. The message format is shown in Figure 86 below. The length, up to the limit of the MAC PDU, depends on the manufacturer definition.

In the case where the MCO value for a manufacturer specific request overlaps the MCO value for a standard request, a manufacturer specific response shall be used. In the case where a manufacturer specific service MCO value overlaps a standard service MCO value and the manufacturer specific service may later be cancelled or modified, a manufacturer specific MAC message shall be used to cancel or modify the manufacturer specific service.

#### Figure 86 – Manufacturer Specific

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Manufacturer's ID | | | | | |
| 3 | Reserved | | | | Length | | | |
| ⁞ | | | Manufacturer Dependent Information | | | | | |
| … | | | | | | | | |

B1 = %1, B2 = %0, MCO values are defined by the manufacturer

---

### 3.37 Individual Paging with Priority

The Individual Paging with Priority message is used to request that an individual unit move from the voice channel to the control channel. A variable number of individual IDs up to four, along with a corresponding priority level may be signaled in this message, see [6] for procedures associated with receiving this message. The message format is shown in Figure 87 below. The message length is dependent upon the number of targeted individuals.

#### Figure 87 – Individual Paging with Priority

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | PRIO1 | PRIO2 | PRIO3 | PRIO4 | Reserved | | LEN | |
| 3 | | | | | | | | |
| 4 | | | | Target Address 1 | | | | |
| 5 | | | | | | | | |
| 6 | | | | | | | | |
| 7 | | | | Target Address 2 (optional) | | | | |
| 8 | | | | | | | | |
| 9 | | | | | | | | |
| 10 | | | | Target Address 3 (optional) | | | | |
| 11 | | | | | | | | |
| 12 | | | | | | | | |
| 13 | | | | Target Address 4 (optional) | | | | |
| 14 | | | | | | | | |

B1 = %0, B2 = %0, MCO = %010010

#### Table 5 – PRIO Bit Definitions

| PRIO Bit | %0 | %1 |
|---|---|---|
| PRIO1 | ID1 page is Low Priority | ID1 page is High Priority |
| PRIO2 | ID2 page is Low Priority | ID2 page is High Priority |
| PRIO3 | ID3 page is Low Priority | ID3 page is High Priority |
| PRIO4 | ID4 page is Low Priority | ID4 page is High Priority |

The LEN field determines the number of IDs present in the message.

#### Table 6 – Individual Paging Message Length

| LEN | Number of Units to be Paged | Valid PRIO Bits | Message Length (octets) |
|---|---|---|---|
| %00 | 1 | PRIO1 | 5 |
| %01 | 2 | PRIO1, PRIO2 | 8 |
| %10 | 3 | PRIO1, PRIO2, PRIO3 | 11 |
| %11 | 4 | PRIO1, PRIO2, PRIO3, PRIO4 | 14 |

---

### 3.38 Indirect Group Paging without Priority

The Indirect Group Paging without Priority message is used to notify radios of other talkgroup activity at this Site. The decision to leave the current channel and move to the control channel for specific assignment information is optional. A variable number of group IDs up to four may be signaled in this message. The message format is shown in Figure 88 below. The message length is dependent upon the number of group IDs identified.

#### Figure 88 – Indirect Group Paging without Priority

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | Reserved | | | | LEN | |
| 3 | | | | Group ID1 | | | | |
| 4 | | | | | | | | |
| 5 | | | | Group ID2 (optional) | | | | |
| 6 | | | | | | | | |
| 7 | | | | Group ID3 (optional) | | | | |
| 8 | | | | | | | | |
| 9 | | | | Group ID4 (optional) | | | | |
| 10 | | | | | | | | |

B1 = %0, B2 = %0, MCO = %010001

The LEN field determines the number of IDs present in the message.

#### Table 7 – Indirect Group Paging Message Length

| LEN | Number of Groups to be Paged | Message Length (octets) |
|---|---|---|
| %00 | 1 | 4 |
| %01 | 2 | 6 |
| %10 | 3 | 8 |
| %11 | 4 | 10 |

---

### 3.39 Power Control Signal Quality

The Power Control Signal Quality message is used for closed loop power control for an SU. The message format is shown in Figure 89 below.

#### Figure 89 – Power Control Signal Quality

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | | | | | | | | |
| 3 | | | | Target Address | | | | |
| 4 | | | | | | | | |
| 5 | | RF Level | | | | BER | | |

B1 = %0, B2 = %0, MCO = %110000

---

### 3.40 MAC_Release

The MAC_Release message is used on the outbound SACCH. It is used for signaling a keyed unit that some sort of call preemption scenario is occurring. The preemption may be forced or unforced as indicated by the U/F field. The message format is shown in Figure 90 below.

#### Figure 90 – MAC Release

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | U/F | C/A | | | Reserved | | | |
| 3 | | | | | | | | |
| 4 | | | | Target Address | | | | |
| 5 | | | | | | | | |
| 6 | | | Reserved | | | | | |
| 7 | | | | Color Code | | | | |

B1 = %0, B2 = %0, MCO = %110001

---

### 3.41 Status Update

The Status Update message is the echo of the status update from a subscriber unit when the destination of the update is another subscriber unit via control channel signaling. The abbreviated format is shown in Figure 91 below and the extended format for the VCH is shown in Figure 92 below. The extended format for the LCCH is a multi-fragment message. The format for the first fragment is shown in Figure 93 below and the format for the second fragment is shown in Figure 94 below.

The abbreviated format is used when the message is destined for the Source SU, the Source SU is active on its Home system, and the Source SU and Target SU have the same Home system. The abbreviated format is also used when the message is destined for the Target SU, the Target SU is active on its Home system, and the Source SU and Target SU have the same Home system. The extended format is used when the abbreviated format is not appropriate.

#### Figure 91 – Status Update - Abbreviated

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 10 | | | |
| 3 | | | | Status | | | | |
| 4 | | | | | | | | |
| 5 | | | | | | | | |
| 6 | | | | Target Address | | | | |
| 7 | | | | | | | | |
| 8 | | | | | | | | |
| 9 | | | | Source Address | | | | |
| 10 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %011000

#### Figure 92 – Status Update - Extended VCH

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 14 | | | |
| 3 | | | | Status | | | | |
| 4 | | | | | | | | |
| 5 | | | | | | | | |
| 6 | | | | Target Address | | | | |
| 7 | | | | | | | | |
| 8 | | | | | | | | |
| 9 | | | | Source SUID | | | | |
| ⁞ | | | | | | | | |
| 13 | | | | | | | | |
| 14 | | | | | | | | |

B1 = %1, B2 = %1, MCO = %011000

#### Figure 93 – Status Update - Extended LCCH (1 of 2)

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 18 | | | |
| 3 | | | Data Length = 25 | | | | | |
| 4 | | | | Status | | | | |
| 5 | | | | | | | | |
| 6 | | | | | | | | |
| 7 | | | | Target Address | | | | |
| 8 | | | | | | | | |
| 9 | | | | | | | | |
| 10 | | | | Source SUID | | | | |
| ⁞ | | | | | | | | |
| 14 | | | | | | | | |
| 15 | | | | | | | | |
| 16 | | | | | | | | |
| 17 | | | | Source Address | | | | |
| 18 | | | | | | | | |

B1 = %1, B2 = %1, MCO = %011001

#### Figure 94 – Status Update - Extended LCCH (2 of 2)

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 11 | | | |
| 3 | | | | | | | | |
| 4 | | | | Target SUID | | | | |
| ⁞ | | | | | | | | |
| 8 | | | | | | | | |
| 9 | | | | | | | | |
| 10 | | | | Multi-Fragment CRC-16 | | | | |
| 11 | | | | | | | | |

B1 = %0, B2 = %0, MCO = %010000

---

### 3.42 Message Update

The Message Update message is the echo of the short data message from a subscriber unit when the destination of the message is another subscriber unit via control channel signaling. The abbreviated format is shown in Figure 95 below and the extended format for the VCH is shown in Figure 96 below. The extended format for the LCCH is a multi-fragment message. The format for the first fragment is shown in Figure 97 below and the format for the second fragment is shown in Figure 98 below.

The abbreviated format is used when the message is destined for the Source SU, the Source SU is active on its Home system, and the Source SU and Target SU have the same Home system. The abbreviated format is also used when the message is destined for the Target SU, the Target SU is active on its Home system, and the Source SU and Target SU have the same Home system. The extended format is used when the abbreviated format is not appropriate.

#### Figure 95 – Message Update - Abbreviated

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 10 | | | |
| 3 | | | | | | | | |
| 4 | | | | Message | | | | |
| 5 | | | | | | | | |
| 6 | | | | Target Address | | | | |
| 7 | | | | | | | | |
| 8 | | | | | | | | |
| 9 | | | | Source Address | | | | |
| 10 | | | | | | | | |

B1 = %0, B2 = %1, MCO = %011100

#### Figure 96 – Message Update - Extended VCH

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 14 | | | |
| 3 | | | | | | | | |
| 4 | | | | Message | | | | |
| 5 | | | | | | | | |
| 6 | | | | Target Address | | | | |
| 7 | | | | | | | | |
| 8 | | | | | | | | |
| 9 | | | | Source SUID | | | | |
| ⁞ | | | | | | | | |
| 13 | | | | | | | | |
| 14 | | | | | | | | |

B1 = %1, B2 = %1, MCO = %011100

#### Figure 97 – Message Update - Extended LCCH (1 of 2)

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 18 | | | |
| 3 | | | Data Length = 25 | | | | | |
| 4 | | | | | | | | |
| 5 | | | | Message | | | | |
| 6 | | | | | | | | |
| 7 | | | | Target Address | | | | |
| 8 | | | | | | | | |
| 9 | | | | | | | | |
| 10 | | | | Source SUID | | | | |
| ⁞ | | | | | | | | |
| 14 | | | | | | | | |
| 15 | | | | | | | | |
| 16 | | | | | | | | |
| 17 | | | | Source Address | | | | |
| 18 | | | | | | | | |

B1 = %1, B2 = %1, MCO = %001110

#### Figure 98 – Message Update - Extended LCCH (2 of 2)

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|---|---|---|---|---|---|---|---|---|
| 1 | B1 | B2 | | | MCO | | | |
| 2 | Reserved | | | | Length = 11 | | | |
| 3 | | | | | | | | |
| 4 | | | | Target SUID | | | | |
| ⁞ | | | | | | | | |
| 8 | | | | | | | | |
| 9 | | | | | | | | |
| 10 | | | | Multi-Fragment CRC-16 | | | | |
| 11 | | | | | | | | |

B1 = %0, B2 = %0, MCO = %010000
