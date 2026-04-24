# TSBK Field Layouts — Trust the Bit Table, Not the Octet Pseudo-Layout

**Applies to:** TIA-102.AABC-E (Trunking Control Channel Messages), and by
extension the TDMA MAC message formats in TIA-102.BBAD-A that reuse the same
field conventions.

**Why this note exists:** the AABC-E PDF defines every TSBK as a 2D bit table
(horizontal axis = bit number 7..0, vertical axis = octet number). Text-based
extractions of those tables routinely collapse the 2D layout into
octet-oriented prose ("Oct 4: Channel(T) high byte; Oct 5: Channel(T) low
byte") that looks tidy but loses the ability to express fields whose boundaries
don't align with octet boundaries. That is every P25 channel grant
(`svc_opts(8) | band(4) | chan(12) | group(16) | source(24)`), every
discriminated-union message (ACK_RSP_FNE, DENY_RSP), and every status
broadcast whose layout uses 4-bit or 12-bit subfields.

Gap report `gap_reports/0020_aabc_e_osp_pseudo_layouts_lossy_vs_tia_bit_tables.md`
documents seven real decoder bugs that an implementer hit by trusting the
octet pseudo-layout instead of the PDF bit table. This note records the
general pitfall so the next reader does not repeat the same pass.

## Symptoms you'll see

- A field appears as "reserved" in the pseudo-layout but SDRTrunk / OP25 /
  live traffic treat it as carrying real data.
- Source Address or Target Address appears "16-bit abbreviated" in the
  pseudo-layout — there is no such thing in P25. Every abbreviated address
  is 24 bits. "Abbreviated" means *fits in a single TSBK body* as opposed
  to the full 56-bit SUID (24-bit WACN + 12-bit System + 24-bit Unit); it
  does not mean truncated.
- Two pseudo-layouts in the same document disagree on whether a field ends
  at octet 7 or octet 9.
- Channel Number is shown as an 8-bit or 12-bit field with no associated
  Frequency Band — the iden-table lookup needs both.

When any of these appears, the bit table is authoritative.

## Where to find the authoritative bit tables

1. **Primary:** TIA-102.AABC-E PDF §4.x (voice service), §5.x (data service),
   §6.x (control/status), §7.x (system broadcasts), §8.x (roaming/auth).
   Each message format has a "Figure X.Y.Z-N" line drawing showing bits 7..0
   across octets 0..11. These are the ground truth.

2. **Secondary (cross-check):** SDRTrunk's
   `module/decode/p25/phase1/message/tsbk/standard/{isp,osp}/*.java`. Each
   class exposes `private static final int[] FIELD_NAME = {bit, bit, ...}`
   arrays with explicit bit indices. These match the AABC-E PDF bit tables
   exactly and are maintained by the SDRTrunk project against live traffic.
   OP25 uses the same conventions.

3. **Implementation spec §4.3 and §5.3** in
   `standards/TIA-102.AABC-E/P25_Trunking_Control_Channel_Messages_Implementation_Spec.md`.
   These were rewritten as bit-indexed tables in 2026-04-24 after gap 0020.

## General conventions that hold across nearly every TSBK

| Convention | Detail |
|------------|--------|
| Bit 0 | First bit of octet 0 (MSB of `LB | P | Opcode`) |
| Bits 0-7 | `LB | P | Opcode[5:0]` (octet 0) |
| Bits 8-15 | MFID (octet 1) |
| Bits 80-95 | CCITT CRC-16 over bits 0-79 (octets 10-11) |
| Source Address | Almost always 24 bits at **bits 56-79** (the trailing 3 octets of the TSBK body) |
| Target Address | When present without a source, 24 bits at 56-79; when both present, target at 32-55 and source at 56-79 |
| Channel field | Always 16 bits = 4-bit Frequency Band + 12-bit Channel Number; never just 12 bits alone |
| WACN + System | WACN is 20 bits, System is 12 bits — these pack into 4 octets when together (bits 24-55 typically) |

The exceptions are worth memorizing: grant-update messages that carry two
channel+group pairs pack two 4+12+16 = 32-bit units into bits 16-79 (bits
16-47 = A, bits 48-79 = B), with no explicit source address in the TSBK body.

## Discriminated unions — ACK_RSP_FNE and friends

A few messages overload the middle bytes with discriminated-union layouts
driven by a 1-bit flag early in the message. Pseudo-layouts that enumerate
only one variant of the union silently corrupt the other. The canonical
example:

```
ACK_RSP_FNE (OSP 0x20):
  bit 16:    AIV (Additional Info Valid)
  bit 17:    EX  (Extended Addressing)
  bits 18-23: Service Type (6)
  bits 24-55: UNION(
    EX=1: WACN(20) | System(12) — full target WSID
    EX=0: reserved(8) | Source Address(24) — caller's WUID
  )
  bits 56-79: Target Address (24)
```

If you decode bits 24-55 always as WACN+System, messages with EX=0 yield
garbage for the source. If you decode always as Source, messages with EX=1
yield garbage for the target WSID. The discriminator is bit 17, and every
parser must check it.

Same pattern appears (with different discriminators) in `DENY_RSP`,
`QUE_RSP`, and `CAN_SRV_REQ`. Always enumerate both variants in your bit
table, never rely on prose to convey the union.

## What we changed in response to this

- Rewrote §4.3 (ISP) and §5.3 (OSP) in
  `P25_Trunking_Control_Channel_Messages_Implementation_Spec.md` to use
  bit-indexed tables (0-based bit ranges from TSBK start) for every opcode.
- Added authority notes at the top of both sections pointing implementers at
  the PDF bit tables as the authoritative source.
- Added PDF figure citations (e.g., "PDF §4.2.1, Fig 4.2.1-1") to each
  opcode so readers can verify against the source.
- Called out the discriminated-union cases (ACK_RSP_FNE, DENY_RSP) with
  explicit per-variant tables rather than narrative prose.

## If you're adding a new TSBK-adjacent message

Follow the same pattern:

1. Locate the PDF bit table in the relevant TIA document (AABC-E for
   trunking, AABG for conventional, BBAD-A for TDMA MAC).
2. Write the derived spec as a bit-indexed table from bit 16 onward,
   citing the PDF figure.
3. For discriminated unions, enumerate every variant as its own row set.
4. Cross-check against SDRTrunk or OP25 — if those disagree with your
   reading of the PDF, it's almost always the reading that is wrong.
5. Note any field wider than 16 bits or not aligned to octet boundaries
   explicitly in the table; those are the fields that pseudo-layouts tend
   to lose.
