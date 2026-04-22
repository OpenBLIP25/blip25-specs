# P25 Packet Data Host Network Interface (Ed) — TIA-102.BAEF Implementation Spec

**Source:** TIA-102.BAEF (2013-11), *Packet Data Host Network Interface
Specification*. 1st edition (original). Standalone successor to the
Ed-interface content formerly embedded in TIA-102.BAEB-A; BAEF cancels
and replaces that earlier material.

**Document type:** minimal profile spec. BAEF is deliberately a
20-page boundary document — it specifies the Ed interface as the
FNE ↔ Data Host Network attachment point and fixes exactly two
mandatory protocol combinations. **Everything above IP and many
things at IP** are deferred to local policy.

**Where this sits in the stack.** The Ed interface is the "north
side" of the FNE — where the P25 infrastructure meets the agency's IP
network and the application servers that host data services for P25
subscribers. BAEA-C §3.2 / §5.3 places Ed on the Conventional-FNE and
Trunked-FNE data paths; the Um radio side is BAAA-B / BAED-A / BAEB-B/C.
BAEF is the Ethernet/802.3 profile only.

```
┌────────────────────────────────────┐
│ SUs (RF side, BAAA-B, BAED-A,      │
│ BAEB-B/C)                          │
└──────────────┬─────────────────────┘
               │
               │  Um air interface
               ▼
┌────────────────────────────────────┐
│ FNE (Zone Controller / Data        │
│ Gateway)                           │
└──────────────┬─────────────────────┘
               │
               │  Ed interface ← THIS SPEC
               ▼
┌────────────────────────────────────┐
│ Data Host Network (DHN): agency IP │
│ network + application servers      │
└────────────────────────────────────┘
```

