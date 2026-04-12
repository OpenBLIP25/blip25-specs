/******************************************************************************
Name:       buildtsb.c
Author:     Mike Bright, Bob LoGalbo and Al Wilson
Date:       5/5/96
Version:    1.0

This code will compose a trunking signalling message as specified by
TIA102.AABB. One file will be generated that can be transmitted by the
CAI Lockdown Platform.

The input format for this routine is specified by the separate read
functions below. This program accepts as input all data before
FEC/Trellis/CRC encoding. This code will error encode the input data
and order the bits in the sequence specified by the CAI. Virtually all
input data is assumed e.g. the values of each bit of the FRAME SYNC,
UNUSED fields, STATUS SYMBOLS, etc. must be specified.

Upon calling the executable, the first two arguments respectively are
<input_file> and <output_file>. Just invoking the executable without
arguments will cause the program to assume a default
input file of "clear.dat" and stdout as the output file.

This routine is data driven i.e. it reads until the input file is empty
and all formats are keyed off of the DUID.
The only valid DUID's
this module will accept is 0x7 and 0xC. All of the pertinent data
following the "Data Block:" section will be read in, and drives the
formatting process.

The output format is either 18 or 36 hex characters
per line.
The left most bit is the first bit to be transmitted.
All frame syncs will appear in the left most position of each line.
******************************************************************************/
#define IDLE_SS 0X3 /* Idle status symbol value */
int ss_count = 0;
                    /* Global variable that counts the number of status
                       symbols generated over the entire output file
                       generation.*/

/******************************************************************************
Name:       build_tsbk_and_print()
Author:     Bob LoGalbo & Al Wilson
Date:       4/15/96
History:    8/5/93 New
            9/5/93 Fixed exception for zero data blocks.

This function accepts a structure with all of the necessary values to create
TSBK data packets consisting of the frame sync, nid, etc. All of the input
values are raw data before error correction. CRC generation, trellis encoding,
BCH parity generation is done by this module (through subroutine calls.)
Note that any extraneous bits read in and pushed into the structure elements
will be trimmed back if too large to fit into a given field (e.g. only 48
bits allowed for sync, 12 bits allowed for NID, etc.)

The program then generates one complete TSBK with all of the appropriate
signalling. The output is formatted in hexadecimal, and is ready to be
presented to the lockdown platform.
******************************************************************************/

#define DUID_TSBK           0x7     /* Data Unit ID for Data Packet */

#include "bch.h"
#include "crc.h"
#include "trellis.h"
#include <stdio.h>
#include "vector_operations.h"
#include "parm.h"
/* This include file contains the structure whose elements
   compose the various components of CAI frames.
*/

int build_tsbk_and_print(struct parm *data,FILE *out)
{
    int blocks_to_follow;
    long int data_crc;
    int data_length, header_crc;
    int data_octets[528];
    int sarq_crc[34];
    int data_block[15];         /* enough for 1 block + 1 extra          */
    int header_block[12];
    int sarq_block[18];         /* 18 octets with the ser. no. + CRC9    */
    int data_output[150][6];    /* 150microslots, 6 ints of 12 bits per slot*/
    int error_state, i, j, k;
    int fs[4];                  /* Frame Sync                            */
    int nid[5];                 /* Network ID code word                  */
    int microslot, word;
    int total_di_bits;          /* The number of dibits, not counting busy bits */
    int total_microslots;       /* The number of status symbols.         */
    int data_block_di_bit;
    int data_block_number;
    int status_for_this_slot;
    /* data_length    = 16 * blocks_to_follow - 4 - pad_octet_count; */
    /* Initialize data_octets. */
    for(i=0;i<528;i++)
        data_octets[i]=0;
    for(i=0;i<15;i++)
        data_block[i]=0;
    /* Do the Frame Sync */
    for ( i=0; i<3;  i++ )
        fs[i] = (data->fs[i] & 0xFFFF);
    fs[3] = 0;

    /* Do the Status Symbols */
    data->status &= 0x3;

    /* Do the Network ID */
    for ( i=0; i<5; i++ ) nid[i] = 0;
    bch_64_encode ( ((data->nac << 4) & 0xFFF0) |
                    (data->duid & 0xf), nid );

    /* Do the header block */
    header_block[0] = ((data->lb[0] & 0x1) << 7)|((data->p[0]&0x1)<<6)
                      | (data->opcode[0] & 0x3f);
    header_block[1] =   data->mfid[0]     & 0xff;
    header_block[2] =   data->octet2[0]   & 0xff;
    header_block[3] =   data->octet3[0]   & 0xff;
    header_block[4] =   data->octet4[0]   & 0xff;
    header_block[5] =   data->octet5[0]   & 0xff;
    header_block[6] =   data->octet6[0]   & 0xff;
    header_block[7] =   data->octet7[0]   & 0xff;
    header_block[8] =   data->octet8[0]   & 0xff;
    header_block[9] =   data->octet9[0]   & 0xff;
    header_crc = crc_ccitt ( header_block );
    header_block[10] = 0xff & (header_crc >> 8);
    header_block[11] = 0xff & header_crc;
    trellis_1_2_encode ( header_block, data_block );
    k=0;
    for(i=1; i<data->block_count; i++)
    {
        header_block[0] = ((data->lb[i] & 0x1) << 7)
                          | ((data->p[i] & 0x1) << 6) | (data->opcode[i] & 0x3f);
        header_block[1] =   data->mfid[i]     & 0xff;
        header_block[2] =   data->octet2[i]   & 0xff;
        header_block[3] =   data->octet3[i]   & 0xff;
        header_block[4] =   data->octet4[i]   & 0xff;
        header_block[5] =   data->octet5[i]   & 0xff;
        header_block[6] =   data->octet6[i]   & 0xff;
        header_block[7] =   data->octet7[i]   & 0xff;
        header_block[8] =   data->octet8[i]   & 0xff;
        header_block[9] =   data->octet9[i]   & 0xff;
        header_crc = crc_ccitt ( header_block );
        header_block[10] = 0xff & (header_crc >> 8);
        header_block[11] = 0xff & header_crc;
        for ( j=0; j<12; k++,j++ )
            data_octets[k] = header_block[j];
    }

/*  for (i=0;i<150;i++ )for(j=0;j<6;j++) data_output[i][j] = 0;
    for(i=0; i < 12 ;i++)
        printf("h_b[%d] = %02x\n",i,header_block[i]);*/

    /* Get data ready to turn into blocks. */
    data_block_number = 1;

    /* There are 24 frame sync dibits
                 32 NID dibits
                 98 dibits for each block (add 1 for header block)
    */
    /* Total di_bits = fs + nid + header + btf(non term) + term
       (btf should include the enhanced block, if it exists. */
    data_block_di_bit = 98;
    /* number of dibits in array */
    total_di_bits = 24 + 32 + (98 * data->block_count);
    total_microslots = (total_di_bits + 34)/35;
    total_di_bits += total_di_bits/35; /* Allow for embedded busy bits */
    microslot = 0; word = 0;
    for ( i=0; i<36*total_microslots; i++ )
    {
        data_output[microslot][word] = data_output[microslot][word] << 2;
        if ( i%36 == 35 )                  /* Status Symbol              */
        {
            if((ss_count%data->microslots_per_slot == 0)
               && (data->outbound == 1))
                status_for_this_slot = IDLE_SS;
            else
                status_for_this_slot = data->status;
            ss_count++;
            data_output[microslot][word] |= status_for_this_slot;
        }
        else if ( i < 24 )                 /* Frame Sync                */
        {
            data_output[microslot][word] |= extract_dibit(3,16,fs);
        }
        else if ( i < 57 )                 /* Network ID                */
        {
            data_output[microslot][word] |= extract_dibit(4,16,nid);
        }
        else if ( i < total_di_bits )       /* Data Block Unpacking */
        {
            data_output[microslot][word] |= extract_dibit(14,14,data_block);
            data_block_di_bit--;
        }
        else                                /* Trailing nulls            */
        {
            data_output[microslot][word] |= 0;
        }
        /* Data blocks (non-header) */
        if ( data_block_di_bit <= 0 && data_block_number < data->block_count)
        {
            trellis_1_2_encode ( &data_octets[12*(data_block_number-1)],
                                 data_block );
            data_block_di_bit = 98;
            data_block_number++;
        }
        if ( i%6 == 5 ) word++;
        if ( word == 6 ) { microslot++;     word = 0; }
    }
    for ( i=0; i<total_microslots; i++ )
    {
        for ( j=0; j<6; j++ )
            fprintf(out, "%03x", data_output[i][j]);
        if ( i % 2 == 1)
            fprintf(out, "\n");
    }
    if ( (i-1) % 2 == 0)
        fprintf(out, "\n");
    return(0);
}

