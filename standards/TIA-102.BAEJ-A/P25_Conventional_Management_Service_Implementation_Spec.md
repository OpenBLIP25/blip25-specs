# P25 Conventional Management Service (CMS) for Packet Data — TIA-102.BAEJ-A Implementation Spec

**Source:** TIA-102.BAEJ-A (2019-03), *Conventional Management Service
Specification for Packet Data*. Revision A of BAEJ (2013); the 2013
edition was cancelled and replaced. Revision A refined Scan Mode
operational text and updated normative references; **PDU wire formats
are unchanged from the 2013 original**.

**Document type:** MESSAGE_FORMAT + PROTOCOL. Defines the Conventional
Management Service (CMS) protocol by which a Fixed Network Equipment
(FNE) manages subscriber unit (SU) access to **conventional** P25
packet data channels. CMS is the management-plane companion to the
CAI Data Bearer Service and the IP Data Bearer Service (with SCEP) on
the Conventional FNE Data configuration — see BAEA-C §6.1.3 / §6.2.3.

**Where it does and does not apply:**

| Packet Data configuration | CMS used? |
|---------------------------|-----------|
| Direct Data | No — no FNE, no registration |
| Repeated Data | No — FSR is pure PHY/MAC, no management |
| **Conventional FNE Data** | **YES** — CMS is the conventional management protocol |
| Trunked FNE Data | No — trunked uses TMS (on the control channel) instead |

Furthermore, **CMS Scan Mode is disabled when IP Data Bearer is used
with SNDCP** on Conventional FNE (§5.1.2 / RO bit b0 requirement) —
SNDCP has its own Scan Mode mechanism per BAEB-B/C.

**Scope of this derived work:**
- §1 — Where CMS fits in the data stack
- §2 — Four CMS functions
- §3 — SU and FNE state machines
- §4 — Registration procedure (static vs dynamic)
- §5 — Common Registration PDU format (the 12-octet RT/RO/LLID/IP layout)
- §6 — Disconnect PDU format (the 8-octet variant)
- §7 — Scan Preamble PDU (empty)
- §8 — Unable to Decrypt PDU format
- §9 — Field encodings (RT, RO, SEI MT, EC)
- §10 — Delivery service and SAP assignments per PDU
- §11 — Motorola MFID=0x90 backward-compatibility
- §12 — Cite-to section references
- §13 — Cross-references

**Pipeline artifacts:**
- `standards/TIA-102.BAEJ-A/TIA-102-BAEJ-A_Full_Text.md` — clean-room
  extraction (copyrighted, git-ignored).
- `standards/TIA-102.BAEJ-A/TIA-102-BAEJ-A_Summary.txt` — retrieval summary.
- `standards/TIA-102.BAEJ-A/TIA-102-BAEJ-A_Related_Resources.md`.
- `annex_tables/baej_cms_pdus.csv` — the 6-PDU catalog with direction,
  delivery service, SAP, length, and Registration Type (RT) value.
- `annex_tables/baej_error_codes.csv` — RT / RO / SEI MT / EC value
  tables.

---

## 1. Where CMS Fits in the Stack

Per BAEA-C §6.1.3 and §6.2.3 + BAEJ-A §1:

```
CAI Bearer, Conventional FNE:       IP Bearer, Conventional FNE:
┌──────────────┐                    ┌──────────────┐
│  Payload     │                    │  Payload     │
├──────────────┤                    ├──────────────┤
│  CMS         │  ← THIS SPEC       │  IPv4        │
├──────────────┤                    ├──────────────┤
│  LLC (BAED-A)│                    │  SCEP or SNDCP│
├──────────────┤                    ├──────────────┤
│  FDMA CAI PHY│                    │  CMS         │ ← THIS SPEC
└──────────────┘                    ├──────────────┤
                                    │  LLC (BAED-A)│
                                    ├──────────────┤
                                    │  FDMA CAI PHY│
                                    └──────────────┘
```

**Key point:** CMS sits above LLC. It's carried as CAI data packets
(Confirmed or Unconfirmed per PDU) using the standard BAAA-B
Confirmed/Unconfirmed Data PDU framing (see `analysis/fdma_pdu_frame.md`
§4.3–§4.4 for the byte-level CAI data PDU view). CMS PDUs are
identified by their **SAP** value in the CAI Data Packet Header Block:

- Registration PDUs use the **Registration and Authorization** SAP.
- Scan Preamble uses the **Packet Data Scan Preamble** SAP.
- Unable to Decrypt uses the **Packet Data Encryption Support** SAP.

SAP values are assigned in TIA-102.BAAC-D. MFID in the CAI header is
`$00` (standard); `Data Header Offset = %000000`.

---

## 2. Four CMS Functions

Per BAEJ-A §2:

| # | Function | Required | Notes |
|---|----------|----------|-------|
| 1 | **Packet Data Registration** | Yes (if Dynamic) | SU must register with FNE before bearer use. Two flavors: Static and Dynamic |
| 2 | **SU Location Tracking** | Yes (FNE side) | FNE notes which conventional channel carried each inbound PDU for outbound routing |
| 3 | **CMS Scan Mode** | Optional | SU periodically leaves the packet-data channel; FNE emits Scan Preamble to call SU back. **Disabled with SNDCP** |
| 4 | **Packet Data Supplementary Encryption Info** | Optional | `Unable to Decrypt` notification from SU → FNE |

---

## 3. State Machines

### 3.1 SU State Machine

Three states (BAEJ-A §3.2):

```
     ┌───────────┐  send Registration Request – Connect
     │  Closed   │ ─────────────────────────────────────┐
     └───────────┘                                      │
           ▲                                            ▼
           │                                    ┌───────────┐
           │         FNE: Denied response       │ Register  │ ← dynamic only;
           │ ◀─────────────────────────────────│(pending)  │   static SU skips
           │                                    └───────────┘   this state
           │                                            │
           │                                            │ FNE: Accepted response,
           │                                            │ OR static timeout elapses
           │                                            ▼
           │                                    ┌───────────┐
           │         send Disconnect            │   Open    │
           │ ◀─────────────────────────────────│(registered)│
                                                └───────────┘
```

Semantics:
- **Closed**: not registered; any bearer-service request fails locally (SU returns error to payload layer).
- **Register**: Connect sent; new bearer requests **queued**; awaiting FNE response. Only reachable in dynamic registration.
- **Open**: registered; bearer services processed normally.

Static registration goes Closed → Open directly (no Register state).
The FNE's acknowledgment, if any, is observational only on the SU
side — the SU transitions immediately after sending the Connect PDU.

### 3.2 FNE State Machine (per-LLID)

Two states (BAEJ-A §3.2):

| State | Entry | Behavior |
|-------|-------|----------|
| **Closed** | Initial / after Disconnect received | Awaiting initial registration from this SU. |
| **Open** | Accepted Connect received, OR any valid inbound PDU received (static case) | SU is registered; bearer services processed; location updated on each inbound PDU. |

The FNE maintains this state **per-LLID**. An FNE serving N SUs has N
independent state machines.

---

## 4. Registration Procedure

### 4.1 Static Registration (§3.1)

- SU is pre-provisioned by a system administrator with its LLID and
  (for IP bearer with SCEP) its IP address.
- On entering packet-data operational mode, SU transmits Registration
  Request – Connect PDU.
- SU **immediately** transitions Closed → Open. No FNE response is
  required for the SU's state transition.
- FNE receives any valid inbound PDU (not necessarily the Connect)
  and records the SU's current channel as its location. FNE
  transitions Closed → Open for that LLID.

### 4.2 Dynamic Registration (§3.1)

- SU transmits Registration Request – Connect PDU. SU transitions
  Closed → Register.
- FNE responds with either:
  - **Accepted** → SU transitions Register → Open. IP address (for
    SCEP bearer) is assigned by the FNE and delivered in the Accepted
    PDU's IP Address field.
  - **Denied** → SU returns to Closed.
- Dynamic registration can negotiate **CMS Scan Mode** via the RO
  field of the Connect / Accepted PDUs.
- For SNDCP bearers: the IP Address field and Scan Mode bit are
  both forced to zero — SNDCP handles these independently (§5.1.4,
  §5.1.2).

### 4.3 De-Registration

- SU sends Registration Request – Disconnect (unconfirmed).
- FNE marks the SU's location as unknown and transitions Open →
  Closed.
- Disconnect is unconfirmed because the SU is typically shutting down
  or leaving the channel — awaiting a confirmation Ack would block.

---

## 5. Common Registration PDU Format (12 octets)

