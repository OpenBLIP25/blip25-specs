# P25 KFD Interface Protocol — TIA-102.AACD-A Implementation Spec

**Source:** ANSI/TIA-102.AACD-A (September 2014), *Digital Land Mobile
Radio — Key Fill Device (KFD) Interface Protocol*. Supersedes
TIA-102.AACD (2003 original). Revision A updates field widths, adds
the **IP transport path**, and incorporates implementation corrections
— the original AACD only supported the 3WI (three-wire interface).

**Companion:** TIA-102.AACD-B (October 2025) supersedes this
document, adding KMM Forwarding, paired/unpaired authentication key
procedures, OTAR provisioning over KFD, and Warm Start WSTEK
transport security. AACD-B detail is delivered as a delta spec at
`standards/TIA-102.AACD-B/P25_KFD_Interface_Protocol_AACD_B_Delta_Implementation_Spec.md`.

**Document type:** MESSAGE_FORMAT + PROTOCOL. Specifies the wire
protocol between a **Key Fill Device (KFD)** — a physically-secured
hand-held key-loading device — and a **Mobile Radio (MR)** over the
radio's key-fill port. Used for initial keying, TEK updates,
zeroize, key inventory, authentication-key provisioning, and
configuration.

**Not an on-air spec.** The KFD port is a tethered wired connection
(3-wire serial or USB/RS-232 → PPP → IP). Nothing defined here appears
on the Um air interface.

**Scope of this derived work:**
- §1 — Why KFD exists (bootstrapping the UKEK for OTAR)
- §2 — Two transport paths: 3WI (legacy) and IP (current)
- §3 — Exchange session state machines (5-step IP, 4-step 3WI)
- §4 — 3WI physical + link layer (4 kbps K/F line, byte framing, CRC-16)
- §5 — IP stack: RS232/USB → PPP → IPv4 → UDP 49165
- §6 — KMM catalog: 33 messages, OTAR-reused vs KFD-specific
- §7 — KMM Header and wire framing
- §8 — Modify-Key-Command wire format (the primary keyload path)
- §9 — Inventory commands (List Active Keys, MNP, KMF RSI, SUID, …)
- §10 — Destination RSI and the `$FFFFFF` broadcast rule
- §11 — CRC-16-CCITT generation and test vectors
- §12 — KIT (Key Fill Inactivity Timer)
- §13 — Keyset initialization rules
- §14 — Feature taxonomy (KFD-only vs KMF-based, 16 features)
- §15 — Cite-to section references
- §16 — Cross-references

**Pipeline artifacts:**
- `standards/TIA-102.AACD-A/TIA-102-AACD-A_Full_Text.md` — clean-room
  extraction (copyrighted, git-ignored).
- `standards/TIA-102.AACD-A/TIA-102-AACD-A_Summary.txt` — retrieval summary.
- `standards/TIA-102.AACD-A/TIA-102-AACD-A_Related_Resources.md`.
- `annex_tables/aacd_kmm_catalog.csv` — 33-row KMM catalog with
  Message ID, Inventory Type, response kind, OTAR-reused flag.

---

## 1. Why KFD Exists

Per AACD-A §1 + AAAB-B §6.

P25 traffic encryption uses symmetric keys (DES, 3DES, AES). Those
keys must reach the radios somehow. Two distribution paths:

- **OTAR** (AACA-D): KMF pushes key updates to radios over the air.
  Relies on a pre-shared **Unit Key Encryption Key (UKEK)** that
  protects the update in transit.
- **KFD**: operator physically connects a hand-held Key Fill Device
  to the radio's accessory port and pushes keys directly.

**KFD is the bootstrap.** OTAR can't install the UKEK itself (no key
to wrap it with). KFD fills that gap: it's a physically-secured
trusted device that loads the UKEK + initial TEKs before the radio
ever transmits. Subsequent key rotation can then happen over OTAR.

**KFD is also the day-to-day tool** for agencies operating without a
KMF — the KFD supports load / erase / inventory / zeroize directly,
with no network-side infrastructure.

---

## 2. Two Transport Paths

### 2.1 Three-Wire Interface (3WI) — legacy

Physical: K/F line (half-duplex), Keyload* trigger, Ground.

