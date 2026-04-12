/******************************************************************************
Name:       crc.c
Author:     Al Wilson
Date:       8/4/93
History:    8/4/93 New

This module contains three functions:
    crc_ccitt ( pointer )
        encodes the header parity check
    crc_9 ( num, pointer ) encodes the confirmed data block parity check
    crc_32 ( length, pointer )
        encodes the data parity check
The pointer points to a list of integers. The 32 bit pointer is a variable
length list. The first two functions return a 16 bit and a 9 bit result
respectively. The last function returns a 32 bit long integer result.
The arguments must follow the following rules.

crc_ccitt

    pointer     --  must point to an array of 10 octets

crc_9

    num         ---  is a 7-bit serial number for the data block
    pointer     ---  must point to an array of 16 octets

crc_32

    length      ---  indicates the length of the array, 1..512
    pointer     ---  must point to an array of (length) octets

******************************************************************************/
#include "crc.h"

#define G_16    ((1 << 12) | (1 << 5) | 1)
#define G_9     ((1 << 6) | (1 << 4) | (1 << 3) | 1)
#define G_32    0x04c11db7

int crc_ccitt ( int * ptr )
{
    int temp;
    int i, j;

    temp = 0;
    for ( i=0; i<10; i++ )
    {
        for ( j=7; j>=0; j-- )
        {
            if ( ( (temp >> 15) ^ ((*(ptr+i)) >> j) ) & 1 )
                temp = (temp << 1) ^ G_16;
            else
                temp = temp << 1;
        }
    }
    temp = ( temp & 0xffff ) ^ 0xffff;
    return ( temp );
}

int crc_9 ( int num, int * ptr )
{
    int temp;
    int i, j;

    temp = 0;
    for ( j=6; j>=0; j-- )
    {
        if ( ( (temp >> 8) ^ (num >> j) ) & 1 )
            temp = (temp << 1) ^ G_9;
        else
            temp = temp << 1;
    }
    for ( i=0; i<16; i++ )
    {
        for ( j=7; j>=0; j-- )
        {
            if ( ( (temp >> 8) ^ ((*(ptr+i)) >> j) ) & 1 )
                temp = (temp << 1) ^ G_9;
            else
                temp = temp << 1;
        }
    }
    temp = ( temp & 0x1ff ) ^ 0x1ff;
    return ( temp );
}

long int crc_32 ( int length, int * ptr )
{
    long int temp;
    int i, j;

    temp = 0;
    for ( i=0; i<length; i++ )
    {
        for ( j=7; j>=0; j-- )
        {
            if ( ( (temp >> 31) ^ ((*(ptr+i)) >> j) ) & 1 )
                temp = ( temp << 1 ) ^ G_32;
            else
                temp = temp << 1;
        }
    }
    temp = temp ^ 0xffffffff;
    return ( temp );
}
