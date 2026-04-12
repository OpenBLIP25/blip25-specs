# TIA-102.BBAD-A — Two-Slot TDMA MAC Layer Messages

## Part 5 of 5: MAC Fields & Annex A (Pages 111–130)


---

### 3.97 MFID90 Deny Response

The Deny Response message is utilized by the MFID90 Airlink Efficient method of the Dynamic Regrouping operation. This outbound message is used to notify an SU that the MFID90 service requested has been denied. The message format is shown in Figure 192 below.

| O\B | 7   | 6   | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|-----|-----|---|---|---|---|---|---|
| 1   | B1  | B2  |   |   | MCO |   |   |   |
| 2   |     |     |   | Manufacturer's ID |   |   |   |   |
| 3   | Reserved |   |   |   | Length = 11 |   |   |   |
| 4   | RC  | STP |   |   | Service Type |   |   |   |
| 5   |     |     |   | Reason Code |   |   |   |   |
| 6   |     |     |   |   |   |   |   |   |
| 7   |     |     |   | Additional Information |   |   |   |   |
| 8   |     |     |   |   |   |   |   |   |
| 9   |     |     |   |   |   |   |   |   |
| 10  |     |     |   | Target Address |   |   |   |   |
| 11  |     |     |   |   |   |   |   |   |

B1 = %1, B2 = %0, MCO = %100111, Manufacturer's ID = $90

**Figure 192 — MFID90 Deny Response**

---

### 3.98 MFID90 Acknowledge Response

The Acknowledge Response message is utilized by the MFID90 Airlink Efficient method of the Dynamic Regrouping operation. This outbound message is used to notify an SU that an MFID90 service request has been received or to notify an SU of an application level acknowledgement. The message format is shown in Figure 193 below.

| O\B | 7   | 6   | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|-----|-----|---|---|---|---|---|---|
| 1   | B1  | B2  |   |   | MCO |   |   |   |
| 2   |     |     |   | Manufacturer's ID |   |   |   |   |
| 3   | Reserved |   |   |   | Length = 10 |   |   |   |
| 4   | Reserved |   |   |   | Service Type |   |   |   |
| 5   |     |     |   |   |   |   |   |   |
| 6   |     |     |   | Source Address |   |   |   |   |
| 7   |     |     |   |   |   |   |   |   |
| 8   |     |     |   |   |   |   |   |   |
| 9   |     |     |   | Target Address |   |   |   |   |
| 10  |     |     |   |   |   |   |   |   |

B1 = %1, B2 = %0, MCO = %101000, Manufacturer's ID = $90

**Figure 193 — MFID90 Acknowledge Response**

---

## 4. MAC Fields

The clauses that follow specify the fields present in various MAC PDUs and messages. See 4.17 below regarding fields specified in other documents.

### 4.1 Opcode

The 3-bit Opcode field indicates what type of MAC PDU is forthcoming. All MAC PDUs have the Opcode field as it is required to correctly interpret the PDU. A list of the Opcode values is presented in Table 9 below.

**Table 9 — Opcode Values**

| Opcode Value | MAC PDU       |
|--------------|---------------|
| %000         | MAC_SIGNAL    |
| %001         | MAC_PTT       |
| %010         | MAC_END_PTT   |
| %011         | MAC_IDLE      |
| %100         | MAC_ACTIVE    |
| %101         | Reserved      |
| %110         | MAC_HANGTIME  |
| %111         | Reserved      |

### 4.2 Offset

The 3-bit Offset field indicates the number of non-SAACH bursts between the current FACCH or SACCH and the first 4V burst in the next voice frame associated with the slot the MAC PDU occurs in. Special values are also provided for use in the random access SACCH and in cases where no voice frames are anticipated or voice framing is unknown. The offset field is present in all MAC PDUs. Table 10 below provides the valid definitions for the Offset field.

**Table 10 — Offset Values**

| Offset Value | Position of the first 4V within the voice burst sequence |
|--------------|----------------------------------------------------------|
| %000 | First 4V is in the next non-SACCH burst on this slot |
| %001 | First 4V is in the 2nd non-SACCH burst from this position on this slot |
| %010 | First 4V is in the 3rd non-SACCH burst from this position on this slot |
| %011 | First 4V is in the 4th non-SACCH burst from this position on this slot |
| %100 | First 4V is in the 5th non-SACCH burst from this position on this slot |
| %101 | Inbound: Reserved / Outbound: First 4V is in the 6th non-SACCH burst from this position on this slot |
| %110 | Inbound: For use by SUs transmitting in the Random Access SACCH / Outbound: Reserved |
| %111 | No voice framing or unknown voice framing |

The offset value of %111 is used when either there is no voice framing or voice framing is not known. For outbound, this could be either during call setup prior to receiving any voice for transmission or during call hangtime when voice has ended. For inbound this value is used when the SU is transmitting the MAC_END_PTT as no voice follows that signaling burst.

If a voice context change occurs after it has been signaled in a FACCH or SACCH burst, such as may happen in a console interrupt or a repeated SU transmission, the voice framing is updated to reflect the voice framing of the new call audio source through the insertion of a MAC PDU with an updated offset field at the time of the audio source change.

