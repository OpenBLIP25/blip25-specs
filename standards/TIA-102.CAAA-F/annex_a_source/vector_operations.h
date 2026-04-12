/******************************************************************************
Name:       vector_operations.h
Author:     Al Wilson
Date:       8/3/93
History:    8/3/93 New

This module declares two functions for operations on vectors:
    shift_vector_right ( int length, int element_length, int shift, int * ptr )
    extract_dibit ( int length, int element_length, int * ptr )
******************************************************************************/
#ifndef VECTOR_OPERATIONS_H
#define VECTOR_OPERATIONS_H
void shift_vector_right(int, int, int, int * );
int extract_dibit(int, int, int * );
#endif
