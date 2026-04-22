# P25 Mobile Data Peripheral Interface (A) — TIA-102.BAEG-A Implementation Spec

**Source:** TIA-102.BAEG-A (2019-03), *Mobile Data Peripheral Interface*.
Revision A of BAEG (2013); supersedes the original. Derived from
earlier BAEA-A and BAEB-A material.

**Document type:** link + network-layer profile. Specifies the
**A interface** — the tethered link between a P25 Subscriber Unit
(the radio) and a Mobile Data Peripheral (laptop / ruggedized tablet
/ vehicle-mounted data terminal). Defines which protocol stacks may
be used, how IP addresses are assigned to each side, and what
IPv4 payload is allowed to traverse the link.

**Not an on-air spec.** The A interface is a physical cable or USB
connection between the radio and its attached peripheral. None of
this traffic appears on Um. For passive Um decoders this spec is
informational; for implementers building MDP software or an active
radio-modem stack, it's normative.

**Revision A delta.** Adds two new protocol combinations:
- **IPv4 over RNDIS/USB** (Windows-native USB networking)
- **IPv4 over Ethernet / IEEE 802.3** (wired LAN over a typical
  M12 / ruggedized Ethernet pigtail)

These supplement the four combinations already in BAEG (original,
2013): PPP/TIA-232, SLIP/TIA-232, PPP/USB, SLIP/USB. Total = 6 (7
counting the RFC 894 vs RFC 1042 Ethernet split as two).

**Scope of this derived work:**
- §1 — Where the A interface fits (radio-side view)
- §2 — Six (+1) protocol combinations catalogued
- §3 — Physical requirements (TIA-232 signal subset, USB options)
- §4 — Four IP address binding methods and when to use each
- §5 — RNDIS/USB DHCP constraints (the only normatively-tight binding)
- §6 — Network-layer payload: data vs control signaling
- §7 — ICMP sub-types and Type-of-Service rules
- §8 — Radio-management traffic routing
- §9 — What's NOT in scope
- §10 — Cite-to section references
- §11 — Cross-references

**Pipeline artifacts:**
- `standards/TIA-102.BAEG-A/TIA-102-BAEG-A_Full_Text.md` — clean-room
  extraction.
- `standards/TIA-102.BAEG-A/TIA-102-BAEG-A_Summary.txt` — retrieval summary.
- `standards/TIA-102.BAEG-A/TIA-102-BAEG-A_Related_Resources.md`.
- `annex_tables/baeg_protocol_combinations.csv` — 7-row catalog with
  each combination's physical, P2P, network, and addressing
  characteristics.

---

## 1. Where the A Interface Fits

Per BAEA-C §3.3 and BAEG-A §2. The A interface is one of three
concurrent interfaces a deployed SU exposes:

```
         ┌──────────────────────────────┐
         │ Data Host Network (DHN)      │
         └──────────────┬───────────────┘
                        │ Ed interface (BAEF)
         ┌──────────────┴───────────────┐
         │ Fixed Network Equipment (FNE)│
         └──────────────┬───────────────┘
                        │ Um interface (BAAA-B / BAED-A / BAEB-B/C)
         ┌──────────────┴───────────────┐
         │ Subscriber Unit (SU = radio) │
         └──────────────┬───────────────┘
                        │ A interface (THIS SPEC)
         ┌──────────────┴───────────────┐
         │ Mobile Data Peripheral (MDP) │
         │   = laptop / MDT / tablet    │
         └──────────────────────────────┘
```

**Two endpoint roles** in BAEG-A network-layer terms:
- **Host role.** SU and MDP can each be an IP endpoint (source or
  destination). IETF RFC guidance for hosts applies to either side
  when it owns the datagram.
- **Router role.** The SU may relay IPv4 datagrams between the A
  interface and the Um interface when the datagram's IP source and
  destination are neither the SU nor the MDP. This is the normal
  case for MDP-originated traffic destined for the DHN.

---