/******************************************************************************
Name:       process_and_print_unconfirmed()
Author:     Bob LoGalbo & Al Wilson
Date:       4/15/96
History:    8/5/93 New
            9/5/93 Fixed exception for zero data blocks.

This function accepts a structure with all of the necessary values to create
unconfirmed data packets consisting of the frame sync, nid, etc. All of the
input values are raw data before error correction. CRC generation, trellis
encoding, BCH parity generation is done by this module
(through calls to various subroutines).

Note that any extraneous bits read in and pushed into the structure elements
will be trimmed back if too large to fit into a given field (e.g. only 48 bits
allowed for sync, 12 bits allowed for NID, etc.)

The program then generates one complete unconfirmed control packet with all
of the appropriate signalling. The output is formatted in hexadecimal, and is
ready to be presented to the lockdown platform.
******************************************************************************/

#define DUID_DATA_UNCONF    0xc     /* Data Unit ID for Data Packet */

#include "bch.h"
#include "crc.h"
#include "trellis.h"
#include <stdio.h>
#include "vector_operations.h"
#include "parm.h"/* This include file contains the structure whose
                    elements compose the various components of CAI frames.*/

int process_and_print_unconfirmed(FILE *out,struct parm *data)
{
    int blocks_to_follow;
    long int data_crc;
    int data_length, header_crc;
    int data_octets[528];
    int sarq_crc[34];
    int data_block[15];         /* enough for 1 block + 1 extra          */
    int header_block[12];
    int sarq_block[18];         /* 18 octets with the ser. no. + CRC9    */
    int data_output[150][6];    /* 150microslots, 6 ints of 12 bits per slot*/
    int error_state, i, j, k;
    int fs[4];                  /* Frame Sync                            */
    int nid[5];                 /* Network ID code word                  */
    int microslot, word;
    int total_di_bits;          /* The number of dibits, not counting busy bits */
    int total_microslots;       /* The number of status symbols.         */
    int data_block_di_bit;
    int data_block_number;
    int status_for_this_slot;
    /* data_length    = 16 * blocks_to_follow - 4 - pad_octet_count; */

    /* Initialize data_octets. */
    for(i=0;i<528;i++)
        data_octets[i]=0;
    for(i=0;i<15;i++)
        data_block[i]=0;
    /* Do the Frame Sync */
    for ( i=0; i<3;  i++ )
        fs[i] = (data->fs[i] & 0xFFFF);
    fs[3] = 0;

    /* Do the Status Symbols */
    data->status &= 0x3;

    /* Do the Network ID */
    for ( i=0; i<5; i++ ) nid[i] = 0;
    bch_64_encode ( ((data->nac << 4) & 0xFFF0) |
                    (data->duid & 0xf), nid );

    /* Do the header block */
    header_block[0] = ((data->unused0 & 0x1) << 7) | ((data->anb & 0x1) << 6)
                      | ((data->ibo & 0x1) << 5) | (data->format & 0x1f);
    header_block[1] = ((data->unused1 & 0x3) << 6) | (data->sap & 0x3f);
    header_block[2] =   (data->mfid[0] & 0xff);
    header_block[3] =   (data->dest[0] & 0xFF);
    header_block[4] = ((data->dest[1] & 0xFF00)>>8);
    header_block[5] =   (data->dest[1] & 0xFF);
    header_block[6] = ((data->fmf     & 0x1) << 7) |
                       (data->btf & 0x7f);
    header_block[7] =  ((data->unused7 & 0x7) << 5) |(data->pad & 0x1f);
    header_block[8] =   (data->reserved[0] & 0xff);
    header_block[9] = ((data->unused9 & 0x3) << 6) |
                       (data->offset & 0x3f);
    header_crc = crc_ccitt ( header_block );
    header_block[10] = 0xff & (header_crc >> 8);
    header_block[11] = 0xff & header_crc;
    trellis_1_2_encode ( header_block, data_block );
    i=0;
    if(data->sap == 0x1f) /* If enhanced addressing push the LLID, SAP ID,
                             etc. into data array */
    {
        header_block[0] =   0x15; /* The format for the intermediate file
                                     does not allow the user to control
                                     the bits in the first octet of the
                                     second header. */
        header_block[1] = ((data->eh_unused & 0x3) << 6)
                          | (data->eh_2nd_sap & 0x3f);
        header_block[2] =   data->mfid[0]     & 0xff;
        header_block[3] =   data->source[0] & 0xFF;
        header_block[4] = ( data->source[1] & 0xFF00)>>8;
        header_block[5] =   data->source[1] & 0xFF;
        header_block[6] = ( data->eh_reserved[0] & 0xFF00)>>8;
        header_block[7] =   data->eh_reserved[0] & 0xFF;
        header_block[8] = ( data->eh_reserved[1] & 0xFF00)>>8;
        header_block[9] =   data->eh_reserved[1] & 0xFF;
        header_crc = crc_ccitt ( header_block );
        header_block[10] = 0xff & (header_crc >> 8);
        header_block[11] = 0xff & header_crc;
        for ( j=0; j<12; i++,j++ )
            data_octets[i] = header_block[j];
    }
    for ( j=0; j<data->nbytes; i++,j++ )
    {
        data_octets[i]   = data->data[j] & 0xff;
    /*  printf("data_octet[%d] = %02x\n",i,data_octets[i]);*/
    }
    if(data->sap == 0x1f)
        data->nbytes+=12;
    if(i%12 > 8)/* i = number of data octets that will be packed
                    and framed up */
        blocks_to_follow = i/12+2;
    else                        /* data_in_last_block <=12 */
        blocks_to_follow = i/12+1;
    for ( i=0; i<150; i++ ) for ( j=0; j<6; j++ ) data_output[i][j] = 0;
/*  for(i=0; i < 12 ;i++)
        printf("h_b[%d] = %02x\n",i,header_block[i]);*/
    /* Get data ready to turn into blocks. */
    if ( data->nbytes > 0 )
    {
        data_crc = crc_32 ( data->nbytes, data_octets );
        for ( i=data->nbytes+3; i>=data->nbytes; i--)
        {
            data_octets[i] = 0xff & data_crc;
            /*printf("data_crc[%d] = %02x\n",i,data_octets[i]);*/
            data_crc = data_crc >> 8;
        }
    }
    data_block_number = 0;

    /* There are 24 frame sync dibits
                 32 NID dibits
                 98 dibits for each block (add 1 for header block)
    */
    /* Total di_bits = fs + nid + header + btf(non term) + term
       (btf should include the enhanced block, if it exists. */
    data_block_di_bit = 98;
    /* number of dibits in array */
    total_di_bits = 24 + 32 + 98 * (1 + blocks_to_follow);
    total_microslots = (total_di_bits + 34)/35;
    total_di_bits += total_di_bits/35; /* Allow for embedded busy bits */
    microslot = 0; word = 0;
    for ( i=0; i<36*total_microslots; i++ )
    {
        data_output[microslot][word] = data_output[microslot][word] << 2;
        if ( i%36 == 35 )                  /* Status Symbol              */
        {
            if((ss_count%data->microslots_per_slot == 0)
               && (data->outbound == 1))
                status_for_this_slot = IDLE_SS;
            else
                status_for_this_slot = data->status;
            ss_count++;
            data_output[microslot][word] |= status_for_this_slot;
        }
        else if ( i < 24 )                 /* Frame Sync                */
        {
            data_output[microslot][word] |= extract_dibit(3,16,fs);
        }
        else if ( i < 57 )                 /* Network ID                */
        {
            data_output[microslot][word] |= extract_dibit(4,16,nid);
        }
        else if ( i < total_di_bits )       /* Data Block Unpacking */
        {
            data_output[microslot][word] |= extract_dibit(14,14,data_block);
            data_block_di_bit--;
        }
        else                                /* Trailing nulls            */
        {
            data_output[microslot][word] |= 0;
        }
        if ( data_block_di_bit <= 0 && data_block_number < blocks_to_follow)
        {
            trellis_1_2_encode ( &data_octets[12*data_block_number],
                                 data_block );
            data_block_di_bit = 98;
            data_block_number++;
        }
        if ( i%6 == 5 ) word++;
        if ( word == 6 ) { microslot++; word = 0; }
    }
    for ( i=0; i<total_microslots; i++ )
    {
        for ( j=0; j<6; j++ )
            fprintf(out, "%03x", data_output[i][j]);
        if ( i % 2 == 1)
            fprintf(out, "\n");
    }
    if ( (i-1) % 2 == 0)
        fprintf(out, "\n");
    return(0);
}

