/******************************************************************************
Name:       parm.h
Author:     Mike Bright
Version:    1.10
Date:       3/15/96
******************************************************************************/
#ifndef PARAMETERS_H
#define PARAMETERS_H

struct parm
{
    /* Encryption parameters */
    int ep_algid;           /* 1 octet ALGORITHM ID for the KEK*/
    int ep_keyid;           /* 2 octet KEY ID for the TEK */
    int iv[4];              /* 8 octet Initialization Vector */
    long tek[2];            /* 8 octet TEK */
    int flags;              /* Valid parm; bit- 0:key 1:iv 2:keyid 3:algid; 1->valid */

    /* Channel Access parameters */
    int fs[3];              /* 6 octet Frame Sync bit pattern */
    int nac;                /* 12 bit NAC */
    int duid;               /* 4 bit DUID */

    /* Voice/Data Parameters */
    int mfid[3];            /* 1 octet Manufacturer ID */
    int mi[5];              /* 9 octet MI (left justified - last octet not used) */
    int ss[256];            /* Status Symbols buffer */

    /* Data specific parameters */
    int unused0;            /* 1 or 2 bits unused in octet 0 of the data header */
    int anb;                /* 1 bit confirmation required (1 = confirmation) */
    int ibo;                /* 1 bit direction (1 = outbound; 0 = inbound) */
    int format;             /* 5 bits defining data packet format */
    int unused1;            /* 2 bits unused in octet 1 of the data header */
    int sap;                /* 6 bit SAP value */
    int dest[2];            /* 3 octet destination LLID (right justified) */
    int fmf;                /* 1 bit Full Message Flag */
    int btf;                /* 7 bits number of blocks to follow */
    int unused7;            /* 3 bits unused in octet 1 of the data header */
    int pad;                /* 5 bits number of pad octets */
    int sync;               /* 1 bit sync flag */
    int ns;                 /* 3 bit sequence number */
    int fsnf;               /* 4 bit Fragment Sequence Number */
    int unused9;            /* 2 bits unused in octet 1 of the data header */
    int offset;             /* 6 bit header offset */
    int reserved[2];        /* 3 octet reserved buffer for unconfirmed/ACK data */
    /* Enhanced addressing (EA) parameters */
    int source[2];          /* 3 octet source address (right justified) */
    int eh_unused;          /* 2 bits unused in SAP octet */
    int eh_2nd_sap;         /* 6 bits enhanced addressing secondary SAP value */
    int eh_reserved[2];     /* 3 octet reserved for unconfirmed enhanced addressing */
    /* Encryption Sync (ES) header */
    int es_unused;          /* 2 bits unused in ES SAP octet */
    int es_2nd_sap;         /* 6 bits encryption sync secondary SAP value */

    /* Data buffer */
    char data[544];         /* Data buffer - includes space for ES, EA & pad + data */

    /* Working storage variables */
    int lfsr[4];            /* LFSR register */
    int ss_pntr;            /* pointer to next status symbol */
    int ss_count;           /* Number of Status Sysbols entered in the data block */
    int nbytes;             /* data octet counter */
    int ignored;            /* buffer to read in integer values to be ignored */

    /* Trunking control channel params */
    int status;
    int outbound;
    int microslots_per_slot;
    int tm1;
    int tm2;
    int lb[3];
    int p[3];
    int opcode[3];
    int octet2[3];
    int octet3[3];
    int octet4[3];
    int octet5[3];
    int octet6[3];
    int octet7[3];
    int octet8[3];
    int octet9[3];
    int block_count;
};
char *find_file_value(FILE *infp,char *pattern,char *source,int number,int *dest);
char *find_value(char *pattern,char *source,int number,int *dest);
int find_fs(FILE *infp,struct parm *pp);
int read_header(FILE *infp,struct parm *pp);
void print_ep(FILE *outfp,struct parm *pp);
void print_header(FILE *outfp,int seq_num,struct parm *pp);
int read_ldu_1(FILE *infp,struct parm *pp);
void print_ldu_1(FILE *outfp,int seq_no,struct parm *pp);
int read_ldu_2(FILE *infp,struct parm *pp);
void print_ldu_2(FILE *outfp,int seq_no,struct parm *pp);
int read_simp_term(FILE *infp,struct parm *pp);
void print_simp_term(FILE *outfp,int seq_no,struct parm *pp);
int read_term(FILE *infp,struct parm *pp);
void print_term(FILE *outfp,int seq_no,struct parm *pp);
int data(FILE *infp,FILE *outfp,struct parm *pp);
int read_conf_data(FILE *infp,struct parm *pp);
int read_unconf_data(FILE *infp,struct parm *pp);
int read_data_block(FILE *infp,struct parm *pp);
int read_ss_block(FILE *infp,struct parm *pp);
void encrypt_data_block(struct parm *pp);
void print_conf_data(FILE *outfp,int seq_no,struct parm *pp);
void print_unconf_data(FILE *outfp,int seq_no,struct parm *pp);
#endif
