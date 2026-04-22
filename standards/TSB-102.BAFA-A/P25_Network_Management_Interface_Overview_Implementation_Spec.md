# P25 Network Management Interface (En) — TSB-102.BAFA / BAFA-A Implementation Spec

**Sources:**
- **TSB-102.BAFA** (APCO, December 1994) — original *Network Management
  Interface*. 13 pages. Preliminary TSB. **Superseded**.
- **TSB-102.BAFA-A** (July 1999, reaffirmed January 2013) — *Project 25
  Network Management Interface Overview*. 22 pages. Current informative
  TSB.

This impl spec combines both because they cover the same architectural
scope. BAFA-A is the current reference; BAFA (APCO) is captured as
historical context — its CMIP/GOSIP variant path is of no operational
relevance now that SNMP is the winner.

**Document type:** Informative TSBs (not normative standards).
Establishes architecture and terminology for the **En interface** — the
wire boundary between an **OMC-RF** (Operations and Maintenance Center
– Radio Frequency) inside an RFSS and an **external Network Management
Center (NMC)**. Neither document defines wire formats beyond referring
to SNMP / MIB-II; a fully-specified OMC-RF MIB was **always deferred as
"subject for further study"** and was never produced in the BAFA
document chain.

**Scope of this derived work:**
- §1 — What the En interface is and where it sits
- §2 — OMC-RF as a cascaded Manager/Agent
- §3 — SNMP and MIB-II as the transport (and why CMIP/GOSIP was dropped)
- §4 — OSI FCAPS functional areas: scope of standardization
- §5 — What BAFA / BAFA-A does NOT define
- §6 — Lineage: 1994 APCO TSB → 1999 BAFA-A → ongoing deferrals
- §7 — Cite-to section references
- §8 — Cross-references

**Pipeline artifacts:**
- `standards/TSB-102.BAFA (APCO)/TSB-102-BAFA (APCO)_Full_Text.md` —
  1994 clean-room extraction (copyrighted, git-ignored).
- `standards/TSB-102.BAFA (APCO)/TSB-102-BAFA (APCO)_Summary.txt` — 1994 summary.
- `standards/TSB-102.BAFA-A/TSB-102-BAFA-A_Full_Text.md` — 1999 clean-room extraction.
- `standards/TSB-102.BAFA-A/TSB-102-BAFA-A_Summary.txt` — 1999 summary.

---

## 1. The En Interface

Per BAFA-A §1 / BAFA §1. The En interface sits between:

- **OMC-RF** — the Operations and Maintenance Center function inside an RFSS. Manages the RFSS's internal RF Network Elements (base radios, base audio, base controllers, RF sub-system switches, gateways, consoles).
- **End System Network Management Center (NMC)** — an external management host, potentially vendor-neutral, that oversees multiple RFSSs across an agency or region.

```
   Vendor-neutral NMC (external)
              │
              │  En interface   ← THIS SPEC
              ▼
          OMC-RF (in RFSS)
              │
              │  Proprietary internal protocols (NOT standardized)
              ▼
┌─────────┬─────────┬──────────┬──────────┐
│ BR      │ BA      │ RFS      │ Consoles │
│ (radio) │ (audio) │ (switch) │          │
└─────────┴─────────┴──────────┴──────────┘
```

**Motivation:** without a standard En interface, each RFSS vendor's
management is accessible only via proprietary protocols. En allows a
single NMC to manage multi-vendor RFSS deployments.

---

## 2. OMC-RF as Cascaded Manager/Agent

Per BAFA-A §3, drawing from ITU-T **X.701** (OSI System Management)
and ITU-T **M.3010** (TMN Principles):

The OMC-RF plays **two roles simultaneously**:

- **Agent role toward the NMC** — accepts SNMP directives from the
  NMC, emits traps / notifications upward.
- **Manager role toward RF Network Elements** — issues vendor-specific
  management commands downward to each RFSS element.

**Mediation function.** The OMC-RF **translates between** the
standardized En interface information model and the various
vendor-proprietary information models of the RFSS elements. BAFA-A
**standardizes only** information-model conversion and protocol
conversion — other mediation functions (data concentration,
thresholding, routing, security, storage) are implementation-defined.

**Implementation consequence for an NMC implementer.** Talk to the
OMC-RF over SNMP. Do not try to bypass it to reach the underlying RF
elements — those are deliberately proprietary.

---

## 3. SNMP and MIB-II (and the CMIP/GOSIP history)

### 3.1 Current: SNMP + MIB-II

Per BAFA-A §3:

- **SNMP** (RFC 1157) over IP as the mandated transport.
- **MIB-II** (RFC 1213) as the MIB framework for the OSI seven-layer
  stack parts of the RF Network Elements.
- Operations: `TRAP`, `SET`, `GET`, `GET-NEXT`, `GET-RESPONSE`.
- **ASN.1** as the managed-object definition language.

### 3.2 Historical: CMIP/GOSIP

BAFA (1994) proposed two candidate transport suites:

| Suite | Protocols | Status |
|-------|-----------|--------|
| Internet | SNMP + MIB-II | **Won** |
| GOSIP / OSI | CMIP + GDMO | **Dropped** by BAFA-A v1.3 (Dec 1997) |

GOSIP/CMIP lost broader industry traction through the 1990s. By the
time BAFA-A was published (1999), the specification committed to SNMP
only. A conformant En implementation today is SNMP.

