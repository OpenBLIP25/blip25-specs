# P25 Location Services — TSB-102.BAJA-B + TIA-102.BAJB-B Implementation Spec

**Sources:**
- **TSB-102.BAJA-B** (November 2017) — *Location Services Overview*.
  Informative TIA Systems Bulletin; cancels and replaces TSB-102.BAJA-A
  (2010). Lineage: original TSB (Feb 2009) → Rev A (Feb 2010) →
  **Rev B (Nov 2017)**.
- **ANSI/TIA-102.BAJB-B** (December 2019) — *Tier 1 Location Services
  Specification*. Normative. Cancels and replaces BAJB-A (2014).

This impl spec **combines the two** because the overview and Tier 1
spec share terminology, architecture, and the entity model. Tier 2
(LRRP over IP Data Bearer) is referenced from the overview but the
normative detail lives in **TIA-102.BAJC-A** — **not in this repo
yet**. Tier 2 is catalogued here for architectural completeness;
Tier 2 wire-format details will need a separate impl spec when
BAJC-A is acquired and processed.

**Document type:** PROTOCOL + overview. Defines how an SU reports its
physical position (typically GPS) to a Location Service Host System
(LSHS) — a mapping / dispatch application.

**Scope of this derived work:**
- §1 — Two-tier model (Tier 1 = NMEA-over-CAI-data; Tier 2 = LRRP-over-IP)
- §2 — Architectural entities and the standardization scope of each interface
- §3 — Four system configurations: which tier supports which
- §4 — Tier 1 wire details: SAP, unconfirmed CAI delivery, 128-octet cap
- §5 — Tier 1 application protocol: NMEA 0183 v3.01
- §6 — Tier 1 NMEA sentence catalog with TX/RX conformance
- §7 — Optional proprietary `$PP25` sentence
- §8 — Triggering and reporting (batch, freshness, 4-retransmit rule)
- §9 — Protocol performance and scalability limits
- §10 — Tier 2 architectural summary (deferred to BAJC-A)
- §11 — Cite-to section references
- §12 — Cross-references

**Pipeline artifacts:**
- `standards/TSB-102.BAJA-B/TSB-102-BAJA-B_Full_Text.md` — overview
  clean-room extraction (copyrighted, git-ignored).
- `standards/TSB-102.BAJA-B/TSB-102-BAJA-B_Summary.txt` — retrieval summary.
- `standards/TSB-102.BAJA-B/TSB-102-BAJA-B_Related_Resources.md`.
- `standards/TIA-102.BAJB-B/TIA-102-BAJB-B_Full_Text.md` — Tier 1
  clean-room extraction.
- `standards/TIA-102.BAJB-B/TIA-102-BAJB-B_Summary.txt` — Tier 1 summary.
- `standards/TIA-102.BAJB-B/TIA-102-BAJB-B_Related_Resources.md`.
- `annex_tables/baj_tier1_nmea_sentences.csv` — 7-sentence catalog
  with TX/RX mandatory-vs-optional.
- `annex_tables/baj_tier1_scalability.csv` — update-interval vs
  user-count at 80% reliability.

---

## 1. Two-Tier Model

Per TSB-102.BAJA-B §4.2:

| Aspect | **Tier 1** (BAJB-B) | **Tier 2** (BAJC-A — not in repo) |
|--------|----------------------|------------------------------------|
| Purpose | Simple field deployment, no network infrastructure | Dispatch center integration; IP routing to a fixed host |
| Application protocol | **NMEA 0183 v3.01** ASCII | **LRRP** (Location Request/Response Protocol, XML, compressed) |
| Transport | CAI Data Bearer Service (unconfirmed) | IP Data Bearer Service (UDP) |
| Supported configurations | Direct Data, Repeated Data | All four (Direct / Repeated / Conventional FNE / Trunked FNE) |
| Fixed host support | **No** — SU-to-SU only | Yes — via FDH over FNE |
| Triggering control | Locally configured in each SU | Remotely configurable from fixed host |
| Typical use | Tactical field ops, small-team mapping | Dispatch center real-time tracking |

**Implementer consequence.** If you observe a P25 location-services
payload on-air:

