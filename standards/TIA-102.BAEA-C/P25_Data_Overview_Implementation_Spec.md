# P25 Data Services Overview — TIA-102.BAEA-C Implementation Spec

**Source:** TIA-102.BAEA-C (2015-12), *Project 25 Data Overview and
Specification*. Supersedes BAEA-B (2012). Fourth edition; maintenance
revision addressing errata from BAEA-B. Adopted by Project 25 Steering
Committee October 2015.

**Document type:** architectural overview of the BAEA–BAEJ packet-data
document suite. BAEA-C is **not** a wire-format spec — it defines the
taxonomy (data services, interfaces, bearer services, configurations,
protocol stacks) and defers every byte-level detail to companion
documents. This derivation is therefore a navigation hub for
implementers working the data-services stack: it fixes the cross-cutting
concepts, pins down which bearer-service × configuration combinations
are valid, and points at the right downstream spec for each stack
layer.

**Scope of this derived work:**
- §1 — The three-tier data services taxonomy (Low-Speed / Supplementary / Packet)
- §2 — Packet Data interface matrix (Um / Ed / A / G / Ef / Ec / Um2 / …)
- §3 — The two bearer services (CAI Data, IP Data)
- §4 — The four configurations (Direct / Repeated / Conventional-FNE / Trunked-FNE)
- §5 — Bearer × configuration matrix and protocol stack layering
- §6 — SCEP vs SNDCP vs TMS decision tree
- §7 — Symmetric vs asymmetric addressing rules
- §8 — Confirmed vs Unconfirmed mandatory-vs-optional table
- §9 — Cross-reference map: which spec answers which question
- §10 — Out-of-scope and future-work catalogue
- §11 — Cite-to section references

**Pipeline artifacts:**
- `standards/TIA-102.BAEA-C/TIA-102-BAEA-C_Full_Text.md` — clean-room
  extraction (copyrighted, git-ignored).
- `standards/TIA-102.BAEA-C/TIA-102-BAEA-C_Summary.txt` — ~1000-word
  summary for retrieval.
- `standards/TIA-102.BAEA-C/TIA-102-BAEA-C_Related_Resources.md` —
  lineage, open-source implementations, external references.

---

## 1. Three-Tier Data Services Taxonomy

BAEA-C §2 partitions "TIA-102 Data Services" into three disjoint
classes. Implementers must know which class a given on-air data payload
belongs to before choosing a decoder path.

| Tier | Lives in | Scope of BAEA-C | Primary spec |
|------|----------|-----------------|--------------|
| **Low-Speed Data Service** | 4 octets embedded in each FDMA voice superframe | Catalogued only — use is not standardized by any TIA-102 doc | BAAA-B §7 (voice LDU LSD slot) |
| **Supplementary Services** | Non-packetized services (Emergency Alarm, Radio Inhibit, Status Update, Short Message, Status Query) riding on TSBKs or conventional control messages | Out of scope — referenced for completeness | AABC-E / AABD-B / AABG / BAAD-B / BACD / BACE |
| **Packet Data Service** | Full packet-data stack on FDMA Um | **The focus of BAEA-C and the BAEA–BAEJ suite** | BAEA-C + BAEB-B/C + BAED-A + BAEE-C + BAEF + BAEG-A + BAEJ-A |

**Implementation consequence for a passive decoder.** If you observe
four mysterious octets per voice superframe that decode as neither
TSBK nor Link Control, that's Low-Speed Data. The use is vendor-defined;
there's no standard parser. Don't conflate it with Packet Data.

BAEA-C § 2.3 (Table 2) documents one designator change that matters
for repo hygiene: **BAEC is withdrawn**, **BAEH is reserved for future
IP Data Security Service**, **BAEI is "not used."**

---

## 2. Packet Data Interface Matrix

BAEA-C §3 classifies each of the 11 TIA-102 Open System Model
interfaces by its relevance to Packet Data:

