# TIA-102.BBAD-A — Part 4: MAC Messages (Continued)

## 3.43 Radio Unit Monitor Command

The Radio Unit Monitor Command message is used to command a radio to execute a radio unit monitor operation. The abbreviated format is shown in Figure 99 below and the extended format for the VCH is shown in Figure 100 below. The extended format for the LCCH is a multi-fragment message. The format for the first fragment is shown in Figure 101 below and the format for the second fragment is shown in Figure 102 below.

The abbreviated format is used when the message is destined for the Source SU, the Source SU is active on its Home system, and the Source SU and Target SU have the same Home system. The abbreviated format is also used when the message is destined for the Target SU, the Target SU is active on its Home system, and the Source SU and Target SU have the same Home system. The extended format is used when the abbreviated format is not appropriate.

**Figure 99 – Radio Unit Monitor Command - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 10 | | | |
| 3   | | | | TX Time | | | | |
| 4   | SM | | | Reserved | | | TX Mult | |
| 5   | | | | | | | | |
| 6   | | | | Target Address | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | Source Address | | | | |
| 10  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %001100

**Figure 100 – Radio Unit Monitor Command - Extended VCH**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 14 | | | |
| 3   | | | | TX Time | | | | |
| 4   | SM | | | Reserved | | | TX Mult | |
| 5   | | | | | | | | |
| 6   | | | | Target Address | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | | | | | |
| ⁞   | | | | Source SUID | | | | |
| 13  | | | | | | | | |
| 14  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %001100

**Figure 101 – Radio Unit Monitor Command - Extended LCCH (1 of 2)**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 18 | | | |
| 3   | | | | Data Length = 25 | | | | |
| 4   | | | | TX Time | | | | |
| 5   | SM | | | Reserved | | | TX Mult | |
| 6   | | | | | | | | |
| 7   | | | | Target Address | | | | |
| 8   | | | | | | | | |
| 9   | | | | | | | | |
| 10  | | | | | | | | |
| ⁞   | | | | Source SUID | | | | |
| 14  | | | | | | | | |
| 15  | | | | | | | | |
| 16  | | | | | | | | |
| 17  | | | | Source Address | | | | |
| 18  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %001101

**Figure 102 – Radio Unit Monitor Command - Extended LCCH (2 of 2)**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 11 | | | |
| 3   | | | | | | | | |
| 4   | | | | | | | | |
| ⁞   | | | | Target SUID | | | | |
| 8   | | | | | | | | |
| 9   | | | | | | | | |
| 10  | | | | Multi-Fragment CRC-16 | | | | |
| 11  | | | | | | | | |

B1 = %0, B2 = %0, MCO = %010000

## 3.44 Telephone Interconnect Voice Channel Grant

The Telephone Interconnect Voice Channel Grant messages indicate the channel assignment for a telephone interconnect call between a PSTN and an individual unit of the system. The implicit format is shown in Figure 103 below and the explicit format is shown in Figure 104 below. If this grant is in response to a PSTN initiated call to an SU, then the Target Address is provided, otherwise the Source Address is provided.

**Figure 103 – Telephone Interconnect Voice Channel Grant - Implicit**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 10 | | | |
| 3   | | | | Service Options | | | | |
| 4   | | | | Channel | | | | |
| 5   | | | | | | | | |
| 6   | | | | Call Timer | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | Source/Target Address | | | | |
| 10  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %001000

**Figure 104 – Telephone Interconnect Voice Channel Grant - Explicit**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 12 | | | |
| 3   | | | | Service Options | | | | |
| 4   | | | | Channel (T) | | | | |
| 5   | | | | | | | | |
| 6   | | | | Channel (R) | | | | |
| 7   | | | | | | | | |
| 8   | | | | Call Timer | | | | |
| 9   | | | | | | | | |
| 10  | | | | | | | | |
| 11  | | | | Source/Target Address | | | | |
| 12  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %001000

## 3.45 Telephone Interconnect Voice Channel Grant Update

The Telephone Interconnect Voice Channel Grant Update messages indicate updates for a telephone interconnect call on the system and may be used to move directly to the specified channel. The implicit format is shown in Figure 105 below and the explicit format is shown in Figure 106 below. If this update is in response to a PSTN initiated call to an SU, then the Target Address is provided, otherwise the Source Address is provided.

**Figure 105 – Telephone Interconnect Voice Channel Grant Update - Implicit**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 10 | | | |
| 3   | | | | Service Options | | | | |
| 4   | | | | Channel | | | | |
| 5   | | | | | | | | |
| 6   | | | | Call Timer | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | Source/Target Address | | | | |
| 10  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %001001

**Figure 106 – Telephone Interconnect Voice Channel Grant Update - Explicit**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 12 | | | |
| 3   | | | | Service Options | | | | |
| 4   | | | | Channel (T) | | | | |
| 5   | | | | | | | | |
| 6   | | | | Channel (R) | | | | |
| 7   | | | | | | | | |
| 8   | | | | Call Timer | | | | |
| 9   | | | | | | | | |
| 10  | | | | | | | | |
| 11  | | | | Source/Target Address | | | | |
| 12  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %001001

## 3.46 Group Affiliation Response

The Group Affiliation Response messages are the response to the request for group affiliation. These messages present the necessary information to the requesting unit to allow it to perform group operations for indicated group identity. The abbreviated format is shown in Figure 107 below and the extended format is shown in Figure 108 below.

The abbreviated format may be used if the target unit is currently registered in the group's Home system. Otherwise the extended format shall be used.

**Figure 107 – Group Affiliation Response - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 10 | | | |
| 3   | LG | | | Reserved | | | GAV | |
| 4   | | | | Announcement Group Address | | | | |
| 5   | | | | | | | | |
| 6   | | | | Group Address | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | Target Address | | | | |
| 10  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %101000

**Figure 108 – Group Affiliation Response - Extended**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 16 | | | |
| 3   | LG | | | Reserved | | | GAV | |
| 4   | | | | Announcement Group Address | | | | |
| 5   | | | | | | | | |
| 6   | | | | Group Address | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | | | | | |
| ⁞   | | | | SGID | | | | |
| 12  | | | | | | | | |
| 13  | | | | | | | | |
| 14  | | | | | | | | |
| 15  | | | | Target Address | | | | |
| 16  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %101000

## 3.47 Unit Registration Response

The Unit Registration Response messages are used to respond to Unit Registration Request messages. The abbreviated format is shown in Figure 109 below and the extended format is shown in Figure 110 below.

The abbreviated format may be used if the target unit is currently registered in the group's Home system. Otherwise the extended format shall be used.

**Figure 109 – Unit Registration Response - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 10 | | | |
| 3   | Reserved | | | | RV | | | |
| 4   | | | | System ID | | | | |
| 5   | | | | | | | | |
| 6   | | | | Source ID | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | Source Address | | | | |
| 10  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %101100

**Figure 110 – Unit Registration Response - Extended**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 13 | | | |
| 3   | | | | Reserved | | | RV | |
| 4   | | | | | | | | |
| 5   | | | | | | | | |
| ⁞   | | | | Source SUID | | | | |
| 9   | | | | | | | | |
| 10  | | | | | | | | |
| 11  | | | | | | | | |
| 12  | | | | Source Address | | | | |
| 13  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %101100

## 3.48 Unit Deregistration Acknowledge

The Unit Deregistration Acknowledge message is used to acknowledge that deregistration has been accomplished by the FNE for the indicated SU identity. The message format is shown in Figure 111 below.

**Figure 111 – Unit Deregistration Acknowledge**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 9 | | | |
| 3   | | | | | | | | |
| 4   | | | | | | | | |
| ⁞   | | | | Source SUID | | | | |
| 8   | | | | | | | | |
| 9   | | | | | | | | |

B1 = %0, B2 = %1, MCO = %101111

## 3.49 Location Registration Response

The Location Registration Response message is used to respond to a Location Registration Request. The response indicates that the subscriber is registered in the new location area. The message format is shown in Figure 112 below.