**Implementation consequence.** Ignore CMIP / GDMO paths in BAFA
(APCO); they're historical artifacts. Any production En deployment is
SNMP.

---

## 4. FCAPS Functional Areas — Scope of Standardization

Both BAFA and BAFA-A adopt the OSI **FCAPS** management model
(Configuration, Fault, Performance, Accounting, Security). BAFA-A §4
restricts the initial TIA-102 standardization scope:

| FCAPS area | Standardized in BAFA-A? | Rationale |
|------------|--------------------------|-----------|
| **Fault Management** | **Yes** — initial focus | Alarm surveillance + diagnostic/test operations |
| **Performance Management** | **Yes** — initial focus | Retrieval of RF traffic statistics |
| Configuration Management | **No** — accessible via X-window display of OMC-RF apps; not on En | Too vendor-specific to standardize |
| Accounting Management | **No** — X-window path only | Billing / usage accounting left to operators |
| Security Management | **No** — X-window path only | Left to agency policy |

**Implementation consequence.** An NMC using En sees **faults and
performance data** via SNMP. It does **not** do configuration,
accounting, or security management over En — those happen via
operator X-sessions directly on OMC-RF platforms.

### 4.1 Fault Management detail

Per BAFA-A §4:

- **Alarm Surveillance**:
  - Alarm Recognition / Notification — trap-driven or polled via `GET`.
  - Functional Status Retrieval — on-demand `GET`.
- **Diagnostic / Test Operations** — initiating diagnostics on RF
  network elements. Specific diagnostic operations "subject for
  further study."

### 4.2 Performance Management detail

Per BAFA-A §4:

- **Performance Retrieval** — RF traffic statistics, e.g.:
  - Count of radio access denials due to unavailable Base Audio
    resources.
  - (Other counters "subject for further study.")

---

## 5. What BAFA / BAFA-A Does NOT Define

Deferred throughout both documents:

| Item | Status |
|------|--------|
| Specific RF-System element definitions in the OMC-RF MIB | **Subject for further study** (never delivered in the BAFA series) |
| NMC registration with OMC-RF for specific faults | Deferred |
| Fault severity filtering | Deferred |
| Specific diagnostic / test operations | Deferred |
| Specific performance counter definitions | Deferred |
| Handling of concurrent management requests from multiple NMCs | Deferred (flagged as requiring further study) |
| Synchronization across multiple concurrent OMC-RF managers | Deferred |
| Management of RF Network Elements accessed over Um (radios, MDPs, etc.) | **Out of scope** — handled in air-interface standards |

**Implementation consequence.** A production En implementation
**cannot be built from BAFA / BAFA-A alone**. Vendors fill the gaps
with proprietary MIBs, documented to customers via their own product
literature. The "standard En" is the SNMP transport + MIB-II +
agreed-vendor-proprietary OMC-RF MIB.

---

## 6. Lineage

```
1994 December  — TSB-102.BAFA (APCO)  — 13 pages, SNMP + CMIP both proposed
                                        [superseded]
1997 December  — BAFA-A v1.3 draft    — CMIP path removed
1999 July      — TSB-102.BAFA-A       — 22 pages, current form (SNMP only)
2013 January   — BAFA-A reaffirmed    — no substantive change
[present]      — no subsequent revision
```

Neither document has ever been superseded by a normative standard
defining the OMC-RF MIB.

---

## 7. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify:

**BAFA-A (current):**
- En interface scope — §1.
- References (SNMP, MIB-II, X.701, M.3010) — §2.
- OMC-RF architecture and Manager/Agent dual role — §3.
- FCAPS functional areas — §4.
- Fault Management (Alarm Surveillance, Diagnostic / Test) — §4.
- Performance Management (Performance Retrieval) — §4.
- "Subject for further study" deferrals — throughout §3 and §4.

**BAFA (APCO, historical):**
- Two-suite (SNMP vs CMIP) discussion — §2 / §3.
- ASN.1 managed-object framework — §4.
- (Historical only — do not cite in new work.)

---

## 8. Cross-References

**Upstream (this doc depends on):**
- **TSB-102-A / TSB-102-D** — Project 25 System and Standards Definition;
  provides the overall system model from which the En reference point
  is drawn.
- ITU-T **X.701** — OSI System Management Overview (Manager / Agent
  roles).
- ITU-T **M.3010** — TMN Telecommunications Management Network
  Principles (the cascaded Manager/Agent architecture).

**Companion specs:**
- `standards/TIA-102.AABA-B/P25_Trunking_Overview_Implementation_Spec.md`
  — architectural context for the RFSS inside which the OMC-RF lives.
- `standards/TIA-102.BAEE-C/P25_Radio_Management_Protocols_Implementation_Spec.md`
  — Radio Management Protocols over the A interface (radio-side
  management, distinct from the infrastructure-side En management
  specified here). BAEE-C's SNMP MIB for radio management is at a
  **different** private enterprise OID (TR8dot5 / 17405) than any
  OMC-RF MIB would be; the two are independent namespaces.

**External references:**
- IETF **RFC 1157** (SNMP v1).
- IETF **RFC 1213** (MIB-II).
- IETF **RFC 1158** (MIB-II candidate, cited in BAFA 1994).
- ISO/IEC **9595:1990** (CMIS, historical).
- FIPS PUB **146-1** (GOSIP, historical).