Figure 194 below gives examples of how the offset field defined in Table 10 above would be applied to the full set of voice framing possibilities.

**Figure 194 — Offset Field Usage**

The figure shows five voice framing scenarios in a 360 ms superframe. Each scenario shows the burst sequence from SACCH through voice bursts to the next SACCH:

| Scenario | SACCH offsets | Burst sequence (after SACCH) | Mid-frame SACCH offset | Post-SACCH voice burst sequence |
|----------|--------------|------------------------------|------------------------|---------------------------------|
| 1 | %001, %001, %000 | PT.1, PT.0, 4V, 4V, 4V, 4V | %001 | 2V, 4V, 4V, 4V, 4V |
| 2 | %111, %111, %001, %000 | PT.1, PT.0, 4V, 4V, 4V | %010 | 4V, 2V, 4V, 4V, 4V |
| 3 | %111, %111, %111, %001, %000 | PT.1, PT.0, 4V, 4V | %011 | 4V, 4V, 2V, 4V, 4V |
| 4 | %111, %111, %111, %111, %001, %000 | PT.1, PT.0, 4V | %100 | 4V, 4V, 4V, 2V, 4V |
| 5 | %111, %111, %111, %111, %111, %001, %000, %000 | PT.1, PT.0 | %000 | 4V, 4V, 4V, 4V, 2V |

Notes:
- PT.1 is 1st MAC PTT
- PT.0 is 2nd MAC PTT
- These sequences of five voice bursts repeat until the talk spurt ends

### 4.3 Protected (P)

The Protected (P) field indicates whether or not the MAC PDU Contents are protected:

- P = 0, for non-protected (clear)
- P = 1, for protected (encrypted)

Receivers that do not support the protection method shall discard protected MAC PDUs.

### 4.4 RF Level

The 4-bit RF Level field indicates the signal strength relative to "S" which is the signal strength that produces approximately 1% static BER as measured at the FNE. A list of the RF Level values is presented in Table 11 below.

**Table 11 — RF Level Values**

| Value | RF Level    |
|-------|-------------|
| %0000 | Unknown    |
| %0001 | S − 15 dBm |
| %0010 | S − 12 dBm |
| %0011 | S − 9 dBm  |
| %0100 | S − 6 dBm  |
| %0101 | S − 3 dBm  |
| %0110 | S dBm      |
| %0111 | S + 3 dBm  |
| %1000 | S + 6 dBm  |
| %1001 | S + 9 dBm  |
| %1010 | S + 12 dBm |
| %1011 | S + 15 dBm |
| %1100 | S + 18 dBm |
| %1101 | S + 21 dBm |
| %1110 | S + 24 dBm |
| %1111 | S + 27 dBm |

### 4.5 BER

The 4-bit BER field indicates the BER calculated by the FNE. A list of the BER values is presented in Table 12 below. Use of the BER field in the Power Control message is optional. The field should be set to %1111 (Unused) if BER measurement is not supported by the FNE.

**Table 12 — BER Values**

| Value | BER                    |
|-------|------------------------|
| %0000 | 0% <= BER < 0.08%     |
| %0001 | 0.08% <= BER <= 0.12% |
| %0010 | 0.12% <= BER <= 0.18% |
| %0011 | 0.18% <= BER <= 0.27% |
| %0100 | 0.27% <= BER <= 0.39% |
| %0101 | 0.39% <= BER <= 0.57% |
| %0110 | 0.57% <= BER <= 0.84% |
| %0111 | 0.84% <= BER <= 1.25% |
| %1000 | 1.25% <= BER <= 1.35% |
| %1001 | 1.35% <= BER <= 2.7%  |
| %1010 | 2.7% <= BER <= 3.9%   |
| %1011 | 3.9% <= BER <= 5.7%   |
| %1100 | 5.7% <= BER <= 8.4%   |
| %1101 | 8.4% <= BER <= 12.5%  |
| %1110 | 12.5% <= BER          |
| %1111 | Unused                 |

### 4.6 CRC-12

The CRC-12 included in the VCH MAC PDU Trailer (see Figure 5 above) is calculated over all bits in the PDU up to, but not including, the CRC-12 field itself.

The CRC-12 generator polynomial is denoted as g₁₂(x). The redundancy, r, is 12. The CRC generator polynomial is as follows:

```
g_12(x) = x^12 + x^11 + x^7 + x^4 + x^2 + x + 1        (1)
```

The information bits are arranged into a vector. The MSB of the first octet is the first bit of the vector and the LSB of the octet immediately preceding the VCH MAC PDU Trailer is the last bit of the vector. For example, the I-OEMI PDU defined in 2.3.1 above has 22.5 octets in total and 21 octets without the CRC-12 field. The information bits, in that case, form a vector of 168 bits, consisting of the 8-bit MAC PDU Header and 160 bits in octets 1 through 20. The k bits in the vector (k=168 in the example) then correspond to k binary coefficients in the polynomial representation denoted as M(x):

```
M(x) = m_(k-1) * x^(k-1) + m_(k-2) * x^(k-2) + ... + m_2 * x^2 + m_1 * x + m_0    (2)
```

Coefficient m_(k-1) corresponds to the 1st bit of the vector, coefficient m_(k-2) corresponds to the 2nd bit of the vector, coefficient m_(k-3) corresponds to the 3rd bit of the vector, etc.

