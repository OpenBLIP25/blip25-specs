# P25 ISSI — Packet Data Services (BACF) Implementation Spec

**Source:** TIA-102.BACF (October 2009, reaffirmed January 2013),
*Inter-RF Subsystem Interface (ISSI) Messages and Procedures for
Packet Data Services*.

**Document type:** PROTOCOL. Defines **MIPv4-based tunneling** across
the ISSI so a roaming subscriber with an active SNDCP context gets
packet data service from a Serving RFSS while its home registration
lives in a Home RFSS.

**Scope:** Trunked FNE Data only. Repeated Data and Direct Data
configurations are **out of scope** — BACF addresses a single
architectural case: **an SU with an active SNDCP context being served
across an ISSI link to its home system**.

**Not an on-air spec.** BACF lives wholly on the wireline ISSI between
two RFSSs. Nothing defined here appears on Um.

**Scope of this derived work:**
- §1 — What BACF covers and what it excludes
- §2 — Architecture: PDIG, Packet Data ISSI Link, PD-SAP
- §3 — Protocol stack: SIP + MIPv4 + IP-over-IP + UDP
- §4 — MIPv4 field rules (T bit, S bit, Lifetime, CoA, HMAC auth)
- §5 — Reply-code dispatch table
- §6 — PD-SAP state machines (4-state, both Serving and Home)
- §7 — Nine procedure types
- §8 — Context transfer race condition (Annex A.8)
- §9 — Header compression resync on context transfer
- §10 — MRC Standby Timer and deactivation
- §11 — Cite-to section references
- §12 — Cross-references

**Pipeline artifacts:**
- `standards/TIA-102.BACF/TIA-102-BACF_Full_Text.md` — clean-room
  extraction (copyrighted, git-ignored).
- `standards/TIA-102.BACF/TIA-102-BACF_Summary.txt` — retrieval summary.
- `standards/TIA-102.BACF/TIA-102-BACF_Related_Resources.md`.
- `annex_tables/bacf_mipv4_field_rules.csv` — ISSI-specific constraints
  on RFC 3344 Registration fields.
- `annex_tables/bacf_mipv4_reply_codes.csv` — reply codes with
  applicable / not-applicable-over-ISSI annotation.
- `annex_tables/bacf_procedures.csv` — 9 procedures + the race condition.

---

## 1. Scope Summary

**In scope:**
- Trunked FNE Data roaming across two RFSSs.
- A single SU per Packet Data ISSI Link — the spec addresses per-SU
  SNDCP-context-level tunneling, not site-wide aggregation.

**Out of scope:**
- **Repeated Data** (SU ↔ FSR ↔ SU). No RFSS-to-RFSS routing.
- **Direct Data** (SU ↔ SU). Peer-to-peer; no infrastructure.
- **Conventional FNE Data** — BACF is specifically trunked.
- **Voice / control / supplementary** ISSI services — those are in
  BACA / BACE / BACD, not BACF.

---

## 2. Architecture

Per BACF §2.

```
          Home RFSS                                  Serving (Foreign) RFSS
┌─────────────────────────────┐                ┌─────────────────────────────┐
│  PDIG (Packet Data ISSI     │                │  PDIG (Packet Data ISSI     │
│  Gateway) acts as Home Agent│                │  Gateway) acts as Foreign   │
│                             │   ISSI link    │  Agent                      │
│  - SNDCP termination        │◀──────────────▶│  - SNDCP termination        │
│  - Home Agent (MIPv4)       │                │  - Foreign Agent (MIPv4)    │
│  - Holds master SNDCP ctx   │                │  - Local SU                 │
└─────────────────────────────┘                └─────────────────────────────┘
                                                       │
                                                       │ Um air interface
                                                       ▼
                                                 ┌─────────┐
                                                 │ Roaming │
                                                 │    SU   │
                                                 └─────────┘
```

### 2.1 PDIG (Packet Data ISSI Gateway)

Every RFSS has one **PDIG** — the functional entity that bridges:
- The **air-interface SNDCP world** (BAEB-B/C on Um).
- The **IP-roaming world** (MIPv4 on the ISSI).

PDIG provides **SNDCP protocol termination** plus **Home Agent /
Foreign Agent** functions per MIPv4.

### 2.2 Packet Data ISSI Link