## 2. Protocol Combinations

Full machine-readable catalog: **`annex_tables/baeg_protocol_combinations.csv`**.

Per BAEG-A §2.3 Figures 1–7, every combination is IPv4 over one of the
layered stacks below. The stack is three layers (physical, point-to-point
framing, IP):

| # | Combination | Physical | Point-to-point | New in Rev A |
|---|-------------|----------|----------------|--------------|
| 1 | IPv4 over SLIP/TIA-232 | TIA-232 serial | SLIP (RFC 1055) | No |
| 2 | IPv4 over PPP/TIA-232 | TIA-232 serial | PPP (RFC 1661, 1332) | No |
| 3 | IPv4 over SLIP/USB | USB (any speed) | SLIP | No |
| 4 | IPv4 over PPP/USB | USB (any speed) | PPP | No |
| 5 | **IPv4 over RNDIS/USB** | USB (any speed) | RNDIS | **Yes** |
| 6 | **IPv4 over Ethernet (RFC 894)** | Ethernet | (direct encapsulation) | **Yes** |
| 7 | **IPv4 over IEEE 802.3 (RFC 1042)** | 802.3 | LLC/SNAP per RFC 1042 | **Yes** |

### 2.1 Stack shapes

Stacks 1–4 (serial or USB + SLIP/PPP):

```
┌──────────────┐
│ Payload      │
├──────────────┤
│ IPv4         │
├──────────────┤
│ SLIP or PPP  │
├──────────────┤
│ TIA-232 or USB│
└──────────────┘
```

Stack 5 (RNDIS/USB):

```
┌──────────────┐
│ Payload      │
├──────────────┤
│ IPv4         │
├──────────────┤
│ RNDIS        │   MDP is RNDIS Host, SU is RNDIS Device
├──────────────┤
│ USB          │
└──────────────┘
```

Stacks 6–7 (Ethernet): same shape as BAEF's Ed interface — IPv4
directly over Ethernet with either RFC 894 or RFC 1042 encapsulation,
no intermediate point-to-point framing.

### 2.2 Role assignments

| Combination | SU role | MDP role |
|-------------|---------|----------|
| TIA-232 (any) | DCE | DTE |
| USB (any) | USB Device | USB Host |
| RNDIS/USB | RNDIS Device | RNDIS Host |
| Ethernet / 802.3 | (symmetric — no host/device distinction) | (symmetric) |

---

## 3. Physical Requirements

### 3.1 TIA-232

Per BAEG-A §2.1.1 + Tables 2–4.

**Required (standard) signals** — three only:

| Mnemonic | V.24 | TIA-232 | Description | Direction |
|----------|------|---------|-------------|-----------|
| C | 102 | AB | Common ground | Both |
| TD | 103 | BA | Transmitted Data (DTE → DCE) | DCE Input |
| RD | 104 | BB | Received Data (DCE → DTE) | DCE Output |

**Optional signals** — used for hardware flow control and clocked
operation if the deployment needs them:

| Mnemonic | V.24 | TIA-232 | Description | Direction |
|----------|------|---------|-------------|-----------|
| DSR | 107 | CC | Data Set Ready | DCE Output |
| DTR | 108 | CD | Data Terminal Ready | DCE Input |
| RFR | 133 | CJ | Ready for Receiving | DCE Input |
| TDC | 113 | DA | Transmit Data Clock | DCE Input |
| CTS | 106 | DB | Clear to Send | DCE Output |
| RDC | 115 | DD | Receive Data Clock | DCE Output |

**On the A interface the SU is the DCE** (BAEG-A §2.1.1). TDC/RDC
support synchronous clocked operation; most public-safety
deployments are async.

**Voltage levels per TIA-232-F / V.24 are recommended but not
required.** **Physical connector specifications are NOT required** —
P25 radios typically use ruggedized manufacturer-specific pinouts.
Conformance here is logical (signal meaning), not mechanical.

### 3.2 USB