The CRC is computed by dividing M(x) · x^r by the CRC generator polynomial g_CRC(x) = g₁₂(x) to compute the quotient, q(x), and the remainder, p(x). The degree of p(x) is r-1 or less.

```
M(x) * x^r = q(x) * g_CRC(x) + p(x)                     (3)

such that degree(p) < degree(g_CRC)
```

The CRC then corresponds to the inverted coefficients of p(x), namely p_(r-1)\*, p_(r-2)\*, … p_1\*, p_0\*, where binary inversion is denoted with a "\*" character. The bits in the penultimate octet correspond to: p_(r-1)\*, p_(r-2)\*, … p_(r-7)\*, p_(r-8)\*. The LSBs of the last octet correspond to p_(r-9)\*, p_(r-10)\*, … p_1\*, and p_0\*.

> **Formula extraction note:** Equations (1)–(3) were reconstructed from the PDF page images with superscript exponents. The exponents in equation (1) are clearly legible: x^12, x^11, x^7, x^4, x^2, x, 1. No ambiguity.

### 4.7 CRC-16

The LCCH MAC PDUs and multi-fragment messages utilize a 16-bit CRC denoted as CRC-16.

The CRC-16 included in the LCCH MAC PDU trailers (see Figure 3 and Figure 4 above) is calculated over all bits in the PDU up to, but not including, the CRC-16 field itself.

The Multi-Fragment CRC-16 field included in the PDU Contents of the last fragment of a multi-fragment message is calculated over all bits of the PDU Contents in each fragment, excluding the first two octets of the PDU Contents in each fragment, up to, but not including, the Multi-Fragment CRC-16 field itself.

Figure 195 and Figure 196 below provide a hypothetical example of an outbound multi-fragment message comprised of two OECI PDUs with seven information fields of various sizes to illustrate the Multi-Fragment CRC-16 calculation. The PDU Contents portion of each OECI PDU is shown with a thicker border line. The MCO of the first fragment is unique for each multi-fragment message and is shown as %xxxxxx in the example. The gray-shaded fields are included in the calculation of the Multi-Fragment CRC-16 field shown in the PDU Contents of the last fragment. The second OECI PDU of the example includes a Null Avoid Zero Bias message following the Multi-Fragment CRC-16 field to fill out the remainder of the PDU. This method also applies to inbound multi-fragment messages.

**Figure 195 — Example Outbound Fragment 1 of 2**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 0   | | | MAC PDU Header | | | | | |
| 1   | B1 | B2 | | | MCO | | | |
| 2   | Reserved | | | | Length = 18 | | | |
| 3   | | | | Data Length = 28 | | | | |
| 4   | | | | Field 1 | | | | |
| 5   | | | | | | | | |
| 6   | | | | Field 2 | | | | |
| 7   | | | | | | | | |
| 8   | | | | | | | | |
| 9–13 | | | | Field 3 | | | | |
| 14  | | | | | | | | |
| 15  | | | | Field 4 | | | | |
| 16  | | | | | | | | |
| 17  | | | | Field 5 | | | | |
| 18  | | | | | | | | |
| 19  | | | | | | | | |
| 20  | | | Outbound LCCH MAC PDU Trailer | | | | | |
| 21  | | | | | | | | |
| 22  | | | | | | | | |

B1 = %1, B2 = %1, MCO = %xxxxxx

**Figure 196 — Example Outbound Fragment 2 of 2**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 0   | | | MAC PDU Header | | | | | |
| 1   | B1 | B2 | | | MCO = %010000 | | | |
| 2   | Reserved | | | | Length = 14 | | | |
| 3   | | | | | | | | |
| 4   | | | | Field 6 | | | | |
| 5   | | | | | | | | |
| 6   | | | | | | | | |
| 7–11 | | | | Field 7 | | | | |
| 12  | | | | | | | | |
| 13  | | | Multi-Fragment CRC-16 | | | | | |
| 14  | | | | | | | | |
| 15  | B1 | B2 | | | MCO = %001000 | | | |
| 16  | Reserved | | | | Length = 4 | | | |
| 17  | | | | Avoid Zero Bias | | | | |
| 18  | | | | | | | | |
| 19  | | | | | | | | |
| 20  | | | Outbound LCCH MAC PDU Trailer | | | | | |
| 21  | | | | | | | | |
| 22  | | | | | | | | |

B1 = %0, B2 = %0 (both occurrences)

The CRC-16 is calculated using the cyclical redundant coding procedure specified for the header block of the FDMA CAI (see [3]). The following paragraphs specify the procedure in the context of an LCCH MAC PDU and a multi-fragment MAC message.

The information bits are arranged into a vector. The MSB of first octet is the first bit of the vector and the LSB of the octet immediately preceding the CRC-16 is the last bit of the vector. For example, the OECI PDU defined in 2.2.1 above has 22.5 octets in total and 20.5 octets without the CRC-16. The information bits, in that case, form a vector of 164 bits, comprising bit 7 of octet 0 through bit 4 of octet 20. In another example, the multi-fragment MAC message shown in Figure 195 and Figure 196 above has 28 octets in total and 26 octets without the Multi-Fragment CRC-16. The information bits, in that case, form a vector of 208 bits, comprising bit 7 of octet 3 through bit 0 of octet 18 shown in Figure 195 above and bit 7 of octet 3 through bit 0 of octet 12 in Figure 196 above.

