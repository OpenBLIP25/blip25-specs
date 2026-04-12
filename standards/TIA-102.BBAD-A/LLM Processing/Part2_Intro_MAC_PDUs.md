# TIA-102.BBAD-A

## 1. Introduction

The objective of this document is to specify the Media Access Control (MAC) layer messages for the two-slot Time Division Multiple Access (TDMA) Common Air Interface (CAI) within a 12.5 kHz bandwidth physical radio channel. An overview of the two-slot TDMA CAI is provided in [8] and is referred to as the U<sub>m2</sub> interface as described in [7].

### 1.1 Scope

This document specifies the MAC layer protocol data units and messages for logical channels on the U<sub>m2</sub> interface.

The information necessary to enable an interoperable MAC layer protocol for a two-slot TDMA logical channels is provided in this document or referenced in other documents as appropriate. This document assumes the reader has a good working knowledge of the two-slot TDMA document suite identified in [8] and of trunked LMR systems as described in [7].

### 1.2 References

The appearance of "Project 25" in references below indicates the TIA document has been adopted by the Project 25 Steering Committee as part of its Project 25 standard, i.e. "the Standard."

#### 1.2.1 Normative References

The following documents contain provisions that, through reference in this text, constitute provisions of this document. At the time of publication, the editions indicated were valid. All documents are subject to revision, and parties to agreements based on this document are encouraged to investigate the possibility of applying the most recent editions of the documents published by them. For dated references, only the edition cited applies. For undated references, the latest edition of the referenced document, including any addenda, applies.

- [1] ANSI/TIA-102.AAAD-B, Project 25 Digital Land Mobile Radio Block Encryption Protocol
- [2] ANSI/TIA-102.AABC-E, Project 25 Trunking Control Channel Messages
- [3] ANSI/TIA-102.BAAA-B, Project 25 FDMA Common Air Interface
- [4] TIA-102.AABH, Project 25 Dynamic Regrouping Messages and Procedures
- [5] TIA-102.BBAC-A, Project 25 Two-Slot TDMA MAC Layer Specification
- [6] TIA-102.BBAE, Project 25 Two-Slot TDMA MAC Layer Procedures

#### 1.2.2 Informative References

- [7] TIA/TSB-102-C, Project 25 TIA-102 Documentation Suite Overview
- [8] TIA/TSB-102.BBAA Project 25 Two-Slot TDMA Overview

### 1.3 Acronyms and Abbreviations

| Acronym | Definition |
|---------|-----------|
| ALGID | Algorithm Identifier |
| ANSI | American National Standards Institute |
| BER | Bit Error Rate |
| CAI | Common Air Interface |
| CCH | Control Channel |
| CRC | Cyclic Redundancy Check |
| DCH | Data Channel |
| ETDU | Expanded Terminator Data Unit |
| FACCH | Fast Associated Control Channel |
| FDMA | Frequency Division Multiple Access |
| FNE | Fixed Network Equipment |
| HDU | Header Data Unit |
| ID | Identifier |
| IECI | Inbound Encoded Control Information |
| IEMI | Inbound Encoded MAC Information |
| ISP | Inbound Signaling Packet |
| I-OEMI | Information OEMI |
| LCH | Logical Channel |
| LSB | Least Significant Bit |
| MAC | Media Access Control |
| MCO | MAC Protocol Data Unit Opcode |
| MFID | Manufacturers ID |
| MI | Message Indicator |
| MSB | Most Significant Bit |
| NAC | Network Access Code |
| OECI | Outbound Encoded Control Information |
| OEMI | Outbound Encoded MAC Information |
| OSP | Outbound Signaling Packet |
| PDU | Protocol Data Unit |
| PTT | Push-To-Talk |
| RS | Random Seed |
| SACCH | Slow Associated Control Channel |
| SGID | Subscriber Group Identification |
| SNDCP | Sub-Network Dependent Convergence Protocol |
| SU | Subscriber Unit |
| SUID | Subscriber Unit Identification |
| S-OEMI | Synchronization OEMI |
| TDMA | Time Division Multiple Access |
| TIA | Telecommunications Industry Association |
| TSBK | Trunking Signaling Block |
| UID | Unit ID |
| VCH | Voice Channel |
| VCU | Voice Channel User |
| VTCH | Voice Transport Channel |
| WACN | Wide Area Communications Network |
| WUID | Working Unit ID |