**Figure 112 – Location Registration Response**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 10 | | | |
| 3   | | | | Reserved | | | RV | |
| 4   | | | | Group Address | | | | |
| 5   | | | | | | | | |
| 6   | | | | RFSS ID | | | | |
| 7   | | | | Site ID | | | | |
| 8   | | | | | | | | |
| 9   | | | | Target Address | | | | |
| 10  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %101011

## 3.50 Roaming Address Command

The Roaming Address Command message is sent from the RFSS to the SU to manipulate the Roaming Address Stack. This command can write a new WACN ID and System ID, it can selectively delete an existing WACN ID and System ID, it can read the roaming address stack contents, and it can clear all of the erasable WACN IDs and System IDs. When the Stack Operation is a Clear operation or a Read operation, the WACN ID and System ID may be null values (zeros). The receiver does not process the WACN ID and System ID values for the Clear or Read Operations. The Read operation is used to request the target SU to send the stack contents to the RFSS. The message format is shown in Figure 113 below.

**Figure 113 – Roaming Address Command**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 10 | | | |
| 3   | | | | Stack Operation | | | | |
| 4   | | | | | | | | |
| 5   | | | | | | | | |
| ⁞   | | | | Target SUID | | | | |
| 9   | | | | | | | | |
| 10  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %110110

## 3.51 Roaming Address Update

The Roaming Address Update message responds to a Roaming Address Request to list the contents of the Roaming Address Stack for the source SU to the target SU. Any number of these messages may be sent consecutively to convey the contents of the stack. The message format is shown in Figure 114 below.

**Figure 114 – Roaming Address Update**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 13 | | | |
| 3   | LM | | Reserved | | | MSN | | |
| 4   | | | | | | | | |
| 5   | | | | Target Address | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| ⁞   | | | | Source SUID | | | | |
| 12  | | | | | | | | |
| 13  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %110111

## 3.52 Authentication Demand

The Authentication Demand message is sent by the FNE to demand that the SU authenticate. The challenge (RAND1) and random seed (RS) for the addressed SU are included. When the message is sent prior to registration, the Target Address shall be set to the Registration Default. Otherwise it shall be the assigned working ID of the addressed SU. This is a multi-fragment message. The format of the first fragment is shown in Figure 115 below and the format of the second fragment is shown in Figure 116 below.

**Figure 115 – Authentication Demand (1 of 2)**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 18 | | | |
| 3   | | | | Data Length = 28 | | | | |
| 4   | | | | | | | | |
| 5   | | | | Target Address | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| ⁞   | | | | Target SUID | | | | |
| 12  | | | | | | | | |
| 13  | | | | | | | | |
| 14  | | | | RS | | | | |
| 15  | | | | (octets 9 – 5) | | | | |
| 16  | | | | | | | | |
| 17  | | | | | | | | |
| 18  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %110001

**Figure 116 – Authentication Demand (2 of 2)**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 14 | | | |
| 3   | | | | | | | | |
| 4   | | | | RS | | | | |
| 5   | | | | (octets 4 – 0) | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | RAND1 | | | | |
| 10  | | | | | | | | |
| 11  | | | | | | | | |
| 12  | | | | | | | | |
| 13  | | | | Multi-Fragment CRC-16 | | | | |
| 14  | | | | | | | | |

B1 = %0, B2 = %0, MCO = %010000

## 3.53 Authentication FNE Response

The Authentication FNE Response messages are sent by the FNE due to the SU challenge during mutual authentication. When the message is sent prior to registration, the Target Address shall be set to the Registration Default. The abbreviated format is shown in Figure 117 below and the extended format is shown in Figure 118 below.

The abbreviated format may be used if the WACN ID and System ID of the target match the WACN ID and System ID of the RFSS transmitting the message. Otherwise the extended format shall be used.

**Figure 117 – Authentication FNE Response - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 9 | | | |
| 3   | | | | | | | | |
| 4   | | | | RES2 | | | | |
| 5   | | | | | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| 8   | | | | Target ID | | | | |
| 9   | | | | | | | | |

B1 = %0, B2 = %1, MCO = %110010

**Figure 118 – Authentication FNE Response - Extended**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 16 | | | |
| 3   | | | | | | | | |
| 4   | | | | Target Address | | | | |
| 5   | | | | | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| ⁞   | | | | Target SUID | | | | |
| 11  | | | | | | | | |
| 12  | | | | | | | | |
| 13  | | | | | | | | |
| 14  | | | | RES2 | | | | |
| 15  | | | | | | | | |
| 16  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %110010

## 3.54 Synchronization Broadcast

The Synchronization Broadcast message conveyed in a MAC PDU indicates the time and date in micro-slots at the beginning of the current TDMA 30ms burst which is the center of the ISCH as shown in Figure 119 below. This figure also shows that the start of the TDMA superframe begins at the minute boundary and that the micro-slot value is always a multiple of four since there are four micro-slots per TDMA 30ms burst. The time base (system time) used should be represented as a monotonically increasing value. International atomic time (TAI) is one example. The message format is shown in Figure 120 below.

*Figure 119 – TDMA Superframe Relationship to Time Micro-slot*

*(Diagram showing Minute Boundary, Micro-slot values 7999, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 with 7.5ms intervals. TDMA CCH, TDMA LCH 0 / Slot 0, TDMA LCH 1 / Slot 1, TDMA LCH 0 / Slot 2 are shown. ISCH centers are indicated. The "Micro-slots" field of Synchronization Broadcast message indicates the micro-slot that corresponds to the beginning of the 30ms burst which is the center of the ISCH. Start of TDMA Superframe begins at 30ms Burst. Slot number of TDMA Superframe is indicated.)*

**Figure 120 – Synchronization Broadcast**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 9 | | | |
| 3   | | Reserved | | | %0 | IST | MM | MC |
| 4   | MC | VL | | Local Time Offset | | | | |
| 5   | | | | Year | | | | Mon |
| 6   | Month | | | | Day | | | |
| 7   | | Hours | | | | Minutes | | |
| 8   | Minutes | | | | Micro-slots | | | |
| 9   | | | | Micro-slots | | | | |

B1 = %0, B2 = %1, MCO = %110000

## 3.55 Unit to Unit Voice Service Answer Response

The Unit to Unit Voice Service Answer Response messages are used to accommodate the response of the called unit to the Unit to Unit Voice Service Answer Request. In the following messages, "Target" refers to the calling unit (initiator of the Unit to Unit call) and "Source" refers to the called unit. The abbreviated format is shown in Figure 121 below and the extended format is shown in Figure 122 below.

The abbreviated format may be used when the Source SU and the Target SU are members of the same Home system, with both SUs residing in the Home System, and both the Target ID and Source Address reference to the Home identities of these units. The extended format shall always be acceptable even in cases where the abbreviated format could be used instead.

**Figure 121 – Unit to Unit Voice Service Answer Response - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 10 | | | |
| 3   | | | | Service Options | | | | |
| 4   | | | | Answer Response | | | | |
| 5   | | | | | | | | |
| 6   | | | | Target ID | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | Source Address | | | | |
| 10  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %000101

**Figure 122 – Unit to Unit Voice Service Answer Response - Extended**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 14 | | | |
| 3   | | | | Service Options | | | | |
| 4   | | | | Answer Response | | | | |
| 5   | | | | | | | | |
| 6   | | | | Source Address | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | | | | | |
| ⁞   | | | | Target SUID | | | | |
| 13  | | | | | | | | |
| 14  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %000101

## 3.56 Telephone Interconnect Request – Explicit Dialing

The Telephone Interconnect Request – Explicit Dialing message is used when an explicit telephone digit sequence is provided in the service request. The minimum number of digits is 1 and the maximum number of digits is 34. This may be a multi-fragment message as specified below. The format of the first fragment is shown in Figure 123 below, the format of the second fragment is shown in Figure 124 below, and the format of the third fragment is shown in Figure 125 below.

If the number of digits does not exceed 12, then one fragment is required. If the number of digits is greater than 12 and does not exceed 32, then two fragments are required. If the number of digits is greater than 32, then three fragments are required.

