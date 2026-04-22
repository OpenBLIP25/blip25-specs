# P25 Radio Management Protocols — TIA-102.BAEE-C Implementation Spec

**Source:** TIA-102.BAEE-C (2015-12), *Radio Management Protocols for
Mobile Data Peripheral Interface*. Supersedes BAEE-B (2010); fourth
edition. Adopted by the Project 25 Steering Committee in October 2015.

**Document type:** MESSAGE_FORMAT + PROTOCOL. Specifies two parallel
protocols for managing a P25 Subscriber Unit (radio) from a Mobile
Data Peripheral (laptop / terminal / dispatch console) over the
**A interface** — the SU ↔ MDP serial link (USB or TIA-232), NOT the
Um air interface. Both protocols ride over UDP.

**Where this sits in the stack.** BAEE-C lives *above* the A interface
link layer defined in BAEG-A. Messages are IPv4 datagrams on the A
link; the SU and MDP each have A-interface IP addresses. This is
management-plane traffic between the radio and its attached peripheral;
it does **not** appear on the Um air interface except as IP datagrams
forwarded through the radio (which would be application payload, not
RCP/SNMP per se).

**Scope of this derived work:**
- §1 — When to use RCP vs SNMP
- §2 — Transport: UDP, port assignments
- §3 — RCP SDU wire format (generic header + class-specific bodies)
- §4 — RCP operator catalog (Request / Response / Report)
- §5 — Configuration Block parameter encoding
- §6 — Information / Statistics Block parsing
- §7 — Report SDU (asynchronous) handling rules
- §8 — SNMP MIB layout + OIDs
- §9 — Volatile vs non-volatile config model (`rmpConfigStatusMode` semantics)
- §10 — MTU constraint and datagram sizing
- §11 — Cross-protocol equivalence (RCP ↔ SNMP)
- §12 — Security caveats
- §13 — Cite-to section references
- §14 — Cross-references

**Pipeline artifacts:**
- `standards/TIA-102.BAEE-C/TIA-102-BAEE-C_Full_Text.md` — clean-room
  extraction (copyrighted, git-ignored).
- `standards/TIA-102.BAEE-C/TIA-102-BAEE-C_Summary.txt` — ~1000-word
  summary for retrieval.
- `standards/TIA-102.BAEE-C/TIA-102-BAEE-C_Related_Resources.md` —
  lineage, external references.
- `annex_tables/baee_rcp_operators.csv` — all RCP operators with
  opcode + operand values + response payload shapes.
- `annex_tables/baee_config_block_values.csv` — Configuration Block
  parameter encoding lookup.
- `annex_tables/baee_snmp_mib_oids.csv` — complete SNMP MIB OID
  catalog (info / config / stats / events).

---

## 1. When to Use RCP vs SNMP

BAEE-C §3–§4 defines two parallel, functionally-equivalent protocols:

| Aspect | Radio Control Protocol (RCP) | Simple Network Management Protocol (SNMP) |
|--------|-------------------------------|-------------------------------------------|
| **Base standard** | Custom to TIA-102 | IETF STD0016/0015/0017/0058/0062 |
| **UDP port (requests)** | 469 (IANA-assigned) | 161 |
| **UDP port (traps/reports)** | 469 | 162 |
| **Encoding** | Binary octets (ASCII-mnemonic opcodes) | ASN.1 / BER |
| **Transaction correlation** | 16-bit SDU Tag | SNMP request-id field |
| **Async events** | Report SDUs (SDU Tag = $FFFF) | SNMP Traps |
| **MIB / schema** | Fixed operator catalog (Table 1) | Formal MIB in Annex A |
| **Off-the-shelf tooling** | None — P25-specific | Extensive (Net-SNMP, etc.) |

**Both protocols are functionally equivalent.** BAEE-C defines the RCP
operator catalog and then provides an SNMP MIB that exposes the same
managed objects. A conformant implementation may offer either or both.
Annex B explicitly maps every RCP operator to its SNMP counterpart.

**When RCP makes sense:** tightly-coupled MDP firmware that ships with
the radio; environments where binary compactness matters; vendors
already operating an RCP stack.

**When SNMP makes sense:** integration with general-purpose network
management stations (HP OpenView, Solarwinds, Net-SNMP); when the MDP
is a laptop running standard management tools; when you need
community-string authentication (RCP has none).

---