- **If it's ASCII starting with `$GP` / `$GL` / `$GN`** (and ends with
  `*HH<CR><LF>`) on a CAI Unconfirmed Data packet using the Location
  Service SAP — that's **Tier 1**. Parse per NMEA 0183.
- **If it's a UDP datagram carrying compressed XML** (payload shape
  depends on LRRP compression; typically binary) on the Location
  Services UDP port — that's **Tier 2**. Parse per BAJC-A (separate
  spec).

---

## 2. Architectural Entities

Per TSB-102.BAJA-B §4.1. Six types of entity, plus the air-interface
elements they ride on:

| Entity | Role | Implementation note |
|--------|------|---------------------|
| **LIS** (Location Information System) | Raw position source (GPS, LORAN, …) | Embedded in SU or external device. **Interface to SU is NOT standardized** (§4.3.1). |
| **SU** (Subscriber Unit) | LMR radio; obtains position from LIS, transmits over Um | Central actor. Handles triggering, batch, freshness. |
| **FS_R** (Fixed Station Repeater) | Relays CAI PDUs in Repeated Data config | Transparent — no location-aware logic. |
| **FNE** (Fixed Network Equipment) | Relays in Conventional/Trunked FNE configs (Tier 2) | Transparent — no location-aware logic. |
| **FDH** (Fixed Data Host) | IP-addressed host that hosts or bridges to LSHS | Tier 2 only. Does LRRP compress/decompress. |
| **MDP** (Mobile Data Peripheral) | IP-addressed device connected to SU | Tier 2: handles UDP/IP and LRRP compression. Tier 1: may relay LIS to SU or host LSHS. |
| **LSHS** (Location Service Host System) | The mapping / dispatch application | May live in SU, MDP, or FDH. **Interface to the hosting element is NOT standardized** (§4.3.3). |

### 2.1 Which interfaces are standardized

Per TSB-102.BAJA-B §4.3:

| # | Interface | Standardized? | Where |
|---|-----------|---------------|-------|
| 4.3.1 | **LIS ↔ SU/MDP** | **NOT standardized** | Manufacturer-specific. Disclosure required. |
| 4.3.2 | SU ↔ SU / SU ↔ FS_R / SU ↔ FNE | Standardized | Tier 1: BAJB-B. Tier 2: BAJC-A |
| 4.3.3 | **SU ↔ LSHS** | **NOT standardized** | Manufacturer-specific |
| 4.3.4 | SU ↔ MDP | Standardized (Tier 2 only) | BAJC-A |
| 4.3.5 | FNE ↔ FDH | Standardized (Tier 2 only) | BAJC-A |
| 4.3.6 | FDH ↔ MDP or LSHS | Standardized (Tier 2 only) | BAJC-A |

**Rationale for the non-standardized interfaces.** LIS and LSHS both
belong to industries (GPS receivers, mapping software) that are
separate from LMR — TIA-102 deliberately does not impose protocols
here because manufacturer-specific certification handles it.
Implementers must disclose what protocols their SUs support on
these interfaces.

---

## 3. System Configurations — Tier Support Matrix

Per TSB-102.BAJA-B §5 + BAJB-B §4 + BAEA-C §5:

| Configuration (BAEA-C) | Tier 1 | Tier 2 |
|-------------------------|--------|--------|
| **Direct Data** (SU ↔ SU) | **Yes** | Yes |
| **Repeated Data** (SU ↔ FS_R ↔ SU) | **Yes** | Yes |
| **Conventional FNE Data** | No | Yes |
| **Trunked FNE Data** | No | Yes |

**Tier 1 deliberately excludes FNE-routed configurations** — that's
the point of the two-tier split. Tier 1 is for scenarios where you
don't have a fixed network (command post, direct-mode field ops,
small-team repeater coverage) and just want SU-to-SU mapping.

---

## 4. Tier 1 Wire Details

Per BAJB-B §4 + §5.

### 4.1 Transport layer

- **Bearer service:** **CAI Data Bearer Service** (not IP Data Bearer).
  See BAEA-C §6.1.
- **Delivery mode:** **Unconfirmed Data Packet Delivery** — no per-block
  CRC-9 ACK loop, no Response SDU. See BAAA-B §5.4 (Unconfirmed Data PDU).