/******************************************************************************
Name:       process_and_print_alternate_control.c
Author:     Bob LoGalbo & Al Wilson
Date:       4/15/96
History:    8/5/93 New
            9/5/93 Fixed exception for zero data blocks.

This function accepts a structure with all of the necessary values to create
alternate control data packets consisting of the frame sync, nid, etc. All of
the input values are raw data before error correction. CRC generation, trellis
encoding, BCH parity generation is done by this module (through calls to various
subroutines).

Note that any extraneous bits read in and pushed into the structure elements
will be trimmed back if too large to fit into a given field (e.g. only 48 bits
allowed for sync, 12 bits allowed for NID, etc.)

The program then generates one complete alternate control data packet with all
of the appropriate signalling. The output is formatted in hexadecimal, and is
ready to be presented to the lockdown platform.
******************************************************************************/

#include "bch.h"
#include "crc.h"
#include "trellis.h"
#include <stdio.h>
#include "vector_operations.h"
#include "parm.h"/* This include file contains the structure whose elements
                    compose the various components of CAI frames.
*/

#define DUID_DATA_UNCONF    0xc     /* Data Unit ID for Data Packet */

int process_and_print_alternate_control(FILE *out,struct parm *data)
{
    int blocks_to_follow;
    long int data_crc;
    int data_length, header_crc;
    int data_octets[528];
    int sarq_crc[34];
    int data_block[15];         /* enough for 1 block + 1 extra          */
    int header_block[12];
    int sarq_block[18];         /* 18 octets with the ser. no. + CRC9    */
    int data_output[150][6];    /* 150microslots, 6 ints of 12 bits per slot */
    int error_state, i, j, k;
    int fs[4];                  /* Frame Sync                            */
    int nid[5];                 /* Network ID code word                  */
    int microslot, word;
    int total_di_bits;          /* The number of dibits, not counting busy bits */
    int total_microslots;       /* The number of status symbols.         */
    int data_block_di_bit;
    int data_block_number;
    int status_for_this_slot;
    /* data_length    = 16 * blocks_to_follow - 4 - pad_octet_count; */
    /* Initialize data_octets. */
    for(i=0;i<528;i++)
        data_octets[i]=0;
    for(i=0;i<15;i++)
        data_block[i]=0;
    /* Do the Frame Sync */
    for ( i=0; i<3;  i++ )
        fs[i] = (data->fs[i] & 0xFFFF);
    fs[3] = 0;

    /* Do the Status Symbols */
    data->status &= 0x3;

    /* Do the Network ID */
    for ( i=0; i<5; i++ ) nid[i] = 0;
    bch_64_encode ( ((data->nac << 4) & 0xFFF0) |
                    (data->duid & 0xf), nid );

    /* Do the header block */
    header_block[0] = ((data->unused0 & 0x1) << 7)|((data->anb&0x1)<<6)
                      | ((data->ibo & 0x1) << 5) | data->format & 0x1f;
    header_block[1] = ((data->unused1 & 0x3) << 6) |(data->sap & 0x3f);
    header_block[2] =   data->mfid[0] & 0xff;
    header_block[3] =   data->dest[0] & 0xFF;
    header_block[4] = ( data->dest[1] & 0xFF00)>>8;
    header_block[5] =   data->dest[1] & 0xFF;
    header_block[6] = ((data->fmf     & 0x1) << 7)|data->btf & 0x7f;
    header_block[7] = ((data->unused7 & 0x3)<<6)|data->opcode[0]&0x3f;
    header_block[8] =   data->tm1 & 0xff;
    header_block[9] =   data->tm2 & 0xff;
    header_crc = crc_ccitt ( header_block );
    header_block[10] = 0xff & (header_crc >> 8);
    header_block[11] = 0xff & header_crc;
    trellis_1_2_encode ( header_block, data_block );
    i=0;
    if(data->sap == 0x1f) /* If enhanced addressing push the LLID,
                             SAP ID, etc. into data array */
    {
        header_block[0] =   0x15; /*The format for the intermediate file
                                    does not allow the user to control
                                    the bits in the first octet of the
                                    second header. */
        header_block[1] = ((data->eh_unused & 0x3) << 6)
                          | (data->eh_2nd_sap & 0x3f);
        header_block[2] =   data->mfid[0]     & 0xff;
        header_block[3] =   data->source[0] & 0xFF;
        header_block[4] = ( data->source[1] & 0xFF00)>>8;
        header_block[5] =   data->source[1] & 0xFF;
        header_block[6] = ( data->eh_reserved[0] & 0xFF00)>>8;
        header_block[7] =   data->eh_reserved[0] & 0xFF;
        header_block[8] = ( data->eh_reserved[1] & 0xFF00)>>8;
        header_block[9] =   data->eh_reserved[1] & 0xFF;
        header_crc = crc_ccitt ( header_block );
        header_block[10] = 0xff & (header_crc >> 8);
        header_block[11] = 0xff & header_crc;
        for ( j=0; j<12; i++,j++ )
            data_octets[i] = header_block[j];
    }
    for ( j=0; j<data->nbytes; i++,j++ )
    {
        data_octets[i]   = data->data[j] & 0xff;
    /*  printf("data_octet[%d] = %02x\n",i,data_octets[i]);*/
    }
    if(data->sap == 0x1f)
        data->nbytes+=12;
    if(i%12 > 8) /* i = number of data octets that will be packed
                     and framed up */
        blocks_to_follow = i/12+2;
    else                        /* data_in_last_block <=12 */
        blocks_to_follow = i/12+1;
    for ( i=0; i<150; i++ )for(j=0;j<6;j++)data_output[i][j] = 0;
/*  for(i=0; i < 12 ;i++)
        printf("h_b[%d] = %02x\n",i,header_block[i]);*/
    /* Get data ready to turn into blocks. */
    if ( data->nbytes > 0 )
    {
        data_crc = crc_32 ( data->nbytes, data_octets );
        for ( i=data->nbytes+3; i>=data->nbytes; i--)
        {
            data_octets[i] = 0xff & data_crc;
            /*printf("data_crc[%d] = %02x\n",i,data_octets[i]);*/
            data_crc = data_crc >> 8;
        }
    }
    data_block_number = 0;

    /* There are 24 frame sync dibits
                 32 NID dibits
                 98 dibits for each block (add 1 for header block)
    */
    /* Total di_bits = fs + nid + header + btf(non term) + term
       (btf should include the enhanced block, if it exists. */
    data_block_di_bit = 98;
    /* number of dibits in array */
    total_di_bits = 24 + 32 + 98 * (1 + blocks_to_follow);
    total_microslots = (total_di_bits + 34)/35;
    total_di_bits += total_di_bits/35;/*Allow for embedded busy bits*/
    microslot = 0; word = 0;
    for ( i=0; i<36*total_microslots; i++ )
    {
        data_output[microslot][word]=data_output[microslot][word]<<2;
        if ( i%36 == 35 )                  /* Status Symbol              */
        {
            if((ss_count%data->microslots_per_slot == 0)
               && (data->outbound == 1))
                status_for_this_slot = IDLE_SS;
            else
                status_for_this_slot = data->status;
            ss_count++;
            data_output[microslot][word] |= status_for_this_slot;
        }
        else if ( i < 24 )                 /* Frame Sync                */
        {
            data_output[microslot][word] |= extract_dibit(3,16,fs);
        }
        else if ( i < 57 )                 /* Network ID                */
        {
            data_output[microslot][word] |= extract_dibit(4,16,nid);
        }
        else if ( i < total_di_bits )       /* Data Block Unpacking      */
        {
            data_output[microslot][word] |= extract_dibit(14,14,data_block);
            data_block_di_bit--;
        }
        else                                /* Trailing nulls            */
        {
            data_output[microslot][word] |= 0;
        } /* Data blocks (non-header) */
        if ( data_block_di_bit <= 0 && data_block_number<blocks_to_follow)
        {
            trellis_1_2_encode ( &data_octets[12*data_block_number],
                                 data_block );
            data_block_di_bit = 98;
            data_block_number++;
        }
        if ( i%6 == 5 ) word++;
        if ( word == 6 ) { microslot++; word = 0; }
    }
    for ( i=0; i<total_microslots; i++ )
    {
        for ( j=0; j<6; j++ )
            fprintf(out, "%03x", data_output[i][j]);
        if ( i % 2 == 1)
            fprintf(out, "\n");
    }
    if ( (i-1) % 2 == 0)
        fprintf(out, "\n");
    return(0);
}