## 2. Transport: UDP, Port Assignments

Per BAEE-C §3.2 and §4.2.2:

- **UDP** is the transport. TCP is not supported.
- **IP** layer uses a standard 20-octet IPv4 header.
- **Port 469** — RCP requests, responses, and reports. Single port for
  all three directions. Registered with IANA as "trnsprntproxy".
  *(Note: the IANA registration text is historical; this is the
  TIA-102 RCP usage per BAEE-C Table [20].)*
- **Port 161** — SNMP requests / responses (MDP → SU and SU → MDP
  within a request/response pair).
- **Port 162** — SNMP traps (SU → MDP asynchronous only).

**One SDU per UDP datagram** (BAEE-C §3.2). Implementations must not
concatenate multiple SDUs into a single UDP payload; they must not
fragment a single SDU across UDP datagrams. If a configuration demands
larger payloads, raise `rmpMtu` — do not fragment RCP.

---

## 3. RCP SDU Wire Format

### 3.1 Generic Header (6 octets)

Every RCP SDU starts with six fixed-layout octets:

```
Octet  Field           Width  Notes
----   -----           -----  -----
 0     Type            1      0x43 RQST / 0x52 RESP / 0x45 RPRT
 1-2   Length          2      MSB first. Count of octets in the Data field (everything after octet 5)
 3-4   SDU Tag         2      MSB first. Request: caller-chosen unique 16-bit tag.
                              Response: echoes the request's tag.
                              Report: always 0xFFFF.
 5     MFID            1      0x00 = standard. Non-zero values = vendor-specific
                              SDU per BAAC-D MFID table.
 6...  Data (Length B) var    Class-specific — see §4
```

**Type values** (BAEE-C §3.2.2):

| Value | Name | Who sends | Semantics |
|-------|------|-----------|-----------|
| `0x43` ('C') | RQST | MDP → SU | Command |
| `0x52` ('R') | RESP | SU → MDP | One-per-request reply |
| `0x45` ('E') | RPRT | SU → MDP | Asynchronous event — no matching request |

**Length encoding.** The Length field counts octets in the Data field
only, NOT octets 0–5. A minimal RQST SDU (Data = operator + operand =
2 octets) has Length = 0x0002.

**SDU Tag uniqueness.** The tag is a 16-bit identifier chosen by the
MDP for each outstanding request. The SU does not interpret the tag's
value — it just echoes it in the corresponding Response. This enables
out-of-order responses (the protocol is multithreaded per §3.1.1).

**Tag `0xFFFF` is reserved** (BAEE-C §3.2.4) for Report SDUs; MDPs
must not use `0xFFFF` as a Request tag. An SU receiving a Request with
Tag = `0xFFFF` should treat the tag as invalid.

### 3.2 Request-Class SDU (Type = RQST)

Per BAEE-C §3.3.1 and Figure 4:

```
Octet  Field         Notes
----   -----         -----
 0     0x43          Type = RQST
 1-2   Length        usually 0x0002 (operator + operand = 2 octets)
                     except SET_CONF which is 0x0005 (operator + 4 config octets)
 3-4   SDU Tag       caller-chosen, != 0xFFFF
 5     MFID          0x00 for standard requests
 6     Operator      one byte from Table 1 — see §4.1
 7     Operand       one byte — see §4.1; for SET_CONF, octets 7-10 are config values
 ...
```

### 3.3 Response-Class SDU (Type = RESP)

Per BAEE-C §3.4.1 and Figure 12:

```
Octet  Field           Notes
----   -----           -----
 0     0x52            Type = RESP
 1-2   Length          Varies: 0x0001 for SUCCESS/FAIL only;
                       0x0005 (GET_CONFG success), 0x0009 (GET_STATS success),
                       0x0019 (GET_INFO success = 25 data octets),
                       0x0002 (FAIL UNKNOWN)
 3-4   SDU Tag         Echoes the request's tag
 5     MFID            0x00
 6     Result Code     0x53 SUCCESS ('S') / 0x46 FAIL ('F')
 7...  Response Info   Present only on success; shape depends on the original
                       request operator. See §6 for each block's layout.
```

On FAIL-with-UNKNOWN (§3.4.2.8), octet 7 = `0x55` ('U'). All other
FAILs have no Response Info — Length = 0x0001.

### 3.4 Report-Class SDU (Type = RPRT)