Used by Connect (inbound), Accepted (outbound), and Denied (outbound)
— see BAEJ-A Figure 6:

```
 Bit 7          4  Bit 3         0
+-----------------+-----------------+
|  RT (4 bits)    |  RO (4 bits)    | Octet 0
+-----------------+-----------------+
|  LLID (24 bits, MSB first)        | Octets 1-3
|                                   |
|                                   |
+-----------------------------------+
|  Reserved (32 bits, all zero)     | Octets 4-7
|                                   |
|                                   |
|                                   |
+-----------------------------------+
|  IP Address (32 bits, IPv4)       | Octets 8-11
|                                   |
|                                   |
|                                   |
+-----------------------------------+
```

**Field rules** (§5.1):
- **RT** (bits 7:4 of octet 0): Registration Type — see §9.1.
- **RO** (bits 3:0 of octet 0): Registration Options bitmask — see §9.2.
- **LLID** (octets 1–3, 24 bits MSB-first): 24-bit SU identifier per BAAC-D.
- **Reserved** (octets 4–7): set to zero on transmission; decoder must
  not reject on non-zero but should log if observed.
- **IP Address** (octets 8–11): IPv4. Value `0x00000000` when not
  applicable (CAI bearer, or IP+SNDCP bearer). Network byte order
  (MSB-first).

### 5.1 Per-PDU Usage

| PDU | RT | RO | LLID | IP Address | Delivery | SAP |
|-----|-----|-----|------|------------|----------|-----|
| Request – Connect | `%0000` | Requested options | SU's LLID | SU's IP (for SCEP bearer) or 0 | Confirmed | Registration and Authorization |
| Response – Accepted | `%0100` | Granted options | SU's LLID | Assigned IP or SU-supplied IP | Confirmed | Registration and Authorization |
| Response – Denied | `%0101` | 0 | SU's LLID | 0 | Confirmed | Registration and Authorization |

---

## 6. Disconnect PDU Format (8 octets)

Per BAEJ-A Figure 7. Same layout as Common except **no IP Address
field** — octets 8–11 are absent entirely:

```
 Bit 7          4  Bit 3         0
+-----------------+-----------------+
|  RT = %0001     |  Reserved (0)   | Octet 0
+-----------------+-----------------+
|  LLID (24 bits, MSB first)        | Octets 1-3
+-----------------------------------+
|  Reserved (32 bits, all zero)     | Octets 4-7
+-----------------------------------+
```

**Delivery:** Unconfirmed.
**SAP:** Registration and Authorization (same as Connect).

---

## 7. Scan Preamble PDU (empty)

Per BAEJ-A §4.3.1:

- An **empty** CAI Unconfirmed Data Packet Header Block with **no
  data blocks**.
- SAP: **Packet Data Scan Preamble** (distinct from Registration and
  Authorization).
- Direction: Outbound (FNE → SU).
- Delivery: Unconfirmed (explicit — §4.3.1).

**Decoder pattern.** If you observe an Unconfirmed Data Packet Header
Block with `BTF = 0` (block count zero), `SAP = Packet Data Scan
Preamble`, interpret as a CMS Scan Preamble. No payload is present,
but the header's source / destination fields still identify the FNE
and the target SU (or broadcast LLID).

**Preamble duration rule:** the FNE should keep sending Scan Preamble
for at least one complete SU scan period (BAEJ-A §3.3, informative
note). The exact interval is local FNE policy, not normatively
specified.

---

## 8. Unable to Decrypt PDU Format (7 octets)

Per BAEJ-A Figure 8:

```
 Bit 7          0
+-----------------+
|  SEI MT = $01   |  Octet 0
+--------+--------+
| Rsvd   | EP SAP |  Octet 1 (upper nibble reserved; lower nibble = EP SAP)
+--------+--------+
|  Key ID         |  Octets 2-3 (16 bits, MSB-first)
|                 |
+-----------------+
|  Algorithm ID   |  Octet 4 (8 bits)
+-----------------+
|  Current Index  |  Octet 5 (keyset index; 0 if multi-keyset not supported)
+-----------------+
|  Error Code     |  Octet 6 — see §9.4
+-----------------+
```

**Direction:** Inbound (SU → FNE).
**Delivery:** Unconfirmed.
**SAP:** Packet Data Encryption Support.