/******************************************************************************
Name:       find_system_config()
Author:     Bob LoGalbo
Date:       5/1/96

This routine will find the initial parameters
OUTBOUND and MICROSLOTS PER SLOT.
*****************************************************************************/
int find_system_config(infp,pp)
FILE *infp;
struct parm *pp;
{
    int dest_buff[4];           /* temporary buffer */
    char inbuff[256];           /* character input buffer */

    while (fgets(inbuff,256,infp) != NULL)
    {
        if (find_value("OUTBOUND",inbuff,1,&pp->outbound) != NULL);
        else if (find_value("MICROSLOTS PER SLOT",inbuff,1,&pp->microslots_per_slot)
                 != NULL)
            return(0);
    }
    return(-300);
}

/******************************************************************************
Name:       find_fs()
Author:     Bob LoGalbo
Date:       5/1/96

This routine will find the Frame Sync.
*****************************************************************************/
int find_fs(infp,pp)
FILE *infp;
struct parm *pp;
{
    int dest_buff[4];           /* temporary buffer */
    char inbuff[256];           /* character input buffer */

    while (fgets(inbuff,256,infp) != NULL)
    {
        if (find_value("FS",inbuff,3,pp->fs) != NULL) return(0);
    }
    return(-200);
}

/******************************************************************************
Name:       read_tsbk()

This function will read in the values necessary to create TSBKs. This function
accepts as input hexadecimal values.
The packet values must be presented with
the following labels and in the following order:

SS        : 1
TSBK #1:
OCTET 0 : 15
              LB = 0; P = 0; Opcode = 35
MFID      : df
OCTET 2 : 00
OCTET 3 : 00
OCTET 4 : 00
OCTET 5 : 01
OCTET 6 : 82
OCTET 7 : 00
OCTET 8 : 00
OCTET 9 : 00
TSBK #2:
OCTET 0 : 15
              LB = 0; P = 0; Opcode = 15
MFID      : c0
OCTET 2 : 00
OCTET 3 : 00
OCTET 4 : 00
OCTET 5 : 02
OCTET 6 : 00
OCTET 7 : 00
OCTET 8 : 00
OCTET 9 : 00
TSBK #3:
OCTET 0 : 15
              LB = 1; P = 0; Opcode = 01
MFID      : 23
OCTET 2 : 45
OCTET 3 : 67
OCTET 4 : 89
OCTET 5 : ab
OCTET 6 : cd
OCTET 7 : ef
OCTET 8 : 00
OCTET 9 : 00

All values should be right justified. (e.g. all SS fields should
have a value between 0 to 3 inclusive, because any higher value will
have the MSB's truncated, etc.)
******************************************************************************/
int read_tsbk(infp,pp)
FILE *infp;             /* Input file pointer */
struct parm *pp;        /* parameter packet */
{
    int tmp;
    char inbuff[256];       /* line input buffer */
    char *sp;               /* pointer to next char in line input string */
    int data_cnt;
    int block_count=0;
    char *store_sp; /* save the string pntr in case of error on missing field */