### 1.4 Definitions

| Term | Definition |
|------|-----------|
| ALGID | See definition in [1] |
| Burst | See definition in [5] |
| FACCH | See definition in [5] |
| Inbound | See definition in [5] |
| Key ID | See definition in [1] |
| LCH | See definition in [5] |
| MI | See definition in [1] |
| Octet | 8 bits grouped together, also called a byte |
| Outbound | See definition in [5] |
| SACCH | See definition in [5] |
| U<sub>m2</sub> Interface | See definition in [7] |
| VCH | See definition in [5] |
| Voice Frame | See definition in [5] |

### 1.5 Conventions

The following conventions are utilized in this document.

#### 1.5.1 Message Bit Numbering

Various figures in this document depict PDUs or message formats. These figures show one octet per row with the left-most bit being the most significant bit and labeled bit 7, and the right-most bit being the least significant bit and labeled bit 0. The octets are transmitted in the order they read on the page, namely from left to right and from top to bottom. This can be the cause for some confusion when compared to IETF RFC documents since the most significant bit is labeled bit 0 in those documents.

#### 1.5.2 PDU and Message Formats

There are several PDU and message formats defined in this document. The upper left corner of the PDU and message format figures is labeled "O/B". "O" refers to the octet number down the left side of the figure and "B" refers to the bit number across the top of the figure.

#### 1.5.3 Symbols

If a number is preceded by a "%", the number is interpreted as a binary representation with the appropriate number of symbols. If a number is preceded by a "0x", the number is interpreted as a hexadecimal representation with the appropriate number of symbols. If a number is presented with neither a "%" nor a "0x" preceding it, the number is considered as a decimal representation with the appropriate number of symbols, unless stated otherwise in the text description. Representative examples follow:

- %1010 is 4 symbol binary for decimal 10
- 0x0A is 2 symbol hexadecimal for decimal 10
- 15 is decimal 15

---

## 2. MAC PDUs

Signaling is conveyed in a MAC signaling burst. The MAC signaling bursts for the two-slot TDMA CAI logical channels are specified in [5].

LCCH signaling is conveyed in the IECI signaling burst type on the inbound LCCH and in the OECI signaling burst type on the outbound LCCH.

VCH signaling is conveyed in the IEMI signaling burst type on the inbound VCH and in either the S-OEMI or I-OEMI signaling burst type on the outbound VCH.

The payload of a MAC signaling burst is a MAC PDU. This section specifies the MAC PDUs utilized on the LCCH and VCH.

### 2.1 MAC PDU Structure

All MAC PDUs begin with a MAC PDU Header and end with a MAC PDU Trailer. The octets between the MAC PDU Header and MAC PDU Trailer depend on the value of the Opcode field in the MAC PDU Header. The Opcode field values are defined in 4.1 below.

Some MAC PDUs carry variable content while others carry fixed content. The general structure of a MAC PDU that carries variable or fixed content is shown in Figure 1 below.

**Figure 1 – General MAC PDU Structure**

```
 Variable Content PDUs          MAC PDU Header         Fixed Content PDUs
 ┌──────────────────┐        ┌──────────────────┐      ┌──────────────────┐
 │ B1  B2    MCO    │        │                  │      │                  │
 │                  │───────►│   MAC PDU         │◄─────│                  │
 │ Generic          │        │   Contents        │      │  Pre-defined     │
 │ Information      │        │                  │      │  Information     │
 └──────────────────┘        │                  │      └──────────────────┘
                             └──────────────────┘
                              MAC PDU Trailer
```

The MAC PDU Header is common for the LCCH and VCH burst types. The 8-bit MAC PDU Header comprises 3-bit Opcode, 3-bit Offset, 1-bit Reserved (Res), and 1-bit Protected (P) fields as shown in Figure 2 below.

**Figure 2 – MAC PDU Header Format**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 0 | Opcode [7:5] ||| Offset [4:2] ||| Res | P |