If there are an odd number of digits, then the even numbered digit position immediately following the last digit is set to %1111 which is the DTMF null digit value. The Multi-Fragment CRC-16 field is not included if the digits fit in a single fragment. The Multi-Fragment CRC-16 field is included in the two octets immediately following the last pair of digits if multiple fragments are required.

The Length field of fragments 1 and 2 is variable and is set to the number of octets included in the fragment. The Data Length field of fragment one is set to the number of octets included in the entire message excluding the first two octets of each fragment.

**Figure 123 – Telephone Interconnect Request – Explicit Dialing (1 of 3)**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 9 to 14 | | | |
| 3   | | | | Data Length = 7 to 25 | | | | |
| 4   | | | | | | | | |
| 5   | | | | Source Address | | | | |
| 6   | | | | | | | | |
| 7   | | | | Digit Count | | | | |
| 8   | | | | Service Options | | | | |
| 9   | Digit 1 | | | | Digit 2 | | | |
| 10  | Digit 3 | | | | Digit 4 | | | |
| 11  | Digit 5 | | | | Digit 6 | | | |
| 12  | Digit 7 | | | | Digit 8 | | | |
| 13  | Digit 9 | | | | Digit 10 | | | |
| 14  | Digit 11 | | | | Digit 12 | | | |

B1 = %0, B2 = %1, MCO = %001000

**Figure 124 – Telephone Interconnect Request – Explicit Dialing (2 of 3)**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 3 to 14 | | | |
| 3   | Digit 13 | | | | Digit 14 | | | |
| 4   | Digit 15 | | | | Digit 16 | | | |
| 5   | Digit 17 | | | | Digit 18 | | | |
| 6   | Digit 19 | | | | Digit 20 | | | |
| 7   | Digit 21 | | | | Digit 22 | | | |
| 8   | Digit 23 | | | | Digit 24 | | | |
| 9   | Digit 25 | | | | Digit 26 | | | |
| 10  | Digit 27 | | | | Digit 28 | | | |
| 11  | Digit 29 | | | | Digit 30 | | | |
| 12  | Digit 31 | | | | Digit 32 | | | |
| 13  | Digit 33 | | | | Digit 34 | | | |
| 14  | | Multi-Fragment CRC-16 (bits 15-9) | | | | | | |

B1 = %0, B2 = %0, MCO = %010000

**Figure 125 – Telephone Interconnect Request – Explicit Dialing (3 of 3)**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 3 | | | |
| 3   | | Multi-Fragment CRC-16 (bits 7-0) | | | | | | |

B1 = %0, B2 = %0, MCO = %010000

## 3.57 Telephone Interconnect Request – Implicit Dialing

The Telephone Interconnect Request – Implicit Dialing message is used when a PSTN address is used as the target address. Each PSTN address can be related to a unique dialing sequence. There is a unique set of PSTN addresses available to each unique unit identity. The message format is shown in Figure 126 below.

**Figure 126 – Telephone Interconnect Request – Implicit Dialing**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 7 | | | |
| 3   | | | | Service Options | | | | |
| 4   | | | | PSTN Address | | | | |
| 5   | | | | | | | | |
| 6   | | | | Source Address | | | | |
| 7   | | | | | | | | |

B1 = %0, B2 = %1, MCO = %001001

## 3.58 Telephone Interconnect Answer Response

The Telephone Interconnect Answer Response message is used by the target unit to respond to the PSTN to unit answer request to express the target user decision for this pending telephone call. The message format is shown in Figure 127 below.

**Figure 127 – Telephone Interconnect Answer Response**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 7 | | | |
| 3   | | | | Service Options | | | | |
| 4   | | | | Answer Response | | | | |
| 5   | | | | | | | | |
| 6   | | | | Source Address | | | | |
| 7   | | | | | | | | |

B1 = %0, B2 = %1, MCO = %000101

## 3.59 SNDCP Data Channel Request

The SNDCP Data Channel Request message is used by the SU on the inbound control channel when it has SNDCP data to send. The SU expects the FNE to transmit an SNDCP Data Channel Grant in response to the message on the outbound control channel. The message format is shown in Figure 128 below.

**Figure 128 – SNDCP Data Channel Request**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 8 | | | |
| 3   | | | | Data Service Options | | | | |
| 4   | | | | Data Access Control | | | | |
| 5   | | | | | | | | |
| 6   | | | | | | | | |
| 7   | | | | Source Address | | | | |
| 8   | | | | | | | | |

B1 = %0, B2 = %1, MCO = %010010

## 3.60 SNDCP Data Page Response

The SNDCP Data Page Response message is used by the SU in response to an SNDCP Data Page Request received from the FNE. The SU responds with PROCEED, DENY, or WAIT as specified in the Answer Response field definition. The message format is shown in Figure 129 below.

**Figure 129 – SNDCP Data Page Response**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 9 | | | |
| 3   | | | | Data Service Options | | | | |
| 4   | | | | Answer Response | | | | |
| 5   | | | | Data Access Control | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| 8   | | | | Source Address | | | | |
| 9   | | | | | | | | |

B1 = %0, B2 = %1, MCO = %010011

## 3.61 SNDCP Reconnect Request

The SNDCP Reconnect Request message is transmitted by the SU on the inbound control channel when it has successfully completed a location registration update while in the Ready State. If the SU has SNDCP data in its buffers to send it shall set the Data to Send (DS) field to %1, else it is set to %0. The MRC expects the FNE to transmit an SNDCP Data Channel Grant in response to this message on the outbound control channel if the DS field has been set to %1. The message format is shown in Figure 130 below.

**Figure 130 – SNDCP Reconnect Request**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 8 | | | |
| 3   | | | | Data Service Options | | | | |
| 4   | | | | Data Access Control | | | | |
| 5   | | | | | | | | |
| 6   | DS | | | | Reserved | | | |
| 7   | | | | | | | | |
| 8   | | | | Source Address | | | | |
| 9   | | | | | | | | |

B1 = %0, B2 = %1, MCO = %010100

## 3.62 Acknowledge Response Unit

The Acknowledge Response Unit message is a generic response supplied by a unit to acknowledge an action when there is no other expected response. The abbreviated format is shown in Figure 131 below and the extended format is shown in Figure 132 below.

The abbreviated format may be used when the Source SU and the Target SU are members of the same Home system, and the Source SU resides in the Home system. The extended format shall always be acceptable even in cases where the abbreviated format could be used instead.

**Figure 131 – Acknowledge Response Unit - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 9 | | | |
| 3   | 0 | 0 | | | Service Type | | | |
| 4   | | | | | | | | |
| 5   | | | | Target ID | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| 8   | | | | Source Address | | | | |
| 9   | | | | | | | | |

B1 = %0, B2 = %1, MCO = %100000

**Figure 132 – Acknowledge Response Unit - Extended**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 13 | | | |
| 3   | 0 | 0 | | | Service Type | | | |
| 4   | | | | | | | | |
| 5   | | | | Source Address | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| ⁞   | | | | Target SUID | | | | |
| 12  | | | | | | | | |
| 13  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %100000

## 3.63 Call Alert Request

The Call Alert Request message is the request for a Target SU to call a Source SU. The abbreviated format is shown in Figure 133 below and the extended format is shown in Figure 134 below.

The abbreviated format may be used when the Source SU and the Target SU are members of the same Home system, and the Source SU resides in the Home system. The extended format shall always be acceptable even in cases where the abbreviated format could be used instead.

**Figure 133 – Call Alert Request - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 8 | | | |
| 3   | | | | | | | | |
| 4   | | | | Target ID | | | | |
| 5   | | | | | | | | |
| 6   | | | | | | | | |
| 7   | | | | Source Address | | | | |
| 8   | | | | | | | | |

B1 = %0, B2 = %1, MCO = %011111

**Figure 134 – Call Alert Request - Extended**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 12 | | | |
| 3   | | | | | | | | |
| 4   | | | | Source Address | | | | |
| 5   | | | | | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| ⁞   | | | | Target SUID | | | | |
| 11  | | | | | | | | |
| 12  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %011111

