# TIA-102.BACF — Related Resources and Context

**Document:** TIA-102.BACF — Project 25 Inter-RF Subsystem Interface (ISSI)
Messages and Procedures for Packet Data Services
**Generated:** 2026-04-22

---

## Status

**Active (no superseding revision published as of September 2024).** The
document was published in October 2009. The P25 Steering Committee re-affirmed
it in January 2013 (the "ANSI Published" column in the PTIG approved-standards
table shows Jul-2012; the P25 SC approval column shows Jan-2013). No revision
letter (e.g., "-A" or "-B") has been issued. The September 2024 PTIG approved
standards list (P25 SC 24-09-002) shows TIA-102.BACF with only its original
Oct-2009 date — no supersession note and no absorbed-by-revision note, unlike
BACA-B addendums which are explicitly marked "Absorbed by Rev C." There is no
companion addendum and no draft revision listed in the TR-8.19 queue for BACF
as of that list.

Contrast with sibling BACE (Conventional ISSI, Jun-2008), which is similarly
listed with a single date and no supersession note.

---

## Scope (from §1.1)

TIA-102.BACF extends the ISSI messages and procedures defined in TIA-102.BACA
to support P25 Packet Data Service over the ISSI among RFSSs within a given
P25 system and across P25 systems.

TIA-102.BAEB defines three Packet Data Services configurations:

1. Radio to Fixed Network Equipment (FNE) Packet Data Bearer Service (FNE Data)
2. Radio to Radio Data Bearer Service (Repeated Data)
3. Radio to Radio Packet Data Bearer Service (Direct Data)

TIA-102.BACF covers **only FNE Data**, because this is the only configuration
that may extend across multiple RFSSs and therefore benefit from ISSI
packet data procedures. The document specifies:

- Mobility management of packet data capable SUs
- IP addressing for roaming SUs and associated Mobile Data Peripherals (MDPs)
- Routing of IP datagrams to and from roaming SUs and MDPs

The core mechanism is Mobile IPv4 (MIPv4, RFC 3344) with reverse tunneling (RFC
3024) and IP-in-IP encapsulation (RFC 2003). Each RFSS hosts a Packet Data ISSI
Gateway (PDIG) functional entity. The Home PDIG acts as Home Agent and provides
the SNDCP function for the SU's home RFSS. When an SU roams to a Serving RFSS,
the Serving PDIG acts as Foreign Agent. A Packet Data ISSI Link — a pair of
forward and reverse IP tunnels — connects the Home PDIG and Serving PDIG for
each active SNDCP context.

---

## Standards Family

TIA-102.BACF belongs to the BAC sub-series of the TIA-102 B-series
(Infrastructure Interfaces), which collectively define the ISSI.

### ISSI Standards Tree

```
TIA-102 (Project 25 Standards Suite)
│
└── B-Series: Infrastructure Interfaces
    │
    └── BAC sub-series: Inter-RF Subsystem Interface (ISSI)
        │
        ├── TSB-102.BACC-B  (Nov-2011)
        │   ISSI Overview — system-level description of the ISSI architecture.
        │   A Technical Service Bulletin (informative), not a normative spec.
        │
        ├── TIA-102.BACA-B  (Nov-2012, with addendums B-1 Jul-2013, B-2 Nov-2016,
        │   B-3 Jan-2021; Rev C draft in TR-8.19 review to merge addendums)
        │   ISSI Messages and Procedures for Voice Services, Mobility Management,
        │   and RFSS Capability Polling Services — the primary ISSI spec.
        │   TIA-102.BACF is normatively dependent on BACA.
        │
        ├── TIA-102.BACD-B  (Jul-2011, with addendums B-1 May-2017, B-2 Mar-2018,
        │   B-3 Feb-2021/Oct-2022)
        │   ISSI Messages and Procedures for Supplementary Data.
        │   Addendum B-2 adds Individual Regrouping; B-3 adds IWF interworking.
        │
        ├── TIA-102.BACE  (Jun-2008, re-affirmed Jan-2013)
        │   ISSI Messages and Procedures for Conventional Operations.
        │   Covers conventional (non-trunked) channels over the ISSI.
        │
        ├── TIA-102.BACF  (Oct-2009, re-affirmed Jan-2013)  ◄── THIS DOCUMENT
        │   ISSI Messages and Procedures for Packet Data Services.
        │   MIPv4-based data mobility across RFSS boundaries.
        │
        └── TIA-102.BACG  (no publication date as of Sep-2024)
            ISSI Messages and Procedures for Group Regrouping.
            Listed in the PTIG standards table as "Draft in progress TR8.19"
            as of the September 2024 list.
```

### Position in the P25 Architecture