When an SU roams to a foreign RFSS, the two PDIGs establish a
**Packet Data ISSI Link** consisting of **two IP-in-IP tunnels**
paired for a single SNDCP context:

| Tunnel | Direction | Carries |
|--------|-----------|---------|
| **Forward Tunnel** | Home PDIG → Serving PDIG | IP datagrams destined for the SU |
| **Reverse Tunnel** | Serving PDIG → Home PDIG | IP datagrams originating from the SU |

**Reverse tunneling is mandatory** (per RFC 3024, direct-delivery
style). The **T bit** in every MIPv4 Registration Request is
**always 1** — the spec does not allow classic triangular routing.

### 2.3 PD-SAP (Packet Data Service Access Point)

PD-SAP is the internal interface between the **Controlling Function**
and the **MIPv4 protocol element** within each RFSS. It is **the
state machine boundary** — all BACF state machine definitions are
expressed in terms of PD-SAP primitives.

**Implementation consequence.** An implementer building the BACF
stack writes the PD-SAP as an internal API. Above PD-SAP: the
Controlling Function consumes SIP + SNDCP-context events. Below
PD-SAP: the MIPv4 engine emits / consumes Registration messages.

---

## 3. Protocol Stack

Per BACF §3.

```
Controlling      ┌──────────────────────────┐
Function         │  SIP REGISTER            │  from BACA-A §4
─ PD-SAP ─       │  (context mobility:       │
                 │   Register / Deregister / │
                 │   Query)                  │
MIPv4 stack:     ├──────────────────────────┤
                 │  MIPv4                    │  RFC 3344 + RFC 3024 extensions
                 │  (Registration Request /  │  (reverse tunneling, T=1)
                 │   Reply)                  │
                 ├──────────────────────────┤
                 │  IP-over-IP               │  RFC 2003 — datagram tunnel
                 ├──────────────────────────┤
                 │  UDP port 434             │  Registration transport
                 ├──────────────────────────┤
                 │  IPv4                     │
                 └──────────────────────────┘
```

**Two separate IP / UDP flows** on the ISSI:

1. **Registration signaling** — MIPv4 Registration messages on UDP 434.
2. **User-data tunnels** — IP-in-IP encapsulation (RFC 2003) on IPv4
   directly.

And **one more channel** (on SIP/BACA-A transport):

3. **SIP REGISTER** — SNDCP context mobility indications between
   Home and Serving PDIGs.

---

## 4. MIPv4 Field Rules

Full table: **`annex_tables/bacf_mipv4_field_rules.csv`**.

BACF imposes ISSI-specific constraints on top of RFC 3344:

| Field | Rule |
|-------|------|
| **`T` bit** (Reverse Tunneling) | **Always 1** — no exceptions |
| **`S` bit** (Simultaneous Bindings) | **`0`** for new activation and context transfer (tear down prior bindings). **`1`** for renewal (retain Forward Tunnel during the exchange) |
| **Lifetime** | Non-zero for registration / renewal. **`0`** for deregistration. **`0xFFFF` (infinity) SHALL NOT be used.** |
| **Care-of Address (CoA)** | Serving PDIG's IP for registration / renewal. Mobile Host Address for deregister-all-bindings |
| **Authentication Extension** | HMAC-MD5 per RFC 2104 with **SPI = 1000**. **Mandatory.** No other extensions present in Registration messages over ISSI. |
| **Identification field** | NTP timestamp format (RFC 1305). Default clock tolerance **7 seconds** for replay protection. |

**Decoder consequence.** A tool inspecting MIPv4 over ISSI should
validate these constraints and flag any deviation — non-T-bit-1
traffic is either non-BACF or non-compliant.

---

## 5. MIPv4 Reply Codes

Full table: **`annex_tables/bacf_mipv4_reply_codes.csv`**.

BACF restricts which RFC 3344 reply codes are valid over ISSI:

| Code | Meaning | Applicable over ISSI? |
|------|---------|------------------------|
| `0` | accepted | **Yes** |
| `1` | accepted; simultaneous bindings unsupported | **Yes** |
| `64`–`88` | various RFC 3344 codes | **NOT applicable** |
| `128` | reason unspecified | Yes |
| `129` | administratively prohibited | Yes |
| `130` | insufficient resources | Yes |
| `131` | HA failed | Yes |
| `132` | not applicable over ISSI | Yes |
| `133` | registration identification mismatch | Yes |
| `134` | poorly formed request | Yes |
| `135` | too many simultaneous bindings | Yes |
| `136` | unknown HA address | Yes |