- 4 kbps (250 µs / bit).
- Half-duplex — one direction at a time.
- Byte format: 1 start bit + **8 data bits MSB-first** + 1 even parity bit + ≥1 ms low gap.
- Logic levels: `V_ILmax = 1.3 V`, `V_IHmin = 3.7 V`.
- **Key signature** (starts a session): 100 ms low + 5 ms pause.
- Max KMM frame size: **512 bytes**.
- Session-initiation timeout `t_out1 = t_out2 = 5 s`.

### 2.2 IP Transport — current

Physical: RS-232 or USB.

Stack (bottom-up per AACD-A §3):

```
┌──────────────────────┐
│ Manual Rekeying App  │   KFD features (§14)
├──────────────────────┤
│ KFD Interface Proto  │   KMM-level exchange (§6)
├──────────────────────┤
│ UDP                  │   port 49165 (KFD IP Proto)
├──────────────────────┤
│ IPv4                 │
├──────────────────────┤
│ PPP                  │   BAEG-A link layer
├──────────────────────┤
│ RS-232 or USB        │
└──────────────────────┘
```

- **UDP port**: **49165** (assigned per BAJD-A). KFD source port is
  in range 49165–65535.
- KFD and MR exchange IP addresses before the session (IP binding
  per BAEG-A §2.4 — typically static or DHCP for the tethered link).

---

## 3. Exchange Session State Machines

### 3.1 IP Path (5 steps)

```
KFD                               MR
 │ 1. Ready Request               │
 ├──────────────────────────────▶│
 │    Ready General Mode           │
 │◀──────────────────────────────┤
 │                                │
 │ 2. KFD→MR transfer (required)  │
 │    Modify-Key / Change-RSI /   │
 │    Zeroize / … KMMs            │
 ├──────────────────────────────▶│
 │    per-KMM Response            │
 │◀──────────────────────────────┤
 │                                │
 │ 3. MR→KFD transfer (optional)  │
 │    (skipped for keyload-only)  │
 │                                │
 │ 4. End Session                 │
 ├──────────────────────────────▶│
 │    End Session Ack             │
 │◀──────────────────────────────┤
 │                                │
 │ 5. Disconnect                  │
 ├──────────────────────────────▶│
 │    Disconnect Ack              │
 │◀──────────────────────────────┤
```

### 3.2 3WI Path (4 steps)

3WI has **no separate End Session** — Disconnect terminates the
session.

```
KFD                               MR
 │ 1. Ready Req ($C0)             │
 ├──────────────────────────────▶│
 │    Ready General Mode ($D0)    │
 │◀──────────────────────────────┤
 │                                │
 │ 2. KFD→MR transfer             │
 ├──────────────────────────────▶│
 │                                │
 │ 3. MR→KFD transfer             │
 │◀──────────────────────────────┤
 │                                │
 │ 4. Disconnect ($92)            │
 ├──────────────────────────────▶│
 │    Disconnect Ack ($90)        │
 │◀──────────────────────────────┤
```

**3WI opcodes** (Annex B §B.2):

| Opcode | Value | Meaning |
|--------|-------|---------|
| Ready Req | `$C0` | KFD wakes up MR |
| Ready General Mode | `$D0` | MR says "ready" |
| Transfer Done | `$C1` | End of a transfer direction |
| KMM (data frame) | `$C2` | Carries a KMM body |
| Disconnect Ack | `$90` | MR confirms session end |
| Disconnect | `$92` | KFD terminates session |

---

## 4. 3WI Physical + Link Layer

Per AACD-A Annex B.

### 4.1 KMM Frame over 3WI

```
┌─────────────┬──────────────┬─────────────┬───────────────┬──────────────────────────────┬─────────┐
│ Opcode=$C2  │ Length (2B)  │ Control(1B) │ Dest RSI (3B) │ [Encryption Sync (optional)] │ KMM body│ CRC-16(2B)
└─────────────┴──────────────┴─────────────┴───────────────┴──────────────────────────────┴─────────┘
```

- **Opcode** `$C2` signals a KMM data frame.
- **Length** — 16-bit MSB-first count of octets from Control through end of KMM body (not CRC).
- **Control** — bit flags; the encryption flag (b1 in AACD-B, reserved in AACD-A) indicates outer-layer encryption is in use.
- **Dest RSI** — 3-byte (24-bit) target radio RSI. `$FFFFFF` is the broadcast/unknown sentinel (§10).
- **Encryption Sync** (if encrypted) — 13 bytes: MI + AlgID + KeyID + SAP per AAAD-B.
- **KMM body** — per §6–§9.
- **CRC-16** — CRC-CCITT on everything from Opcode through end of KMM body. Byte-swapped before transmission (§11).