## 3.64 Cancel Service Request

The Cancel Service Request message is the set of requests from the requesting unit to cancel any further actions with regard to the current (or indicated) service that has been initiated by this unit and is viable whether or not the indicated service is currently assigned to a channel resource. The abbreviated format is shown in Figure 135 below and the extended format is shown in Figure 136 below.

The abbreviated format may be used in all cases for all services except for Unit to Unit call service. When the Cancel Service Request message is used for Unit to Unit call service, the abbreviated format may be used when the Source SU and the Target SU are members of the same Home system, with both SUs residing in the Home system. In all other cases of Unit to Unit call service, the extended format is used.

**Figure 135 – Cancel Service Request - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 10 | | | |
| 3   | AIV | 0 | | | Service Type | | | |
| 4   | | | | Reason Code | | | | |
| 5   | | | | | | | | |
| 6   | | | | Additional Information | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | Source Address | | | | |
| 10  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %100011

**Figure 136 – Cancel Service Request - Extended**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 14 | | | |
| 3   | AIV | 0 | | | Service Type | | | |
| 4   | | | | Reason Code | | | | |
| 5   | | | | | | | | |
| 6   | | | | Source Address | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | SUID | | | | |
| ⁞   | | | | (of other SU in unit to unit call) | | | | |
| 13  | | | | | | | | |
| 14  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %100011

## 3.65 Emergency Alarm Request

The Emergency Alarm Request message is a special status indication typically reserved for the "life threatening" situation. The message format is shown in Figure 137 below.

**Figure 137 – Emergency Alarm Request**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 9 | | | |
| 3   | | | | Special Information 1 (SI-1) | | | | |
| 4   | | | | Special Information 2 (SI-2) | | | | |
| 5   | | | | Group Address | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| 8   | | | | Source Address | | | | |
| 9   | | | | | | | | |

B1 = %0, B2 = %1, MCO = %100111

## 3.66 Extended Function Response

The Extended Function Response message is the response to an Extended Function Command message. The abbreviated format is shown in Figure 138 below and the extended format is shown in Figure 139 below.

The abbreviated format shall be used when the source SU is active on its Home system, and the source SU and target SU have the same Home system. The extended format shall be used if the abbreviated format is not appropriate.

**Figure 138 – Extended Function Response - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 10 | | | |
| 3   | | | | Class | | | | |
| 4   | | | | Operand | | | | |
| 5   | | | | | | | | |
| 6   | | | | Arguments | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | Source Address | | | | |
| 10  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %100100

**Figure 139 – Extended Function Response - Extended**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 14 | | | |
| 3   | | | | Class | | | | |
| 4   | | | | Operand | | | | |
| 5   | | | | | | | | |
| 6   | | | | Arguments | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | Source Address | | | | |
| 10  | | | | | | | | |
| 11  | | | | WACN ID | | | | |
| 12  | | | | | | | | |
| 13  | | | | | | | | |
| 14  | | | | System ID | | | | |

B1 = %1, B2 = %1, MCO = %100100

## 3.67 Group Affiliation Query Response

The Group Affiliation Query Response message is the response from a SU which has been queried for its current group affiliation data. The message format is shown in Figure 140 below.

**Figure 140 – Group Affiliation Query Response**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 10 | | | |
| 3   | E | | | | Reserved | | | |
| 4   | | | | Announcement Group Address | | | | |
| 5   | | | | | | | | |
| 6   | | | | Group Address | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | Source Address | | | | |
| 10  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %101010

## 3.68 Group Affiliation Request

The Group Affiliation Request message is the normal mode for an update of current group selection to the communication system. This is valid when the WACN ID of the Group (WACN ID:SYSTEM ID:GROUP ID) and the WACN ID of the current Site match. The abbreviated format is shown in Figure 141 below and the extended format is shown in Figure 142 below.

The extended format may be used for any Group Affiliation Request. The extended format shall be used if the abbreviated format is not appropriate.

**Figure 141 – Group Affiliation Request - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 9 | | | |
| 3   | E | | | Reserved | | | | |
| 4   | | | | System ID | | | | |
| 5   | | | | Group ID | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| 8   | | | | Source Address | | | | |
| 9   | | | | | | | | |

B1 = %0, B2 = %1, MCO = %101000

**Figure 142 – Group Affiliation Request - Extended**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 12 | | | |
| 3   | E | | | | Reserved | | | |
| 4   | | | | | | | | |
| 5   | | | | Source Address | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| ⁞   | | | | SGID | | | | |
| 11  | | | | | | | | |
| 12  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %101000

## 3.69 Identifier Update Request

The Identifier Update Request message allows a SU to request information concerning a specific channel identifier, or all current identifiers at this location. The message format is shown in Figure 143 below.

**Figure 143 – Identifier Update Request**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 6 | | | |
| 3   | | Flags | | | Channel Identifier | | | |
| 8   | | | | | | | | |
| 9   | | | | Source Address | | | | |
| 10  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %110010

## 3.70 Message Update Request

The Message Update Request message permits the originating SU (Source Address) to request that the embedded short data message be sent to a recipient SU (Target ID) via the FNE using control channel signaling. The abbreviated format is shown in Figure 144 below and the extended format is shown in Figure 145 below.

The abbreviated format may be used when the Source SU and the Target SU are members of the same Home system, and the Source SU resides in the Home system. The extended format shall always be acceptable even in cases where the abbreviated format could be used instead.

**Figure 144 – Message Update Request - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 10 | | | |
| 3   | | | | Message | | | | |
| 4   | | | | | | | | |
| 5   | | | | | | | | |
| 6   | | | | Target ID | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | Source Address | | | | |
| 10  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %011100

**Figure 145 – Message Update Request - Extended**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 14 | | | |
| 3   | | | | Message | | | | |
| 4   | | | | | | | | |
| 5   | | | | | | | | |
| 6   | | | | Source Address | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | | | | | |
| ⁞   | | | | Target SUID | | | | |
| 13  | | | | | | | | |
| 14  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %011100

## 3.71 Status Query Request

The Status Query Request message permits an appropriate unit to request the current user and unit status condition for a particular unit. The abbreviated format is shown in Figure 146 below and the extended format is shown in Figure 147 below.

The abbreviated format may be used when the Source SU and the Target SU are members of the same Home system, and the Source SU resides in the Home system. The extended format shall always be acceptable even in cases where the abbreviated format could be used instead.

**Figure 146 – Status Query Request - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 8 | | | |
| 3   | | | | | | | | |
| 4   | | | | Target ID | | | | |
| 5   | | | | | | | | |
| 6   | | | | | | | | |
| 7   | | | | Source Address | | | | |
| 8   | | | | | | | | |

B1 = %0, B2 = %1, MCO = %011010

**Figure 147 – Status Query Request - Extended**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 12 | | | |
| 3   | | | | | | | | |
| 4   | | | | Source Address | | | | |
| 5   | | | | | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| ⁞   | | | | Target SUID | | | | |
| 11  | | | | | | | | |
| 12  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %011010

## 3.72 Status Query Response

The Status Query Response message is from a queried SU for its current status information. This includes both the current user status and the current unit status information in the Status field. The abbreviated format is shown in Figure 148 below and the extended format is shown in Figure 149 below.

The abbreviated format may be used when the Source SU and the Target SU are members of the same Home system, and the Source SU resides in the Home system. The extended format shall always be acceptable even in cases where the abbreviated format could be used instead.

**Figure 148 – Status Query Response - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 10 | | | |
| 3   | | | | Status | | | | |
| 4   | | | | | | | | |
| 5   | | | | | | | | |
| 6   | | | | Target ID | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | Source Address | | | | |
| 10  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %011001

**Figure 149 – Status Query Response - Extended**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 14 | | | |
| 3   | | | | Status | | | | |
| 4   | | | | | | | | |
| 5   | | | | | | | | |
| 6   | | | | Source Address | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | | | | | |
| ⁞   | | | | Target SUID | | | | |
| 13  | | | | | | | | |
| 14  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %011001