Consider the k information bits of the vector as the coefficients of the polynomial M(x) of degree k-1, associating the first bit of the vector with x^(k-1) and the last bit of the vector with x^0. Define the generator polynomial, G(x), and the inversion polynomial, I(x).

```
G(x) = x^16 + x^12 + x^5 + 1                             (4a)

I(x) = x^15 + x^14 + x^13 + ... + x^2 + x + 1           (4b)
```

The CRC-16 polynomial, F(x), is computed from the formula:

```
F(x) = ( x^16 * M(x)  mod  G(x) ) + I(x)     modulo 2   (5)
```

The coefficients of F(x) are placed in the CRC-16 field with the MSB of the zero-th octet of the CRC corresponding to x^15 and the LSB of the next octet of the CRC-16 corresponding to x^0.

> **Formula extraction notes:**
> - Equation (4a): Generator polynomial exponents clearly read x^16, x^12, x^5, 1 from the page image. No ambiguity.
> - Equation (4b): Inversion polynomial I(x) is the sum of all powers from x^15 down to x^0 (i.e., all 16 coefficients are 1). The page shows "x^15 + x^14 + x^13 + … +x^2 + x + 1".
> - Equation (5): The formula shows x^16 multiplied by M(x), taken mod G(x), then added to I(x), all modulo 2. No ambiguity.

### 4.8 Unforced/Forced (U/F)

The Unforced/Forced (U/F) field indicates the type of preemption:

- U/F = 0, for an unforced preemption (see [6])
- U/F = 1, for a forced preemption (see [6])

### 4.9 Call Preemption/Audio (C/A)

The Call Preemption/Audio (C/A) field indicates whether a call preemption or talker preemption is taking place. This field is used to signal the current talker that a change in the call has taken place:

- C/A = 0, the call is no longer for the talker, so the talker SU should go back to CCH
- C/A = 1, the call is still for the talker's group, however, the outbound audio is no longer the talkers; in this case the talker SU is allowed to stay on the VCH

### 4.10 Color Code

The Color Code is the same value as the NAC for FDMA channels, specifically the FDMA Control Channel if this is used. The NAC code for FDMA channels and the Color Code for TDMA channels are used to identify and reject co-channel interfering sources. Co-channel rejection on FDMA channels is accomplished through detection of an invalid NAC. Co-channel rejection on TDMA channels is accomplished through the process of scrambling or by detection of an invalid Color Code in MAC PDUs that are not scrambled. The Color Code is one of the unique elements used to initialize the scrambling sequence. See [6] for a complete description of the scrambling process.

On systems having TDMA voice channels and FDMA control channels, the Color Code of the TDMA traffic channel can be determined by monitoring the NAC associated with the Trunking Signaling Blocks (TSBK)) transmitted on the FDMA CCH. Additionally, the Color Code is transmitted in the Network Status Broadcast MAC messages sent on the TDMA VCH.

The Color Code is also embedded within the non-scrambled MAC_END_PTT PDUs to allow simple identification of the system of an interfering TDMA VCH.

### 4.11 Length

The Length field is set to the number of octets in the MAC message within the current MAC PDU and includes the B1/B2/MCO octet as well as the octet the Length field occupies. The Length field does not include any octets that are conveyed in another PDU of a multi-fragment message.

### 4.12 Data Length

The Data Length field is set to the total number of octets in a multi-fragment MAC message and is always the third octet of the first fragment. This field excludes the B1/B2/MCO octet as well as the Length octet of each PDU.

### 4.13 Subscriber Unit Identification (SUID)

The general SUID field format shown in Figure 197 below is used to provide the SUID of a subscriber unit for messages that require the extended address. This general format may be used to identify a Source SUID or a Target SUID in messages.

**Figure 197 — General SUID Field Format**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   |   |   |   | WACN ID |   |   |   |   |
| 2   |   |   |   | *(continued)* |   |   |   |   |
| 3   |   |   |   | *(continued)* |   |   |   |   |
| 4   |   |   |   | System ID |   |   |   |   |
| 5   |   |   |   | *(continued)* |   |   |   |   |
| 6   |   |   |   | Unit ID |   |   |   |   |
| 7   |   |   |   | *(continued)* |   |   |   |   |

Field spans:
- **WACN ID**: 20 bits — octet 1 bit 7 through octet 3 bit 4
- **System ID**: 12 bits — octet 3 bit 3 through octet 5 bit 0
- **Unit ID**: 24 bits — octet 5 bit 7 through octet 7 bit 0