**Semantics.** SU emits this when it receives an encrypted CAI data
packet it cannot decrypt. Key ID / ALG ID / EP SAP come from the
**ES Auxiliary Header of the encrypted packet that triggered the
failure** (not the SU's own configuration) — the receiver reports
what the sender used. EP SAP terminology varies across companion
specs: AAAD-B calls this "2ndary SAP"; AACA-A calls it "KM SAP".
Same field, different names.

**Usage rule:** Only **one** Unable to Decrypt PDU per logical message
— don't send one per failing block if the message fragments. This
avoids ambiguity with block-level retransmission.

**Use case.** FNE uses this notification to invoke a key-management
response. In particular, it may trigger an **OTAR warm-start** per
AACA-A § warm-start sequence. The notification is advisory — OTAR can
be triggered by other means.

---

## 9. Field Encodings

Full machine-readable lookup: **`annex_tables/baej_error_codes.csv`**.

### 9.1 RT — Registration Type (4 bits)

| Value | Meaning |
|-------|---------|
| `%0000` (0x0) | Registration Request – Connect (inbound) |
| `%0001` (0x1) | Registration Request – Disconnect (inbound) |
| `%0010` (0x2) | reserved |
| `%0011` (0x3) | reserved |
| `%0100` (0x4) | Registration Response – Accepted (outbound) |
| `%0101` (0x5) | Registration Response – Denied (outbound) |
| `%0110`–`%1111` | reserved |

### 9.2 RO — Registration Options (4-bit bitmap)

| Bit | Option |
|-----|--------|
| b0 | **CMS Scan Mode** — `%1` enabled, `%0` disabled. **MUST be 0 if IP+SNDCP bearer** (SNDCP owns scan mode) |
| b1 | reserved (must be 0) |
| b2 | reserved (must be 0) |
| b3 | reserved (must be 0) |

### 9.3 SEI MT — Supplementary Encryption Info Message Type (1 octet)

| Value | Meaning |
|-------|---------|
| `$01` | Unable to Decrypt (the only currently-defined value) |
| `$00`, `$02`–`$FF` | reserved |

### 9.4 EC — Error Code (Unable to Decrypt, 1 octet)

| Value | Meaning |
|-------|---------|
| `$00` | Received Key ID not matched OR received Algorithm ID not recognized |
| `$01` | reserved |
| `$02` | Encryption failure — hardware failure; encryption function offline (key-load or out-of-service); received MI is all zeros; encryption disabled by radio configuration |
| `$03`–`$FF` | reserved |

---

## 10. Delivery Service and SAP Summary

Full catalog: **`annex_tables/baej_cms_pdus.csv`**.

| PDU | Delivery | SAP (per BAAC-D) | Direction |
|-----|----------|-------------------|-----------|
| Registration Request – Connect | Confirmed | Registration and Authorization | SU → FNE |
| Registration Request – Disconnect | Unconfirmed | Registration and Authorization | SU → FNE |
| Registration Response – Accepted | Confirmed | Registration and Authorization | FNE → SU |
| Registration Response – Denied | Confirmed | Registration and Authorization | FNE → SU |
| Scan Preamble | Unconfirmed | Packet Data Scan Preamble | FNE → SU |
| Unable to Decrypt | Unconfirmed | Packet Data Encryption Support | SU → FNE |

**Decoder dispatch pattern:**

```
on_cai_data_pdu(header, payload):
    if header.sap == PACKET_DATA_SCAN_PREAMBLE and header.btf == 0:
        emit_cms_event(scan_preamble, fne=header.src, target=header.dst)
    elif header.sap == REGISTRATION_AND_AUTHORIZATION:
        rt = (payload[0] >> 4) & 0x0F
        ro = payload[0] & 0x0F
        llid = (payload[1] << 16) | (payload[2] << 8) | payload[3]
        match rt:
            0x0: emit_cms_event(reg_connect, llid, ro, ip=payload[8:12])
            0x1: emit_cms_event(reg_disconnect, llid)
            0x4: emit_cms_event(reg_accepted, llid, ro, ip=payload[8:12])
            0x5: emit_cms_event(reg_denied, llid)
            _  : log_warning("reserved RT value", rt)
    elif header.sap == PACKET_DATA_ENCRYPTION_SUPPORT:
        if payload[0] == 0x01:
            emit_cms_event(unable_to_decrypt,
                           ep_sap  = payload[1] & 0x0F,
                           key_id  = (payload[2] << 8) | payload[3],
                           alg_id  = payload[4],
                           ci      = payload[5],
                           ec      = payload[6])
```

---

## 11. Motorola MFID=0x90 Backward-Compatibility (Annex A, Informative)

BAEJ-A Annex A recommends:
- FNE implementations should accept **both** `MFID = $00` (standard)
  and `MFID = $90` (Motorola) forms of Registration, Scan Preamble,
  and Unable to Decrypt PDUs.
- SUs should be capable of trying both forms — if standard MFID
  Registration fails, retry with Motorola MFID for legacy-FNE
  operation.
- When an FNE accepts an SU's PDU with a given MFID, it should emit
  outbound PDUs using the **same MFID** (mirror the SU's choice).

**As of 2019 publication: MFID=$00 and MFID=$90 wire formats are
identical**; the MFID byte is the only distinguishing feature. Annex A
notes this may diverge in future revisions, hence the license-agreement
advisory for `MFID=$90` users.

**Decoder implication.** A passive decoder should accept both MFID
values as CMS and parse identically. Log the MFID as a "flavor"
attribute for correlation but don't branch parser behavior.

---

## 12. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify:

- CMS functions overview — TIA-102.BAEJ-A §2.
- Packet Data Registration procedure — §3.1.
- SU and FNE state descriptions — §3.2.
- CMS Scan Mode operation — §3.3.
- Unable to Decrypt operation — §3.4.
- PDU summary table — §4.1, Table 2.
- Common Registration PDU format — §4.2, Figure 6.
- Disconnect PDU format — §4.2.4, Figure 7.
- Scan Preamble PDU — §4.3.1.
- Unable to Decrypt PDU format — §4.4.1, Figure 8.
- RT values — §5.1.1, Table 3.
- RO bitmap — §5.1.2, Table 4 (plus the SNDCP interlock at the end of
  §5.1.2 forcing RO.b0 = 0 when SNDCP bearer active).
- LLID / IP Address field definitions — §5.1.3 / §5.1.4.
- SEI MT / EP SAP / KEY ID / ALG ID / CI / EC — §5.2.1–§5.2.6,
  Tables 5–6.
- Manufacturer MFID considerations — Annex A (informative).

---

## 13. Cross-References

**Upstream (this doc depends on):**
- `standards/TIA-102.BAEA-C/P25_Data_Overview_Implementation_Spec.md`
  — establishes Conventional FNE Data configuration and the
  CMS layer's position in the stack.
- `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md`
  — Confirmed / Unconfirmed CAI data packet framing that carries
  CMS PDUs.
- `standards/TIA-102.BAED-A/P25_Packet_Data_LLC_Implementation_Spec.md`
  — LLC layer beneath CMS.
- `standards/TIA-102.BAAC-D/P25_Reserved_Values_Implementation_Spec.md`
  — SAP values used in CAI Data Packet Header Blocks to
  identify the three CMS SAPs.
- `standards/TIA-102.BAEB-B/P25_IP_Data_Bearer_BAEB_B_Implementation_Spec.md`
  — SNDCP Scan Mode / Context Management, which override the CMS
  Scan Mode when active.
- `standards/TIA-102.AAAD-B/P25_Block_Encryption_Protocol_Implementation_Spec.md`
  — ES Auxiliary Header format from which EP SAP / KEY ID / ALG ID
  are sourced when emitting Unable to Decrypt.
- *(external)* AACA-A — OTAR warm-start triggered by Unable to
  Decrypt. (AACA-C is superseded; AACA-D is currently processed in
  the repo.)

**Supporting annex tables:**
- `annex_tables/baej_cms_pdus.csv` — 6-PDU catalog with direction,
  delivery, SAP, length, RT.
- `annex_tables/baej_error_codes.csv` — RT / RO / SEI MT / EC value
  enumeration.

**Related analysis:**
- `analysis/fdma_pdu_frame.md` §4.3–§4.4 — Unconfirmed / Confirmed
  CAI Data PDU wire layout that carries CMS.
- `analysis/motorola_conventional_scep_vs_trunked_sndcp.md` — the
  architectural split between conventional (SCEP / zero-wrapper) and
  trunked (SNDCP + LLC) data wrappers; CMS lives in the conventional
  branch.