    if (find_file_value(infp,"SS",inbuff,1,&pp->status) == NULL) return(-100);
    do{
        if (fgets(inbuff,256,infp) == NULL) return(-101);
        /* Header line */
        if ((sp = find_file_value(infp,"OCTET 0",inbuff,1,&pp->ignored)) == NULL)
            return(-102);
        /* Read unused bits */
        if ((sp = find_value("LB",sp,1,&pp->lb[block_count])) == NULL)
            return(-103);
        if ((sp = find_value("P",sp,1,&pp->p[block_count])) == NULL)
            return(-104);
        if (find_value("Opcode",sp,1,&pp->opcode[block_count]) == NULL)
            return(-105);
        if (find_file_value(infp,"MFID",inbuff,1,&pp->mfid[block_count]) == NULL)
            return(-106);
        if ((find_file_value(infp,"OCTET 2",inbuff,1,&pp->octet2[block_count]))==NULL)
            return(-107);
        if ((find_file_value(infp,"OCTET 3",inbuff,1,&pp->octet3[block_count]))==NULL)
            return(-108);
        if ((find_file_value(infp,"OCTET 4",inbuff,1,&pp->octet4[block_count]))==NULL)
            return(-109);
        if ((find_file_value(infp,"OCTET 5",inbuff,1,&pp->octet5[block_count]))==NULL)
            return(-110);
        if ((find_file_value(infp,"OCTET 6",inbuff,1,&pp->octet6[block_count]))==NULL)
            return(-111);
        if ((find_file_value(infp,"OCTET 7",inbuff,1,&pp->octet7[block_count]))==NULL)
            return(-112);
        if ((find_file_value(infp,"OCTET 8",inbuff,1,&pp->octet8[block_count]))==NULL)
            return(-113);
        if ((find_file_value(infp,"OCTET 9",inbuff,1,&pp->octet9[block_count]))==NULL)
            return(-114);
    } while(!pp->lb[block_count++]);
    pp->block_count = block_count ;
    return(0);
}