> Note: The WACN ID occupies bits [7:0] of octets 1–2 and bits [7:4] of octet 3 (20 bits). System ID occupies bits [3:0] of octet 3 and all of octets 4–5 (12 bits total — *correction: this is actually bits [3:0] of octet 3 = 4 bits, plus all 8 bits of octet 4 = 12 bits total, with octet 5 beginning Unit ID*). Per the standard figure, WACN ID spans octets 1–3 (bits 7 of octet 1 through bit 4 of octet 3 = 20 bits), System ID spans octets 3–4 (bit 3 of octet 3 through bit 0 of octet 4, but the figure shows System ID occupying octets 4–5 area meaning bits [3:0] of octet 3 + all of octet 4 = 12 bits), and Unit ID spans octets 6–7 (but the figure shows octets 5–7 area for a 24-bit field, which would be bits of octets 5, 6, 7 = 24 bits). The visual layout in the standard is: WACN ID = 20 bits (octets 1-2 full + octet 3 upper nibble), System ID = 12 bits (octet 3 lower nibble + octet 4 full), Unit ID = 24 bits (octets 5-6-7 full). Total = 56 bits = 7 octets.

### 4.14 Subscriber Group Identification (SGID)

The general SGID field format shown in Figure 198 below is used in messages that require the extended Group ID.

**Figure 198 — General SGID Field Format**

| O\B | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
|-----|---|---|---|---|---|---|---|---|
| 1   |   |   |   | WACN ID |   |   |   |   |
| 2   |   |   |   | *(continued)* |   |   |   |   |
| 3   |   |   |   | *(continued)* |   |   |   |   |
| 4   |   |   |   | System ID |   |   |   |   |
| 5   |   |   |   | Group ID |   |   |   |   |
| 6   |   |   |   | *(continued)* |   |   |   |   |

Field spans:
- **WACN ID**: 20 bits — octet 1 bit 7 through octet 3 bit 4
- **System ID**: 12 bits — octet 3 bit 3 through octet 4 bit 0
- **Group ID**: 16 bits — octet 5 bit 7 through octet 6 bit 0

### 4.15 Avoid Zero Bias

Each octet of the Avoid Zero Bias field is set to 0x88.

### 4.16 Service Type

The Service Type field is set to the MCO value for the service being acted upon. The service being acted upon may be initiated with a MAC message from the B1=%0 / B2=%1 partition or a MAC message from the B1=%1 / B2=%1 partition. Regardless of which MAC message was used to initiate the service, the MCO value from the B1=%0 / B2=%1 partition is used to identify the service in the Service Type field.

### 4.17 Other Fields from FDMA

Several fields used in MAC messages correspond to fields used in FDMA TSBKs specified in [2]. These field names are listed in Table 13 below which also provides the size of the field and a reference to the subclause in [2] that specifies the field contents.

The fields used in Dynamic Regrouping MAC messages are specified in [4] for the corresponding FDMA TSBKs.

**Table 13 — FDMA Field References**

| Name | Size (bits) | Reference [2] | | Name | Size (bits) | Reference [2] |
|------|-------------|---------------|-|------|-------------|---------------|
| A | 1 | 6.2.2.1 | | LM | 1 | 6.2.26 |
| AA | 1 | 5.2.7 | | Local Time Offset | 12 | 6.2.27 |
| Additional Information | var | 2.3.1 | | LRA | 8 | 2.3.20 |
| AIV | 1 | 2.3.2 | | Manufacturer's ID | 8 | 2.1 |
| ALGID (Alg ID) | 8 | 6.1.26 | | MC | 2 | 6.2.33 |
| Announcement Group | 16 | 2.3.3 | | Message | 16 | 2.3.21 |
| Answer Response | 8 | 2.3.4 | | Micro-slots | 13 | 6.2.33 |
| Arguments | 24 | 2.3.15 | | Minutes | 6 | 6.2.33 |
| Base Frequency | 32 | 2.3.5 | | MM | 1 | 6.2.33 |
| BW | 9 | 2.3.6 | | Month | 4 | 6.2.33 |
| BW VU | 4 | 2.3.7 | | MSN | 4 | 6.2.26 |
| C | 1 | 6.2.2.1 | | Operand | 8 | 2.3.15 |
| Call Timer | 16 | 2.3.8 | | PSTN Address | 8 | 4.1.5 |
| Capabilities | 7 | 2.3.39 | | R | 1 | 6.2.15 |
| Channel | 16 | 2.3.9 | | R2 | 1 | 6.1.22.1 |
| Channel (R) | 16 | 2.3.9 | | RA | 1 | 5.2.7 |
| Channel (T) | 16 | 2.3.9 | | RAND1 | 40 | 6.2.30 |
| Channel Identifier | 4 | 2.3.9.1 | | RAND2 | 40 | 6.1.24 |
| Channel Spacing | 10 | 2.3.10 | | Reason Code | 8 | 2.3.22 |
| Channel Type | 4 | 2.3.40 | | Request Priority Level | 8 | 6.2.19 |
| Class | 8 | 2.3.15 | | RES1 | 32 | 6.1.23.1 |
| Data Access Control | 16 | 2.3.11 | | RES2 | 32 | 6.2.31.1 |
| Data Service Options | 8 | 2.3.12 | | RFSS ID | 8 | 2.3.23 |
| Date | 24 | 6.2.27 | | RV | 2 | 6.2.21.1 |
| Day | 5 | 6.2.33 | | RS | 80 | 6.2.30 |
| Digit | 4 | 2.3.13 | | S | 1 | 6.1.22.1 |
| Digit Count | 8 | 2.3.14 | | Service Options | 8 | 2.3.24 |
| DS | 1 | 5.1.5 | | SI-1 | 8 | 6.1.6 |
| E | 1 | 2.3.24 | | SI-2 | 8 | 6.1.6 |
| EX | 1 | 6.2.1.1 | | Site ID | 8 | 2.3.26 |
| Extended Function | 40 | 2.3.15 | | SM | 1 | 6.1.19 |
| F | 1 | 6.2.2.1 | | Source Address | 24 | 2.3.27 |
| Flags | 4 | 6.1.10 | | Source ID | 24 | 2.3.28 |
| GAV | 2 | 6.2.8.1 | | Stack Operation | 8 | 6.2.25 |
| Group Address | 16 | 2.3.16 | | Status | 16 | 2.3.29 |
| Group ID | 16 | 2.3.17 | | System ID | 12 | 2.3.31 |
| Hours | 5 | 6.2.33 | | System Service Class | 8 | 2.3.32 |
| IST | 1 | 6.2.33 | | System Services | 24 | 6.2.19 |
| Key ID | 16 | 6.1.26 | | Target Address | 24 | 2.3.33 |
| LG | 1 | 6.2.8.1 | | Target ID | 24 | 2.3.34 |
| TG | 1 | 6.1.26 | | US | 1 | 6.2.33 |
| Time | 24 | 6.2.27 | | V | 1 | 6.2.2.1 |
| Transmit Offset | 9 | 2.3.35 | | VD | 1 | 6.2.27 |
| Transmit Offset TDMA | 14 | 2.3.41 | | VL | 1 | 6.2.27 |
| Transmit Offset VU | 14 | 2.3.36 | | VT | 1 | 6.2.27 |
| Twuid Validity | 8 | 6.2.19 | | WACN ID | 20 | 2.3.37 |
| TX Mult | 2 | 6.1.19 | | Year | 7 | 6.2.33 |
| TX Time | 8 | 6.1.19 | | | | |