```
  SU (radio with data capability)
       |  Um (CAI — TIA-102.BAAA-B, BAEB-C)
       v
  Serving RFSS
       |  SNDCP context + MIPv4 Foreign Agent (Serving PDIG)
       |  Packet Data ISSI Link (IP tunnel — RFC 2003 / RFC 3024)
       v
  Home RFSS
       |  SNDCP Home Agent (Home PDIG)
       v
  Customer/FNE Network
       |  Ed interface (TIA-102.BAEB-C)
       v
  Data End System (server / host)
```

The ISSI Packet Data Link sits between the Serving PDIG and the Home PDIG.
TIA-102.BACF defines the messages and procedures for establishing, transferring,
and tearing down that link (SNDCP Context Activation, Deactivation, Query,
Transfer, and MIPv4 Registration Renewal).

### Companion / Dependent Documents

| Document            | Role                                                                 |
|---------------------|----------------------------------------------------------------------|
| TIA-102.BACA-B      | Primary ISSI voice spec; BACF normatively extends [102BACA].         |
| TIA-102.BAEB-C      | P25 IP Data Bearer Service Specification; defines SNDCP, FNE Data bearer service that BACF extends across the ISSI. Referenced as [102BAEB] (was a draft at BACF publication time, labeled SP-3-4632-RV2; subsequently published as BAEB-B then BAEB-C). |
| TIA-102.BAAA-B      | FDMA Common Air Interface; defines the Um interface over which SUs connect to the Serving RFSS. |
| RFC 3344            | IP Mobility Support for IPv4 (MIPv4); the primary mobility protocol used by BACF for SU address management and datagram routing. |
| RFC 3024            | Reverse Tunneling for Mobile IP; used for the reverse path (SU to Home Agent through Serving PDIG) of the Packet Data ISSI Link. Direct-delivery style (§3024) is mandated. |
| RFC 2003            | IP Encapsulation within IP; the encapsulation method for the forward and reverse tunnels of the Packet Data ISSI Link. |
| RFC 2104            | HMAC: Keyed-Hashing for Message Authentication; used for MIPv4 mobility security associations (HMAC-MD5, 128-bit). |
| RFC 1305            | Network Time Protocol v3; used for MIPv4 replay-protection timestamps per §5.7.1 of RFC 3344. |
| TIA-102.AABC-C      | Trunking Control Channel Messages (informative reference only in BACF). |

**Note on [102BAEB] reference status at publication time:** When TIA-102.BACF
was published in October 2009, TIA-102.BAEB had not yet been formally published.
The BACF normative reference section carries an Editor's Note stating the
reference to [102BAEB] is "for informational purposes only" until that document
is approved and published. TIA-102.BAEB-C (the current revision) is the document
that resolved this dependency.

---

## Open-Source Implementations

No known open-source project implements TIA-102.BACF (ISSI Packet Data). The
interface sits between RFSS infrastructure components (Home PDIG and Serving
PDIG) — not between radios and infrastructure — so it is outside the scope of
SDR receiver software such as OP25 or SDRTrunk.

The underlying protocols (MIPv4, IP-in-IP) have mature open-source
implementations (e.g., OpenMobileIP, MIPL Linux Mobile IPv4), but none are
integrated into a P25 ISSI packet data stack.

Related open-source projects that operate at adjacent layers:

- **OP25** (https://github.com/osmocom/op25) — P25 Phase 1 CAI decoder.
  Handles the SNDCP PDU structure at the air interface (BAEB layer), not ISSI.
- **SDRTrunk** (https://github.com/DSheirer/sdrtrunk) — P25 Phase 1 and 2
  decoder; handles ISSI radio identifiers and roaming call correlation in the
  receive path, but does not implement the ISSI Packet Data Link.

---

## Vendor Equipment and Infrastructure Products

No vendor has publicly listed conformance to TIA-102.BACF specifically. ISSI
Packet Data (FNE Data across RFSS boundaries via MIPv4) has not been included
in the DHS/FEMA P25 Compliance Assessment Program (CAP) test suites. The P25
CAP ISSI/CSSI program, which expanded in November 2019, tests voice ISSI
(BACA-based services) and CSSI, not packet data ISSI.

Commercial ISSI products from the following vendors implement the voice ISSI
(TIA-102.BACA) and have been demonstrated for interoperability, but none have
published claims of TIA-102.BACF Packet Data ISSI support:

- **Motorola Solutions** — ISSI.1 Network Gateway Subsystem (ISSI.1 NGW);
  implements group call, emergency call, and end-to-end encryption over ISSI
  per TIA-102.BACA. Datasheet (circa 2011) lists call services "specified in
  TIA-102.BACA" only. No BACF packet data services listed.
- **L3Harris** — ISSI Gateway (software application on standard hardware for
  VIDA networks); supports voice trunked ISSI and supplementary data. Product
  literature references voice roaming and group call but does not mention BACF
  packet data.
- **Tait Communications**, **Cassidian (Airbus DS)**, **EF Johnson** — have
  demonstrated ISSI voice interoperability at IWCE and APCO trade shows since
  2009; no published BACF claims.

The P25 Best Practice guide (p25bestpractice.com) lists "P25 packet data on the
Data Network Interface (including OTAR)" as a capability supported over the
ISSI, but this appears to describe the data capability at the RFSS level (BAEB),
not specifically BACF inter-RFSS packet data mobility.

---

## Key Online Resources

- **TIA-102.BACF listing (Engineering360/GlobalSpec):**
  https://standards.globalspec.com/std/1198745/TIA-102.BACF

- **TIA-102.BACF purchase (Accuris/IHS):**
  https://store.accuristech.com/tia (search TIA-102.BACF)

- **PTIG Approved P25 TIA Standards (Sep 2024) — confirms active status:**
  https://project25.org/images/stories/ptig/P25_Standards_Documents/P25%20SC%2024-09-002%20P25%20TIA%20Standards_Approved%20Sep2024.pdf

- **TSB-102.BACC-B (ISSI Overview) listing:**
  https://standards.globalspec.com/std/1405399/TIA%20TSB-102.BACC

- **TIA-102.BACA listing (primary ISSI voice spec that BACF extends):**
  https://standards.globalspec.com/std/1558641/tia-102-baca

- **TIA-102.BACD listing (Supplementary Data ISSI):**
  https://standards.globalspec.com/std/1384529/tia-tia-102.bacd-b

- **CISA ISSI/CSSI Primer (2019):**
  https://www.cisa.gov/sites/default/files/publications/20190130%20ISSI-CSSI%20Primer%20FINAL%20508C.pdf

- **DHS P25 CAP ISSI/CSSI expansion (Nov 2019) — covers voice ISSI only:**
  https://www.dhs.gov/archive/science-and-technology/news/2019/11/22/news-release-p25-cap-expands-include-issicssi-equipment

- **P25 Best Practice — ISSI interoperability guidance:**
  https://www.p25bestpractice.com/specifying/interoperability_with_issi/

- **PTIG Non-CAP ISSI/CSSI Interoperability Testing:**
  https://project25.org/index.php/compliance-assessment/p25-non-cap-issi-cssi-interoperability-testing

- **L3Harris ISSI Gateway product page:**
  https://www.l3harris.com/all-capabilities/inter-rf-subsystem-interface-issi-gateway

- **Motorola Solutions ISSI.1 NGW datasheet:**
  https://www.motorolasolutions.com/content/dam/msi/docs/business/solutions/technologies/project_25_standards/_documents/_static_files/p25_issi.1_data_sht_.pdf

- **RFC 3344 (MIPv4):** https://datatracker.ietf.org/doc/html/rfc3344
- **RFC 3024 (Reverse Tunneling):** https://datatracker.ietf.org/doc/html/rfc3024
- **RFC 2003 (IP-in-IP):** https://datatracker.ietf.org/doc/html/rfc2003
- **RFC 2104 (HMAC):** https://datatracker.ietf.org/doc/html/rfc2104
- **RFC 1305 (NTP v3):** https://datatracker.ietf.org/doc/html/rfc1305

---

## Phase 3 Flag — Implementation Spec Needed

Based on Phase 1 classification (PROTOCOL + MESSAGE_FORMAT), this document
warrants a Phase 3 implementation spec covering:

1. **PDIG functional entity architecture** — Home PDIG vs. Serving PDIG roles,
   Home Agent / Foreign Agent mapping, SNDCP context state machine.

2. **MIPv4 qualification table** — which fields of RFC 3344 Registration
   Request/Reply are used as-is vs. constrained by BACF (direct delivery style,
   HMAC-MD5 security extension, NTP timestamp replay protection, specific
   extension restrictions).

3. **SNDCP Context procedures** — all nine procedure flows with message
   sequences: SU-Initiated Activation, Home-RFSS-Initiated Activation,
   SU-Initiated Deactivation, Serving-RFSS-Initiated Deactivation,
   Home-RFSS-Initiated Deactivation, Serving-RFSS-Initiated Context Query,
   Home-RFSS-Initiated Context Query, SNDCP Context Transfer, and MIPv4
   Registration Renewal (including during a Context Transfer).

4. **Packet Data ISSI Link message formats** — PDU structures for all BACF
   ISSI messages (distinct from the voice ISSI SIP-based messages of BACA).

These were not produced in this run. Flag for follow-up.
