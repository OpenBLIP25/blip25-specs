/******************************************************************************
Name:       bch.h
Author:     Al Wilson
Date:       7/28/93
History:    7/28/93 New

This module declares one function:
    bch_64_encode ( in, out )
        encodes (64,16,23) primitive BCH code
******************************************************************************/
#ifndef BCH_H
#define BCH_H
void bch_64_encode ( int, int * );
#endif