## 3.73 Status Update Request

The Status Update Request message is used to indicate a status condition (User and Unit) for the Source SU to some Target ID via the control channel. The abbreviated format is shown in Figure 150 below and the extended format is shown in Figure 151 below.

The abbreviated format may be used when the Source SU and the Target SU are members of the same Home system, and the Source SU resides in the Home system. The extended format shall always be acceptable even in cases where the abbreviated format could be used instead.

**Figure 150 – Status Update Request - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 10 | | | |
| 3   | | | | Status | | | | |
| 4   | | | | | | | | |
| 5   | | | | | | | | |
| 6   | | | | Target ID | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | Source Address | | | | |
| 10  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %011000

**Figure 151 – Status Update Request - Extended**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 14 | | | |
| 3   | | | | Status | | | | |
| 4   | | | | | | | | |
| 5   | | | | | | | | |
| 6   | | | | Source Address | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | | | | | |
| ⁞   | | | | Target SUID | | | | |
| 13  | | | | | | | | |
| 14  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %011000

## 3.74 Unit Registration Request

The Unit Registration Request message is used to signal the unit's presence in the communication system after the unit has attained a valid system identity for the communication network. The message format is shown in Figure 152 below.

**Figure 152 – Unit Registration Request**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 10 | | | |
| 3   | E | | | | Capabilities | | | |
| 4   | | | | | | | | |
| 5   | | | | | | | | |
| ⁞   | | | | Source SUID | | | | |
| 9   | | | | | | | | |
| 10  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %101100

## 3.75 Unit Deregistration Request

The Unit Deregistration Request message is used to request deregistration of an SU in a system. The message format is shown in Figure 153 below.

**Figure 153 – Unit Deregistration Request**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 9 | | | |
| 3   | | | | | | | | |
| 4   | | | | | | | | |
| ⁞   | | | | Source SUID | | | | |
| 8   | | | | | | | | |
| 9   | | | | | | | | |

B1 = %0, B2 = %1, MCO = %101111

## 3.76 Location Registration Request

The Location Registration Request message is used when a subscriber unit enters a new Site within the same Location Registration Area of the previous Site. The abbreviated format is shown in Figure 154 below. The extended format is a multi-fragment message. The format for the first fragment is shown in Figure 155 below and the format for the second fragment is shown in Figure 156 below.

The abbreviated format may be used to update registration at a new Site in the Location Registration Area. The extended format shall be used if the abbreviated format is not appropriate.

**Figure 154 – Location Registration Request - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 9 | | | |
| 3   | E | | | | Capabilities | | | |
| 4   | | | | LRA | | | | |
| 5   | | | | Group Address | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| 8   | | | | Source Address | | | | |
| 9   | | | | | | | | |

B1 = %0, B2 = %1, MCO = %101101

**Figure 155 – Location Registration Request – Extended (1 of 2)**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 14 | | | |
| 3   | | | | Data Length = 17 | | | | |
| 4   | | | | | | | | |
| 5   | | | | Source Address | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| ⁞   | | | | Source SUID | | | | |
| 12  | | | | | | | | |
| 13  | | | | | | | | |
| 14  | | | | Previous LRA | | | | |

B1 = %1, B2 = %1, MCO = %101101

**Figure 156 – Location Registration Request – Extended (2 of 2)**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 7 | | | |
| 3   | | | | Group Address | | | | |
| 4   | | | | | | | | |
| 5   | E | | | | Capabilities | | | |
| 6   | | | | Multi-Fragment CRC-16 | | | | |
| 7   | | | | | | | | |

B1 = %0, B2 = %0, MCO = %010000

## 3.77 Radio Unit Monitor Request

The Radio Unit Monitor Request message is used to request a radio unit monitor operation at another subscriber unit indicated by the target ID. The abbreviated format is shown in Figure 157 below and the extended format is shown in Figure 158 below.

The abbreviated format may be used when the Source SU and the Target US are members of the same Home system, and the Source SU resides in the Home system. The extended format shall always be acceptable even in cases where the abbreviated format could be used instead.

**Figure 157 – Radio Unit Monitor Request - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 10 | | | |
| 3   | | | | TX Time | | | | |
| 4   | SM | | | Reserved | | | TX Mult | |
| 5   | | | | | | | | |
| 6   | | | | Source Address | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | Target ID | | | | |
| 10  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %011101

**Figure 158 – Radio Unit Monitor Request - Extended**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 14 | | | |
| 3   | | | | TX Time | | | | |
| 4   | SM | | | Reserved | | | TX Mult | |
| 5   | | | | | | | | |
| 6   | | | | Source Address | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | | | | | |
| ⁞   | | | | Target SUID | | | | |
| 13  | | | | | | | | |
| 14  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %011101

## 3.78 Roaming Address Request

The Roaming Address Request message is sent by a source SU to query the RFSS for the roaming address stack contents of the target SU. This is the first step in the SU Read Operation for the Roaming Address Stack. The abbreviated format is shown in Figure 159 below and the extended format is shown in Figure 160 below.

The extended format is used when the full address of the target SU is required.

**Figure 159 – Roaming Address Request - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 8 | | | |
| 3   | | | | | | | | |
| 4   | | | | Source Address | | | | |
| 5   | | | | | | | | |
| 6   | | | | | | | | |
| 7   | | | | Target Address | | | | |
| 8   | | | | | | | | |

B1 = %0, B2 = %1, MCO = %110110

**Figure 160 – Roaming Address Request - Extended**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 12 | | | |
| 3   | | | | | | | | |
| 4   | | | | Source Address | | | | |
| 5   | | | | | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| ⁞   | | | | Target SUID | | | | |
| 11  | | | | | | | | |
| 12  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %110110

## 3.79 Roaming Address Response

The Roaming Address Response message is sent by the SU in a Read Operation. It contains the contents of the Roaming Address Stack. A Roaming Address Stack entry is a WACN ID / System ID pair. The minimum number of stack entries is one and the maximum is 10. If the number of stack entries exceeds this maximum, then multiple Roaming Address Response messages are sent.

The Roaming Address Response message may be a multi-fragment message as specified below. The format for the first fragment is shown in Figure 161 below. The format for other fragments that may convey one to three stack entries is shown in Figure 162 below and the format of a fragment that conveys a Multi-Fragment CRC-16 field only is shown in Figure 163 below. The number of fragments sent depends on the number of stack entries and may range from one to five as described below. The actual location of the Multi-Fragment CRC-16 field also depends on the number of stack entries as described below.

If the number of stack entries is one, then one fragment is required with Length = 11 and Data Length = 9. The Multi-Fragment CRC-16 field is not needed in the single fragment case. If there are multiple stack entries, then the Length = 14 in the first fragment and additional fragments are utilized depending on the number of stack entries conveyed in the multi-fragment message. Each additional fragment may convey one to three stack entries. If the last fragment containing stack entries contains three stack entries, then an additional fragment is required for the Multi-Fragment CRC-16 as shown in Figure 163 below. Otherwise, the Multi-Fragment CRC-16 is included as the last field of the last fragment containing stack entries with the Length field set accordingly. Table 8 below specifies the Data Length field for the message and the Length field for each fragment based on the number of stack entries being conveyed in the message.

**Figure 161 – Roaming Address Response (1)**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 11 or 14 | | | |
| 3   | | | | Data Length = 9 to 50 | | | | |
| 4   | LM | | Reserved | | | MSN | | |
| 5   | | | | | | | | |
| 6   | | | | Source Address | | | | |
| 7   | | | | | | | | |
| 8   | | | | WACN ID (1) | | | | |
| 9   | | | | | | | | |
| 10  | | | | | | | | |
| 11  | | | | System ID (1) | | | | |
| 12  | | | | | | | | |
| 13  | | | | Reserved | | | | |
| 14  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %110111