/******************************************************************************
Name:       data()

This function will read in the values to determine the data format as well as
other header field values. A case statement driven off of the Format field will
call the format specific routines to finish reading in the data packet, process
them, and print them out.
Note the value following OCTET 0 is ignored, and only the Unused, ANb, IbO and
Format are read in. This function accepts as input hexadecimal values, in the
following order:

SS       : x
xxxonfirmed Header Block
OCTET 0 : xx
              Unused = x; ANb = x; IbO = x; Format = xx

x is a hex number and should have values right justified.
(e.g. all SS fields should have a value between 0 to 3 inclusive, because
any higher value will have the MSB's truncated, etc.)

NOTE: The following field values must precede the SS line above:
Packet Data Unit - 0
FS        : 5575 F5FF 77FF
NID       : 293C
              NAC = 293; DUID = C
These 2 lines were read in by the Main() function below.
******************************************************************************/

int data(infp,outfp,pp)
FILE *infp;             /* Input file pointer */
FILE *outfp;
struct parm *pp;        /* parameter packet */
{
    int tmp;
    char inbuff[256];       /* line input buffer */
    char *sp;               /* pointer to next char in line input string */
    int data_cnt;
    char *store_sp;/* save the string pntr in case of error on missing field */
    if (find_file_value(infp,"SS",inbuff,1,&pp->status) == NULL) return(-100);
    if (fgets(inbuff,256,infp) == NULL) return(-101);
    /* Header line */
    if ((sp = find_file_value(infp,"OCTET 0",inbuff,1,&pp->ignored)) == NULL)
        return(-102);
    /* Read unused bits */
    if ((sp = find_value("Unused",sp,1,&pp->unused0)) == NULL) return(-103);
    /* Read confirmation flag if conf or unconf data */
    store_sp = sp;
    if ((sp = find_value("ANb",sp,1,&pp->anb)) == NULL)
    {
        sp = store_sp;
        store_sp = NULL;
    }
    if ((sp = find_value("IbO",sp,1,&pp->ibo)) == NULL) return(-104);
    if (find_value("Format",sp,1,&pp->format) == NULL) return(-105);

    switch(pp->format)
    {
    case 0x17:
        /* Confirmed Data */
        tmp=read_alternate_control(infp,pp);
        if( tmp < 0)
            printf("e =%d Invalid Alternate Conrol Data File. \n\n",tmp);
        else
        {
            process_and_print_alternate_control(outfp,pp);
        }
        break;
    case 0x15:
        /* Unconfirmed Data */
        tmp=read_unconf_data(infp,pp);
        if( tmp < 0)
            printf("e = %dInvalid Unconfirmed Data File. \n\n",tmp);
        else
            process_and_print_unconfirmed(outfp,pp);
        break;
    default:
        printf("\nInvalid Data Format Specified. \n\n");
        break;
    }
    return(0);
}