- **SAP identifier:** **Location Service** value from TIA-102.BAAC-D.
- **Data Header Offset:** `0` (LSB = first octet is the application
  payload; no intermediate management layer).
- **Maximum application message size: 128 octets**.
- No acknowledgement, no retry, no session state. Fire-and-forget.

### 4.2 Why 128 octets and not more

Per BAJB-B §6 (protocol performance):

| Packet size | Unencrypted TX time | Encrypted TX time |
|-------------|---------------------|--------------------|
| 24 B (minimum GPS update) | 175 ms | 200 ms |
| 71 B (typical Magellan GPS 315) | 300 ms | 325 ms |
| 83 B (max standard NMEA 0183) | 325 ms | 350 ms |
| **128 B (NMEA + proprietary)** | **385 ms** | **425 ms** |
| 512 B (full CAI packet) | 1540 ms | 1700 ms |

The 128-octet cap is a channel-capacity choice: a full 512-byte
packet takes ~1.5 s on-air, unacceptable for periodic automatic
updates from a group of units. Capping at 128 octets keeps the
typical update under ½ second.

### 4.3 Decoder dispatch

```
on_cai_unconfirmed_data_pdu(header, payload):
    if header.sap == SAP_LOCATION_SERVICE and header.data_header_offset == 0:
        if payload.startswith("$"):
            parse_nmea_0183(payload)
        else:
            log_warning("Location Service SAP but non-NMEA payload", payload)
```

Per BAAC-D, Location Service SAP has a specific value — look it up in
`standards/TIA-102.BAAC-D/P25_Reserved_Values_Implementation_Spec.md`
SAP table. Do not assume the decimal or hex value without checking.

---

## 5. Tier 1 Application Protocol: NMEA 0183 v3.01

Per BAJB-B §5. Standard GPS sentence format originally designed for
marine electronics.

**Wire format (NMEA 0183 general):**

```
$<talker><sentence_id>,<field1>,<field2>,...*<HH><CR><LF>
```

- `$` — sentence start.
- `<talker>` — usually `GP` (GPS), `GL` (GLONASS), `GN` (multi-GNSS).
  For P25 Tier 1 purposes, `GP` is expected most of the time.
- `<sentence_id>` — 3-letter identifier (e.g., `GGA`, `GLL`, `RMC`).
- Fields — comma-separated ASCII, empty fields represented as back-to-back commas.
- `*` — checksum delimiter.
- `<HH>` — 2-hex-character XOR checksum of all characters **between `$` and `*`**
  (exclusive on both ends).
- Terminator: `<CR><LF>` (`0x0D 0x0A`).

**Baud rate:** 4800 bps nominal over the NMEA serial interface (the
LIS-to-SU side, which is NOT standardized). Some LIS units support
9600 / 19200 / 38400 bps. Over the P25 air interface, baud rate is
irrelevant — the sentence is conveyed as opaque octets.

---

## 6. Tier 1 NMEA Sentence Catalog

Full machine-readable catalog: **`annex_tables/baj_tier1_nmea_sentences.csv`**.

Per BAJB-B §5, Table of NMEA sentences:

| Sentence | Meaning | TX (sender) | RX (receiver) |
|----------|---------|-------------|----------------|
| **GGA** | GPS Fix Data (time, position, quality, altitude) | **Mandatory** | **Mandatory** |
| **GLL** | Geographic position (lat/lon) | Optional | Mandatory |
| **GSA** | DOP and active satellites | Optional | Mandatory |
| **GSV** | Satellites in view | Optional | Mandatory |
| **RMA** | Recommended Minimum Loran-C Data | Optional | Mandatory |
| **RMC** | Recommended Minimum GPS/TRANSIT Data | Optional | Mandatory |
| **VTG** | Track made good and speed over ground | Optional | Mandatory |

**Implementer rule:** the **receiving** SU must decode all 7
sentences. The **transmitting** SU only has to emit GGA — the other
6 are optional on the TX side.

### 6.1 Minimum mandatory GGA format

Per BAJB-B §5.2 (the minimum acceptable GGA for Tier 1):

```
$GPGGA,hhmmss.sss,ddmm.mmmm,N/S,dddmm.mmmm,E/W,Q,,,a.a,M,,,*HH<CR><LF>
```