**Implementation rule:** a Home PDIG emitting a reply code not in
the "applicable" set is non-conformant.

---

## 6. PD-SAP State Machines

Per BACF §5 (Tables 1–8). Each RFSS maintains a **4-state PD-SAP
state machine per SNDCP context**.

### 6.1 Serving RFSS state machine

```
     ┌──────────┐     start reg        ┌─────────────┐
     │ Initial  │───────────────────▶│ Registering │
     └──────────┘                      └──────┬──────┘
        ▲                                     │  RegReply accepted
        │   DeReg complete                    ▼
  ┌─────┴───────┐   start dereg        ┌─────────────┐
  │ DeRegister- │◀────────────────────│ Registered  │
  │    ing      │                      └─────────────┘
  └─────────────┘
```

### 6.2 Home RFSS state machine

Symmetric — same 4 states (`Initial` / `Registering` / `Registered` /
`DeRegistering`), driven by received MIPv4 messages and SIP
REGISTER indications.

### 6.3 Handling of context mobility

The state machine tables enumerate, for each state:
- Valid incoming events (PD-SAP primitives, timers, received messages).
- Required actions for each event (send, clear context, start timer).

Handled events include:
- Registration Request / Reply (both directions).
- SIP REGISTER indications (context mobility from BACA-A).
- Deregistration from all three possible initiators (SU, Serving,
  Home).
- Context transfer.
- Renewal.
- Error conditions: identification mismatch, resource failure, timer
  expiry.

---

## 7. Nine Procedure Types

Full catalog: **`annex_tables/bacf_procedures.csv`**.

Per BACF §6.3:

| # | Procedure | §ref | Annex A |
|---|-----------|------|---------|
| 1 | SU Initiated Activation | §6.3.1 | A.1 |
| 2 | Home RFSS Initiated Activation | §6.3.2 | A.2 |
| 3 | SU Initiated Deactivation | §6.3.3 | A.3 |
| 4 | Serving RFSS Initiated Deactivation | §6.3.4 | A.4 |
| 5 | Home RFSS Initiated Deactivation | §6.3.5 | A.5 |
| 6 | Home RFSS Context Query | §6.3.6 | — |
| 7 | Serving RFSS Context Query | §6.3.7 | — |
| 8 | **SNDCP Context Transfer** (SU roams) | §6.3.8 | A.6 |
| 9 | Registration Renewal | §6.3.9 | A.7 |

### 7.1 SNDCP Context Transfer in detail (§6.3.8 / A.6)

The roaming case — most complex:

1. SU roams from Old Serving RFSS to New Serving RFSS (on Um).
2. New Serving RFSS sends **MIPv4 Registration Request with `S = 0`**
   to Home RFSS (tear down prior bindings).
3. Home RFSS updates Forward Tunnel binding to point at New Serving
   PDIG.
4. Home RFSS sends **SIP REGISTER-deregister** to Old Serving RFSS.
5. Old Serving RFSS sends **Deregistration (Lifetime=0)** to Home
   RFSS, completing cleanup.
6. **First IP packet in each direction MUST use a full uncompressed
   header** — header compression resync (see §9).

---

## 8. Context Transfer Race Condition (Annex A.8)

Per BACF Annex A.8. The **critical implementation pitfall** in BACF.

**Scenario:** an Old Serving RFSS sends a **Registration Renewal
(S=1)** just as an SNDCP Context Transfer begins:

```
Old Serving                   Home                    New Serving
    │                          │                          │
    │ Renewal (S=1, Lifetime>0)│                          │
    ├────────────────────────▶│                          │
    │                          │                          │
    │                          │ MIPv4 RegReq (S=0)       │
    │                          │◀─────────────────────────┤
    │                          │                          │
    │                          │◀ SIP REGISTER-deregister │
    │◀─────────────────────────┤                          │
    │                          │                          │
    │ Now Home briefly holds   │                          │
    │ bindings to BOTH Old and │                          │
    │ New Serving              │                          │
    │                          │                          │
    │ DeReg (Lifetime=0)       │                          │
    ├────────────────────────▶│                          │
    │                          │ Binding resolves         │
    │                          │ (only New Serving remains)│
```