The size and content of the MAC PDU Trailer is dependent on the burst type. The 16-bit CRC is denoted as CRC-16 and the 12-bit CRC is denoted as CRC-12 in the MAC PDU Trailer formats that follow.

The 28-bit MAC PDU Trailer for the OECI burst type on the outbound LCCH comprises 12-bit Color Code and 16-bit CRC fields as shown in Figure 3 below.

**Figure 3 – Outbound LCCH MAC PDU Trailer Format**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 19 | Color Code [7:0] ||||||||
| 20 | Color Code [11:8] |||| CRC-16 [15:12] ||||
| 21 | CRC-16 [11:4] ||||||||
| 22 | CRC-16 [3:0] |||| — |||| |

> Color Code: 12 bits (octets 19–20). CRC-16: 16 bits (octets 20–22, half-octet boundary).

The MAC PDU Trailer for the IECI burst type on the inbound LCCH is a 16-bit CRC field as shown in Figure 4 below.

**Figure 4 – Inbound LCCH MAC PDU Trailer Format**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 15 | CRC-16 [15:8] ||||||||
| 16 | CRC-16 [7:0] ||||||||

The MAC PDU Trailer for the I-OEMI, S-OEMI, and IEMI burst types on the VCH is a 12-bit CRC field as shown in Figure 5 below. The octet numbers for this MAC PDU Trailer are dependent on the burst type and are shown in 2.3 below.

**Figure 5 – VCH MAC PDU Trailer Format**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| \* | CRC-12 [11:4] ||||||||
| \* | CRC-12 [3:0] |||| — |||| |

> CRC-12: 12 bits. Octet numbers (\*) vary by burst type.

All reserved bits are set to zero unless otherwise specified. All MAC PDU fields are defined in section 4 below.

### 2.2 LCCH PDUs

The LCCH PDU formats and definitions are specified in the following subclauses.

All LCCH PDUs end with a CRC-16 which is defined in 4.7 below.

#### 2.2.1 Outbound LCCH PDU

The outbound LCCH PDU is carried by the OECI, an outbound signaling burst without synchronization having a length of 22.5 octets. The outbound LCCH OECI PDU format is shown in Figure 6 below. The MAC PDU Header is shown in Figure 2 above and the Outbound LCCH MAC PDU Trailer is shown in Figure 3 above.

**Figure 6 – OECI PDU Format**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 0 | MAC PDU Header ||||||||
| 1 | MAC PDU Contents ||||||||
| 2 | (continued) ||||||||
| ⁞ | (continued) ||||||||
| 17 | (continued) ||||||||
| 18 | (continued) ||||||||
| 19 | Outbound LCCH MAC PDU Trailer ||||||||
| 20 | (continued) ||||||||
| 21 | (continued) ||||||||
| 22 | (continued, half-octet) ||||||||

#### 2.2.2 Inbound LCCH PDU

The inbound LCCH PDU is carried by the IECI, an inbound signaling burst with synchronization having a length of 17 octets. The inbound LCCH IECI PDU format is shown in Figure 7 below. The MAC PDU Header is shown in Figure 2 above and the Inbound LCCH MAC PDU Trailer is shown in Figure 4 above.

**Figure 7 – IECI PDU Format**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 0 | MAC PDU Header ||||||||
| 1 | MAC PDU Contents ||||||||
| 2 | (continued) ||||||||
| ⁞ | (continued) ||||||||
| 13 | (continued) ||||||||
| 14 | (continued) ||||||||
| 15 | Inbound LCCH MAC PDU Trailer ||||||||
| 16 | (continued) ||||||||

#### 2.2.3 LCCH PDU Definitions

All of the LCCH MAC PDUs utilize a CRC-16 to validate the PDU content.

The only PDU defined for the LCCH is the MAC_SIGNAL PDU. The first octet is the common MAC PDU Header and the second octet has two bits designated B1 and B2 and a 6-bit MCO which indicate the contents of the remaining octets of the message.

The general inbound LCCH MAC_SIGNAL PDU format for the IECI is shown in Figure 8 below.