| Interface | What it connects | Packet Data? | Normative spec |
|-----------|------------------|--------------|-----------------|
| **Um** | SU ↔ SU / FSR / FNE (FDMA air interface) | **YES — D** (this suite) | BAAA-B PHY/MAC + BAED-A LLC + BAEB-B/C (IP) |
| **Ed** | FNE ↔ Data Host Network | **YES — D** | BAEF (IPv4 over Ethernet / 802.3) |
| **A** | SU ↔ Mobile Data Peripheral | **YES — D** | BAEG-A (IPv4 over PPP / SLIP over USB / TIA-232) + BAEE-C (SU mgmt) |
| **G** | Inter-RFSS (ISSI) | W — wireline suite | TSB-102.BACC-B (ISSI wireline) |
| **Ef** | Fixed Station Subsystem Interface | **future work** | — |
| **Ec** | Console Subsystem Interface | **future work** | — |
| **Um2** | TDMA air interface | **future work** (T — TDMA suite when specified) | — |
| **Et** | Telephone interconnect | N/R | — |
| **En** | Network Management | N/R | — |
| **KFD-MR** | KFD ↔ Mobile Radio | N/R | — |
| **IKI** | Inter-KMF | N/R | — |

**Implementation consequence.** Packet Data on Phase 2 TDMA (Um2), the
Ef (fixed station) interface, and the Ec (console) interface are all
deferred in BAEA-C. As of publication (2015-12), any data payload
observed on those interfaces is **out of standard scope** — vendor
extensions only.

---

## 3. Packet Data Bearer Services

BAEA-C §4 standardizes exactly two bearer services that ride on Um:

| Bearer service | Payload shape | Layer above LLC | Use when |
|----------------|---------------|-----------------|----------|
| **CAI Data Bearer Service** | Opaque octets (any format) | (none — payload directly on LLC) | Purpose-specific services that define their own framing above LLC. Example: **Tier 1 Location Service** (§6.1.1, §6.1.2) |
| **IP Data Bearer Service** | IPv4 datagrams (routable) | IPv4 + (SCEP or SNDCP) | General packet data. **Preferred** over CAI Data per §4. Example: **Tier 2 Location Service**, **Data Link Independent OTAR** |

**Transport protocols above IP (UDP, TCP) are permitted** on the IP Data
Bearer Service but their use is not further specified by BAEA-C;
payload semantics above IPv4 are caller-defined.

**No IPv6.** BAEA-C §4.2 + §6.2.3.6 confirm IP = IPv4 only; an "IP Data
Security Service" (v6 + IPsec-style) is catalogued as future work.

---

## 4. Packet Data Configurations

BAEA-C §5 defines four configurations (i.e., physical topologies) for
packet data:

```
┌────────────────────────────────────────────────────────────────┐
│ Direct Data — SU1 ←[Direct Um]→ SU2                            │
│   • Peer-to-peer; no infrastructure.                           │
│   • Both ends terminate the full stack.                        │
│   • Symmetric addressing.                                      │
├────────────────────────────────────────────────────────────────┤
│ Repeated Data — SU1 ←[Repeated Um]→ FSR ←[Repeated Um]→ SU2    │
│   • Fixed Station Repeater (FSR) relays PHY/MAC only.          │
│   • FSR has NO LLC or IP awareness; it regenerates bits.       │
│   • Stack terminates at the two SUs; symmetric addressing.     │
├────────────────────────────────────────────────────────────────┤
│ Conventional FNE Data — SU ←[Conventional Um]→ FNE ←[Ed]→ DH   │
│   • Conventional (non-trunked) channel.                        │
│   • FNE terminates the radio stack, bridges to Data Host       │
│     Network via Ed.                                            │
│   • Introduces the Conventional Management Service (CMS)       │
│     for channel management (registration, location, scan).     │
│   • Asymmetric addressing (SU addr ≠ FNE addr).                │
├────────────────────────────────────────────────────────────────┤
│ Trunked FNE Data — SU ←[Trunked Um]→ FNE ←[Ed]→ DH             │
│   • Trunked channel pair: control CH + traffic CH.             │
│   • Control channel runs TMS (Trunked Management Services) on  │
│     top of PHY/MAC — NOT on LLC.                               │
│   • Traffic channel runs the standard IPv4 / SNDCP / LLC /     │
│     PHY/MAC stack; SCEP is NOT an option here.                 │
│   • Asymmetric addressing.                                     │
└────────────────────────────────────────────────────────────────┘
```

