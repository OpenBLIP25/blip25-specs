# TSB-102.BADA-A — Related Resources & Context

**Document:** TSB-102.BADA-A  
**Title:** Project 25 — Telephone Interconnect Overview (Voice Service)  
**Published:** June 2012  

---

## Status

This document is a **TIA Telecommunications Systems Bulletin (TSB)** — an informative publication, not a normative standard. It is the second revision of a document that originated as Interim Standard IS 102.BADA in May 1996, was upgraded to a full standard as TIA/EIA-102.BADA in March 2000, and was revised to TSB-102.BADA-A in June 2012.

As of the date of this extraction (2026), **TSB-102.BADA-A appears to remain the current version** of this bulletin. No successor document or formal withdrawal notice has been identified. Because it is a systems bulletin rather than a normative standard, it does not carry a formal "active/superseded" lifecycle in the same way as ANSI/TIA standards. The underlying telephone interconnect normative behavior is governed by the signaling message standards it references (AABC, AABG, AABF) rather than by this bulletin itself.

The original IS 102.BADA document predates the broader P25 Phase 1 freeze and reflects the telephony interconnect features that were a core part of early P25 design requirements from APCO, NASTD, and FED.

---

## Standards Family

This document sits within the **TIA-102 Project 25 suite**, under the **BADA sub-series** which covers telephone interconnect.

### Companion documents directly referenced:

| Document | Title | Role |
|----------|-------|------|
| TSB102-A | APCO Project 25 System and Standards Definition | Top-level system architecture |
| TIA-102.AABC-C | Project 25 Trunking Control Channel Messages | Normative trunked signaling (SU↔RFSS for interconnect) |
| TIA-102.AABF-C | Project 25 Link Control Work Formats and Messages | Call tear-down messaging |
| TIA-102.AABG | Project 25 Conventional Control Messages | Normative conventional signaling |
| TIA-102.BAAA-A | Project 25 FDMA Common Air Interface | Physical/MAC layer |
| TIA-102.BABA | Project 25 Vocoder Description | IMBE vocoder (impacts tone relay) |
| TIA-102.BABA-1 | Project 25 Half-Rate Vocoder Annex | AMBE+2 half-rate vocoder |
| TIA/EIA-464-C | PBX Switch Equipment Requirements | Analog Et interface access methods |
| ITU I.430 | ISDN BRI Layer 1 | Digital Et: BRI access |
| ITU I.431 | ISDN PRI Layer 1 | Digital Et: PRI access |
| IETF RFC 3261 | SIP | VoIP Et: call control |
| IETF RFC 4566 | SDP | VoIP Et: session description |
| IETF RFC 3550 | RTP | VoIP Et: voice transport |

### Standards lineage (ASCII tree):

```
TIA-102 (Project 25 Suite)
├── A-series: Common Air Interface & Control
│   ├── TIA-102.AABC — Trunking Control Channel Messages  ← normative Et signaling
│   ├── TIA-102.AABF — Link Control Words/Messages        ← call tear-down
│   └── TIA-102.AABG — Conventional Control Messages      ← normative Et signaling
├── B-series: RF Subsystem
│   ├── BA-sub: Air Interface
│   │   └── TIA-102.BAAA — FDMA Common Air Interface
│   ├── BB-sub: (MAC, physical layer standards)
│   └── BD-sub: Telephone Interconnect
│       ├── TSB-102.BADA-A  ← THIS DOCUMENT (informative overview)
│       └── (no separate normative BADA procedure standard identified;
│            normative behavior is embedded in AABC/AABG/AABF)
└── C-series: Conformance & Testing
```

The BADA sub-series is the dedicated telephone interconnect node within TIA-102. This document is the primary (and apparently only) BADA-series publication — it serves as the architectural overview that frames how the normative signaling documents (AABC, AABG) apply to the telephone interconnect use case.

---

## Practical Context

**Telephone Interconnect** (also called "phone patch" or "autopatch" in LMR operator vernacular) allows P25 radio subscribers to place and receive telephone calls through their radio systems without accessing a separate telephone handset. It is widely deployed in:

- **Public safety dispatch centers**: Allows dispatchers and field units to bridge radio and telephone communications.
- **Utility and infrastructure operations**: Field technicians can be reached by telephone through the radio system.
- **Small agency "night answer"**: A single conventional channel RFSS with Et interface allows calls to ring through to on-duty mobile units after hours.