**Figure 8 – General IECI MAC_SIGNAL PDU Format**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 0 | MAC PDU Header ||||||||
| 1 | B1 | B2 | MCO #1 [5:0] ||||||
| 2 | MAC Message #1 ||||||||
| ⁞ | (continued) ||||||||
| … | B1 | B2 | MCO #2 [5:0] ||||||
|  | MAC Message #2 ||||||||
| ⁞ | (continued) ||||||||
| 14 | (continued) ||||||||
| 15 | Inbound LCCH MAC PDU Trailer ||||||||
| 16 | (continued) ||||||||

The general outbound LCCH MAC_SIGNAL PDU format for the OECI is shown in Figure 9 below.

**Figure 9 – General OECI MAC_SIGNAL PDU Format**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 0 | MAC PDU Header ||||||||
| 1 | B1 | B2 | MCO #1 [5:0] ||||||
| 2 | MAC Message #1 ||||||||
| ⁞ | (continued) ||||||||
| … | B1 | B2 | MCO #2 [5:0] ||||||
|  | MAC Message #2 ||||||||
| ⁞ | (continued) ||||||||
| 18 | (continued) ||||||||
| 19 | Outbound LCCH MAC PDU Trailer ||||||||
| 20 | (continued) ||||||||
| 21 | (continued) ||||||||
| 22 | (continued, half-octet) ||||||||

The majority of the generic messages required for LCCH signaling are already defined in [2]; however, due to limited bandwidth on the LCCH, an abbreviated version of these messages is appropriate. Specifically, these abbreviated messages were constructed without the Manufacturer's ID (MFID), CRC, and some reserved fields. The CRC in each of the Outbound Signaling Packet / Inbound Signaling Packet (OSP/ISP) messages defined in [2] is replaced with the CRC-16 over the entire PDU as illustrated in Figure 8 and Figure 9 above.

> **Note:** The MFID concept is still supported by MAC messages; it is done in a different manner per section 3 below.

Multiple MAC messages may be conveyed within any given MAC_SIGNAL PDU as message size allows, with any unused space within the PDU Contents being filled as follows:

- If there is one octet of unused space in the MAC_SIGNAL PDU, it is filled using the Null Information message
- If there is more than one octet of unused space in the MAC_SIGNAL PDU, it is filled using the:
  - Null Information message if the MAC signaling burst is scrambled
  - Null Avoid Zero Bias Information message if the MAC signaling burst is not scrambled

If multiple MAC messages are conveyed in an IECI MAC_SIGNAL PDU, only one of the messages shall be a request that expects a response. There are no restrictions regarding multiple MAC messages conveyed in an OECI MAC_SIGNAL PDU.

Table 1 below lists all MAC Messages that may be conveyed on the LCCH by category, indicates whether they are inbound or outbound signaling messages, and provides a reference to the subclause where the message is defined. For messages with multiple formats (e.g. abbreviated/extended), all formats are permitted unless a specific format is specified in Table 1 below.

**Table 1 – LCCH MAC Messages**