Per BAEG-A §2.1.2:

- Any speed defined in **USB Rev 3.1** is acceptable (low speed 1.5
  Mb/s, full speed 12 Mb/s, high speed 480 Mb/s, SuperSpeed).
- Any USB Device Class capable of supporting networking per the USB
  Communications Device Class specification is permitted.
- **SU is the USB Device, MDP is the USB Host.**
- **Physical connector conformance is NOT required** — the SU side
  may use a manufacturer-specific receptacle that is logically
  equivalent to a standard USB receptacle.

### 3.3 Ethernet / 802.3

Per BAEG-A §2.1.3 / §2.1.4: conform to IEEE 802.3-2000 provisions.
Same EtherType (`0x0800`) / LLC/SNAP decoder branch discriminator
as BAEF §2 (see BAEF impl spec §2.3).

---

## 4. IP Address Binding Methods

Per BAEG-A §2.4. Four methods; the right choice depends on the P2P
framing in use.

### 4.1 Static (§2.4.1)

- Both SU and MDP are pre-configured with each other's IP addresses.
- Typical for **SLIP** (no negotiation capability).
- Simple, brittle: changing MDPs requires re-provisioning.

### 4.2 Passive (§2.4.2)

- SU learns the MDP's IP address by inspecting the source address of
  the **first** IP datagram the MDP sends.
- **The SU cannot send first.** The SU must wait for the MDP's first
  inbound datagram (which may or may not be destined for the SU itself).
- Works with SLIP (no negotiation needed). Typical for casually-paired
  laptop / radio connections.

### 4.3 Active (§2.4.3)

- SU sends an **ICMP Information Request** with destination IP set to
  `0.0.0.0` (meaning "this network").
- MDP replies with an **ICMP Information Reply** containing its IP
  address as the source.
- SU extracts the MDP address from the reply's source.
- Legitimate approach when SLIP is used and the SU must initiate
  communication but hasn't been statically configured.

### 4.4 Dynamic (§2.4.4)

Two sub-methods:

- **PPP IPCP negotiation.** On PPP links, IP address assignment can
  happen during standard PPP Link Control Protocol / IP Control
  Protocol negotiation. MDP learns the IP addressing info from the
  SU during PPP establishment.
- **DHCP** (mandatory for RNDIS/USB — see §5 below).

### 4.5 Binding × Combination Matrix

Recommended / common choice per combination:

| Combination | Recommended binding |
|-------------|----------------------|
| SLIP/TIA-232 | Static or Passive |
| PPP/TIA-232 | Dynamic (IPCP) or Static |
| SLIP/USB | Static or Passive |
| PPP/USB | Dynamic (IPCP) |
| **RNDIS/USB** | **Dynamic (DHCP — REQUIRED)** |
| Ethernet | Local policy (typically DHCP from an external server) |

---

## 5. RNDIS/USB DHCP Constraints (Normative)

Per BAEG-A §2.4.4 + Table 5. **RNDIS/USB is the only combination
where BAEG-A normatively pins IP binding** — the SU acts as a DHCP
server for the MDP.

### 5.1 Roles

- **SU = DHCP Server.** Exposes a DHCP service on the RNDIS link.
- **MDP = DHCP Client.** Requests addressing from the SU.
- Only the **"dynamic allocation"** DHCP mechanism is required
  (not "automatic" or "manual"). This means each MDP gets a
  lease-bound address from a server-managed pool.

### 5.2 DHCP Field Restrictions

**Table 5** of the spec lists per-field constraints for the A
interface DHCP exchange:

| DHCP field | Constraint |
|------------|------------|
| `op` | No restriction |
| `htype` | **Ignored by the server.** May be set to 1 (10mb Ethernet) by convention |
| `hlen` | **Ignored by the server.** May be set to 6 (10mb Ethernet) by convention |
| `hops` | **Always 0** |
| `xid` | No restriction |
| `secs` | No restriction |
| `flags` | No restriction |
| `ciaddr` | No restriction |
| `yiaddr` | No restriction |
| `siaddr` | **Always 0** |
| `giaddr` | No restriction |
| `chaddr` | No restriction |
| `sname` | **Ignored by server; always 0 in server-to-client messages** |
| `file` | **Ignored by server; always 0 in server-to-client messages** |
| `options` | See below |