**Combinations are permissible:** one FNE may simultaneously support
Repeated Data, Conventional FNE Data, and Trunked FNE Data on different
frequencies / modes.

**Endpoint scenario matrix.** Conventional-FNE and Trunked-FNE
configurations both have 12 addressable scenarios (per BAEA-C Table 10),
spanning {SU, MDP, DH} at either end via one or two intermediary hops
(SU↔SU, MDP↔SU, SU↔MDP, MDP↔MDP, SU↔DH, MDP↔DH in both directions).
The SU-to-SU-via-FNE and SU-to-SU-via-DHN cases are the ones most
commonly exercised in real dispatch deployments.

---

## 5. Bearer × Configuration Matrix and Stacks

BAEA-C §6 Table 11 defines which of the 8 bearer × configuration
combinations are actually supported:

| Configuration | CAI Data Bearer | IP Data Bearer |
|---------------|-----------------|----------------|
| Direct Data | **§6.1.1** — supported | **§6.2.1** — supported |
| Repeated Data | **§6.1.2** — supported | **§6.2.2** — supported |
| Conventional FNE Data | **§6.1.3** — supported (CMS layer) | **§6.2.3** — supported (CMS + SCEP or SNDCP) |
| Trunked FNE Data | **Not Supported** | **§6.2.4** — supported (SNDCP only) |

**The seven supported stacks:**

### 5.1 CAI Data Bearer (three configs)

```
CAI + Direct / Repeated:            CAI + Conventional FNE:
┌──────────────┐                    ┌──────────────┐
│  Payload     │                    │  Payload     │
├──────────────┤                    ├──────────────┤
│  LLC         │                    │  CMS         │
├──────────────┤                    ├──────────────┤
│  FDMA PHY/MAC│                    │  LLC         │
└──────────────┘                    ├──────────────┤
                                    │  FDMA PHY/MAC│
                                    └──────────────┘
```

Note the **CMS sits above LLC on Conventional FNE but below the
payload**. For an IP-over-CAI-on-Conventional-FNE stack (§6.2.3) the
CMS sits between LLC and SCEP/SNDCP. See the next subsection.

### 5.2 IP Data Bearer (four configs)

```
IP + Direct / Repeated:              IP + Conventional FNE:
┌──────────────┐                    ┌──────────────┐
│  Payload     │                    │  Payload     │
├──────────────┤                    ├──────────────┤
│  IPv4        │                    │  IPv4        │
├──────────────┤                    ├──────────────┤
│  SCEP        │                    │  SCEP or SNDCP│   (implementer chooses)
├──────────────┤                    ├──────────────┤
│  LLC         │                    │  CMS         │
├──────────────┤                    ├──────────────┤
│  FDMA PHY/MAC│                    │  LLC         │
└──────────────┘                    ├──────────────┤
                                    │  FDMA PHY/MAC│
                                    └──────────────┘

IP + Trunked FNE (control channel):        (traffic channel):
┌──────────────┐                           ┌──────────────┐
│  TMS         │                           │  Payload     │
├──────────────┤                           ├──────────────┤
│  FDMA PHY/MAC│                           │  IPv4        │
└──────────────┘                           ├──────────────┤
                                           │  SNDCP       │    (SCEP NOT allowed)
                                           ├──────────────┤
                                           │  LLC         │
                                           ├──────────────┤
                                           │  FDMA PHY/MAC│
                                           └──────────────┘
```

**Invariants to lock in:**
1. **LLC is always present** above PHY/MAC on every packet-data stack
   (BAED-A). Exception: FSRs in the Repeated configuration, which
   regenerate at PHY/MAC only.
2. **TMS runs on PHY/MAC directly, without LLC** — it's a
   control-channel management protocol, not a data protocol. TMS
   messages are carried as TSBKs.
3. **SCEP and SNDCP are alternatives**, not siblings: exactly one is
   present on any given IP-data stack, chosen per the decision tree
   in §6 below.