| Category | Name | Inbound | Outbound | Clause |
|----------|------|---------|----------|--------|
| Voice Service | Group Voice Service Request | X | | 3.25 |
| Voice Service | Unit to Unit Voice Service Request | X | | 3.5 |
| Voice Service | Unit to Unit Voice Service Answer Response | X | | 3.55 |
| Voice Service | Telephone Interconnect Request Explicit Dialing | X | | 3.56 |
| Voice Service | Telephone Interconnect Request Implicit Dialing | X | | 3.57 |
| Voice Service | Telephone Interconnect Answer Response | X | | 3.58 |
| Voice Service | Group Voice Channel Grant | | X | 3.7 |
| Voice Service | Group Voice Channel Grant Update | | X | 3.8 |
| Voice Service | Unit to Unit Answer Request | | X | 3.10 |
| Voice Service | Unit to Unit Voice Service Channel Grant - Abbreviated | | X | 3.9 |
| Voice Service | Unit to Unit Voice Service Channel Grant - Extended LCCH | | X | 3.9 |
| Voice Service | Telephone Interconnect Voice Channel Grant | | X | 3.44 |
| Voice Service | Telephone Interconnect Answer Request | | X | 3.12 |
| Voice Service | Unit to Unit Voice Channel Grant Update - Abbreviated | | X | 3.13 |
| Voice Service | Unit to Unit Voice Channel Grant Update - Extended LCCH | | X | 3.13 |
| Voice Service | Telephone Interconnect Voice Channel Grant Update | | X | 3.45 |
| Data Service | SNDCP Data Channel Request | X | | 3.59 |
| Data Service | SNDCP Data Page Response | X | | 3.60 |
| Data Service | SNDCP Reconnect Request | X | | 3.61 |
| Data Service | SNDCP Data Channel Grant | | X | 3.15 |
| Data Service | SNDCP Data Page Request | | X | 3.16 |
| Data Service | SNDCP Data Channel Announcement - Explicit | | X | 3.17 |
| Control and Status Service | Acknowledge Response Unit | X | | 3.62 |
| Control and Status Service | Call Alert Request | X | | 3.63 |
| Control and Status Service | Cancel Service Request | X | | 3.64 |
| Control and Status Service | Emergency Alarm Request | X | | 3.65 |
| Control and Status Service | Extended Function Response | X | | 3.66 |
| Control and Status Service | Group Affiliation Query Response | X | | 3.67 |
| Control and Status Service | Group Affiliation Request | X | | 3.68 |
| Control and Status Service | Identifier Update Request | X | | 3.69 |
| Control and Status Service | Message Update Request | X | | 3.70 |
| Control and Status Service | Status Query Request | X | | 3.71 |
| Control and Status Service | Status Query Response | X | | 3.72 |
| Control and Status Service | Status Update Request | X | | 3.73 |
| Control and Status Service | Unit Registration Request | X | | 3.74 |
| Control and Status Service | Unit Deregistration Request | X | | 3.75 |
| Control and Status Service | Location Registration Request | X | | 3.76 |
| Control and Status Service | Radio Unit Monitor Request | X | | 3.77 |
| Control and Status Service | Roaming Address Request | X | | 3.78 |
| Control and Status Service | Roaming Address Response | X | | 3.79 |
| Control and Status Service | Authentication FNE Result | X | | 3.80 |
| Control and Status Service | Authentication Response | X | | 3.81 |
| Control and Status Service | Authentication Response Mutual | X | | 3.82 |
| Control and Status Service | Authentication SU Demand | X | | 3.83 |
| Control and Status Service | Radio Unit Monitor Enhanced Request | X | | 3.84 |
| Control and Status Service | Acknowledge Response FNE | | X | 3.14 |
| Control and Status Service | Adjacent Status Broadcast | | X | 3.18 |
| Control and Status Service | Call Alert - Abbreviated | | X | 3.19 |
| Control and Status Service | Call Alert - Extended LCCH | | X | 3.19 |
| Control and Status Service | Deny Response | | X | 3.30 |
| Control and Status Service | Extended Function Command - Abbreviated | | X | 3.20 |
| Control and Status Service | Extended Function Command - Extended LCCH | | X | 3.20 |
| Control and Status Service | Group Affiliation Query - Abbreviated | | X | 3.21 |
| Control and Status Service | Group Affiliation Response | | X | 3.46 |
| Control and Status Service | Identifier Update | | X | 3.22 |
| Control and Status Service | Message Update - Abbreviated | | X | 3.42 |
| Control and Status Service | Message Update - Extended LCCH | | X | 3.42 |
| Control and Status Service | Network Status Broadcast | | X | 3.24 |
| Control and Status Service | Queued Response | | X | 3.29 |
| Control and Status Service | RFSS Status Broadcast | | X | 3.26 |
| Control and Status Service | Secondary Control Channel Broadcast | | X | 3.27 |
| Control and Status Service | Status Query - Abbreviated | | X | 3.28 |
| Control and Status Service | Status Query - Extended LCCH | | X | 3.28 |
| Control and Status Service | Status Update - Abbreviated | | X | 3.41 |
| Control and Status Service | Status Update - Extended LCCH | | X | 3.41 |
| Control and Status Service | System Service Broadcast | | X | 3.31 |
| Control and Status Service | Unit Registration Command | | X | 3.32 |
| Control and Status Service | Unit Registration Response | | X | 3.47 |
| Control and Status Service | Unit Deregistration Acknowledge | | X | 3.48 |
| Control and Status Service | Location Registration Response | | X | 3.49 |
| Control and Status Service | Radio Unit Monitor Command - Abbreviated | | X | 3.43 |
| Control and Status Service | Radio Unit Monitor Command - Extended LCCH | | X | 3.43 |
| Control and Status Service | Roaming Address Command | | X | 3.50 |
| Control and Status Service | Roaming Address Update | | X | 3.51 |
| Control and Status Service | Time and Date Announcement | | X | 3.23 |
| Control and Status Service | Identifier Update for VHF/UHF Bands | | X | 3.34 |
| Control and Status Service | Authentication Demand | | X | 3.52 |
| Control and Status Service | Authentication FNE Response | | X | 3.53 |
| Control and Status Service | Identifier Update for TDMA | | X | 3.35 |
| Control and Status Service | Synchronization Broadcast | | X | 3.54 |
| Control and Status Service | Radio Unit Monitor Enhanced Command | | X | 3.11 |
| MAC Specific | Null Information | X | X | 3.1 |
| MAC Specific | Manufacturer Specific | X | X | 3.36 |
| MAC Specific | Null Avoid Zero Bias Information | X | X | 3.85 |
| Dynamic Regrouping | MFID90 Group Regroup Voice Request | X | | 3.89 |
| Dynamic Regrouping | MFID90 Extended Function Response | X | | 3.90 |
| Dynamic Regrouping | MFIDA4 Group Regroup Explicit Encryption Command | | X | 3.91 |
| Dynamic Regrouping | MFID90 Group Regroup Add Command | | X | 3.92 |
| Dynamic Regrouping | MFID90 Group Regroup Delete Command | | X | 3.93 |
| Dynamic Regrouping | MFID90 Group Regroup Channel Grant | | X | 3.94 |
| Dynamic Regrouping | MFID90 Group Regroup Channel Update | | X | 3.95 |
| Dynamic Regrouping | MFID90 Extended Function Command | | X | 3.88 |
| Dynamic Regrouping | MFID90 Queued Response | | X | 3.96 |
| Dynamic Regrouping | MFID90 Deny Response | | X | 3.97 |
| Dynamic Regrouping | MFID90 Acknowledge Response | | X | 3.98 |

