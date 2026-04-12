/******************************************************************************
Name:       trellis.c
Author:     Al Wilson
Date:       8/5/93
History:    8/5/93 New

This module defines two functions:
    trellis_3_4_encode ( in, out )
        encodes the rate 3/4 trellis code
    trellis_1_2_encode ( in, out )
        encodes the rate 1/2 trellis code
The input and output arguments are pointers a integer vectors. The input
arguments must point to octets while the output arguments point to an array
of 14 integers with 14 bits in each (196 bits in total).
******************************************************************************/
#include <stdio.h>
#include "trellis.h"
#include "vector_operations.h"

static int sigIndx_3_4[8][8] = {
    0, 8, 4, 12, 2, 10, 6, 14,
    4, 12, 2, 10, 6, 14, 0, 8,
    1, 9, 5, 13, 3, 11, 7, 15,
    5, 13, 3, 11, 7, 15, 1, 9,
    3, 11, 7, 15, 1, 9, 5, 13,
    7, 15, 1, 9, 5, 13, 3, 11,
    2, 10, 6, 14, 0, 8, 4, 12,
    6, 14, 0, 8, 4, 12, 2, 10};

static int sigIndx_1_2[4][4] = {
    0, 15, 12, 3,
    4, 11, 8, 7,
    13, 2, 1, 14,
    9, 6, 5, 10};

static int signalP[16][2] = {
    0, 2, 2, 2,
    1, 3, 3, 3,
    3, 2, 1, 2,
    2, 3, 0, 3,
    3, 1, 1, 1,
    2, 0, 0, 0,
    0, 1, 2, 1,
    1, 0, 3, 0};

static int intlvRule[98] = {
    0, 1,
    8, 9, 16, 17, 24, 25, 32, 33, 40, 41,
    48, 49, 56, 57, 64, 65, 72, 73, 80, 81,
    2, 3, 10, 11, 18, 19, 26, 27, 34, 35, 42, 43,
    50, 51, 58, 59, 66, 67, 74, 75, 82, 83,
    4, 5, 12, 13, 20, 21, 28, 29, 36, 37, 44, 45,
    52, 53, 60, 61, 68, 69, 76, 77, 84, 85,
    6, 7, 14, 15, 22, 23, 30, 31, 38, 39, 46, 47,
    54, 55, 62, 63, 70, 71, 78, 79, 86, 87,
    88, 89,
    96, 97,
    90, 91,
    92, 93,
    94, 95 };

void trellis_3_4_encode(int * inBlock, int * outBlock)
{
    int i, k;
    int state, sig;
    int tri_bits[49];
    int di_bits[98];
    for(i=0;i<49;i++)
        tri_bits[i]=0;
    for(i=0;i<98;i++)
        di_bits[i]=0;
    for ( k=0; k<18; k++ ) tri_bits[k] = inBlock[k];
    tri_bits[18] = 0; /* This will become the null tri-bit at the end. */
    for ( k=0; k<49; k++ ) shift_vector_right( 49-k, 8, 5, &tri_bits[k]);
    state = 0;
    for ( k=0; k<49; k++)
    {
        sig = sigIndx_3_4[state][ tri_bits[k] ];
        di_bits[2*k    ] = signalP[sig][0];
        di_bits[2*k + 1] = signalP[sig][1];
        state = tri_bits[k];
    }
    for ( k=0; k<14; k++ )
    {
        sig = 0;
        for ( i=0; i<7; i++ )
        {
            sig = (sig << 2) | di_bits[intlvRule[7*k + i]];
        }
        *(outBlock + k) = sig;
    }
}

void trellis_1_2_encode(int * inBlock, int * outBlock)
{
    int i, k;
    int state, sig;
    int di_bits_in[49];
    int di_bits[98];

    for(i=0;i<49;i++)
        di_bits_in[i]=0;
    for(i=0;i<98;i++)
        di_bits[i]=0;
    for ( k=0; k<12; k++ ) di_bits_in[k] = inBlock[k];
    di_bits_in[12] = 0; /* This will become the null di-bit at the end. */
    for ( k=0; k<49; k++ ) shift_vector_right( 49-k, 8, 6, &di_bits_in[k]);
    state = 0;
    for ( k=0; k<49; k++)
    {
        sig = sigIndx_1_2[state][ di_bits_in[k] ];
        di_bits[2*k    ] = signalP[sig][0];
        di_bits[2*k + 1] = signalP[sig][1];
        state = di_bits_in[k];
    }
    for ( k=0; k<14; k++ )
    {
        sig = 0;
        for ( i=0; i<7; i++ )
        {
            sig = (sig << 2) | di_bits[intlvRule[7*k + i]];
        }
        *(outBlock + k) = sig;
    }
}