**Figure 162 – Roaming Address Response (Multiple Stack Entries)**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 14 | | | |
| 3   | | | | WACN ID (2 or 5 or 8) | | | | |
| 4   | | | | | | | | |
| 5   | | | | | | | | |
| 6   | | | | System ID (2 or 5 or 8) | | | | |
| 7   | | | | WACN ID (3 or 6 or 9) | | | | |
| 8   | | | | | | | | |
| 9   | | | | | | | | |
| 10  | | | | System ID (3 or 6 or 9) | | | | |
| 11  | | | | WACN ID (4 or 7 or 10) | | | | |
| 12  | | | | | | | | |
| 13  | | | | | | | | |
| 14  | | | | System ID (4 or 7 or 10) | | | | |

B1 = %0, B2 = %0, MCO = %010000

**Figure 163 – Roaming Address Response (Multi-Fragment CRC-16 Only)**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 4 | | | |
| 3   | | | | Multi-Fragment CRC-16 | | | | |
| 4   | | | | | | | | |

B1 = %0, B2 = %0, MCO = %010000

**Table 8 – Roaming Address Response Length Fields**

| Stack Entries | Data Length | Fragment 1 | Fragment 2 | Fragment 3 | Fragment 4 | Fragment 5 |
|---------------|-------------|------------|------------|------------|------------|------------|
| 1             | 9           | 11         |            |            |            |            |
| 2             | 18          | 14         | 8          |            |            |            |
| 3             | 22          | 14         | 12         |            |            |            |
| 4             | 26          | 14         | 14         | 4          |            |            |
| 5             | 30          | 14         | 14         | 8          |            |            |
| 6             | 34          | 14         | 14         | 12         |            |            |
| 7             | 38          | 14         | 14         | 14         | 4          |            |
| 8             | 42          | 14         | 14         | 14         | 8          |            |
| 9             | 46          | 14         | 14         | 14         | 12         |            |
| 10            | 50          | 14         | 14         | 14         | 14         | 4          |

## 3.80 Authentication FNE Result

The Authentication FNE Result message is sent by the SU due to the FNE response during mutual authentication and indicates the result. The abbreviated format is shown in Figure 164 below and the extended format is shown in Figure 165 below.

The abbreviated format may be used if the WACN ID and System ID of the SU match the WACN ID and System ID of the challenging RFSS. The extended format may be used for any authentication result transaction. The extended format shall be used when the abbreviated format is not applicable. The Source Address field of the extended format shall be populated with the working address if it has been assigned, and otherwise with the Registration Default.

**Figure 164 – Authentication FNE Result - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 6 | | | |
| 3   | R2 | | | Reserved | | | S | |
| 4   | | | | | | | | |
| 5   | | | | Source ID | | | | |
| 6   | | | | | | | | |

B1 = %0, B2 = %1, MCO = %111010

**Figure 165 – Authentication FNE Result - Extended**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 13 | | | |
| 3   | R2 | | | Reserved | | | S | |
| 4   | | | | | | | | |
| 5   | | | | Source Address | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| ⁞   | | | | Source SUID | | | | |
| 12  | | | | | | | | |
| 13  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %111010

## 3.81 Authentication Response

The Authentication Response message is sent by the SU due to the FNE challenge when mutual authentication is performed and contains the response (RES1). The abbreviated format is shown in Figure 166 below. The extended format is a multi-fragment message. The first fragment is shown in Figure 167 below and the second fragment is shown in Figure 168 below.

The abbreviated format may be used if the WACN ID and System ID of the SU match the WACN ID and System ID of the challenging RFSS. The extended format may be used for any authentication response transaction. The extended format shall be used when the abbreviated format is not applicable. The Source Address field of the extended format shall be populated with the working address if it has been assigned, and otherwise with the Registration Default.

**Figure 166 – Authentication Response - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 10 | | | |
| 3   | | | | Reserved | | | S | |
| 4   | | | | | | | | |
| 5   | | | | RES1 | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | Source ID | | | | |
| 10  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %111000

**Figure 167 – Authentication Response - Extended (1 of 2)**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 14 | | | |
| 3   | | | | Data Length = 18 | | | | |
| 4   | | | | Reserved | | | S | |
| 5   | | | | | | | | |
| 6   | | | | Source Address | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | | | | | |
| ⁞   | | | | Source SUID | | | | |
| 13  | | | | | | | | |
| 14  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %111000

**Figure 168 – Authentication Response - Extended (2 of 2)**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 8 | | | |
| 3   | | | | | | | | |
| 4   | | | | RES1 | | | | |
| 5   | | | | | | | | |
| 6   | | | | | | | | |
| 7   | | | | Multi-Fragment CRC-16 | | | | |
| 8   | | | | | | | | |

B1 = %0, B2 = %0, MCO = %010000

## 3.82 Authentication Response Mutual

The Authentication Response Mutual message is sent by the SU due to the FNE challenge when the SU challenges the FNE with mutual authentication and contains the response (RES1) and challenge (RAND2). This is a multi-fragment message. The first fragment is shown in Figure 169 below and the second fragment is shown in Figure 170 below.

The Source Address field shall be populated with the working address if it has been assigned, and otherwise with the Registration Default.

**Figure 169 – Authentication Response Mutual (1 of 2)**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 14 | | | |
| 3   | | | | Data Length = 23 | | | | |
| 4   | | | | Reserved | | | S | |
| 5   | | | | | | | | |
| 6   | | | | Source Address | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | | | | | |
| ⁞   | | | | Source SUID | | | | |
| 13  | | | | | | | | |
| 14  | | | | | | | | |

B1 = %0, B2 = %1, MCO = %111001

**Figure 170 – Authentication Response Mutual (2 of 2)**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 13 | | | |
| 3   | | | | | | | | |
| 4   | | | | | | | | |
| 5   | | | | RAND2 | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9   | | | | RES1 | | | | |
| 10  | | | | | | | | |
| 11  | | | | | | | | |
| 12  | | | | Multi-Fragment CRC-16 | | | | |
| 13  | | | | | | | | |

B1 = %0, B2 = %0, MCO = %010000

## 3.83 Authentication SU Demand

The Authentication SU Demand message is sent by the SU to demand that the FNE start an authentication so the SU can later make it mutual. The abbreviated format is shown in Figure 171 below and the extended format is shown in Figure 172 below.

The abbreviated format may be used if the WACN ID and System ID of the SU match the WACN ID and System ID of the receiving RFSS. The extended format may be used for any authentication SU demand transaction. The extended format shall be used when the abbreviated format is not applicable. The Source Address field of the extended format shall be populated with the working address if it has been assigned, and otherwise with the Registration Default.

**Figure 171 – Authentication SU Demand - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 5 | | | |
| 3   | | | | | | | | |
| 4   | | | | Source ID | | | | |
| 5   | | | | | | | | |

B1 = %0, B2 = %1, MCO = %111011

**Figure 172 – Authentication SU Demand - Extended**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 12 | | | |
| 3   | | | | | | | | |
| 4   | | | | Source Address | | | | |
| 5   | | | | | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| ⁞   | | | | Source SUID | | | | |
| 11  | | | | | | | | |
| 12  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %111011

## 3.84 Radio Unit Monitor Enhanced Request

The Radio Unit Monitor Enhanced Request message is used to request an enhanced radio unit monitor operation at another subscriber unit by the target address. The initiator may request either a clear or encrypted unit-to-unit call or a clear or encrypted group call. The abbreviated format is a multi-fragment message. The first fragment is shown in Figure 173 below and the second fragment is shown in Figure 174 below. The extended format is a multi-fragment message. The first fragment is shown in Figure 175 below and the second fragment is shown in Figure 176 below.

The abbreviated format may be used when the Source SU and the Target SU are members of the same Home system, and the Source SU resides in the Home system. The extended format shall always be acceptable even in cases where the abbreviated format could be used instead.

**Figure 173 – Radio Unit Monitor Enhanced Request - Abbreviated (1 of 2)**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 14 | | | |
| 3   | | | | Data Length = 16 | | | | |
| 4   | | | | | | | | |
| 5   | | | | Source Address | | | | |
| 6   | | | | | | | | |
| 7   | | | | Group Address | | | | |
| 8   | | | | | | | | |
| 9   | | | | | | | | |
| 10  | | | | Target ID | | | | |
| 11  | | | | | | | | |
| 12  | SM | TG | | | Reserved | | | |
| 13  | | | | TX Time | | | | |
| 14  | | | | ALGID | | | | |