/******************************************************************************
Name:       read_alternate_control()

This function will read in the values necessary to create alternate control
data packets. This function accepts as input hexadecimal values. The packet
values must be presented with the following labels and in the following order:

SS       : 1
Multiple Trunked Block OSP:
OCTET 0 : 37
              Unused = 0; ANb = 0; IbO = 1; Format = 17
OCTET 1 : c0
              Unused = 3; SAP = 00
MFID     : 00
LLID     : 00 0001
OCTET 6 : 83
              FMF = 1; Blocks to Follow = 02
OCTET 7 : 0b
              Unused = 0; Opcode = 08
OCTET 8 : 00
              Trunked Message 1 = 00
OCTET 9 : 00
              Trunked Message 2 = 02
Data Block:
00 01 02 03 04 05 06 07 08 09 0a 0b 00 00 00 00 00 00 00 00
#
All values should be right justified.
(e.g. all SS fields should have a value between 0 to 3
inclusive, because any higher value will have the MSB's truncated,
etc.)
******************************************************************************/
int read_alternate_control(infp,pp)
FILE *infp;             /* Input file pointer */
struct parm *pp;        /* parameter packet */
{
    char inbuff[256];       /* line input buffer */
    char *sp;               /* pointer to next char in line input string */

    if ((sp = find_file_value(infp,"OCTET 1",inbuff,1,&pp->ignored)) == NULL)
        return(-1);
    if ((sp = find_value("Unused",sp,1,&pp->unused1)) == NULL)
        return(-2);
    if (find_value("SAP",sp,1,&pp->sap) == NULL) return(-3);
    if (find_file_value(infp,"MFID",inbuff,1,&pp->mfid[0]) == NULL) return(-4);
    if (find_file_value(infp,"LLID",inbuff,2,pp->dest) == NULL) return(-5);
    if ((sp = find_file_value(infp,"OCTET 6",inbuff,1,&pp->ignored)) == NULL)
        return(-6);
    if ((sp = find_value("FMF",sp,1,&pp->fmf)) == NULL) return(-7);
    if (find_value("Blocks to Follow",sp,1,&pp->btf) == NULL) return(-8);
    if ((sp = find_file_value(infp,"OCTET 7",inbuff,1,&pp->ignored)) == NULL)
        return(-9);
    if ((sp = find_value("Unused",sp,1,&pp->unused7)) == NULL) return(-10);
    if (find_value("Opcode",sp,1,&pp->opcode[0]) == NULL) return(-11);
    if ((sp = find_file_value(infp,"OCTET 8",inbuff,1,&pp->ignored)) == NULL)
        return(-12.1);
    if ((find_value("Trunked Message 1",sp,1,&pp->tm1)) == NULL) return(-12.2);
    if ((sp = find_file_value(infp,"OCTET 9",inbuff,1,&pp->ignored)) == NULL)
        return(-13);
    if ((find_value("Trunked Message 2",sp,1,&pp->tm2)) == NULL) return(-14);
    if (fgets(inbuff,256,infp) == NULL) return(-1);
    /* Header line */

    /* Read in the data block */
    if (read_data_block(infp,pp) == -1) return(-31);

    return(0);
}

/******************************************************************************
Name:       read_unconf_data()

This function will read in the values necessary to create unconfirmed data
packets. This function accepts as input hexadecimal values.
The packet values
must be presented with the following labels and in the following order:

OCTET 1 : DF
              Unused = 3; SAP = 1F
MFID     : 00
LLID     : 00 0001
OCTET 6 : 84
              FMF = 1; Blocks to Follow = 04
OCTET 7 : 06
              Unused = 0; Pad = 06
Reserved: 00
OCTET 9 : 02
              Unused = 0; Offset = 02
Data Block:
55 F9 91 A9 78 EB 9A 54 00 81 00 01 E0
B7 AD 98 26 3A 43 EF AC 2E 39 0C 8C 32 74 FD 6E 88 E1 6D
#
All values should be right justified. (e.g. all SS fields should
have a value between 0 to 3 inclusive, because any higher value will
have the MSB's truncated, etc.)
******************************************************************************/
int read_unconf_data(infp,pp)
FILE *infp;             /* Input file pointer */
struct parm *pp;        /* parameter packet */
{
    char inbuff[256];       /* line input buffer */
    char *sp;               /* pointer to next char in line input string */

    if ((sp = find_file_value(infp,"OCTET 1",inbuff,1,&pp->ignored)) == NULL)
        return(-1);
    if ((sp = find_value("Unused",sp,1,&pp->unused1)) == NULL) return(-2);
    if (find_value("SAP",sp,1,&pp->sap) == NULL) return(-3);
    if (find_file_value(infp,"MFID",inbuff,1,&pp->mfid[0]) == NULL) return(-4);
    if (find_file_value(infp,"LLID",inbuff,2,pp->dest) == NULL) return(-5);
    if ((sp = find_file_value(infp,"OCTET 6",inbuff,1,&pp->ignored)) == NULL)
        return(-6);
    if ((sp = find_value("FMF",sp,1,&pp->fmf)) == NULL) return(-7);
    if (find_value("Blocks to Follow",sp,1,&pp->btf) == NULL) return(-8);
    if ((sp = find_file_value(infp,"OCTET 7",inbuff,1,&pp->ignored)) == NULL)
        return(-9);
    if ((sp = find_value("Unused",sp,1,&pp->unused7)) == NULL) return(-10);
    if (find_value("Pad",sp,1,&pp->pad) == NULL) return(-11);
    if (find_file_value(infp,"Reserved",inbuff,1,pp->reserved) == NULL)
        return(-12);
    if ((sp = find_file_value(infp,"OCTET 9",inbuff,1,&pp->ignored)) == NULL)
        return(-13);
    if ((sp = find_value("Unused",sp,1,&pp->unused9)) == NULL) return(-14);
    if (find_value("Offset",sp,1,&pp->offset) == NULL) return(-15);
    /* Read Enhanced Addressing header if SAP = 0x1F */
    if (pp->sap == 0x1F)
    {
        if (fgets(inbuff,256,infp) == NULL) return(-17); /* Header line */
        if ((sp = find_file_value(infp,"2ND SAP",inbuff,1,&pp->ignored))==NULL)
            return(-18);
        if ((sp = find_value("Unused",sp,1,&pp->eh_unused)) == NULL)
            return(-19);
        if (find_value("SAP",sp,1,&pp->eh_2nd_sap) == NULL) return(-20);
        if (find_file_value(infp,"SOURCE",inbuff,2,pp->source) == NULL)
            return(-21);
        if (find_file_value(infp,"RESERVED",inbuff,2,pp->eh_reserved) == NULL)
            return(-22);
    }
    if (fgets(inbuff,256,infp) == NULL) return(-1);