4. **CMS is conventional-only** — it sits between LLC and SCEP/SNDCP
   on Conventional-FNE, and doesn't appear at all on
   Direct/Repeated/Trunked-FNE.
5. **No IP stack on FSRs.** The FSR regenerates the PHY/MAC bits
   verbatim; it cannot inspect or route at the IP layer.

---

## 6. SCEP vs SNDCP vs TMS — Decision Tree

This is the single most common implementer question in the data
suite. BAEA-C §6 gives the answer implicitly; the table below makes it
explicit.

| Configuration | Channel type | Above LLC | Above SCEP/SNDCP | Notes |
|---------------|--------------|-----------|------------------|-------|
| Direct Data | — | **SCEP** | IPv4 | BAEB-B §4 defines SCEP: static binding + ARP-style dynamic binding + IPv4 conveyance |
| Repeated Data | — | **SCEP** | IPv4 | Same SCEP as Direct; FSR is transparent |
| Conventional FNE Data | Conventional voice/data | **SCEP or SNDCP** (impl choice) | IPv4 | SCEP: simpler, static FNE/SU binding. SNDCP: richer — context mgmt, confirmed/unconfirmed datagrams, header compression, data user auth, host network selection, scan mode. |
| Trunked FNE Data | Control channel | **TMS** (on PHY/MAC; no LLC, no SCEP/SNDCP) | — | TMS = TSBK-borne control messages (BAEB-B/BAEB-C + AABC-E). |
| Trunked FNE Data | Traffic channel | **SNDCP** (SCEP not allowed) | IPv4 | BAEB-C §6: SNDCP is the only option; context activation happens on control channel via TMS. |

**Implementation consequence for blip25-style passive decoders.**
When you see IPv4 riding over the Um air interface:

1. Is this a trunked-system traffic channel? → SNDCP-only. BAEB-C
   §6.4 Figure 26 for the SNDCP header layout.
2. Is this a conventional-system channel carrying IP? → SCEP or SNDCP.
   Check for the BAEB-B §4 SCEP ARP message pattern first; if it's
   absent and you see the BAEB-C 2-byte SNDCP header shape at
   `ip_offset - 2`, it's SNDCP. See
   `analysis/motorola_sndcp_npdu_preamble.md` for the Motorola
   trunked case byte-by-byte and
   `analysis/motorola_conventional_scep_vs_trunked_sndcp.md` for the
   architectural split.
3. Is this a peer-to-peer or repeated-data capture? → SCEP-only.

---

## 7. Symmetric vs Asymmetric Addressing

BAEA-C §6 annotates every bearer × configuration combination with
"Symmetric Address" or "Asymmetric Address":

| Configuration | Addressing | Implication |
|---------------|------------|-------------|
| Direct Data | **Symmetric** | Both endpoints use the same addressing format (SU LLID ↔ SU LLID). |
| Repeated Data | **Symmetric** | Same — FSR is address-transparent. |
| Conventional FNE Data | **Asymmetric** | SU addressing ≠ FNE addressing. FNE has a distinct LLID; SU-to-FNE LLC frames set source = SU, dest = FNE reserved address. |
| Trunked FNE Data | **Asymmetric** | Same asymmetry. FNE's identity is provisioned; SUs learn it via TMS registration. |

**Implementation consequence.** A decoder that hard-assumes symmetric
addressing will mis-attribute FNE-originated frames on conventional or
trunked captures. The BAEB-B / BAED-A LLC layer carries SAP + LLID
fields that resolve the asymmetry — honor them.

---

## 8. Confirmed vs Unconfirmed Delivery — Mandatory Table

BAEA-C §6 distinguishes mandatory (SHALL) from optional (MAY) support
per combination. This matters for conformance:

| Bearer × Config | Confirmed Delivery | Unconfirmed Delivery |
|-----------------|--------------------|-----------------------|
| CAI × Direct | **SHALL** (mandatory) | MAY (optional) |
| CAI × Repeated | **SHALL** | MAY |
| CAI × Conventional FNE | **SHALL** | MAY |
| IP × Direct | **SHALL** | MAY |
| IP × Repeated | **SHALL** | MAY |
| IP × Conventional FNE | **SHALL** | MAY |
| IP × Trunked FNE | (defined by BAEB-B/C via SNDCP) | (same) |