### 4.2 Max frame sizes

- Max KMM frame (including Opcode + Length + Control + …): **512 bytes**.
- Max data-link-independent KMM body: **478 octets**.

---

## 5. IP Stack Details

Per AACD-A §3.5–§3.7.

### 5.1 UDP Port

**49165** — KFD Interface Protocol. Assigned by TIA-102.BAJD(-A).

### 5.2 PPP and IPv4

PPP over RS-232 or USB carries IPv4. Address binding is done through
standard PPP IPCP negotiation, or statically configured. The KFD and
MR exchange known IP addresses before the session.

### 5.3 Session Control on IP

The `Session Control` KMM (`Message ID = 0x31`) carries Ready Request /
Ready General Mode / End Session / End Session Ack / Disconnect /
Disconnect Ack as its body — the 5-step sequence is layered over KMMs,
not over raw opcodes as on 3WI.

---

## 6. KMM Catalog (33 messages)

Full machine-readable catalog: **`annex_tables/aacd_kmm_catalog.csv`**.

AACD-A reuses 13 KMMs from AACA-A (OTAR) and adds 20 KFD-specific
KMMs. Summary by family:

### 6.1 Keyload / Key Management (11 KMMs)

| KMM | Msg ID | Notes |
|-----|--------|-------|
| Modify-Key-Command | `$13` | Primary keyload — carries one or more TEKs |
| Change-RSI-Command | `$03` | Set MR's Individual RSI |
| Change-RSI-Response | `$04` | — |
| Changeover-Command | `$05` | Activate a loaded keyset |
| Changeover-Response | `$06` | — |
| Negative-Ack | `$16` | Generic failure |
| Rekey-Acknowledgment | `$1D` | — |
| Zeroize-Command | `$21` | Erase all keys |
| Zeroize-Response | `$22` | — |
| Unable-to-Decrypt-Response | `$27` | **NOT supported on 3WI** |
| Session Control | `$31` | IP-path session lifecycle |

### 6.2 Inventory / View (12 KMMs)

Paired Command/Response with Inventory Type selecting subtype:

| Inventory Type | What it lists |
|----------------|---------------|
| `$02` | Active Kset IDs |
| `$0B` | RSI Items |
| `$F7` | Active SUID (LLA) |
| `$F8` | SUID Items |
| `$F9` | Keyset Tagging Info |
| `$FD` | Active Keys — **uses Inventory Marker for pagination** |
| `$FE` | MNP (Message Number Period) |
| `$FF` | KMF RSI |

Command Msg ID `$0D`; Response Msg ID `$0E`.

### 6.3 Authentication Key (4 KMMs)

| KMM | Msg ID | Purpose |
|-----|--------|---------|
| Load Authentication Key-Command | `$28` | Install SUID-keyed LLA material per AACE-A |
| Load Authentication Key-Response | `$29` | — |
| Delete Authentication Key-Command | `$2A` | — |
| Delete Authentication Key-Response | `$2B` | — |

### 6.4 Configuration (2 KMMs)

| KMM | Msg ID | Purpose |
|-----|--------|---------|
| Load-Config-Command | `$FD` | Bulk MR config parameters |
| Load-Config-Response | `$FC` | — |

**Reused vs new.** KMMs with `reused_from_otar = yes` in the CSV have
their body format defined in **AACA-A** (OTAR) — this spec just reuses
them. KMMs marked `no` are KFD-specific; their body format is defined
inline in AACD-A §3.9.

---

## 7. KMM Header

Per AACD-A §3.9.1. All KMMs share a common header (defined in full in
AACA-A):

| Field | Setting for AACD-A |
|-------|---------------------|
| Message ID | Per KMM (see §6) |
| Message Length | Calculated per KMM |
| Response field (`Rsp` in Message Format) | Set per KMM |
| Message Number (`MN`) | **Always 0** — no Message Number used |
| MAC field | **Always 0** — no MAC used |
| Destination RSI | Target MR's individual RSI, or `$FFFFFF` if unknown (§10) |
| Source RSI | KFD's own RSI |

---

## 8. Modify-Key-Command (the primary keyload path)

Per AACD-A §3.9.2.17 Table 11. This is the most complex message;
understand it and the rest of the protocol follows.

### 8.1 Outer structure