| Field | Meaning |
|-------|---------|
| `hhmmss.sss` | UTC time (hh, mm, ss plus 3-digit millisecond fraction) |
| `ddmm.mmmm` | Latitude: 2-digit degrees, 2-digit minutes, 4-digit decimal minutes |
| `N/S` | Latitude hemisphere |
| `dddmm.mmmm` | Longitude: 3-digit degrees, 2-digit minutes, 4-digit decimal minutes |
| `E/W` | Longitude hemisphere |
| `Q` | Fix quality (`0` invalid, `1` GPS fix, `2` DGPS, …) |
| *(empty)* | Number of satellites (optional) |
| *(empty)* | HDOP (optional) |
| `a.a` | Altitude above mean sea level |
| `M` | Altitude units (meters) |
| *(empty)* | Geoidal separation (optional) |
| *(empty)* | Age of differential GPS data (optional) |
| *(empty)* | DGPS reference station ID (optional) |

**Receiving SUs must support all GGA fields** — including the ones
empty-by-default above.

---

## 7. Optional Proprietary `$PP25` Sentence

Per BAJB-B §5.3. An SU may optionally append a P25-proprietary sentence
to the NMEA payload in the same 128-octet packet:

```
$PP25,UID,u...u*HH<CR><LF>
```

| Field | Meaning |
|-------|---------|
| `UID` | SUID (24-bit subscriber ID) **or** user-defined alphanumeric string up to 8 characters |
| `u...u` | Optional free-form user data |

Combined `<NMEA sentence>` + `<$PP25 block>` must not exceed 128
octets. The proprietary data may originate from the LIS or be
programmed into the SU.

**Decoder pattern.** After parsing the primary NMEA sentence, check
for a following `$PP25` sentence in the same payload. The `UID`
field is useful for correlating which SU the location belongs to
when the SU's LLC source address isn't sufficient.

---

## 8. Triggering and Reporting

### 8.1 Triggers (BAJB-B §6.1)

All triggering is **locally configured in each SU** — BAJB-B does not
mandate which triggers must be supported, only that they're
configurable. Common triggers:

- **Timer expiry** — periodic updates.
- **PTT event** — each time the user keys the radio.
- **Emergency activation** — auto-report on emergency; typically with
  automatic repeat without requiring manual PTT.

All SU triggers and the LSHS destination address **must be configured
in the SU prior to use** (BAJB-B §6.1).

### 8.2 Batch reporting (BAJB-B §6.2)

SUs may **buffer multiple location messages** and send them together
on user request. Use case: path-tracking when the radio was out of
range or otherwise unable to transmit during a traversal.

### 8.3 Freshness rule (BAJB-B §6.3)

Before retransmitting on a trigger, the SU **should verify that
position data has changed**. Stale data may be retransmitted **up to
4 times** — after which the SU should suppress the update until the
position changes.

**Exception:** data may **always** be retransmitted in response to
an **explicit query**, regardless of freshness.

---

## 9. Protocol Performance and Scalability

Per BAJB-B §7.

### 9.1 Channel-contention reliability

Full table: **`annex_tables/baj_tier1_scalability.csv`**.

Tier 1 uses **unconfirmed** delivery — packets that collide on the
channel are simply lost. Reliability drops as more SUs report at
short intervals.

**Reference point:** at 10-second intervals with 10 users,
**≈58% delivery reliability**.

**To sustain 80% reliability (inverse table):**

| Update interval | Max users |
|-----------------|-----------|
| 3 s | 2 |
| 10 s | 5 |
| 50 s | 18 |
| 100 s | 34 |
| 200 s | 100 |

### 9.2 Mitigations

- **Status-symbol channel sensing.** Recommended: use P25 status
  symbols to detect channel activity and queue transmission until
  the channel is clear. Trades latency for collision avoidance.
- **Longer intervals.** For large groups, move to 100 s / 200 s
  intervals rather than trying to use 10 s.
- **Batch reporting.** Reduces channel occupancy by bundling buffered
  updates into fewer transmissions.

### 9.3 Implementation advice for operators

Scale update interval to group size at deployment time. Monitor
channel utilization during drills — if reliability drops, lengthen
the interval or reduce the group.

