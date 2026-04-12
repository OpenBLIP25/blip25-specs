/******************************************************************************
Name:       trellis.h
Author:     Al Wilson
Date:       8/5/93
History:    8/5/93 New

This module declares two functions:
    trellis_3_4_encode ( in, out )
        encodes the rate 3/4 trellis code
    trellis_1_2_encode ( in, out )
        encodes the rate 1/2 trellis code
The input and output arguments are pointers a integer vectors. The input
arguments must point to octets while the output arguments point to an array
of 14 integers with 14 bits in each (196 bits in total).
******************************************************************************/

#ifndef TRELLIS_H
#define TRELLIS_H
void trellis_3_4_encode(int *, int *);
void trellis_1_2_encode(int *, int *);
#endif