B1 = %0, B2 = %1, MCO = %011110

**Figure 174 – Radio Unit Monitor Enhanced Request - Abbreviated (2 of 2)**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 6 | | | |
| 3   | | | | Key ID | | | | |
| 4   | | | | | | | | |
| 5   | | | | Multi-Fragment CRC-16 | | | | |
| 6   | | | | | | | | |

B1 = %0, B2 = %0, MCO = %010000

**Figure 175 – Radio Unit Monitor Enhanced Request - Extended (1 of 2)**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 14 | | | |
| 3   | | | | Data Length = 24 | | | | |
| 4   | | | | | | | | |
| 5   | | | | Source Address | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| ⁞   | | | | Target SUID | | | | |
| 12  | | | | | | | | |
| 13  | | | | | | | | |
| 14  | SM | TG | | | Reserved | | | |

B1 = %1, B2 = %1, MCO = %011110

**Figure 176 – Radio Unit Monitor Enhanced Request - Extended (2 of 2)**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 14 | | | |
| 3   | | | | TX Time | | | | |
| 4   | | | | ALGID | | | | |
| 5   | | | | Key ID | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| ⁞   | | | | SGID | | | | |
| 11  | | | | | | | | |
| 12  | | | | | | | | |
| 13  | | | | Multi-Fragment CRC-16 | | | | |
| 14  | | | | | | | | |

B1 = %0, B2 = %0, MCO = %010000

## 3.85 Null Avoid Zero Bias Information

The Null Avoid Zero Bias Information message is used to fill any unused portion of a MAC PDU with two or more unused octets. The length is sufficient to fill the MAC PDU. If the number of unused octets is two, the Length = 2 and the Avoid Zero Bias field is omitted. Otherwise, the Length = 2 + the number of Avoid Zero Bias octets needed to fill the MAC PDU. The message format is shown in Figure 177 below.

**Figure 177 – Null Avoid Zero Bias Information**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | Reserved | | | | Length = 2 to 18 | | | |
| 3   | | | | | | | | |
| ⁞   | | | | Avoid Zero Bias | | | | |
| …   | | | | | | | | |

B1 = %0, B2 = %0, MCO = %001000

## 3.86 MFID90 Group Regroup Voice Channel User

The Group Regroup Voice Channel User message is utilized by the MFID90 Airlink Efficient method of the Dynamic Regrouping operation. This message is used both inbound and outbound during a supergroup call to identify the audio sourcing endpoint (console or subscriber) and the current supergroup address. The abbreviated format is shown in Figure 178 below and the extended format is shown in Figure 179 below.

**Figure 178 – MFID90 Group Regroup Voice Channel User - Abbreviated**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | | | | Manufacturer's ID | | | | |
| 3   | Res | P | | | Reserved | | | |
| 4   | | | | Supergroup Address | | | | |
| 5   | | | | | | | | |
| 6   | | | | | | | | |
| 7   | | | | Source Address | | | | |
| 8   | | | | | | | | |

B1 = %1, B2 = %0, MCO = %000000, Manufacturer's ID = $90

**Figure 179 – MFID90 Group Regroup Voice Channel User - Extended**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | | | | Manufacturer's ID | | | | |
| 3   | Reserved | | | | Length = 16 | | | |
| 4   | Res | P | | | Reserved | | | |
| 5   | | | | Supergroup Address | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| 8   | | | | Source Address | | | | |
| 9   | | | | | | | | |
| 10  | | | | | | | | |
| 11  | | | | | | | | |
| ⁞   | | | | Source SUID | | | | |
| 15  | | | | | | | | |
| 16  | | | | | | | | |

B1 = %1, B2 = %0, MCO = %100000, Manufacturer's ID = $90

## 3.87 MFID90 Group Regroup Voice Channel Update

The Group Regroup Voice Channel Update message is utilized by the MFID90 Airlink Efficient method of the Dynamic Regrouping operation. This outbound message is used to indicate the presence of a supergroup call on another channel. The message format is shown in Figure 180 below.

**Figure 180 – MFID90 Group Regroup Voice Channel Update**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | | | | Manufacturer's ID | | | | |
| 3   | Res | P | | | Reserved | | | |
| 4   | | | | Supergroup Address | | | | |
| 5   | | | | | | | | |
| 6   | | | | Channel | | | | |
| 7   | | | | | | | | |

B1 = %1, B2 = %0, MCO = %000011, Manufacturer's ID = $90

## 3.88 MFID90 Extended Function Command

The Extended Function Command message is utilized by the MFID90 Airlink Efficient method of the Dynamic Regrouping operation. This message is used to send an extended function command or acknowledgement to a target address. The message format is shown in Figure 181 below.

**Figure 181 – MFID90 Extended Function Command**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | | | | Manufacturer's ID | | | | |
| 3   | Reserved | | | | Length = 11 | | | |
| 4   | | | | Class | | | | |
| 5   | | | | Operand | | | | |
| 6   | | | | | | | | |
| 7   | | | | Arguments | | | | |
| 8   | | | | | | | | |
| 9   | | | | | | | | |
| 10  | | | | Target Address | | | | |
| 11  | | | | | | | | |

B1 = %1, B2 = %0, MCO = %000100, Manufacturer's ID = $90

## 3.89 MFID90 Group Regroup Voice Request

The Group Regroup Voice Request message is utilized by the MFID90 Airlink Efficient method of the Dynamic Regrouping operation. This inbound message is used by the SU to request a channel grant for a supergroup. The message format is shown in Figure 182 below.

**Figure 182 – MFID90 Group Regroup Voice Request**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | | | | Manufacturer's ID | | | | |
| 3   | Reserved | | | | Length = 9 | | | |
| 4   | Res | P | | | Reserved | | | |
| 5   | | | | Supergroup Address | | | | |
| 6   | | | | | | | | |
| 7   | | | | | | | | |
| 8   | | | | Source Address | | | | |
| 9   | | | | | | | | |

B1 = %1, B2 = %0, MCO = %100001, Manufacturer's ID = $90

## 3.90 MFID90 Extended Function Response

The Extended Function Response message is utilized by the MFID90 Airlink Efficient method of the Dynamic Regrouping operation. This inbound message is used to send an extended function command or acknowledgement. The message format is shown in Figure 183 below.

**Figure 183 – MFID90 Extended Function Response**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | | | | Manufacturer's ID | | | | |
| 3   | Reserved | | | | Length = 11 | | | |
| 4   | | | | Class | | | | |
| 5   | | | | Operand | | | | |
| 6   | | | | | | | | |
| 7   | | | | Arguments | | | | |
| 8   | | | | | | | | |
| 9   | | | | | | | | |
| 10  | | | | Source Address | | | | |
| 11  | | | | | | | | |

B1 = %1, B2 = %0, MCO = %100010, Manufacturer's ID = $90

## 3.91 MFIDA4 Group Regroup Explicit Encryption Command

The Group Regroup Explicit Encryption Command message is utilized by the MFIDA4 Explicit Encryption method of the Dynamic Regrouping operation. This outbound message is used to create, remove, or update a Supergroup Address associated with member WGID(s) or WUID(s) with explicit encryption parameters. The message format for the case where the GRG_Options field indicates "unit address" (WUID) is shown in Figure 184 below. The message format for the case where the GRG_Options field indicates "group address" (WGID) is shown in Figure 185 below.

The WUID and WGID message formats use the same B1/B2/MCO. The GRG_Options field shall be used to determine if the Target Regroup Addresses are WUIDs or WGIDs.

The Length field is set as follows for the WUID format shown in Figure 184 below:
Length = 8 + (number of WUIDs * 3) where 1 ≤ number of WUIDs ≤ 3