### 2.3 VCH PDUs

The VCH PDU formats and definitions are specified in the following subclauses.

All VCH PDUs include the MAC PDU Header shown in Figure 2 above and the VCH MAC PDU Trailer shown in Figure 5 above, and end with a CRC-12 which is defined in 4.6 below.

The MAC_PTT PDU and the MAC_END_PTT PDU are fixed content PDUs that carry pre-defined information fields specific to beginning and ending calls. The MAC_ACTIVE PDU, MAC_HANGTIME PDU, and MAC_IDLE PDU are variable content PDUs that carry generic information fields. The PDU type itself conveys information about the state of the VCH and the PDU contents contain one or more generic MAC messages.

#### 2.3.1 Outbound VCH PDU

There are two types of outbound VCH PDUs. The first type of outbound VCH PDU is carried by the I-OEMI, an outbound signaling burst without synchronization having a length of 22.5 octets. The second type of outbound VCH PDU is carried by the S-OEMI, an outbound signaling burst with synchronization having a length of 19.5 octets. The outbound VCH I-OEMI PDU format is shown in Figure 10 below and the outbound VCH S-OEMI PDU format shown in Figure 11 below.

**Figure 10 – I-OEMI PDU Format**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 0 | MAC PDU Header ||||||||
| 1 | MAC PDU Contents ||||||||
| 2 | (continued) ||||||||
| ⁞ | (continued) ||||||||
| 19 | (continued) ||||||||
| 20 | (continued) ||||||||
| 21 | VCH MAC PDU Trailer ||||||||
| 22 | (continued, half-octet) ||||||||

**Figure 11 – S-OEMI and IEMI PDU Format**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 0 | MAC PDU Header ||||||||
| 1 | MAC PDU Contents ||||||||
| 2 | (continued) ||||||||
| ⁞ | (continued) ||||||||
| 16 | (continued) ||||||||
| 17 | (continued) ||||||||
| 18 | VCH MAC PDU Trailer ||||||||
| 19 | (continued, half-octet) ||||||||

#### 2.3.2 Inbound VCH PDU