\* Announcement Group = Announcement Group Address

---

## Annex A — MAC Message Mapping (Informative)

This annex provides a mapping of FDMA control channel signaling packets defined in [2] and in [4] to two-slot TDMA MAC messages.

### A.1 Inbound MAC Message Mapping

Table 14 below provides a mapping of FDMA ISP's Opcode and Alias defined in [2] to TDMA MAC messages.

**Table 14 — ISP Opcode Summary**

| Opcode | Alias | MAC Message Name | Subclause |
|--------|-------|------------------|-----------|
| %000000 | GRP_V_REQ | Group Voice Service Request | 3.25 |
| %000001 | Reserved | | |
| %000010 | Reserved | | |
| %000011 | Reserved | | |
| %000100 | UU_V_REQ | Unit to Unit Voice Service Request | 3.5 |
| %000101 | UU_ANS_RSP | Unit to Unit Voice Service Answer Response | 3.55 |
| %000110 | Reserved | | |
| %000111 | Reserved | | |
| %001000 | TELE_INT_DIAL_REQ | Telephone Interconnect Request Explicit Dialing | 3.56 |
| %001001 | TELE_INT_PSTN_REQ | Telephone Interconnect Request Implicit Dialing | 3.57 |
| %001010 | TELE_INT_ANS_RSP | Telephone Interconnect Answer Response | 3.58 |
| %001011 | Reserved | | |
| %001100 | Reserved | | |
| %001101 | Reserved | | |
| %001110 | Reserved | | |
| %001111 | Reserved | | |
| %010000 | Obsolete | | |
| %010001 | Obsolete | | |
| %010010 | SN-DATA_CHN_REQ | SNDCP Data Channel Request | 3.59 |
| %010011 | SN-DATA_PAGE_RES | SNDCP Data Page Response | 3.60 |
| %010100 | SN-DATA_REC_REQ | SNDCP Reconnect Request | 3.61 |
| %010101 | Reserved | | |
| %010110 | Reserved | | |
| %010111 | Reserved | | |
| %011000 | STS_UPDT_REQ | Status Update Request | 3.73 |
| %011001 | STS_Q_RSP | Status Query Response | 3.72 |
| %011010 | STS_Q_REQ | Status Query Request | 3.71 |
| %011011 | Reserved | | |
| %011100 | MSG_UPDT_REQ | Message Update Request | 3.70 |
| %011101 | RAD_MON_REQ | Radio Unit Monitor Request | 3.77 |
| %011110 | RAD_MON_ENH_REQ | Radio Unit Monitor Enhanced Request | 3.84 |
| %011111 | CALL_ALRT_REQ | Call Alert Request | 3.63 |
| %100000 | ACK_RSP_U | Acknowledge Response Unit | 3.62 |
| %100001 | Reserved | | |
| %100010 | Reserved | | |
| %100011 | CAN_SRV_REQ | Cancel Service Request | 3.64 |
| %100100 | EXT_FNCT_RES | Extended Function Response | 3.66 |
| %100101 | Reserved | | |
| %100110 | Reserved | | |
| %100111 | EMERG_ALRM_REQ | Emergency Alarm Request | 3.65 |
| %101000 | GRP_AFF_REQ | Group Affiliation Request | 3.68 |
| %101001 | GRP_AFF_Q_RSP | Group Affiliation Query Response | 3.67 |
| %101010 | Reserved | | |
| %101011 | U_DE_REG_REQ | Unit Deregistration Request | 3.75 |
| %101100 | U_REG_REQ | Unit Registration Request | 3.74 |
| %101101 | LOC_REG_REQ | Location Registration Request | 3.76 |
| %101110 | Reserved | | |
| %101111 | Reserved | | |
| %110000 | Reserved | | |
| %110001 | Reserved | | |
| %110010 | IDEN_UP_REQ | Identifier Update Request | 3.69 |
| %110011 | Reserved | | |
| %110100 | Reserved | | |
| %110101 | Reserved | | |
| %110110 | ROAM_ADDR_REQ | Roaming Address Request | 3.78 |
| %110111 | ROAM_ADDR_RES | Roaming Address Response | 3.79 |
| %111000 | AUTH_RESP | Authentication Response | 3.81 |
| %111001 | AUTH_RESP_M | Authentication Response Mutual | 3.82 |
| %111010 | AUTH_FNE_RST | Authentication FNE Result | 3.80 |
| %111011 | AUTH_SU_DMD | Authentication SU Demand | 3.83 |
| %111100 | Reserved | | |
| %111101 | Reserved | | |
| %111110 | Reserved | | |
| %111111 | Reserved | | |