### 5.3 Required DHCP Options

The DHCP server (SU) **must support** these options:

| Option | Purpose |
|--------|---------|
| Subnet Mask | IP prefix length |
| Router Option | Default gateway for the MDP (typically the SU's own A-interface IP) |
| **DHCP Message Type** | **Must be present in every DHCP message** |
| Server Identifier | SU's DHCP server identity |
| Parameter Request List | Client can request additional parameters |

Other parameters (e.g., DNS servers, NTP servers) are out of scope —
delivery of host-specific configuration is explicitly not required.
DHCP is used **solely to assign the MDP's IP address** here.

### 5.4 Decoder pattern for an A-interface sniffer

A tool capturing RNDIS/USB traffic will see standard DHCP DISCOVER /
OFFER / REQUEST / ACK exchanges between MDP and SU. The SU responds
as the server; the `siaddr` field will always be zero (so don't key
server identification on it — use the Server Identifier option
instead).

---

## 6. Network-Layer Payload

Per BAEG-A §2.5. Two payload types:

### 6.1 Data Application Information

Upper-layer protocols (TCP, UDP) carrying application data. Header
fields follow RFC 791; application-specific fields above transport
are out of scope.

This is the normal data path — an MDP generating dispatch / CAD /
status traffic to remote servers, or receiving such traffic back.

### 6.2 Control Signaling

Two sub-types:

**Error Reporting via ICMP (RFC 792):**

| ICMP type | Use |
|-----------|-----|
| Destination Unreachable (3) | Network, host, or port not reachable — triggered by the SU when relaying fails |
| Parameter Problem (12) | Bad checksum; IP version ≠ 4; MTU exceeded without fragmentation permitted; IP header too short |
| Echo Request (8) / Reply (0) | Ping; the SU's Echo Reply capability **must be configurable on/off** (security/policy) |

**Radio Management Traffic** — uses BAEE-C RCP or SNMP:

- IPv4 **Type of Service** field must be set to `0` (§2.5 explicit).
- IPv4 **Protocol** field identifies the specific radio-management
  transport (typically UDP = `17` for both RCP and SNMP).
- Destination port = 469 (RCP), 161/162 (SNMP).
- See `standards/TIA-102.BAEE-C/P25_Radio_Management_Protocols_Implementation_Spec.md`
  for the full RMP wire format running inside these UDP datagrams.

---

## 7. What's NOT in Scope

Per BAEG-A Foreword / §2.4 / §2.5:

| Item | Why out of scope |
|------|------------------|
| Application-layer protocol specifics | Application providers' domain |
| Transport-layer (TCP/UDP) header particulars | IETF RFCs suffice |
| IP fragmentation behavior | RFC 791; local policy |
| ICMP error handling policy | Local policy — spec only enumerates what CAN be emitted |
| Mapping FNE/Um exceptions to ICMP | Local policy |
| **IPv6** | Not addressed |
| Multicast | Not addressed |
| QoS marking | Only ToS=0 for radio-management traffic is specified |
| TIA-232 / USB **connector pinouts** | Physical conformance explicitly waived |
| Cable harness specifications | Local / vendor / integrator concern |

---

## 8. Implementer Checklist

For MDP software talking to a P25 radio via the A interface:

1. Pick a protocol combination that the target radio supports (check
   vendor docs; BAEG-A mandates at least one of the 6+, but vendors
   typically support 2–4).
2. Implement the corresponding stack — standard RFCs / IEEE specs
   cover SLIP/PPP/RNDIS/Ethernet.
3. Choose an IP binding method consistent with the combination:
   - RNDIS → DHCP client (mandatory).
   - PPP → IPCP client or static.
   - SLIP → static or passive (passive if the MDP will always initiate).
4. Use `ToS=0` for any radio-management traffic to the SU (BAEE-C).
5. Respect the SU's configurable Echo Reply policy — don't assume
   ICMP Echo succeeds.
6. Clamp IP datagram size to the SU's reported MTU (see BAEE-C
   `rmpMtu` / `GET_INFO` response).