The inbound VCH PDU is carried by the IEMI, an inbound signaling burst with synchronization having a length of 19.5 octets. The inbound VCH IEMI PDU format is shown in Figure 11 above.

#### 2.3.3 VCH PDU Definitions

All of the VCH MAC PDUs utilize a CRC-12 to validate the PDU content.

The PDUs defined for the VCH are specified in the following subclauses.

##### 2.3.3.1 VCH MAC_PTT PDU

The VCH MAC_PTT PDUs are the equivalent of the Header Data Unit (HDU) in the FDMA CAI specified in [3]. They provide the Message Indicator (MI), Algorithm ID (ALGID), and Key ID for encrypted voice calls as well as the source and target (Group) addresses.

The MAC_PTT PDU format for the IEMI and MAC_PTT PDU format for the S-OEMI is shown in Figure 12 below.

**Figure 12 – IEMI and S-OEMI MAC_PTT PDU Format**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 0 | MAC PDU Header ||||||||
| 1 | Message Indicator (MI) ||||||||
| 2 | (continued) ||||||||
| ⁞ | (continued) ||||||||
| 8 | (continued) ||||||||
| 9 | (continued) ||||||||
| 10 | ALGID ||||||||
| 11 | Key ID ||||||||
| 12 | (continued) ||||||||
| 13 | Source Address ||||||||
| 14 | (continued) ||||||||
| 15 | (continued) ||||||||
| 16 | Group Address ||||||||
| 17 | (continued) ||||||||
| 18 | VCH MAC PDU Trailer ||||||||
| 19 | (continued, half-octet) ||||||||

> MI: 72 bits (octets 1–9). ALGID: 8 bits (octet 10). Key ID: 16 bits (octets 11–12). Source Address: 24 bits (octets 13–15). Group Address: 16 bits (octets 16–17). CRC-12 trailer: 12 bits (octets 18–19).

The MAC_PTT PDU format for the I-OEMI is shown in Figure 13 below.

**Figure 13 – I-OEMI MAC_PTT PDU Format**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 0 | MAC PDU Header ||||||||
| 1 | Message Indicator (MI) ||||||||
| 2 | (continued) ||||||||
| ⁞ | (continued) ||||||||
| 8 | (continued) ||||||||
| 9 | (continued) ||||||||
| 10 | ALGID ||||||||
| 11 | Key ID ||||||||
| 12 | (continued) ||||||||
| 13 | Source Address ||||||||
| 14 | (continued) ||||||||
| 15 | (continued) ||||||||
| 16 | Group Address ||||||||
| 17 | (continued) ||||||||
| 18 | Reserved ||||||||
| 19 | (continued) ||||||||
| 20 | (continued) ||||||||
| 21 | VCH MAC PDU Trailer ||||||||
| 22 | (continued, half-octet) ||||||||

> MI: 72 bits (octets 1–9). ALGID: 8 bits (octet 10). Key ID: 16 bits (octets 11–12). Source Address: 24 bits (octets 13–15). Group Address: 16 bits (octets 16–17). Reserved: 24 bits (octets 18–20). CRC-12 trailer: 12 bits (octets 21–22).

##### 2.3.3.2 VCH MAC_END_PTT PDU

The VCH MAC_END_PTT PDUs are the equivalent of the Expanded Terminator Data Unit (ETDU) in the FDMA CAI specified in [3]. They provide the source and group addresses of the caller that has released its PTT. They are conveyed on the FACCH and optionally on a SACCH in a manner similar to the MAC_PTT PDUs.

The MAC_END_PTT PDU format for the IEMI and MAC_END_PTT PDU format for the S-OEMI is shown in Figure 14 below.

**Figure 14 – IEMI and S-OEMI MAC_END_PTT PDU Format**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 0 | MAC PDU Header ||||||||
| 1 | Reserved [7:4] |||| Color Code [11:8] ||||
| 2 | Color Code [7:0] ||||||||
| 3 | Reserved ||||||||
| 4 | (continued) ||||||||
| ⁞ | (continued) ||||||||
| 11 | (continued) ||||||||
| 12 | (continued) ||||||||
| 13 | Source Address ||||||||
| 14 | (continued) ||||||||
| 15 | (continued) ||||||||
| 16 | Group Address ||||||||
| 17 | (continued) ||||||||
| 18 | VCH MAC PDU Trailer ||||||||
| 19 | (continued, half-octet) ||||||||