---

## 10. Tier 2 Architectural Summary (deferred to BAJC-A)

Tier 2 is referenced architecturally by TSB-102.BAJA-B but normatively
specified in **TIA-102.BAJC-A** — **not processed in this repo yet**.
Key characteristics captured here for cross-reference:

| Aspect | Tier 2 |
|--------|--------|
| Application | **LRRP** (Location Request/Response Protocol), XML-based |
| Compression | **Mandatory** — XML is too verbose without it |
| Transport | UDP over IP Data Bearer Service |
| Configurations | All four (Direct, Repeated, Conventional FNE, Trunked FNE) |
| Triggering | Locally configured AND remotely configurable from fixed host |
| IP addressing | Yes — SU has an IP address assigned via SCEP or SNDCP |

**Implementation consequence.** A full passive decoder wanting to
cover both Tier 1 and Tier 2 location traffic needs:

- Tier 1: NMEA 0183 parser over CAI Unconfirmed Data on Location Service SAP.
- Tier 2: LRRP parser over UDP on the Location Services UDP port
  (see BAJD-A for port assignment). Requires an LRRP compression
  decoder — compression scheme is normative in BAJC-A.

BAJC-A is a Phase 3 candidate for future work — not in scope for
this pass.

---

## 11. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify:

**TSB-102.BAJA-B (overview):**
- Two-tier model — §4.2.
- Architectural entities (LIS, SU, FS_R, FNE, FDH, MDP, LSHS) — §4.1, §4.3.
- Standardized vs non-standardized interfaces — §4.3.1–§4.3.6.
- Supported system configurations — §5.
- Triggering and reporting overview — §6.
- Group location out of scope — §2 (Limitations).

**TIA-102.BAJB-B (Tier 1):**
- Architecture and three entities — §3.
- Transport parameters (SAP, unconfirmed, 128-octet cap) — §4.
- NMEA 0183 v3.01 application layer — §5.
- GGA minimum format — §5.2.
- NMEA sentence catalog — §5.1.
- `$PP25` proprietary sentence — §5.3.
- Triggering — §6.1.
- Batch reporting — §6.2.
- Freshness / 4-retransmit rule — §6.3.
- Protocol performance table — §7.1.
- Scalability vs update interval — §7.2.

---

## 12. Cross-References

**Upstream (this doc depends on):**
- `standards/TIA-102.BAEA-C/P25_Data_Overview_Implementation_Spec.md`
  — defines Direct Data, Repeated Data, Conventional FNE Data,
  Trunked FNE Data configurations and CAI vs IP Data Bearer Services.
- `standards/TIA-102.BAAA-B/P25_FDMA_Common_Air_Interface_Implementation_Spec.md`
  — Unconfirmed Data PDU framing used by Tier 1.
- `standards/TIA-102.BAAC-D/P25_Reserved_Values_Implementation_Spec.md`
  — Location Service SAP identifier value.
- `standards/TIA-102.BAED-A/P25_Packet_Data_LLC_Implementation_Spec.md`
  — LLC layer under the CAI data bearer.
- `standards/TIA-102.AAAD-B/P25_Block_Encryption_Protocol_Implementation_Spec.md`
  — encryption when Protected bit is set (encrypted packet sizes in
  BAJB-B §7.1 table).

**Tier 2 cross-references (not in repo):**
- **TIA-102.BAJC-A** — Tier 2 Location Services Specification (LRRP,
  compression, UDP transport, triggering from fixed host).
- **TIA-102.BAJD-A** — `standards/TIA-102.BAJD-A/P25_Port_Number_Assignments_Implementation_Spec.md`
  — UDP ports used by Tier 2 LRRP.

**External references:**
- **NMEA 0183 Version 3.01** — the standard governing the Tier 1
  application layer. Proprietary (NMEA is a trade association);
  implementations typically embed a third-party NMEA parser.

**Supporting annex tables:**
- `annex_tables/baj_tier1_nmea_sentences.csv` — 7-sentence TX/RX
  conformance catalog.
- `annex_tables/baj_tier1_scalability.csv` — update-interval vs
  user-count at 80% reliability (from BAJB-B Table 7.2).