### A.2 Outbound MAC Message Mapping

Table 15 below provides a mapping of FDMA OSP's Opcode and Alias defined in [2] to TDMA MAC messages.

There was a need to add fields to the original Radio Unit Monitor Command. To account for fielded equipment that may have implemented the original message format, a previously reserved Opcode from the FDMA OSPs was used as the MCO for the new message format. Also, the original message name was changed to Radio Unit Monitor Command – Obsolete in 3.33 above and the new Radio Unit Monitor Command message is defined in 3.43 above. This change took effect when the TIA-102.BBAC-1 addendum was published.

Several of the Extended message formats defined for the VCH did not have all the fields necessary for use on the LCCH. For these Extended message formats, VCH was appended to the message name and a new Extended LCCH message format was defined using a previously reserved Opcode from the FDMA OSPs as the MCO. This change took effect when TIA-102.BBAD was published (August 2017). The message formats for Abbreviated and Extended VCH were not changed, only VCH was added to the extended format name. The following is a list of the messages for which this applies and the subclause where the Abbreviated, Extended VCH, and Extended LCCH message formats are defined: Unit to Unit Voice Service Channel Grant (3.9), Unit to Unit Voice Channel Grant Update (3.13), Call Alert (3.19), Extended Function Command (3.20), Status Query (3.28), Status Update (3.41), Message Update (3.42), Radio Unit Monitor Command (3.43).

**Table 15 — OSP Opcode Summary**