**Implementation consequence.** An implementation claiming BAEA-C
conformance on any supported combination must provide the Confirmed
Data Packet Delivery path (BAAA-B §5.3 Confirmed Data PDU + per-block
CRC-9 + Packet CRC-32 + Response/ACK loop). Unconfirmed is optional
but commonly present.

---

## 9. Cross-Reference Map

BAEA-C's practical value is as a router. The table below maps
overview-level questions to the normative doc that actually answers
them.

| Question | Normative doc | Impl spec in this repo |
|----------|---------------|------------------------|
| How are data PDUs framed on the FDMA air interface? | BAAA-B §5.3–§5.4 | `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md` |
| What does the LLC layer look like (source/dest LLID, SAP, sliding-window sequence)? | BAED-A | `standards/TIA-102.BAED-A/P25_Packet_Data_LLC_Implementation_Spec.md` |
| How does SCEP work (static/dynamic IP binding, ARP)? | BAEB-B §4 | `standards/TIA-102.BAEB-B/P25_IP_Data_Bearer_BAEB_B_Implementation_Spec.md` |
| How does SNDCP work (context management, V1/V2/V3, header compression)? | BAEB-B §5–§7 (SNDCP V1/V2); BAEB-C (SNDCP V3) | `standards/TIA-102.BAEB-B/P25_IP_Data_Bearer_BAEB_B_Implementation_Spec.md` + `standards/TIA-102.BAEB-C/P25_IP_Data_Bearer_Implementation_Spec.md` |
| What UDP/TCP ports are assigned? | BAJD-A | `standards/TIA-102.BAJD-A/P25_Port_Number_Assignments_Implementation_Spec.md` |
| What messages does CMS exchange on a conventional channel? | BAEJ-A | `standards/TIA-102.BAEJ-A/…` (pending Phase 3) |
| What does the Ed interface to the Data Host Network look like? | BAEF | `standards/TIA-102.BAEF/…` (pending Phase 3) |
| What does the A interface to a Mobile Data Peripheral look like? | BAEG-A | `standards/TIA-102.BAEG-A/…` (pending Phase 3) |
| How do radio management messages on the A interface work? | BAEE-C | `standards/TIA-102.BAEE-C/…` (pending Phase 3) |
| What messages does TMS exchange on the trunking control channel for SNDCP context management? | Control-channel side: AABC-E; SNDCP side: BAEB-B §6 / BAEB-C §6 | `standards/TIA-102.AABC-E/P25_Trunking_Control_Channel_Messages_Implementation_Spec.md` + BAEB-B/C impl specs |
| How is encryption applied on packet-data Um? | AAAD-B | `standards/TIA-102.AAAD-B/P25_Block_Encryption_Protocol_Implementation_Spec.md` |
| How are supplementary services (Emergency Alarm, Status Update, Short Message) carried? | AABC-E / AABD-B on trunked; BAAD-B on conventional | `standards/TIA-102.AABC-E/…` + `standards/TIA-102.AABD-B/…` |

**For a first-time data-services implementer:** read BAEA-C first for
the topology, then pair BAAA-B + BAED-A for the bottom of every stack,
then BAEB-B/C for SCEP and SNDCP. Add BAJD-A for the port catalog.
BAEF / BAEG / BAEE / BAEJ are only needed if you're implementing the
host-side (RFSS ↔ Data Host) or terminal-side (SU ↔ MDP) surface.

---

## 10. Out-of-Scope and Future-Work Catalogue

BAEA-C explicitly flags these as **not yet standardized** or **out of
scope** — a decoder or implementer that claims to support them is
doing so outside the standard:

| Item | BAEA-C reference | Status |
|------|------------------|--------|
| Low-Speed Data payload semantics | §2.1 | Not standardized by any TIA-102 doc |
| Circuit Data bearer service | Removed in BAEA-B (2012) | Withdrawn, not to be implemented |
| IP Data Security Service (IPsec-equivalent) | §6.2.3.6 | Reserved as BAEH for future publication |
| Packet Data Test Suite | §7 | Placeholder — "subject of future work" |
| Multipoint A Interface | §3.3 | Not defined |
| Packet Data on Um2 (TDMA) | §3.7 | Future work |
| Packet Data on Ef (Fixed Station) | §3.5 | Future work |
| Packet Data on Ec (Console) | §3.6 | Future work |
| BAEC designator | — | Withdrawn |
| BAEI designator | — | Not used |

**Implementation consequence.** If an operational system claims Packet
Data on TDMA or on a Fixed Station interface, that implementation is
vendor-defined or using a later revision of BAEA that post-dates
BAEA-C.

---

## 11. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify:

- Three-tier data services taxonomy — TIA-102.BAEA-C §2.
- Packet Data document suite table (BAEA–BAEJ) — §2.3, Table 2.
- Interface relevance matrix — §3, Table 3.
- Um, Ed, A interface protocol combinations — §3.1–§3.3, Tables 4–5.
- Bearer services (CAI vs IP) — §4, Table 6.
- The four configurations — §5, Tables 7–10.
- Bearer × configuration support matrix — §6, Table 11.
- CAI bearer stacks — §6.1 (Figures 5–7), Table 12 (CMS functions).
- IP bearer stacks — §6.2 (Figures 8–12), Tables 13–17 (SCEP/SNDCP/TMS functions).
- CMS function list — Table 12.
- TMS function list — Table 17.
- IP Data Security future work — §6.2.3.6.
- Out-of-scope / future work — §3.5–§3.7, §7, and the Table 2 designator notes.

---

## 12. Cross-References

**Upstream (this doc is the root):**
- `standards/TIA-102.BAEA-C/TIA-102-BAEA-C_Full_Text.md` — full
  clean-room extraction.

**Downstream spec impl specs (already processed):**
- `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md`
  — PHY/MAC for every stack in §5.
- `standards/TIA-102.BAED-A/P25_Packet_Data_LLC_Implementation_Spec.md`
  — LLC layer shared by all stacks except TMS-on-control-channel.
- `standards/TIA-102.BAEB-B/P25_IP_Data_Bearer_BAEB_B_Implementation_Spec.md`
  — SCEP (all configs except Trunked-FNE) and SNDCP V1/V2/V3 state
  machines. Authoritative for SCEP.
- `standards/TIA-102.BAEB-C/P25_IP_Data_Bearer_Implementation_Spec.md`
  — SNDCP-V3 consolidated (SN-Data / SN-UData / context mgmt).
- `standards/TIA-102.BAJD-A/P25_Port_Number_Assignments_Implementation_Spec.md`
  — well-known UDP/TCP ports on the IP Data Bearer.
- `standards/TIA-102.AAAD-B/P25_Block_Encryption_Protocol_Implementation_Spec.md`
  — encryption when the Protected bit is set on a Data PDU.
- `standards/TIA-102.AABC-E/P25_Trunking_Control_Channel_Messages_Implementation_Spec.md`
  — control-channel messages that carry TMS (SNDCP packet data channel
  management).

**Downstream spec impl specs (pending — next in this processing pass):**
- `standards/TIA-102.BAEE-C/…` — Radio Management Controls (A-interface SU mgmt).
- `standards/TIA-102.BAEF/…` — Ed interface (Data Host Network).
- `standards/TIA-102.BAEG-A/…` — A interface (Mobile Data Peripheral).
- `standards/TIA-102.BAEJ-A/…` — Conventional Management Service.

**Analysis notes that build on this overview:**
- `analysis/fdma_pdu_frame.md` — unified FDMA PDU view spanning
  BAAA-B / AABB-B / BAED-A / BAEB-C.
- `analysis/motorola_sndcp_npdu_preamble.md` — trunked IV&D byte
  layering (LLC + SNDCP + IPv4) observed on real Motorola captures.
- `analysis/motorola_conventional_scep_vs_trunked_sndcp.md` —
  architectural split between conventional (SCEP) and trunked (SNDCP)
  wrappers.