The Length field is set as follows for the WGID format shown in Figure 185 below:
Length = 9 + (number of WGIDs * 2) where 1 ≤ number of WGIDs ≤ 4

**Figure 184 – MFIDA4 Group Regroup Explicit Encryption Command - WUID**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | | | | Manufacturer's ID | | | | |
| 3   | Reserved | | | | Length = variable | | | |
| 4   | | | | GRG_Options | | | | |
| 5   | | | | Supergroup Address | | | | |
| 6   | | | | | | | | |
| 7   | | | | Key ID | | | | |
| 8   | | | | | | | | |
| 9   | | | | | | | | |
| 10  | | | | Regroup Target WUID | | | | |
| 11  | | | | | | | | |
| 12  | | | | | | | | |
| 13  | | | | Regroup Target WUID | | | | |
| 14  | | | | | | | | |
| 15  | | | | | | | | |
| 16  | | | | Regroup Target WUID | | | | |
| 17  | | | | | | | | |

B1 = %1, B2 = %0, MCO = %110000, Manufacturer's ID = $A4

**Figure 185 – MFIDA4 Group Regroup Explicit Encryption Command - WGID**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | | | | Manufacturer's ID | | | | |
| 3   | Reserved | | | | Length = variable | | | |
| 4   | | | | GRG_Options | | | | |
| 5   | | | | Supergroup Address | | | | |
| 6   | | | | | | | | |
| 7   | | | | Key ID | | | | |
| 8   | | | | | | | | |
| 9   | | | | ALGID | | | | |
| 10  | | | | Regroup Target WGID | | | | |
| 11  | | | | | | | | |
| 12  | | | | Regroup Target WGID | | | | |
| 13  | | | | | | | | |
| 14  | | | | Regroup Target WGID | | | | |
| 15  | | | | | | | | |
| 16  | | | | Regroup Target WGID | | | | |
| 17  | | | | | | | | |

B1 = %1, B2 = %0, MCO = %110000, Manufacturer's ID = $A4

## 3.92 MFID90 Group Regroup Add Command

The Group Regroup Add Command message is utilized by the MFID90 Airlink Efficient method of the Dynamic Regrouping operation. This outbound message is used to create or update a supergroup with a list of up to six member Working Groups. The message format is shown in Figure 186 below.

The Length field is set as follows: Length = 5 + (number of WGIDs * 2) where 1 ≤ number of WGIDs ≤ 6

**Figure 186 – MFID90 Group Regroup Add Command**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | | | | Manufacturer's ID | | | | |
| 3   | Reserved | | | | Length = variable | | | |
| 4   | | | | Supergroup Address | | | | |
| 5   | | | | | | | | |
| 6   | | | | WGID to add to the Supergroup | | | | |
| 7   | | | | | | | | |
| 8   | | | | WGID to add to the Supergroup | | | | |
| 9   | | | | | | | | |
| 10  | | | | WGID to add to the Supergroup | | | | |
| 11  | | | | | | | | |
| 12  | | | | WGID to add to the Supergroup | | | | |
| 13  | | | | | | | | |
| 14  | | | | WGID to add to the Supergroup | | | | |
| 15  | | | | | | | | |
| 16  | | | | WGID to add to the Supergroup | | | | |
| 17  | | | | | | | | |

B1 = %1, B2 = %0, MCO = %000001, Manufacturer's ID = $90

## 3.93 MFID90 Group Regroup Delete Command

The Group Regroup Add Command message is utilized by the MFID90 Airlink Efficient method of the Dynamic Regrouping operation. This outbound message is used to dissolve a supergroup or remove up to six Working Group members from a supergroup. The message format is shown in Figure 187 below.

The Length field is set as follows: Length = 5 + (number of WGIDs * 2) where 1 ≤ number of WGIDs ≤ 6

**Figure 187 – MFID90 Group Regroup Delete Command**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | | | | Manufacturer's ID | | | | |
| 3   | Reserved | | | | Length = variable | | | |
| 4   | | | | Supergroup Address | | | | |
| 5   | | | | | | | | |
| 6   | | | | WGID to remove from regrouped status | | | | |
| 7   | | | | | | | | |
| 8   | | | | WGID to remove from regrouped status | | | | |
| 9   | | | | | | | | |
| 10  | | | | WGID to remove from regrouped status | | | | |
| 11  | | | | | | | | |
| 12  | | | | WGID to remove from regrouped status | | | | |
| 13  | | | | | | | | |
| 14  | | | | WGID to remove from regrouped status | | | | |
| 15  | | | | | | | | |
| 16  | | | | WGID to remove from regrouped status | | | | |
| 17  | | | | | | | | |

B1 = %1, B2 = %0, MCO = %001001, Manufacturer's ID = $90

## 3.94 MFID90 Group Regroup Channel Grant

The Group Regroup Channel Grant message is utilized by the MFID90 Airlink Efficient method of the Dynamic Regrouping operation. This outbound message is used to initially grant a channel for a given supergroup. The implicit format is shown in Figure 188 below and the explicit format is shown in Figure 189 below.

**Figure 188 – MFID90 Group Regroup Channel Grant – Implicit**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | | | | Manufacturer's ID | | | | |
| 3   | Reserved | | | | Length = 11 | | | |
| 4   | Res | P | | | Reserved | | | |
| 5   | | | | Channel | | | | |
| 6   | | | | | | | | |
| 7   | | | | Supergroup Address | | | | |
| 8   | | | | | | | | |
| 9   | | | | | | | | |
| 10  | | | | Source Address | | | | |
| 11  | | | | | | | | |

B1 = %1, B2 = %0, MCO = %100011, Manufacturer's ID = $90

**Figure 189 – MFID90 Group Regroup Channel Grant – Explicit**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | | | | Manufacturer's ID | | | | |
| 3   | Reserved | | | | Length = 13 | | | |
| 4   | Res | P | | | Reserved | | | |
| 5   | | | | Channel (T) | | | | |
| 6   | | | | | | | | |
| 7   | | | | Channel (R) | | | | |
| 8   | | | | | | | | |
| 9   | | | | Supergroup Address | | | | |
| 10  | | | | | | | | |
| 11  | | | | | | | | |
| 12  | | | | Source Address | | | | |
| 13  | | | | | | | | |

B1 = %1, B2 = %0, MCO = %100100, Manufacturer's ID = $90

## 3.95 MFID90 Group Regroup Channel Update

The Group Regroup Channel Update message is utilized by the MFID90 Airlink Efficient method of the Dynamic Regrouping operation. This outbound message is used to update a channel grant for a given supergroup. The message format is shown in Figure 190 below.

**Figure 190 – MFID90 Group Regroup Channel Update**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | | | | Manufacturer's ID | | | | |
| 3   | Reserved | | | | Length = 11 | | | |
| 4   | | | | Channel – A | | | | |
| 5   | | | | | | | | |
| 6   | | | | Supergroup Address – A | | | | |
| 7   | | | | | | | | |
| 8   | | | | Channel – B | | | | |
| 9   | | | | | | | | |
| 10  | | | | Supergroup Address – B | | | | |
| 11  | | | | | | | | |

B1 = %1, B2 = %0, MCO = %100101, Manufacturer's ID = $90

## 3.96 MFID90 Queued Response

The Queued Response message is utilized by the MFID90 Airlink Efficient method of the Dynamic Regrouping operation. This outbound message is used to notify an SU that the MFID90 service requested cannot be granted immediately. The message format is shown in Figure 191 below.

**Figure 191 – MFID90 Queued Response**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   | B1 | B2 | | | | MCO | | |
| 2   | | | | Manufacturer's ID | | | | |
| 3   | Reserved | | | | Length = 11 | | | |
| 4   | AIV | 0 | | | Service Type | | | |
| 5   | | | | Reason Code | | | | |
| 6   | | | | | | | | |
| 7   | | | | Additional Information | | | | |
| 8   | | | | | | | | |
| 9   | | | | | | | | |
| 10  | | | | Target Address | | | | |
| 11  | | | | | | | | |

B1 = %1, B2 = %0, MCO = %100110, Manufacturer's ID = $90