| Opcode | Alias | MAC Message Name | Subclause |
|--------|-------|------------------|-----------|
| %000000 | GRP_V_CH_GRANT | Group Voice Channel Grant | 3.7 |
| %000001 | Reserved | | |
| %000010 | GRP_V_CH_GRANT_UPDT | Group Voice Channel Grant Update | 3.8 |
| %000011 | GRP_V_CH_GRANT_UPDT_EXP | Group Voice Channel Grant Update - Explicit | 3.8 |
| %000100 | UU_V_CH_GRANT | Unit to Unit Voice Service Channel Grant | 3.9 |
| %000101 | UU_ANS_REQ | Unit to Unit Answer Request | 3.10 |
| %000110 | UU_V_CH_GRANT_UPDT | Unit to Unit Voice Channel Grant Update | 3.13 |
| %000111 | Reserved | Unit to Unit Voice Channel Grant Update LCCH | 3.13 |
| %001000 | TELE_INT_CH_GRANT | Telephone Interconnect Voice Channel Grant | 3.44 |
| %001001 | TELE_INT_CH_GRANT_UPDT | Telephone Interconnect Voice Channel Grant Update | 3.45 |
| %001010 | TELE_INT_ANS_REQ | Telephone Interconnect Answer Request | 3.12 |
| %001011 | Reserved | Call Alert - Extended LCCH | 3.19 |
| %001100 | Reserved | Radio Unit Monitor Command | 3.43 |
| %001101 | Reserved | Radio Unit Monitor Command LCCH | 3.43 |
| %001110 | Reserved | Message Update LCCH | 3.42 |
| %001111 | Reserved | Unit to Unit Voice Service Channel Grant LCCH | 3.9 |
| %010000 | Obsolete | | |
| %010001 | Obsolete | | |
| %010010 | Obsolete | | |
| %010011 | Obsolete | | |
| %010100 | SN-DATA_CHN_GNT | SNDCP Data Channel Grant | 3.15 |
| %010101 | SN-DATA_PAGE_REQ | SNDCP Data Page Request | 3.16 |
| %010110 | SN-DATA_CHN_ANN_EXP | SNDCP Data Channel Announcement - Explicit | 3.17 |
| %010111 | Reserved | | |
| %011000 | STS_UPDT | Status Update | 3.41 |
| %011001 | Reserved | Status Update LCCH | 3.41 |
| %011010 | STS_Q | Status Query | 3.28 |
| %011011 | Reserved | Status Query LCCH | 3.28 |
| %011100 | MSG_UPDT | Message Update | 3.42 |
| %011101 | RAD_MON_CMD | Radio Unit Monitor Command - Obsolete | 3.33 |
| %011110 | RAD_MON_ENH_CMD | Radio Unit Monitor Enhanced Command | 3.11 |
| %011111 | CALL_ALRT | Call Alert | 3.19 |
| %100000 | ACK_RSP_FNE | ACK Response FNE | 3.14 |
| %100001 | QUE_RSP | Queued Response | 3.29 |
| %100010 | Reserved | | |
| %100011 | Reserved | | |
| %100100 | EXT_FNCT_CMD | Extended Function Command | 3.20 |
| %100101 | Reserved | Extended Function Command LCCH | 3.20 |
| %100110 | Reserved | | |
| %100111 | DENY_RSP | Deny Response | 3.30 |
| %101000 | GRP_AFF_RSP | Group Affiliation Response | 3.46 |
| %101001 | SCCB_EXP | Secondary Control Channel Broadcast - Explicit | 3.27 |
| %101010 | GRP_AFF_Q | Group Affiliation Query | 3.21 |
| %101011 | LOC_REG_RSP | Location Registration Response | 3.49 |
| %101100 | U_REG_RSP | Unit Registration Response | 3.47 |
| %101101 | U_REG_CMD | Unit Registration Command | 3.32 |
| %101110 | Reserved | | |
| %101111 | U_DE_REG_ACK | Unit Deregistration Acknowledge | 3.48 |
| %110000 | SYNC_BCST | Synchronization Broadcast | 3.54 |
| %110001 | AUTH_DMD | Authentication Demand | 3.52 |
| %110010 | AUTH_FNE_RESP | Authentication FNE Response | 3.53 |
| %110011 | IDEN_UP_TDMA | Identifier Update for TDMA | 3.35 |
| %110100 | IDEN_UP_VU | Identifier Update for VHF/UHF Bands | 3.34 |
| %110101 | TIME_DATE_ANN | Time and Date Announcement | 3.23 |
| %110110 | ROAM_ADDR_CMD | Roaming Address Command | 3.50 |
| %110111 | ROAM_ADDR_UPDT | Roaming Address Update | 3.51 |
| %111000 | SYS_SRV_BCST | System Service Broadcast | 3.31 |
| %111001 | SCCB | Secondary Control Channel Broadcast - Implicit | 3.27 |
| %111010 | RFSS_STS_BCST | RFSS Status Broadcast | 3.26 |
| %111011 | NET_STS_BCST | Network Status Broadcast | 3.24 |
| %111100 | ADJ_STS_BCST | Adjacent Status Broadcast | 3.18 |
| %111101 | IDEN_UP | Identifier Update | 3.22 |
| %111110 | ADJ_STS_BCST_UNC | Adjacent Status Broadcast - Extended - Explicit | 3.18 |
| %111111 | Reserved | | |

### A.3 Inbound Dynamic Regrouping MAC Message Mapping

Table 16 below provides a mapping of Dynamic Regrouping FDMA ISP's Opcode and Alias defined in [4] to TDMA MAC messages.

**Table 16 — Dynamic Regrouping ISP Opcode Summary**

| Opcode | Alias | MAC Message Name | Subclause |
|--------|-------|------------------|-----------|
| %000000 | MOT_GRP_V_REQ | MFID90 Group Regroup Voice Service Request | 3.89 |
| %000001 | MOT_EXT_FNCT_RSP | MFID90 Extended Function Response | 3.90 |
| %000010 – %111111 | Reserved | | |

### A.4 Outbound Dynamic Regrouping MAC Message Mapping

Table 17 below provides a mapping of Dynamic Regrouping FDMA OSP's Opcode and Alias defined in [4] to TDMA MAC messages.

**Table 17 — Dynamic Regrouping OSP Opcode Summary**

| Opcode | Alias | MAC Message Name | Subclause |
|--------|-------|------------------|-----------|
| %000000 | MOT_GRG_ADD_CMD | MFID90 Group Regroup Add Command | 3.92 |
| %000001 | MOT_GRG_DEL_CMD | MFID90 Group Regroup Delete Command | 3.93 |
| %000010 | MOT_GRG_CN_GRANT | MFID90 Group Regroup Channel Grant | 3.94 |
| %000011 | MOT_GRG_CN_GRANT_UPDT | MFID90 Group Regroup Channel Update | 3.95 |
| %000100 | MOT_EXT_FNCT_CMD | MFID90 Extended Function Command | 3.88 |
| %000101 | Reserved | | |
| %000110 | MOT_QUE_RSP | MFID90 Queued Response | 3.96 |
| %000111 | MOT_DENY_RSP | MFID90 Deny Response | 3.97 |
| %001000 | MOT_ACK_RSP_FNE | MFID90 Acknowledge Response | 3.98 |
| %001001 – %101111 | Reserved | | |
| %110000 | GRG_EXENC_CMD | MFIDA4 Group Regrouping Explicit Encryption Command | 3.91 |
| %110001 – %111111 | Reserved | | |

---

*End of Part 5 of 5 — TIA-102.BBAD-A*