Per BAEE-C §3.5.1 and Figure 21:

```
Octet  Field                 Notes
----   -----                 -----
 0     0x45                  Type = RPRT
 1-2   Length                0x0001 (POWER_UP), 0x0002 (REGISTRATION, DATA_SVC_AVAILABILITY)
 3-4   SDU Tag               Always 0xFFFF (unsolicited)
 5     MFID                  0x00
 6     Report ID             0x50 / 0x4E / 0x44 — see §7
 7     Status                Present on REGISTRATION and DATA_SVC_AVAILABILITY reports
```

---

## 4. RCP Operator Catalog

Full machine-readable catalog: **`annex_tables/baee_rcp_operators.csv`**.
Summary:

### 4.1 Request Operators (7 total)

| Operator | Hex | Operand | Length | Purpose |
|----------|-----|---------|--------|---------|
| `GET_INFO` | `0x47` | `INFO_BLOCK = 0x43` | 0x0002 | Read-only SU info (SW version, manufacturer, LLID, MTU, serial, registration status, channel quality, signal strength, battery) |
| `GET_CONFG` | `0x33` | `CONFIG_BLOCK = 0x39` | 0x0002 | Read current config (power mode, delivery mode, encryption, data config) |
| `RST_DFLT` | `0x34` | `CONFIG_BLOCK = 0x39` | 0x0002 | Restore NV defaults into volatile |
| `SET_CONF` | `0x35` | Config values (4 octets at 7-10) | 0x0005 | Write volatile config |
| `SAVE_DFLT` | `0x36` | `CONFIG_BLOCK = 0x39` | 0x0002 | Persist current volatile config to NV memory |
| `RESET_STATS` | `0x4C` | `STATS_BLOCK = 0x4B` | 0x0002 | Zero packet counters |
| `GET_STATS` | `0x4D` | `STATS_BLOCK = 0x4B` | 0x0002 | Read counters (total sent, total received) |

### 4.2 Response Result Codes (2 + 1 variant)

| Code | Hex | ASCII | Meaning |
|------|-----|-------|---------|
| `SUCCESS` | `0x53` | `S` | Request completed successfully |
| `FAIL` | `0x46` | `F` | Request failed |
| `UNKNOWN` | `0x55` | `U` | Appended after `FAIL` when the operator is not recognized by the SU (Figure 20) |

### 4.3 Report IDs (3 total)

| ID | Hex | ASCII | Payload | Trigger |
|----|-----|-------|---------|---------|
| `POWER_UP` | `0x50` | `P` | None | SU has completed power-up |
| `RADIO_REGISTRATION` | `0x4E` | `N` | 1 status octet | Registration state change (FNE Data only) |
| `DATA_SVC_AVAILABILITY` | `0x44` | `D` | 1 status octet | Data service mode entered/exited |

---

## 5. Configuration Block Parameter Encoding

Full machine-readable lookup: **`annex_tables/baee_config_block_values.csv`**.

The Configuration Block is a fixed 4-octet sequence, one byte per
parameter. Each byte takes one of a small set of ASCII digit values:

| Offset in block | Parameter | Legal values (hex) |
|------|-----------|--------------------|
| `+0` | `POWER_MODE` | `0x30` High / `0x31` Low / `0x32` Adaptive |
| `+1` | `DATA_DELIVERY` | `0x33` Confirmed / `0x34` Unconfirmed |
| `+2` | `ENCRYPTION` | `0x35` Disabled / `0x36` Enabled |
| `+3` | `DATA_CONFIG` | `0x37` Repeated / `0x38` FNE / `0x39` Direct |

**A SET_CONF request sends this 4-octet block as operands at octets 7–10.**
A GET_CONFG response places this 4-octet block at octets 7–10 of the
Response SDU.

**Draft note:** BAEE-C Annex B flags `DATA_DELIVERY` as "future support"
— the Confirmed vs Unconfirmed toggle may not be implemented on all
2015-era firmware. Vendors should verify against the target radio's
behavior.

---

## 6. Information / Statistics Block Parsing

### 6.1 Information Block (GET_INFO success response, 25 octets at 7–30)

Per BAEE-C Table 4:

| Offset in block | Width | Field | Format |
|------|-------|-------|--------|
| `+0` | 4 | `SW_version` | ASCII |
| `+4` | 1 | `Radio Manufacturer` | MFID byte per BAAC-D |
| `+5` | 3 | `Radio CAI_LLI` | Hex, 24-bit Logical Link ID |
| `+8` | 2 | `SU_MTU` | u16 MSB-first, ≤ 2028 |
| `+10` | 10 | `Serial_Number` | ASCII |
| `+20` | 1 | `Registration_Status` | Signed int8: 0 = registered; 1–100 = error code; -1 (`0xFF`) = unknown |
| `+21` | 1 | `Channel_Quality` | 0–100 percent or -1 unknown |
| `+22` | 1 | `Signal_Strength` | 0–100 percent or -1 unknown |
| `+23` | 1 | `Battery_Level` | 0–100 percent or -1 unknown |
| `+24` | 1 | (end of defined block) | — |

**Parser note.** The BAEE-C text says 25 data octets (Length = `0x19`)
but Table 4 enumerates 9 fields totaling 24 octets (4+1+3+2+10+1+1+1+1).
The discrepancy is one extra octet not accounted for in the field
list; assume it's padding/reserved and skip it. If the observed SU
emits 25 octets with a usable value at offset `+24`, cross-check
against vendor-specific behavior.

### 6.2 Statistics Block (GET_STATS success response, 8 octets at 7–14)

Per BAEE-C Table 6:

| Offset in block | Width | Field | Format |
|------|-------|-------|--------|
| `+0` | 4 | `TOTAL_NUM_SENT_PACKETS` | u32 hex (MSB first); IPv4 datagrams sent over CAI |
| `+4` | 4 | `TOTAL_NUM_RCVD_PACKETS` | u32 hex (MSB first); IPv4 datagrams received over CAI |

Counters wrap at `2^32`. A `RESET_STATS` request clears both.

### 6.3 Configuration Block (GET_CONFG success response, 4 octets at 7–10)

See §5 above — same 4-octet layout as SET_CONF's operand region.

---

## 7. Report SDU Handling

Per BAEE-C §3.5:

- Reports are **asynchronous**. The MDP must handle an incoming RPRT
  SDU at any time, not just in response to a prior request.
- **SDU Tag is always `0xFFFF`** — this is how the receiver
  disambiguates a Report from a delayed Response.
- Reports are NOT acknowledged. There is no "Report ACK" SDU in RCP.

**Implementation pattern:**

```
on_rcp_udp_packet(datagram):
    if len(datagram) < 6: drop
    type = datagram[0]
    length = u16_be(datagram[1:3])
    tag = u16_be(datagram[3:5])
    mfid = datagram[5]

    if type == 0x45:  # RPRT
        handle_async_report(report_id = datagram[6], status = datagram[7:])
    elif type == 0x52:  # RESP
        match_pending_request(tag = tag, body = datagram[6:])
    elif type == 0x43:  # RQST
        # SU-side only; MDPs don't receive RQSTs
        ...
```

**POWER_UP report** (Figure 22) carries no status byte — Length =
`0x0001`. Receipt is the trigger for the MDP to re-issue GET_INFO +
GET_CONFG to rebuild its view of the radio.

**RADIO_REGISTRATION report** (Figure 23) carries 1 status octet at
offset `+7`:
- `0x41` ('A') = `REGISTERED`
- `0x42` ('B') = `NOT_REGISTERED` *(the spec misspells this as
  "NOT_REGISTERD" in Table 8 — preserve the spelling when matching
  but treat as NOT_REGISTERED semantically)*

**DATA_SVC_AVAILABILITY report** (Figure 24) carries 1 status octet:
- `0x30` ('0') = `SERVICE_NOT_AVAILABLE`
- `0x31` ('1') = `SERVICE_AVAILABLE`

---

## 8. SNMP MIB Layout and OIDs

Full OID catalog: **`annex_tables/baee_snmp_mib_oids.csv`**.

**Enterprise OID root:** `1.3.6.1.4.1.17405` = TIA Telecommunications
Industry Association TR8dot5 (IANA PEN 17405).