```
┌────────────────────────────┐
│ Decryption Instruction Fmt │  1 octet — from AACA-A
├────────────────────────────┤
│ Extended Decryption Instr  │  1 octet — AACD-A extension; reserved
├────────────────────────────┤
│ Algorithm ID               │  1 octet — AlgID of the key-wrapping key
├────────────────────────────┤
│ Key ID                     │  2 octets — KID of the wrapping key
├────────────────────────────┤
│ Update Count (optional)    │  0 or 1 octet
├────────────────────────────┤
│ Message Indicator          │  0 or 9 octets — only present if encrypted
├────────────────────────────┤
│ Keyset ID                  │  1 octet — target keyset for the new keys
├────────────────────────────┤
│ Algorithm ID (new key)     │  1 octet
├────────────────────────────┤
│ Key Length                 │  1 octet
├────────────────────────────┤
│ Number of Keys (x)         │  1 octet — count of Unique Key Items
│                            │
│ SEQUENCE [1..x] of Unique  │  repeated x times:
│ Key Items:                 │    • Key Format (1B)
│                            │    • Storage Location Number (2B)
│                            │    • Key ID (2B)
│                            │    • Key (Key Length octets)
│                            │    • Checksum (0 or 4B, optional)
│                            │    • Key Name (0..31 ASCII, optional)
└────────────────────────────┘
```

### 8.2 Encrypted vs unencrypted key transfer

- **Unencrypted** (the typical case for tethered KFD): Algorithm ID
  of the wrapping key = `$80` (Unencrypted / Type I clear). Key field
  carries plaintext key bytes.
- **Encrypted**: key material wrapped per AACA-A (KEK-based inner
  protection). MI is present (9 octets).

### 8.3 DES key format (Table 9)

Each of 8 octets carries 7 data bits + 1 parity bit:

```
Octet 0: [B1..B7][P1]
Octet 1: [B8..B14][P2]
...
Octet 7: [B50..B56][P8]
```

B1 is the MSB of the plaintext key; P1 is its parity (even).

### 8.4 Block-cipher key format (TDES / AES, Table 10)

Straight 8-bit-per-byte MSB-first, left-justified, zero-padded if the
key length isn't a multiple of 8:

```
Octet 0: [b1..b8]   ← b1 is the plaintext key MSB
Octet 1: [b9..b16]
...
```

Keys transmitted **without check vectors** — verification is done
locally by the MR after loading.

### 8.5 Extended Decryption Instruction octet

AACD-A adds one octet beyond the AACA-A-defined Decryption Instruction
Format octet. In AACD-A this byte has:
- `b7` — unavailable; always 0.
- `b6–b0` — reserved; must be 0.
- May be defined in future revisions.

Decoder: read as reserved; don't emit non-zero.

---

## 9. Inventory Commands

### 9.1 List Active Keys — the paginated case

Per AACD-A §3.9.2.11 Table 3, §3.9.2.12 Table 4. The only KMM with
normative pagination logic:

**Command:**
```
Octet 0:    Inventory Type = $FD
Octet 1-3:  Inventory Marker (24-bit; 0 = start at beginning)
Octet 4-5:  Max # of Keys Requested (16-bit; typical 78)
```

**Response:**
```
Octet 0:    Inventory Type = $FD
Octet 1-3:  Inventory Marker (0 = no more; nonzero = position of next key)
Octet 4-5:  Number of Items (x)
Octets 6+:  SEQUENCE [1..x] of Key Info Items:
              Keyset ID (1B) | SLN (2B) | Algorithm ID (1B) | Key ID (2B)
```

**Pagination rule:** to limit the response to 478 octets (the 3WI
KMM body cap), the command typically sets Max Keys = 78. If the MR
has more keys than fit, it sets the Inventory Marker in the response
to mark the 79th position. The KFD then re-issues the Command with
Marker = that value to fetch the next page.

### 9.2 Other List-type Inventories

Follow similar patterns but fit in a single KMM:

- **List MNP** (`$FE`) — Response carries 2-byte Message Number Period.
- **List KMF RSI** (`$FF`) — Response carries 3-byte KMF RSI.
- **List Active Kset IDs** (`$02`) — reused from OTAR.
- **List RSI Items** (`$0B`) — reused from OTAR.
- **List Active SUID** (`$F7`), **List SUID Items** (`$F8`) — for LLA provisioning introspection.
- **List Keyset Tagging Info** (`$F9`) — keyset metadata.

---

## 10. Destination RSI and `$FFFFFF`