**Implementation rule:** the Home PDIG **must** accept simultaneous
bindings transiently — the race resolves when the Old Serving RFSS
sends its `Lifetime=0` Deregistration in response to the SIP
REGISTER-deregister. A Home PDIG that rejects the second binding
attempt because it "already has a binding" breaks roaming.

**Pitfall risk.** Naive implementations that enforce
single-binding-invariant fail in the race window. Flag this in test
plans.

---

## 9. Header Compression Resync on Context Transfer

Per BACF §6.3.8.

SNDCP uses header compression (per BAEB-C) to save air-interface
bandwidth. The compression state is **per-context** and does not
transfer across RFSSs.

**Rule:** after SNDCP Context Transfer completes, the **first IP
packet in each direction** (Serving → SU via Forward Tunnel; SU →
Serving via Reverse Tunnel and then onward) **MUST be sent with a
full uncompressed header**. This resynchronizes the compression
state on both sides.

**Decoder consequence.** A decoder observing packet data after a
roaming event should expect uncompressed headers briefly. If it
sees consistent compression immediately post-transfer, something's
wrong with the handoff.

---

## 10. MRC Standby Timer

Per BACF §5 + TIA-102.BAEB.

The **MRC (Mobile Radio Control) Standby Timer** is the inactivity
mechanism that governs when a Serving RFSS tears down an inactive
SNDCP context:

- Timer runs in the Serving RFSS.
- Reset on user-data activity.
- On expiry, Serving RFSS initiates **Procedure 4: Serving RFSS
  Initiated Deactivation** — sends Deregistration (Lifetime=0) to
  Home RFSS.

Timer value is specified in BAEB-C, not BACF.

---

## 11. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify:

- Scope (trunked FNE only) — TIA-102.BACF §1.
- Architecture: PDIG, Packet Data ISSI Link — §2.
- PD-SAP — §2 / §5.
- Protocol stack (SIP, MIPv4, IP-in-IP, UDP 434) — §3.
- MIPv4 Registration field rules — §4.
- Reply codes — §4 (applicable / not applicable tables).
- PD-SAP state machine tables — §5 (Tables 1–8).
- Nine procedure types — §6.3.1–§6.3.9.
- Annex A message sequence diagrams — Annex A.1–A.7.
- **Annex A.8 race condition** (critical).
- Header compression resync on context transfer — §6.3.8.

---

## 12. Cross-References

**Upstream (this doc depends on):**
- **TIA-102.BACA-A** — parent ISSI spec; **BACF delegates §4 (SIP
  REGISTER for context mobility) entirely to BACA-A**. Not yet
  processed in this repo.
- `standards/TIA-102.BAEB-C/P25_IP_Data_Bearer_Implementation_Spec.md`
  — SNDCP context protocol, MRC Standby Timer, APN structure, header
  compression.
- **IETF RFC 3344** — Mobile IPv4 Registration Request / Reply base.
- **IETF RFC 3024** — Reverse Tunneling for Mobile IP (T bit, direct
  delivery style).
- **IETF RFC 2003** — IP Encapsulation within IP.
- **IETF RFC 2104** — HMAC.
- **IETF RFC 1305** — NTP timestamp.

**Companion ISSI specs:**
- `standards/TIA-102.BACE/P25_ISSI_Conventional_Implementation_Spec.md`
  — Conventional ISSI (Ec interface).
- `standards/TIA-102.BACA-B-3/P25_ISSI_IWF_Interworking_Implementation_Spec.md`
  — IWF interworking addendum (packet data explicitly out of scope
  for IWF — see that spec's §3.2).
- **TIA-102.BACD-B** — ISSI Supplementary Data (status updates,
  short messages).
- **TIA-102.BACG** — ISSI other (per BACF references).

**Supporting annex tables:**
- `annex_tables/bacf_mipv4_field_rules.csv` — MIPv4 field constraints.
- `annex_tables/bacf_mipv4_reply_codes.csv` — reply codes with
  applicability annotation.
- `annex_tables/bacf_procedures.csv` — 9 procedures + race condition.
