/******************************************************************************
Name:       vector_operations.c
Author:     Al Wilson
Date:       8/3/93
History:    8/3/93 New

This module defines two functions for operations on vectors:
    shift_vector_right ( int length, int element_length, int shift, int * ptr )
    extract_dibit ( int length, int element_length, int * ptr )
The arguments to the two functions are as follows:
    length          -- indicates the number of elements in the vector
    element_length  -- indicates the number of bits in each element
    shift           -- indicates the number of bits to shift [ to the right ]
    ptr             -- pointer to the first element of the vector

The shift_vector_right function has the following restrictions:
    length          -- must be >= 1
    element_length  -- must be 1..16
    shift           -- must be < element_length
The function operates by shifting each element to the right, starting with
the first element pointed to by the pointer. It takes the LSBs that shift
out of the element, and shifts them into the next element, starting at the
MSB indicated by the element_length argument. The bits shifted out of the
last element in the vector are discarded.

The extract_dibit function uses the shift_vector_right function to extract
the 2 MSBs of the first vector element (pointed to by the ptr argument). This
is done by shifting to the right (element_length - 2) bits. The result in the
first element is simply the 2 MSBs, in the LSB positions. This is then
returned as the extracted dibit. The vector is then left shifted by moving
each element from position i+1 to position i.

To prevent the last element in the vector from loosing 2 bits, an additional
element is assumed to exist in the vector beyond the indicated length.
Consequently, the actually allocated length of the vector must be at least
(length + 1).
******************************************************************************/
#include "vector_operations.h"

void shift_vector_right(int vector_length,   /* should be >= 1           */
                        int element_length,  /* should be 1..16          */
                        int shift,           /* should be < element_length */
                        int * pointer)
{
    int i, temp;
    for ( i=vector_length-1; i>=0; i-- )
    {
        if ( i > 0 ) temp = *(pointer+i-1);
        else         temp = 0;
        temp &= (1 << shift) - 1;
        *(pointer+i) = (unsigned int)(*(pointer+i)) >> shift;
        *(pointer+i) = (*(pointer+i)) | (temp << (element_length - shift));
    }
}

int extract_dibit ( int vector_length,   /* should be >= 1   */
                    int element_length,  /* should be 1..16  */
                    int * pointer )
{
    int temp, i;

    *(pointer+vector_length) = 0; /* vector should have one extra element */
    shift_vector_right ( vector_length+1, element_length,
                         element_length-2, pointer );
    temp = *pointer;
    for ( i=0; i<vector_length; i++ )
        *(pointer + i) = *(pointer + i + 1);
    *(pointer+vector_length) = 0; /* clean up the residue */
    return ( temp & 0x3 );
}