**Scope of this derived work:**
- §1 — What BAEF specifies (and what it explicitly doesn't)
- §2 — The two protocol combinations and their RFC/IEEE references
- §3 — Network-layer behavior and out-of-scope rules
- §4 — Implementer checklist
- §5 — Cite-to section references
- §6 — Cross-references

**Pipeline artifacts:**
- `standards/TIA-102.BAEF/TIA-102-BAEF_Full_Text.md` — clean-room
  extraction (copyrighted, git-ignored).
- `standards/TIA-102.BAEF/TIA-102-BAEF_Summary.txt` — ~1000-word summary.
- `standards/TIA-102.BAEF/TIA-102-BAEF_Related_Resources.md` — external refs.

---

## 1. What BAEF Specifies (and What It Doesn't)

### 1.1 In scope

BAEF §1.1 + §2:

- **Link-layer encapsulation** on the Ed interface: exactly one of two
  IEEE-family encapsulations must be used (see §2).
- **Network layer**: IPv4 (RFC 791) only.
- **Normative references** to the IETF and IEEE standards that govern
  the above.

### 1.2 Explicitly out of scope

BAEF Foreword + §2.3 + §2.4:

- **IP address binding** (how the FNE and DHN learn each other's IP
  addresses). Local policy. ARP (RFC 826) is cited informatively as a
  common method; DHCP is not mentioned.
- **Application-layer protocols** (data formats above IP). Local
  policy / application-specific.
- **Transport above IPv4**. TCP and UDP are cited informatively as the
  intended transports, but specific port numbers and protocol choices
  are beyond BAEF's scope.
- **FNE-to-DHN control signaling exceptions** (how unreachable
  notifications propagate). Local policy. ICMP (RFC 792) is cited
  informatively.
- **IPv6.** Not addressed in this edition.
- **MTU, QoS, VLAN, security.** No explicit requirements.
- **Physical layer specifics.** Delegated to IEEE 802.3-2000 (for the
  802.3 option) and to generic Ethernet hardware (for the RFC 894
  option).

**Implementation consequence.** A vendor claiming BAEF conformance
need only ensure their FNE's Ed port speaks one (or both) of the two
mandated combinations carrying IPv4. Any higher-layer behavior
(address assignment, VPN, TLS, authentication) is negotiated between
the operator and the integrator, NOT by this standard.

---

## 2. The Two Mandated Protocol Combinations

BAEF §2 Figures 1 and 2. Both FNE and DHN implement **the same** stack.

### 2.1 IPv4 over Ethernet (RFC 894)

```
┌──────────────────┐
│  Payload         │
├──────────────────┤
│  IPv4 (RFC 791)  │
├──────────────────┤
│  RFC 894         │  Encapsulation of IP over Ethernet v2 frames
├──────────────────┤
│  Ethernet        │  DIX / Ethernet II — EtherType = 0x0800 for IPv4
└──────────────────┘
```

Normative references: **RFC 894** (IP over Ethernet), **RFC 791** (IPv4).

### 2.2 IPv4 over IEEE 802.3 (RFC 1042)

```
┌──────────────────┐
│  Payload         │
├──────────────────┤
│  IPv4 (RFC 791)  │
├──────────────────┤
│  RFC 1042        │  LLC/SNAP encapsulation for IP over 802 networks
├──────────────────┤
│  IEEE 802.2      │  LLC
├──────────────────┤
│  IEEE 802.3      │  CSMA/CD MAC + physical
└──────────────────┘
```

Normative references: **RFC 1042** (IP over 802 Networks), **IEEE
802.2-1998** (LLC), **IEEE 802.3-2000** (CSMA/CD MAC + PHY), **RFC 791**
(IPv4).

### 2.3 Choosing Between Them

BAEF does **not** require supporting both. A conformant FNE or DHN
must support at least one; the two are operationally interchangeable
for agencies because modern Ethernet hardware handles both framings
indistinguishably. RFC 894 (Ethernet II / DIX) is far more common in
practice than RFC 1042 (802.3 + LLC/SNAP); the latter survives mainly
for legacy or industrial-Ethernet compatibility.

**Wire difference (for decoder sanity).** The distinguishing field is
the **Length/Type field at offset 12 of the Ethernet header**:

- Value `≥ 0x0600` (1536) → Ethernet II framing; the field is an
  EtherType. For IPv4, EtherType = `0x0800`.
- Value `≤ 0x05DC` (1500) → 802.3 framing; the field is a Length.
  The first byte of the payload is the 802.2 LLC DSAP (`0xAA` for
  SNAP), followed by SSAP (`0xAA`), Control (`0x03`), 3-byte OUI
  (`0x000000`), and a 2-byte EtherType (`0x0800` for IPv4).

A decoder of BAEF traffic that wants to be flavor-agnostic should
inspect byte 12 once and branch accordingly — the rest of the stack
(IPv4 header onward) is identical between the two options.

---

## 3. Network-Layer Behavior

BAEF §2.3 + §2.4:

### 3.1 What the FNE carries

IPv4 datagrams traverse the Ed interface in both directions:

| Direction | Source | Destination | Payload source |
|-----------|--------|-------------|----------------|
| FNE → DHN | SU (after unwrapping LLC/CMS/SNDCP/SCEP) | Data Host or agency network | Application data from an SU |
| DHN → FNE | Data Host or agency network | SU (after wrapping in appropriate CAI stack) | Application data destined for an SU |

**Control signaling exceptions** (e.g., host unreachable) may be
forwarded from FNE to the originating IPv4 source. The mapping is
explicitly local policy — BAEF recommends (informatively) that ICMP
(RFC 792) is the likely mechanism, but doesn't require it.

### 3.2 What the FNE does with address binding

Per BAEF §2.3: IP address binding between FNE and DHN is **local
policy**. ARP is cited as a common approach; nothing forbids static
address assignment, DHCP, or a vendor-proprietary scheme.

**Practical consequence.** An operator integrating a P25 system with
an existing IP network cannot look to BAEF for address-assignment
rules — they must consult the FNE vendor's documentation and the
agency's network-engineering policy. BAEF's silence on this is
deliberate.

---

## 4. Implementer Checklist

For a vendor claiming BAEF conformance on a specific FNE product:

| # | Requirement | Source | Test |
|---|-------------|--------|------|
| 1 | Ed port accepts IPv4 datagrams | §2 | Send a valid IPv4 datagram; observe acceptance |
| 2 | Uses RFC 894 OR RFC 1042 encapsulation | §2.1, §2.2 | Packet capture; verify encapsulation |
| 3 | Implements RFC 791 IPv4 correctly (header, fragmentation, TTL, etc.) | §2.3, Normative [4] | Standard IPv4 conformance testing |
| 4 | Does NOT depend on IPv6 | §2 (v4-only) | Verify no IPv6 emitted |
| 5 | IP address binding documented at deployment | §2.3 (local policy) | Vendor docs |
| 6 | Control signaling (unreachable) policy documented | §2.4 (local policy) | Vendor docs |

**For a DHN-side integrator**, the checklist is simpler: provide a
conformant Ethernet or 802.3 network attachment point with a
reachable IPv4 address, and document that address for the FNE vendor.

---

## 5. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify:

- Ed interface scope — TIA-102.BAEF §1.1.
- RFC and IEEE normative references — §1.2.
- Protocol combinations overview — §2.
- IPv4 over Ethernet (RFC 894) — §2.1, Figure 1.
- IPv4 over IEEE 802.3 (RFC 1042) — §2.2, Figure 2.
- Network-layer procedures + scope of local policy — §2.3.
- Network-layer payload and control-signaling exceptions — §2.4.

---

## 6. Cross-References

**Upstream (this doc depends on):**
- `standards/TIA-102.BAEA-C/P25_Data_Overview_Implementation_Spec.md`
  — establishes Ed as the FNE ↔ DHN attachment point in the packet
  data architecture (§3.2 of that spec; Table 4 lists the same two
  protocol combinations BAEF mandates).
- IETF **RFC 791** (IPv4), **RFC 894** (IP over Ethernet), **RFC 1042**
  (IP over 802 networks), **RFC 826** (ARP, informative), **RFC 792**
  (ICMP, informative).
- IEEE **802.2-1998** (LLC), **802.3-2000** (Ethernet MAC + PHY).

**Companion P25 data specs:**
- `standards/TIA-102.BAEB-B/P25_IP_Data_Bearer_BAEB_B_Implementation_Spec.md`
  — the SU-side IP bearer that carries datagrams which ultimately
  egress through Ed.
- `standards/TIA-102.BAEB-C/P25_IP_Data_Bearer_Implementation_Spec.md`
  — SNDCP V3 consolidated; same trajectory.
- `standards/TIA-102.BAED-A/P25_Packet_Data_LLC_Implementation_Spec.md`
  — LLC layer under the SU-side IP bearer; unwrapped by the FNE
  before forwarding to Ed.
- `standards/TIA-102.BAEJ-A/P25_Conventional_Management_Service_Implementation_Spec.md`
  — Conventional-FNE management; on the Um side of the FNE, not Ed.
- `standards/TIA-102.BAEG-A/…` — A-interface (SU ↔ MDP); adjacent
  interface, same architectural model.

**Related reading:**
- BAEF is a minimal profile; the companion BAEE-C (Radio Management
  Protocols) shows how richer protocols are layered on UDP inside
  the IPv4 frames BAEF carries on the Ed side.