For a P25 radio implementer exposing the A interface:

1. Implement at least one protocol combination to BAEG-A §2.3 stack
   figures.
2. On RNDIS/USB, run a conformant DHCP server per §5.
3. Support ICMP Echo Reply with a user-configurable on/off toggle
   (§2.5.2).
4. Route IPv4 datagrams appropriately between A and Um (when in
   router role); return ICMP Destination Unreachable when the Um
   path fails.
5. Expose BAEE-C radio-management protocols on UDP 469 (RCP) and/or
   161/162 (SNMP) on the A-interface address.

---

## 9. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify:

- A-interface architecture and protocol combinations overview —
  TIA-102.BAEG-A §2, Table 1, Figures 1–7.
- TIA-232 signal subset — §2.1.1, Tables 2–4.
- USB requirements — §2.1.2.
- Ethernet / IEEE 802.3 — §2.1.3 / §2.1.4.
- SLIP / PPP / RNDIS — §2.2.1 / §2.2.2 / §2.2.3.
- Network-layer procedures and host/router role — §2.4.
- IP binding methods (Static / Passive / Active / Dynamic) —
  §2.4.1–§2.4.4.
- DHCP restrictions (RNDIS/USB) — §2.4.4, Table 5.
- Network-layer payload (data / ICMP / radio management) — §2.5.
- Type-of-Service for radio management — §2.5.2.

---

## 10. Cross-References

**Upstream (this doc depends on):**
- `standards/TIA-102.BAEA-C/P25_Data_Overview_Implementation_Spec.md`
  — defines the A interface architecturally (§3.3) as SU ↔ MDP with
  four / six protocol combinations.

**Companion specs on the A interface:**
- `standards/TIA-102.BAEE-C/P25_Radio_Management_Protocols_Implementation_Spec.md`
  — RCP and SNMP management protocols carried over the A interface
  UDP bearer defined here.

**Companion P25 data specs:**
- `standards/TIA-102.BAEF/P25_Packet_Data_Host_Network_Interface_Implementation_Spec.md`
  — architectural sibling (Ed interface, FNE ↔ DHN) with the same
  Ethernet / 802.3 encapsulation choices.
- `standards/TIA-102.BAEB-B/P25_IP_Data_Bearer_BAEB_B_Implementation_Spec.md`
  + `standards/TIA-102.BAEB-C/P25_IP_Data_Bearer_Implementation_Spec.md`
  — the Um-side IP bearer that ultimately carries MDP traffic.
- `standards/TIA-102.BAEJ-A/P25_Conventional_Management_Service_Implementation_Spec.md`
  — conventional-mode management on the Um side; adjacent
  architectural peer.

**External normative references:**
- IETF **RFC 791** (IPv4), **RFC 792** (ICMP), **RFC 826** (ARP),
  **RFC 894** (IP over Ethernet), **RFC 1042** (IP over 802 Networks),
  **RFC 1055** (SLIP), **RFC 1661 / 1332** (PPP / IPCP),
  **RFC 2131** (DHCP).
- IEEE **802.3-2000** (Ethernet MAC + PHY).
- **USB Revision 3.1** (Universal Serial Bus Specification).
- **TIA-232-F / V.24** (serial interface signals).
- Microsoft **RNDIS** (Remote NDIS) specification.

**Supporting annex table:**
- `annex_tables/baeg_protocol_combinations.csv` — machine-readable
  catalog of all 7 protocol combinations with physical / P2P / IP
  binding characteristics.
