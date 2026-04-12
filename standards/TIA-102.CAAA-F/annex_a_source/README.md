# TIA-102.CAAA-F Annex A -- Trunking Software Module Listings

This directory contains the normative reference C source code from
**TIA-102.CAAA-F Annex A (normative)**, "Trunking Software Module Listings."

## Authors

- Al Wilson (Motorola) -- BCH, CRC, trellis, and vector operations modules (1993)
- Mike Bright (Motorola) -- parameter structure and main program framework (1996)
- Bob LoGalbo (Motorola) -- TSBK builder, data readers, and I/O routines (1996)
- Jim Holthaus (Motorola) -- contributor

## Purpose

This code generates **P25 trunking control channel test vectors**. It reads
a structured input file describing one or more trunking signalling packets
(TSBKs or unconfirmed/alternate control data units), applies all required
forward error correction encoding, and outputs hex-formatted bit streams
ready for transmission on a CAI Lockdown Platform.

## What Each File Does

| File | Description |
|------|-------------|
| `bch.h` / `bch.c` | BCH(64,16,23) primitive encoder -- encodes the 16-bit Network ID into a 64-bit BCH codeword |
| `crc.h` / `crc.c` | CRC functions: CRC-CCITT (16-bit header parity), CRC-9 (confirmed data block parity), CRC-32 (data parity check) |
| `trellis.h` / `trellis.c` | Rate 3/4 and rate 1/2 trellis encoders with constellation mapping and interleaving per the P25 CAI |
| `vector_operations.h` / `vector_operations.c` | Bit-level vector utilities: right-shift across element boundaries and dibit extraction |
| `parm.h` | Parameter structure definition (`struct parm`) holding all CAI frame fields, plus function prototypes |
| `buildtsb.c` | Main program and all frame-building routines: `build_tsbk_and_print()`, `process_and_print_unconfirmed()`, `process_and_print_alternate_control()`, `find_system_config()`, `find_fs()`, `read_tsbk()`, `data()`, `read_alternate_control()`, `read_unconf_data()`, `main()`, plus I/O helpers `find_value()`, `find_file_value()`, and `read_data_block()` |

## Encoding Algorithms Implemented

- **BCH(64,16,23)** -- Network Identifier (NID) encoding per TIA-102.BAAC
- **CRC-CCITT** -- 16-bit header parity check (generator polynomial x^12 + x^5 + 1)
- **CRC-9** -- 9-bit confirmed data block parity check (generator polynomial x^6 + x^4 + x^3 + 1)
- **CRC-32** -- 32-bit data parity check (generator polynomial 0x04C11DB7)
- **Rate 1/2 trellis encoding** -- used for data blocks and TSBK encoding
- **Rate 3/4 trellis encoding** -- used for voice LDU encoding
- **Interleaving** -- 98-dibit block interleaver per the P25 Common Air Interface

## Building

This is ANSI C code from the early 1990s. It uses K&R-style function definitions
in some places. To compile with a modern compiler:

```sh
gcc -w -o buildtsb buildtsb.c bch.c crc.c trellis.c vector_operations.c
```

The `-w` flag suppresses warnings about the K&R-style declarations and
implicit `int` returns that were standard practice in 1993-era C.

## Notes

- This code was extracted from the PDF of TIA-102.CAAA-F (published 2021-09-14)
  using pdftotext, then manually cleaned to remove PDF watermark artifacts,
  page headers/footers, and page numbers.
- The `buildtsb.c` file is a single compilation unit containing multiple
  logical modules (the main program and all the frame builder/reader functions)
  as presented in the original standard.
- Some functions declared in `parm.h` (e.g., `read_ldu_1`, `print_ldu_1`,
  `encrypt_data_block`) are not defined in this annex -- they are part of the
  larger CAI test vector generation suite defined in other TIA-102.CAAA-F annexes.