    /* Header line */

    /* Read in the data block */
    if (read_data_block(infp,pp) == -1) return(-31);

    return(0);
}

/******************************************************************************
Name:       main()
Date:       3/6/96

The switch statement will switch off of the DUID just read in, then pick the
case statement to read in, error encode, order and print the frames/packets.
It will then go onto the next info block and read in the subsequent DUID.
The routine will continue to read in data until the input file is exhausted.
The main routine reads in the following fields that drive the switch
statement below:

FS       : 5575 F5FF 77FF
NID      : xxxx
              NAC = xxx; DUID = x
*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parm.h" /* This include file contains the structure whose
                     elements compose the various components of CAI frames.*/

void main(argc,argv)
int argc;
char *argv[];
{
    FILE *infp;             /* Input file pointer */
    FILE *outfp;            /* Output file pointer */
    struct parm p,*pp;      /* parameter packet */
    char inbuff[256];       /* line input buffer */
    char *sp;               /* pointer to next char in line input string */
    int i;                  /* loop counter */
    int debug_var;

    /* Initialize parameters */
    pp = &p;

    /* GET INPUT FILE */
    strcpy(inbuff,argv[1]);
    if (inbuff[0] == '\0')
    {
        printf("Default input <clear.dat> assumed; no input file specified.\n");
        strcpy(inbuff,"clear.dat");
    }
    while((infp = fopen(inbuff,"r")) == NULL)
    {
        printf("Input file %s does not exist.\n",inbuff);
        printf("\nEnter input file name: ");
        fgets(inbuff,256,stdin);
    }
    /* GET OUTPUT FILE */
    strcpy(inbuff,argv[2]);
    if (inbuff[0] == '\0') outfp = stdout;
    else if ((outfp = fopen(inbuff,"w")) == NULL)
    {
        printf("Output file %s cannot be opened.\nSTDOUT by default.\n\n",inbuff);
        outfp = stdout;
    }
    /* Determine the system configuration */
    if (find_system_config(infp,pp) == -1) exit(-1);

    /* Continue reading input driven by DUID until file is exhausted. */
    while (feof(infp) == 0)
    {
        if (find_fs(infp,pp) == -1) exit(-1);
        if ((sp = find_file_value(infp,"NID",inbuff,1,&pp->ignored)) == NULL)
            exit(-1);
        if ((sp = find_value("NAC",sp,1,&pp->nac)) == NULL) exit(-1);
        if (find_value("DUID",sp,1,&pp->duid) == NULL) exit(-1);
        switch(pp->duid)
        {
        case 0x7:
            debug_var = read_tsbk(infp,pp);
            if( debug_var <= -1)
                printf("e= %d ***Invalid TSBK File.******\n\n",debug_var);
            else
            {
                build_tsbk_and_print(pp,outfp);
            }
            break;
        case 0xC:
            debug_var = data(infp,outfp,pp);
            if( debug_var <= -1)
                printf("e= %d Invalid Data Header File.*\n\n",debug_var);
            break;
        default:
            printf("*Undefined DUID (%X) specified!**\n\n",pp->duid);
            break;
        }
    }
    fclose(infp);
    fclose(outfp);
}

char *find_file_value(infp,pattern,source,number,dest)
FILE *infp;             /* Input file pointer */
char *pattern;          /* Character string to be found */
char *source;           /* pointer to the input string */
int number;             /* number of integer values to be returned */
int *dest;              /* destination array of integers */
{
    if (fgets(source,256,infp) == NULL) return(NULL);
    return (find_value(pattern,source,number,dest));
}

char *find_value(pattern,source,number,dest)
char *pattern;          /* Character string to be found */
char *source;           /* pointer to the input string */
int number;             /* number of integer values to be returned */
int *dest;              /* destination array of integers */
{
    int i;              /* loop counter */
    char *eol;          /* end of line pointer */

    i = 0;
    eol = strchr(source,'\0');
    while(isspace(*source)) source++;
    if (pattern != NULL)
        if (strncmp(pattern,source,(int)strlen(pattern)) != 0) return(NULL);
    source = strtok(source,":=");
    for (i = 0; i < number; ++i)
    {
        source = strtok(NULL," \n\0");
        sscanf(source,"%x",&*dest++);
    }
    source = strchr(source,'\0');
    if((unsigned char*)source < (unsigned char*)eol) source++;
    return(source);
}

/************************ DATA UTILITIES ************************/

/* READ DATA BLOCK */
int read_data_block(infp,pp)
FILE *infp;             /* Input file pointer */
struct parm *pp;        /* parameter packet */
{
    char inbuff[256];       /* line input buffer */
    char *sp;               /* pointer to next char in line input string */
    int temp;               /* number conversion buffer for sscanf */

    /* Input the data */
    pp->nbytes = 0;
    while(fgets(inbuff,256,infp) != NULL)
    {
        if ((inbuff[0] == '#') || (inbuff[0] == '\0')) break;
        sp = inbuff;
        while(isspace(*sp)) sp++;
        sp = strtok(sp," \t\n\0");
        while(sp != NULL)
        {
            sscanf(sp,"%x",&temp);
            pp->data[pp->nbytes++] = temp & 0xFF;
            if (pp->nbytes >= 545) return(-1);
            sp = strtok(NULL," \t\n");
        }
    }
    return(0);
}