The Et interface connects the RFSS to the PSTN or a PABX. In modern P25 deployments this is most commonly implemented as:
- **Analog loop-start or E&M trunks** for legacy PSTN connections.
- **ISDN PRI (T1 or E1)** for larger installations requiring multiple simultaneous interconnect calls.
- **SIP trunking** for IP-based PSTN gateways, which is increasingly common in contemporary deployments as traditional TDM telephony infrastructure is retired.

The document's coverage of **Group Incoming Calls** is especially relevant to public safety: a single telephone number can ring all members of a talkgroup, supporting scenarios like emergency call-in lines or after-hours answering for dispatch groups.

The **availability check** procedure described in Clause 3.7.3 is significant for field deployability: it allows the RFSS to determine whether a target radio is within coverage and able to receive a call before seizing a telephone circuit and connecting voice. This reduces wasted airtime and prevents calls from connecting to out-of-range radios.

The mention of **vocoder distortion of call progress tones** (Note in Clause 3.7.2, referencing BABA) is a known practical issue: IMBE and AMBE+2 codecs are not transparent to DTMF and call-progress tones. Systems relying on in-band tone relay must either use in-band DTMF detection/regeneration or route tones via the half-rate vocoder's signaling channel.

---

## Key Online Resources

- **TIA standards catalog** (purchase/access point for TIA-102 documents):  
  https://www.tiaonline.org/standards/catalog/

- **APCO International P25 resources** (technology portal for P25 standards and procurement):  
  https://www.apcointl.org/technology/p25/

- **DHS SAFECOM P25 CAP** (Compliance Assessment Program — references P25 feature sets including interconnect):  
  https://www.cisa.gov/safecom/p25

- **P25 Technology Interest Group (PTIG)** (industry group with P25 technical documentation):  
  https://ptig.org/

- **RadioReference P25 wiki** (community documentation of P25 system features including phone patches):  
  https://wiki.radioreference.com/index.php/P25

- **IETF RFC 3261 (SIP)** — referenced as a VoIP Et interface option:  
  https://www.rfc-editor.org/rfc/rfc3261

---

## Open-Source Implementations

No open-source project specifically implements the TSB-102.BADA-A Et interface behavior in isolation (this document is informative/overview; the normative Et signaling is in AABC/AABG). However, the following projects implement the P25 signaling that underlies telephone interconnect:

- **SDRTrunk** (Java, P25 decoder/monitor):  
  https://github.com/DSheirer/sdrtrunk  
  Decodes P25 trunking control channel messages including telephone interconnect signaling defined in AABC. Does not implement RFSS-side Et gateway behavior.

- **OP25** (GNU Radio-based P25 software):  
  https://github.com/boatbod/op25  
  Implements P25 CAI decoder. Supports conventional and trunked P25. Telephone interconnect is not a focus of OP25 but the underlying signaling is decoded.

- **Asterisk** / **FreePBX** with P25 integration:  
  Some public safety organizations have integrated Asterisk-based VoIP systems at the Et interface using SIP trunking. No standardized open-source P25-to-Asterisk gateway project is known as of this writing; implementations are typically proprietary or agency-specific.

- **MMDVM** and related projects implement digital voice bridging (including P25) but focus on amateur radio repeater use cases rather than PSTN interconnect.

**Note on implementation scope:** Because TSB-102.BADA-A is an overview bulletin, implementing "telephone interconnect" in a P25 RFSS requires implementing the AABC/AABG message formats (SU↔RFSS signaling) and then providing an Et gateway using SIP, ISDN, or analog trunks. There is no single open-source project that packages all of these components in a P25-compliant way.

---

## Standards Lineage (Narrative)

The concept of telephone interconnect in LMR systems predates P25 — analog "autopatch" via E&M trunk connections was common in pre-digital trunked systems (Motorola Type I/II, LTR, EDACS). The P25 standardization effort, initiated by the 1992 APCO/NASTD/FED MOU, included telephone interconnect as a required feature from the outset.

The original IS 102.BADA (1996) was one of the early P25 interim standards, establishing the functional framework during the initial P25 Phase 1 deployment period. The 2000 revision to TIA/EIA-102.BADA elevated it from interim to full standard status. The 2012 TSB-102.BADA-A revision reclassified it as a bulletin (reflecting its informative rather than normative nature) and updated references to point to current versions of AABC, AABF, AABG, and BABA-1.

The normative telephone interconnect signaling (what specific messages are sent and when) has always lived in the control channel message specifications: AABC for trunked systems defines the Telephone Interconnect Request, Voice Channel Grant, Answer Request/Response, and related messages. AABG defines the equivalent for conventional systems. This architectural separation — informative overview in BADA, normative signaling in AABC/AABG — reflects the TIA-102 suite's general pattern of separating descriptive/overview documents from normative protocol specifications.