> Reserved (upper nibble octet 1): 4 bits. Color Code: 12 bits (octet 1 lower nibble + octet 2). Reserved: octets 3–12 (80 bits). Source Address: 24 bits (octets 13–15). Group Address: 16 bits (octets 16–17). CRC-12 trailer: 12 bits (octets 18–19).

The MAC_END_PTT PDU format for the I-OEMI is shown in Figure 15 below.

**Figure 15 – I-OEMI MAC_END_PTT PDU Format**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 0 | MAC PDU Header ||||||||
| 1 | Reserved [7:4] |||| Color Code [11:8] ||||
| 2 | Color Code [7:0] ||||||||
| 3 | Reserved ||||||||
| 4 | (continued) ||||||||
| ⁞ | (continued) ||||||||
| 11 | (continued) ||||||||
| 12 | (continued) ||||||||
| 13 | Source Address ||||||||
| 14 | (continued) ||||||||
| 15 | (continued) ||||||||
| 16 | Group Address ||||||||
| 17 | (continued) ||||||||
| 18 | Reserved ||||||||
| 19 | (continued) ||||||||
| 20 | (continued) ||||||||
| 21 | VCH MAC PDU Trailer ||||||||
| 22 | (continued, half-octet) ||||||||

> Reserved (upper nibble octet 1): 4 bits. Color Code: 12 bits (octet 1 lower nibble + octet 2). Reserved: octets 3–12 (80 bits). Source Address: 24 bits (octets 13–15). Group Address: 16 bits (octets 16–17). Reserved: 24 bits (octets 18–20). CRC-12 trailer: 12 bits (octets 21–22).

##### 2.3.3.3 VCH MAC_IDLE, MAC_ACTIVE, & MAC_HANGTIME PDU

The general structure of the MAC_IDLE PDU, MAC_ACTIVE PDU, and MAC_HANGTIME PDU is shown in Figure 16 and Figure 17 below. The first octet is the MAC PDU Header and the second octet has two bits designated B1 and B2 and a 6-bit MCO which indicate the contents of the remaining octets of the message.

The general MAC PDU format for the I-OEMI is shown in Figure 16 below.

**Figure 16 – General I-OEMI MAC PDU Format**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 0 | MAC PDU Header ||||||||
| 1 | B1 | B2 | MCO #1 [5:0] ||||||
| 2 | MAC Message #1 ||||||||
| ⁞ | (continued) ||||||||
| … | B1 | B2 | MCO #2 [5:0] ||||||
|  | MAC Message #2 ||||||||
| ⁞ | (continued) ||||||||
| 20 | (continued) ||||||||
| 21 | VCH MAC PDU Trailer ||||||||
| 22 | (continued, half-octet) ||||||||

The general MAC PDU format for the S-OEMI and IEMI is shown in Figure 17 below.

**Figure 17 – General S-OEMI and IEMI MAC PDU Format**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 0 | MAC PDU Header ||||||||
| 1 | B1 | B2 | MCO #1 [5:0] ||||||
| 2 | MAC Message #1 ||||||||
| ⁞ | (continued) ||||||||
| … | B1 | B2 | MCO #2 [5:0] ||||||
|  | MAC Message #2 ||||||||
| ⁞ | (continued) ||||||||
| 17 | (continued) ||||||||
| 18 | VCH MAC PDU Trailer ||||||||
| 19 | (continued, half-octet) ||||||||

The majority of the generic messages required for VCH signaling are already defined in [2]; however, due to limited bandwidth on the VCH, an abbreviated version of these messages is appropriate. Specifically, these abbreviated messages were constructed without the Manufacturer's ID (MFID), CRC, and some reserved fields. The CRC in each of the Outbound Signaling Packet / Inbound Signaling Packet (OSP/ISP) messages defined in [2] is replaced with the CRC-12 over the entire PDU as illustrated in Figure 16 and Figure 17 above.

> **Note:** The MFID concept is still supported by MAC messages; it is done in a different manner per section 3 below.