Per AACD-A §3.9.1:

- If the KFD does **not know** the MR's individual RSI, it uses
  **`$FFFFFF`** (broadcast / unknown) as Destination RSI.
- An MR **must always accept** a KMM with Destination RSI =
  `$FFFFFF`.
- Otherwise the KFD may use the MR's actual Individual RSI (or
  continue using `$FFFFFF` — both are permitted).
- For replies, the MR sets Destination RSI to the KFD's Individual
  RSI (learned from the command's Source RSI).
- The MR is **permitted to accept any Source RSI**, including the
  `$FFFFFF` all-call sentinel.

**Implementation consequence.** A naive "validate Destination RSI
matches my own" check in the MR will silently drop valid KFD
sessions. Accept `$FFFFFF` in addition to the MR's own RSI.

---

## 11. CRC-16-CCITT

Per AACD-A Annex A.

**Polynomial:** `G(X) = X^16 + X^12 + X^5 + 1` (the standard CRC-CCITT).
**Initial register value:** `$FFFF`.
**Final transformation:** byte-swap the computed CRC before placing
on the wire.

**Test vector** (from Annex A):
- Input bytes: `$DE $AD`
- Pre-swap CRC register: `$4B7C`
- **Transmitted CRC**: `$7C4B`

### 11.1 Decoder reproducibility

```python
# CRC-16-CCITT per AACD-A Annex A
POLY = 0x1021
INIT = 0xFFFF

def crc16_ccitt(bits_or_bytes):
    reg = INIT
    for b in bits_or_bytes:  # iterating bytes; MSB-first within each byte
        reg ^= (b << 8)
        for _ in range(8):
            if reg & 0x8000:
                reg = ((reg << 1) ^ POLY) & 0xFFFF
            else:
                reg = (reg << 1) & 0xFFFF
    return reg

# Verify: input "DE AD" → 0x4B7C; wire (byte-swapped) → 0x7C4B
assert crc16_ccitt([0xDE, 0xAD]) == 0x4B7C
```

**Note the byte swap.** The register value and the wire bytes are
different — a common implementation bug. The receiver computes the
CRC the same way, byte-swaps its computed value, and compares to
the received CRC.

---

## 12. KIT (Key Fill Inactivity Timer)

Per AACD-A §3.8.

The MR maintains a session inactivity timer:

| Parameter | Value |
|-----------|-------|
| **Default** | **15 seconds** |
| Minimum | 5 seconds |
| Maximum | 30 seconds |

**Reset trigger.** The MR resets KIT **each time it sends a KMM**
(not on receive). If KIT expires before the session terminates
normally, the MR:
- Abandons the session.
- Returns to idle.
- Does NOT send a Disconnect — the KFD detects the timeout on its
  own timer.

**KFD-side responsibility.** A well-behaved KFD keeps traffic moving
— for long operations (large key inventories, multiple Modify-Key-
Commands), pace them so the MR has regular outbound KMMs to reset
its KIT. Bursting many Commands to the MR without waiting for
Responses can cause KIT expiry mid-burst.

---

## 13. Keyset Initialization Rules

Per AACD-A §3.9 and AACA-A background:

- The **first keyset loaded** into an empty MR is **automatically
  enabled AND activated**.
- Subsequent keyset loads produce **enabled-not-activated** keysets —
  they exist in the MR but are not selected for encryption / decryption.
- **At most one keyset can be active at a time.**
- A **Changeover-Command** switches the active keyset. The enabled
  keyset is always treated as active if no explicit activation has
  occurred.

**Implementation consequence.** An MR that has received three
Modify-Key-Commands (to keysets 1, 2, 3) has 3 enabled keysets but
only keyset 1 is active. A Changeover-Command to keyset 3 makes
keyset 3 active; keyset 1 is still enabled but dormant.

---

## 14. Feature Taxonomy (16 Features)

AACD-A §2.3 enumerates 16 manual rekeying features, partitioned by
whether they require a KMF:

### 14.1 KFD-only (no KMF required)

| Feature | KMMs used |
|---------|-----------|
| Keyload | Modify-Key-Command, Rekey-Acknowledgment |
| Key Erase | Modify-Key-Command (zero-length key), Rekey-Acknowledgment |
| Erase All Keys (Zeroize) | Zeroize-Command / Response |
| View Key Info | Inventory-Cmd/Rsp (List Active Keys) |

### 14.2 KMF-based (require a KMF for subsequent OTAR)

| Feature | KMMs used |
|---------|-----------|
| UKEK management | (Modify-Key-Command; UKEK stored like a TEK but semantically distinguished by SLN) |
| RSI configuration | Change-RSI-Command / Response, Inventory-Cmd/Rsp (List RSI Items, List KMF RSI) |
| MNP provisioning | Inventory-Cmd/Rsp (List MNP) + Load-Config-Command |
| Keyset operations (enable, activate) | Changeover-Command / Response |
| View Keyset Info | Inventory-Cmd/Rsp (List Active Kset IDs, List Keyset Tagging Info) |

### 14.3 Authentication (§2.4)

| Feature | KMMs used |
|---------|-----------|
| Load Authentication Key | Load Authentication Key-Command / Response |
| Delete Authentication Key | Delete Authentication Key-Command / Response |
| View Active SUID | Inventory-Cmd/Rsp (List Active SUID) |
| View SUID Info | Inventory-Cmd/Rsp (List SUID Items) |

LLA keys are tied to SUID per AACE-A; the KFD is the **only** path
for loading them (no OTAR path per AAAB-B §7.2.1).

---

## 15. Cite-To Section References

Per project convention, cite the PDF's own section numbers when
sending readers to verify:

- Purpose and bootstrap rationale — TIA-102.AACD-A §1.1 / §1.2.
- Manual rekeying overview and 16 features — §2.
- Auth features — §2.4.
- Interface protocol architecture — §3.
- KFD-MR interface description — §3.1.
- Physical layer (RS-232) — §3.2.
- Physical layer (USB) — §3.3.
- Link / Network / Transport layers — §3.4–§3.6.
- KFD Interface Protocol Application Sublayer (IP path) — §3.7.
- Manual Rekeying Application Sublayer — §3.8.
- KMM list (Table 1) — §3.9.
- KMM Header definition — §3.9.1.
- Modify-Key-Command (Table 11) — §3.9.2.17.
- Inventory-Command (List Active Keys) Table 3 — §3.9.2.11.
- Inventory-Response (List Active Keys) Table 4 — §3.9.2.12.
- Inventory MNP / KMF RSI Tables 5–8 — §3.9.2.13–§3.9.2.16.
- DES key format (Table 9) — §3.9.2.17.
- Block-cipher key format (Table 10) — §3.9.2.17.
- CRC-16-CCITT — Annex A.
- Three-Wire Half Duplex Interface — Annex B (§B.1 physical, §B.2 link + app).

---

## 16. Cross-References

**Upstream (this doc depends on):**
- `standards/TIA-102.AAAB-B/P25_Security_Services_Overview_Implementation_Spec.md`
  — security-taxonomy context; AACD is the wireline-Key-Management path.
- **AACA-A** — OTAR Messages and Procedures. **AACD-A reuses 13 KMM
  body formats from AACA-A directly**. Check AACA-A for Change-RSI,
  Changeover, Inventory (Active Kset IDs / RSI Items), Negative-Ack,
  Rekey-Acknowledgment, Zeroize, Unable-to-Decrypt, KMM Header.
- `standards/TIA-102.AACE-A/P25_Link_Layer_Authentication_Implementation_Spec.md`
  — SUID structure and LLA authentication keys (managed through the
  Load/Delete Authentication Key KMMs).
- `standards/TIA-102.BAJD-A/P25_Port_Number_Assignments_Implementation_Spec.md`
  — assigns UDP port 49165 to KFD.
- `standards/TIA-102.BAEG-A/P25_Mobile_Data_Peripheral_Interface_Implementation_Spec.md`
  — A-interface link layer (PPP over USB or RS-232) that the KFD IP
  path rides on.
- `standards/TIA-102.BAAC-D/P25_Reserved_Values_Implementation_Spec.md`
  — ALGID values (e.g., `$80` Unencrypted, `$85` AES-128).

**Downstream:**
- `standards/TIA-102.AACD-B/P25_KFD_Interface_Protocol_AACD_B_Delta_Implementation_Spec.md`
  — AACD-B (October 2025) delta: KMM Forwarding, paired/unpaired
  auth keys, OTAR Provisioning, Warm Start WSTEK transport security,
  View Active SUID.

**Supporting annex table:**
- `annex_tables/aacd_kmm_catalog.csv` — 33-row KMM catalog with
  Message ID, Inventory Type, response kind, OTAR-reused flag, body
  reference.