```
TR8dot5 (1.3.6.1.4.1.17405)
 └── TR8dot5Generic (.1)
      └── TR8dot5RmpMIB (.1.1)
           └── TR8dot5Rmp (.1)                              -- container
                ├── TR8dot5RmpEvents (.0)                   -- traps
                │   ├── rmpPowerUpEvent (.1)
                │   ├── rmpRegistrationEvent (.2)
                │   └── rmpDataServiceEvent (.3)
                ├── TR8dot5RmpInfoObjs (.1)                 -- read-only info (10 objects)
                │   ├── rmpSwVersion (.1)        OctetString
                │   ├── rmpRadioMan (.2)         Unsigned32
                │   ├── rmpLlid (.3)             Unsigned32
                │   ├── rmpMtu (.4)              Unsigned32 ≤ 2028
                │   ├── rmpRadioSn (.5)          OctetString (10 char)
                │   ├── rmpRegStatus (.6)        Integer (-1..100)
                │   ├── rmpChanQual (.7)         Integer (-1..100)
                │   ├── rmpSigStrength (.8)      Integer (-1..100)
                │   ├── rmpBattLevel (.9)        Integer (-1..100)
                │   └── rmpDataService (.10)     Enumerated
                ├── TR8dot5RmpConfigObjs (.2)               -- read-write config (5 objects)
                │   ├── rmpPowerMode (.1)
                │   ├── rmpDataDelivery (.2)
                │   ├── rmpEncryption (.3)
                │   ├── rmpDataMode (.4)
                │   └── rmpConfigStatusMode (.5)  -- see §9
                └── TR8dot5RmpStatsObjs (.3)                -- counters (3 objects)
                    ├── rmpTotalNumRcvdPackets (.1)  Unsigned32
                    ├── rmpTotalNumSentPackets (.2)  Unsigned32
                    └── rmpStatisticsState (.9)      Enumerated  ("Cleared" to reset)
```

**ASN.1 MIB source.** Annex A of BAEE-C is normative; it provides the
full ASN.1 definitions. A formal MIB file for `snmpwalk` / Net-SNMP
can be constructed directly from Annex A — that's an implementer
downstream task, not reproduced here (ASN.1 verbatim is
copyrightable).

---

## 9. Volatile vs Non-Volatile Config Model

Per BAEE-C §4.3.3 (`rmpConfigStatusMode` semantics). This is the
subtle part — implementers routinely get this wrong:

```
          Power-up or channel change
                    │
                    ▼
             NV → volatile
                    │
       rmpConfigStatusMode = "Default"
                    │
     Set-request on any other config object:
                    │
       rmpConfigStatusMode = "Modified"
       (volatile only; NV untouched)
                    │
      ┌─────────────┴─────────────┐
      │                           │
 Set mode=Stored              Set mode=Default
  → persist volatile to NV     → reload NV into volatile
      │                           │
      ▼                           ▼
 (NV updated)                   (volatile reverts)
```

Three states:

| State | Meaning |
|-------|---------|
| `Default` | Current volatile config matches NV. Set on power-up and after a Set to `Default` (reload). |
| `Modified` | Volatile has been changed but not persisted. NV still holds the old values. |
| `Stored` | Transient — set as the trigger to commit volatile → NV. Equivalent to the RCP `SAVE_DFLT` operation. |

**RCP equivalents** (Annex B):
- `SAVE_DFLT` = Set `rmpConfigStatusMode = Stored`.
- `RST_DFLT` = Set `rmpConfigStatusMode = Default`.
- `SET_CONF` = Set any individual object → automatically transitions to `Modified`.

---

## 10. MTU Constraint and Datagram Sizing

BAEE-C §3.4.2.1 / Table 4 and §4.3.2.4:

- **SU_MTU** (octets) is the maximum IPv4 datagram the radio will
  accept from the MDP for transmission over the CAI.
- **Hard ceiling: 2028 octets** — the maximum CAI packet capacity
  per BAAA-B. A radio will not report `SU_MTU > 2028`.
- **MTU includes IP and UDP headers** but excludes A-interface
  link-layer overhead (PPP / SLIP framing).

**Implementation rule for MDPs:** MDP application code generating IPv4
datagrams for radio transmission must clamp the datagram size at
`SU_MTU`. Larger datagrams will either be silently dropped or split by
the radio using the BAAA-B / BAED-A block-level fragmentation —
neither is what the application intended.

---

## 11. Cross-Protocol Equivalence (RCP ↔ SNMP)

Per BAEE-C Table 10 (§4.1.2) and Annex B:

| RCP Operator | SNMP Equivalent |
|--------------|-----------------|
| `GET_INFO` INFO_BLOCK | `Get-request` on `TR8dot5RmpInfoObjs` subtree (all 10 info objects) |
| `GET_CONFG` CONFIG_BLOCK | `Get-request` on `TR8dot5RmpConfigObjs.*` |
| `SET_CONF` | `Set-request` on individual config object(s) |
| `RST_DFLT` | `Set-request` `rmpConfigStatusMode = Default` |
| `SAVE_DFLT` | `Set-request` `rmpConfigStatusMode = Stored` |
| `GET_STATS` STATS_BLOCK | `Get-request` on `TR8dot5RmpStatsObjs.1` and `.2` |
| `RESET_STATS` | `Set-request` `rmpStatisticsState = Cleared` |
| `POWER_UP` Report | `rmpPowerUpEvent` Trap |
| `RADIO_REGISTRATION` Report | `rmpRegistrationEvent` Trap (with `rmpRegStatus`) |
| `DATA_SVC_AVAILABILITY` Report | `rmpDataServiceEvent` Trap (with `rmpDataService`) |

A radio that implements both protocols must keep the two views
consistent — they're alternate bindings over the same managed-object
model.

---

## 12. Security Caveats

BAEE-C §3 and §4 are explicit about security:

- **RCP has no authentication or encryption.** Anything on the
  A interface can issue `SET_CONF` / `SAVE_DFLT` against the radio.
  The assumption is that the A interface is a physically-secure
  tethered link (USB cable, serial cable to a trusted MDP).
- **SNMP security is community-string-based** per SNMPv1/v2c. A
  community-string mismatch rejects the request. This is trivially
  bypassable on an attacker-controlled A interface but defends against
  accidental crosstalk.
- **SNMPv3 is not specified** — BAEE-C is built on SNMPv1/v2c RFCs.

**Implementation consequence.** Do not expose the A interface over an
unauthenticated wireless tether (e.g., Bluetooth without pairing).
Treat the A interface as a privileged boundary equivalent to physical
access to the radio.

---

## 13. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify:

- Protocol taxonomy and OSI management areas — TIA-102.BAEE-C §2.
- RCP transaction model — §3.1.
- RCP SDU generic structure — §3.2, Figure 3.
- Type / Length / SDU Tag / MFID fields — §3.2.1–§3.2.5.
- Request-Class SDUs — §3.3, Figures 4–11, Tables 1–2.
- Response-Class SDUs — §3.4, Figures 12–20, Tables 3–6.
- Report-Class SDUs — §3.5, Figures 21–24, Tables 7–9.
- SNMP topology and ports — §4.1, §4.2.2.
- Enterprise OID — §4.2.1.
- MIB subtree — §4.3, Figures 29–30, Tables 11–16.
- Volatile vs NV config model — §4.3.3 (rmpConfigStatusMode semantics).
- MTU constraint — §3.4.2.1, Table 4.
- RCP ↔ SNMP equivalence — §4.1.2 Table 10, Annex B.
- Normative ASN.1 MIB — Annex A.

---

## 14. Cross-References

**Upstream (this doc depends on):**
- `standards/TIA-102.BAEA-C/P25_Data_Overview_Implementation_Spec.md`
  — establishes the A interface as the MDP ↔ SU management surface.
- `standards/TIA-102.BAEG-A/…` (next in queue) — defines the A
  interface link layer (PPP / SLIP over USB / TIA-232) that carries
  the UDP datagrams used here.
- `standards/TIA-102.BAAC-D/P25_Reserved_Values_Implementation_Spec.md`
  — MFID byte values used in the RCP SDU MFID field.
- `standards/TIA-102.BAJD-A/P25_Port_Number_Assignments_Implementation_Spec.md`
  — reference for well-known P25 UDP port catalog (RCP = 469, SNMP
  = 161/162).

**Supporting annex tables:**
- `annex_tables/baee_rcp_operators.csv` — machine-readable operator
  catalog for RQST / RESP / RPRT SDUs.
- `annex_tables/baee_config_block_values.csv` — parameter encoding for
  the 4-octet Configuration Block.
- `annex_tables/baee_snmp_mib_oids.csv` — complete SNMP MIB OID list
  with types, access, and range constraints.

**Related reading:**
- IETF RFC 768 (UDP), RFC 1155/1157/1212/1213 (SNMPv1),
  RFC 2578-2580 (SMIv2), RFC 3411-3418 (SNMPv2c/v3) — normative
  references for SNMP.
