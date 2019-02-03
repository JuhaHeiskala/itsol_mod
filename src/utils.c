
#include "utils.h"

#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
void errexit(char *f_str, ...)
{
    va_list argp;
    char out1[512], out2[1024];

    va_start(argp, f_str);
    vsprintf(out1, f_str, argp);
    va_end(argp);

    sprintf(out2, "Error! %s\n", out1);

    fprintf(stdout, "%s", out2);
    fflush(stdout);

    exit(-1);
}

void *Malloc(int nbytes, char *msg)
{
    void *ptr;

    if (nbytes == 0)
        return NULL;

    ptr = (void *)malloc(nbytes);
    if (ptr == NULL)
        errexit("Not enough mem for %s. Requested size: %d bytes", msg, nbytes);

    return ptr;
}

/*----------------------------------------------------------------------
  | Initialize SpaFmt structs.
  |----------------------------------------------------------------------
  | on entry:
  |==========
  | ( amat )  =  Pointer to a SpaFmt struct.
  |     len   =  size of matrix
  |     job   =  0: pattern only
  |              1: data and pattern
  |
  | On return:
  |===========
  |
  |  amat->n
  |      ->*nzcount
  |      ->**ja
  |      ->**ma
  |
  | integer value returned:
  |             0   --> successful return.
  |             1   --> memory allocation error.
  |--------------------------------------------------------------------*/
int setupCS(csptr amat, int len, int job)
{
    amat->n = len;
    amat->nzcount = (int *)Malloc(len * sizeof(int), "setupCS");
    amat->ja = (int **)Malloc(len * sizeof(int *), "setupCS");
    if (job == 1)
        amat->ma = (double **)Malloc(len * sizeof(double *), "setupCS");
    else
        amat->ma = NULL;
    return 0;
}

/*----------------------------------------------------------------------
  | Free up memory allocated for SpaFmt structs.
  |----------------------------------------------------------------------
  | on entry:
  |==========
  | ( amat )  =  Pointer to a SpaFmt struct.
  |--------------------------------------------------------------------*/
int cleanCS(csptr amat)
{
    int i;
    if (amat == NULL)
        return 0;
    if (amat->n < 1)
        return 0;
    for (i = 0; i < amat->n; i++) {
        if (amat->nzcount[i] > 0) {
            if (amat->ma)
                free(amat->ma[i]);
            free(amat->ja[i]);
        }
    }
    if (amat->ma)
        free(amat->ma);
    free(amat->ja);
    free(amat->nzcount);
    free(amat);
    return 0;
}

/*----------------------------------------------------------------------
  | Convert CSR matrix to SpaFmt struct
  |----------------------------------------------------------------------
  | on entry:
  |==========
  | ( amat )   = Matrix stored in SpaFmt format
  |
  |
  | On return:
  |===========
  |
  | ( bmat )  =  Matrix stored as SpaFmt struct containing a copy
  |              of amat 
  |
  |       integer value returned:
  |             0   --> successful return.
  |             1   --> memory allocation error.
  |--------------------------------------------------------------------*/
int cscpy(csptr amat, csptr bmat)
{
    int j, len, size = amat->n;
    double *bma;
    int *bja;
    /*------------------------------------------------------------*/
    for (j = 0; j < size; j++) {
        len = bmat->nzcount[j] = amat->nzcount[j];
        if (len > 0) {
            bja = (int *)Malloc(len * sizeof(int), "cscpy:1");
            bma = (double *)Malloc(len * sizeof(double), "cscpy:2");
            memcpy(bja, amat->ja[j], len * sizeof(int));
            memcpy(bma, amat->ma[j], len * sizeof(double));
            bmat->ja[j] = bja;
            bmat->ma[j] = bma;
        }
    }
    return 0;
}

/*----------------------------------------------------------------------
  | Initialize ILUSpar structs.
  |----------------------------------------------------------------------
  | on entry:
  |==========
  |   ( lu )  =  Pointer to a ILUSpar struct.
  |       n   =  size of matrix
  |
  | On return:
  |===========
  |
  |    lu->n
  |      ->L     L matrix, SpaFmt format
  |      ->D     Diagonals
  |      ->U     U matrix, SpaFmt format
  |      ->work  working buffer of length n
  |      ->bf    buffer
  |
  | integer value returned:
  |             0   --> successful return.
  |            -1   --> memory allocation error.
  |--------------------------------------------------------------------*/
int setupILU(iluptr lu, int n)
{
    lu->n = n;
    lu->D = (double *)Malloc(sizeof(double) * n, "setupILU");
    lu->L = (csptr) Malloc(sizeof(SparMat), "setupILU");
    setupCS(lu->L, n, 1);
    lu->U = (csptr) Malloc(sizeof(SparMat), "setupILU");
    setupCS(lu->U, n, 1);
    lu->work = (int *)Malloc(sizeof(int) * n, "setupILU");
    return 0;
}

/*----------------------------------------------------------------------
  | Free up memory allocated for ILUSpar structs.
  |----------------------------------------------------------------------
  | on entry:
  |==========
  |   ( lu )  =  Pointer to a ILUSpar struct.
  |--------------------------------------------------------------------*/
int cleanILU(iluptr lu)
{
    if (NULL == lu)
        return 0;
    if (lu->D) {
        free(lu->D);
    }
    cleanCS(lu->L);
    cleanCS(lu->U);
    if (lu->work)
        free(lu->work);
    free(lu);
    return 0;
}

/*----------------------------------------------------------------------
  | Initialize VBSpaFmt structs.
  |----------------------------------------------------------------------
  | on entry:
  |==========
  | ( vbmat ) =  Pointer to a VBSpaFmt struct.
  |       n   =  size of block matrix
  |      nB   =  size of diagonal block, so the real size of the matrix
  |              is nB[0] + nB[1] + ... + nB[n-1]
  |              do nothing if nB is NULL
  |
  | On return:
  |===========
  |
  | vbmat->n
  |      ->*bsz
  |      ->*nzcount
  |      ->**ja
  |      ->**ba
  |      ->*D
  |
  | integer value returned:
  |             0   --> successful return.
  |            -1   --> memory allocation error.
  |--------------------------------------------------------------------*/
int setupVBMat(vbsptr vbmat, int n, int *nB)
{
    int i;
    vbmat->n = n;
    if (nB) {
        vbmat->bsz = (int *)Malloc(sizeof(int) * (n + 1), "setupVBMat");
        vbmat->bsz[0] = 0;
        for (i = 1; i <= n; i++) {
            vbmat->bsz[i] = vbmat->bsz[i - 1] + nB[i - 1];
        }
    }
    else
        vbmat->bsz = NULL;
    vbmat->nzcount = (int *)Malloc(sizeof(int) * n, "setupVBMat");
    vbmat->ja = (int **)Malloc(sizeof(int *) * n, "setupVBMat");
    vbmat->ba = (BData **) Malloc(sizeof(BData *) * n, "setupVBMat");
    vbmat->D = NULL;
    return 0;
}

/*----------------------------------------------------------------------
  | Free up memory allocated for VBSpaFmt structs.
  |----------------------------------------------------------------------
  | on entry:
  |==========
  | ( vbmat )  =  Pointer to a VBSpaFmt struct.
  |--------------------------------------------------------------------*/
int cleanVBMat(vbsptr vbmat)
{
    int i, j;
    if (vbmat == NULL)
        return 0;
    if (vbmat->n < 1)
        return 0;

    for (i = 0; i < vbmat->n; i++) {
        if (vbmat->nzcount[i] > 0) {
            free(vbmat->ja[i]);
            if (vbmat->ba && vbmat->ba[i]) {
                for (j = 0; j < vbmat->nzcount[i]; j++) {
                    free(vbmat->ba[i][j]);
                }
                free(vbmat->ba[i]);
            }
        }
        if (vbmat->D && vbmat->D[i])
            free(vbmat->D[i]);
    }
    if (vbmat->D)
        free(vbmat->D);
    free(vbmat->ja);
    if (vbmat->ba)
        free(vbmat->ba);
    free(vbmat->nzcount);
    if (vbmat->bsz)
        free(vbmat->bsz);
    free(vbmat);
    return 0;
}

int nnzVBMat(vbsptr vbmat)
{
    int nnz = 0, i, n = vbmat->n;
    for (i = 0; i < n; i++) {
        nnz += vbmat->nzcount[i];
    }
    return nnz;
}

int memVBMat(vbsptr vbmat)
{
    int mem = 0, nnz, i, j, n = vbmat->n, *bsz = vbmat->bsz, dm;
    for (i = 0; i < n; i++) {
        nnz = vbmat->nzcount[i];
        dm = 0;
        for (j = 0; j < nnz; j++) {
            dm += B_DIM(bsz, vbmat->ja[i][j]);
        }
        mem += dm * B_DIM(bsz, i);
    }
    return mem;
}

/*----------------------------------------------------------------------
  | Initialize VBILUSpar structs.
  |----------------------------------------------------------------------
  | on entry:
  |==========
  |   ( lu )  =  Pointer to a VBILUSpar struct.
  |       n   =  size of block matrix
  |     bsz   =  the row/col of the first element of each diagonal block
  |
  | On return:
  |===========
  |
  |    lu->n
  |      ->bsz
  |      ->L     L matrix, VBSpaFmt format
  |      ->D     Diagonals
  |      ->U     U matrix, VBSpaFmt format
  |      ->work  working buffer of length n
  |      ->bf    buffer
  |
  | integer value returned:
  |             0   --> successful return.
  |            -1   --> memory allocation error.
  |--------------------------------------------------------------------*/
int setupVBILU(vbiluptr lu, int n, int *bsz)
{
    int i;
    int max_block_size = sizeof(double) * MAX_BLOCK_SIZE * MAX_BLOCK_SIZE;
    lu->n = n;
    lu->bsz = (int *)Malloc(sizeof(int) * (n + 1), "setupVBILU");
    for (i = 0; i <= n; i++)
        lu->bsz[i] = bsz[i];
    lu->D = (BData *) Malloc(sizeof(BData) * n, "setupVBILU");
    lu->L = (vbsptr) Malloc(sizeof(VBSparMat), "setupVBILU");
    setupVBMat(lu->L, n, NULL);
    lu->U = (vbsptr) Malloc(sizeof(VBSparMat), "setupVBILU");
    setupVBMat(lu->U, n, NULL);
    lu->work = (int *)Malloc(sizeof(int) * n, "setupVBILU");
    lu->bf = (BData) Malloc(max_block_size, "setupVBILU");
    return 0;
}

/*----------------------------------------------------------------------
  | Free up memory allocated for VBILUSpar structs.
  |----------------------------------------------------------------------
  | on entry:
  |==========
  |   ( lu )  =  Pointer to a VBILUSpar struct.
  |--------------------------------------------------------------------*/
int cleanVBILU(vbiluptr lu)
{
    int n = lu->n, i;
    if (NULL == lu)
        return 0;
    if (lu->D) {
        for (i = 0; i < n; i++) {
            if (lu->D[i])
                free(lu->D[i]);
        }
        free(lu->D);
    }
    if (lu->bsz)
        free(lu->bsz);
    cleanVBMat(lu->L);
    cleanVBMat(lu->U);
    if (lu->work)
        free(lu->work);
    if (lu->bf)
        free(lu->bf);
    free(lu);
    return 0;
}

/*----------------------------------------------------------------------
  | Prepare space of a row according to the result of level structure
  |----------------------------------------------------------------------
  | on entry:
  |==========
  |   ( lu )  =  Pointer to a ILUSpar struct.
  |     nrow  =  the current row to deal with
  |
  | On return:
  |===========
  |
  |    lu->L->ma[nrow][...]
  |      ->U->ma[nrow][...]
  |
  | integer value returned:
  |             0   --> successful return.
  |            -1   --> memory allocation error.
  |--------------------------------------------------------------------*/
int mallocRow(iluptr lu, int nrow)
{
    int nzcount = lu->L->nzcount[nrow];
    lu->L->ma[nrow] = (double *)Malloc(sizeof(double) * nzcount, "mallocRow");
    nzcount = lu->U->nzcount[nrow];
    lu->U->ma[nrow] = (double *)Malloc(sizeof(double) * nzcount, "mallocRow");
    return 0;
}

/*----------------------------------------------------------------------
  | Prepare space of a row according to the result of level structure
  |----------------------------------------------------------------------
  | on entry:
  |==========
  |   ( lu )  =  Pointer to a VBILUSpar struct.
  |     nrow  =  the current row to deal with
  |
  | On return:
  |===========
  |
  |    lu->L->ba[nrow][...]
  |      ->D[nrow]
  |      ->U->ba[nrow][...]
  |
  | integer value returned:
  |             0   --> successful return.
  |            -1   --> memory allocation error.
  |--------------------------------------------------------------------*/
int mallocVBRow(vbiluptr lu, int nrow)
{
    int j, nzcount, ncol, szOfBlock;
    int *bsz = lu->bsz;

    nzcount = lu->L->nzcount[nrow];
    lu->L->ba[nrow] = (BData *) Malloc(sizeof(BData) * nzcount, "mallocVBRow");
    for (j = 0; j < nzcount; j++) {
        ncol = lu->L->ja[nrow][j];
        szOfBlock = B_DIM(bsz, nrow) * B_DIM(bsz, ncol) * sizeof(double);
        lu->L->ba[nrow][j] = (BData) Malloc(szOfBlock, "mallocVBRow");
    }

    szOfBlock = sizeof(double) * B_DIM(bsz, nrow) * B_DIM(bsz, nrow);
    lu->D[nrow] = (BData) Malloc(szOfBlock, "mallocVBRow");

    nzcount = lu->U->nzcount[nrow];
    lu->U->ba[nrow] = (BData *) Malloc(sizeof(BData) * nzcount, "mallocVBRow");
    for (j = 0; j < nzcount; j++) {
        ncol = lu->U->ja[nrow][j];
        szOfBlock = B_DIM(bsz, nrow) * B_DIM(bsz, ncol) * sizeof(double);
        lu->U->ba[nrow][j] = (BData) Malloc(szOfBlock, "mallocVBRow");
    }
    return 0;
}

void zrmC(int m, int n, BData data)
{
    int mn = m * n, i;
    for (i = 0; i < mn; i++)
        data[i] = 0;
}

void copyBData(int m, int n, BData dst, BData src, int isig)
{
    int mm = m * n, i;
    if (isig == 0)
        for (i = 0; i < mm; i++)
            dst[i] = src[i];
    else
        for (i = 0; i < mm; i++)
            dst[i] = -src[i];
}

/*----------------------------------------------------------------------
  | initialize PerMat4 struct given the F, E, blocks.  
  |----------------------------------------------------------------------
  | on entry:
  |==========
  | ( amat )  =  Pointer to a PerMat4 struct.
  |     Bn    =  size of B block
  |     Cn    =  size of C block
  |     F, E  = the two blocks to be assigned to srtruct - without the
  |
  | On return:
  |===========
  |
  |  amat->L                for each block: amat->M->n
  |      ->U                                       ->nzcount
  |      ->E                                       ->ja
  |      ->F                                       ->ma
  |      ->perm
  |      ->rperm       (if meth[1] > 0)
  |      ->D1          (if meth[2] > 0)
  |      ->D2          (if meth[3] > 0)
  |
  |  Scaling arrays are initialized to 1.0.
  |
  |       integer value returned:
  |             0   --> successful return.
  |             1   --> memory allocation error.
  |--------------------------------------------------------------------*/
int setupP4(p4ptr amat, int Bn, int Cn, csptr F, csptr E)
{
    int n;
    /* size n */
    n = amat->n = Bn + Cn;
    amat->nB = Bn;
    /* amat->perm = (int *) Malloc(n*sizeof(int), "setupP4:1" ); */
    /*   assign space for wk -- note that this is only done at 1st level
         at other levels, copy pointer of wk from previous level */
    if (amat->prev == NULL)     /* wk has 2 * n entries now */
        amat->wk = (double *)Malloc(2 * n * sizeof(double), "setupP4:2");
    else
        amat->wk = (amat->prev)->wk;

    /*-------------------- L and U */
    amat->L = (csptr) Malloc(sizeof(SparMat), "setupP4:3");
    if (setupCS(amat->L, Bn, 1))
        return 1;
    /*    fprintf(stdout,"  -- BN %d   Cn   %d \n", Bn,Cn);  */
    amat->U = (csptr) Malloc(sizeof(SparMat), "setupP4:4");
    if (setupCS(amat->U, Bn, 1))
        return 1;

    amat->F = F;
    amat->E = E;
    return 0;
}

/*----------------------------------------------------------------------
  | Free up memory allocated for Per4Mat structs.
  |----------------------------------------------------------------------
  | on entry:
  |==========
  | ( amat )  =  Pointer to a Per4Mat struct.
  |--------------------------------------------------------------------*/
int cleanP4(p4ptr amat)
{
    if (amat == NULL) return 0;
    if (amat->n < 1) return 0;

    if (amat->perm) {
        if (amat->perm) free(amat->perm);
        amat->perm = NULL;
    }

    if (!amat->symperm) {
        if (amat->rperm) free(amat->rperm);
        amat->rperm = NULL;
    }

    if (amat->F) {
        cleanCS(amat->F);
        amat->F = NULL;
    }
    if (amat->E) {
        cleanCS(amat->E);
        amat->E = NULL;
    }
    if (amat->L) {
        cleanCS(amat->L);
        amat->L = NULL;
    }
    if (amat->U) {
        cleanCS(amat->U);
        amat->U = NULL;
    }

    if (amat->prev == NULL)
        if (amat->wk) free(amat->wk);

    if (amat->D1) free(amat->D1);
    if (amat->D2) free(amat->D2);

    return 0;
}

/*----------------------------------------------------------------------
  | Allocate pointers for ILUTfac structs.
  |----------------------------------------------------------------------
  | on entry:
  |==========
  | ( amat )  =  Pointer to a ILUTfac struct.
  |     len   =  size of L U  blocks
  |
  | On return:
  |===========
  |
  |  amat->L                for each block: amat->M->n
  |      ->U                                       ->nzcount
  |                                                ->ja
  |                                                ->ma
  |      ->rperm       (if meth[0] > 0)
  |      ->perm2       (if meth[1] > 0)
  |      ->D1          (if meth[2] > 0)
  |      ->D2          (if meth[3] > 0)
  |
  |  Permutation arrays are initialized to the identity.
  |  Scaling arrays are initialized to 1.0.
  |
  |       integer value returned:
  |             0   --> successful return.
  |             1   --> memory allocation error.
  |--------------------------------------------------------------------*/
int setupILUT(ilutptr amat, int len)
{
    amat->n = len;
    amat->wk = (double *)Malloc(2 * len * sizeof(double), "setupILUT:5");
    amat->L = (csptr) Malloc(sizeof(SparMat), "setupILUT:6");
    if (setupCS(amat->L, len, 1))
        return 1;
    amat->U = (csptr) Malloc(sizeof(SparMat), "setupILUT:7");
    if (setupCS(amat->U, len, 1))
        return 1;
    return 0;
}

/*----------------------------------------------------------------------
  | Free up memory allocated for IluSpar structs.
  |----------------------------------------------------------------------
  | on entry:
  |==========
  | ( amat )  =  Pointer to a IluSpar struct.
  |  indic    = indicator for number of levels.  indic=0 -> zero level.
  |--------------------------------------------------------------------*/
int cleanILUT(ilutptr amat, int indic)
{
    if (amat->wk) {
        free(amat->wk);
        amat->wk = NULL;
    }
    cleanCS(amat->L);
    cleanCS(amat->U);

    if (indic) cleanCS(amat->C);

    if (amat->rperm) {
        free(amat->rperm);
        amat->rperm = NULL;
    }

    if (amat->perm) {
        free(amat->perm);
        amat->perm = NULL;
    }

    if (amat->perm2) free(amat->perm2);
    if (amat->D1) free(amat->D1);
    if (amat->D2) free(amat->D2);

    return 0;
}

void setup_arms(arms Levmat)
{
    Levmat->ilus = (ilutptr) Malloc(sizeof(IluSpar), "setup_arms:ilus");
    Levmat->levmat = (p4ptr) Malloc(sizeof(Per4Mat), "setup_arms:levmat");
}

/*----------------------------------------------------------------------
  | Free up memory allocated for entire ARMS preconditioner.
  |----------------------------------------------------------------------
  | on entry:
  |==========
  | ( amat )  =  Pointer to a Per4Mat struct.
  | ( cmat )  =  Pointer to a IluSpar struct.
  |--------------------------------------------------------------------*/
int cleanARMS(arms ArmsPre)
{
    p4ptr amat = ArmsPre->levmat;
    ilutptr cmat = ArmsPre->ilus;
    /* case when nlev == 0 */
    int indic = (amat->nB != 0);

    p4ptr levc, levn;

    levc = amat;

    if (indic) {
        while (levc) {
            if (cleanP4(levc))
                return (1);
            levn = levc->next;
            free(levc);
            levc = levn;
        }
    }
    else if (amat) {
        free(amat);
        amat = NULL;
    }

    cleanILUT(cmat, indic);

    if (cmat) {
        free(cmat);
        cmat = NULL;
    }
    free(ArmsPre);
    return 0;
}

/*---------------------------------------------------------------------
  | Convert permuted csrmat struct to PerMat4 struct 
  |                - matrix already permuted
  |----------------------------------------------------------------------
  | on entry:
  |========== 
  | ( amat )  =  Matrix stored in SpaFmt format.
  |              Internal pointers (and associated memory) destroyed before
  |              return.
  |
  | On return:
  |===========
  |
  | B, E, F, C = 4 blocks in 
  | 
  |          | B   F |      
  |   Amat = |       | 
  |          | E   C | 
  | 
  |
  |       integer value returned:
  |             0   --> successful return.
  |             1   --> memory allocation error.
  |--------------------------------------------------------------------*/
int csSplit4(csptr amat, int bsize, int csize, csptr B, csptr F, csptr E, csptr C)
{
    int j, j1, numr, numl, ind, newj, rowz, *rowj, *new1j, *new2j;
    double *rowm, *new1m, *new2m;

    if (setupCS(B, bsize, 1)) goto label111;
    if (setupCS(F, bsize, 1)) goto label111;
    if (setupCS(E, csize, 1)) goto label111;
    if (setupCS(C, csize, 1)) goto label111;

    new1j = (int *)Malloc(bsize * sizeof(int), "csSplit4:1");
    new2j = (int *)Malloc(csize * sizeof(int), "csSplit4:2");
    new1m = (double *)Malloc(bsize * sizeof(double), "csSplit4:3");
    new2m = (double *)Malloc(csize * sizeof(double), "csSplit4:4");
    /* B and F blocks */
    for (j = 0; j < bsize; j++) {
        numl = numr = 0;
        rowz = amat->nzcount[j];
        rowj = amat->ja[j];
        rowm = amat->ma[j];
        for (j1 = 0; j1 < rowz; j1++) {
            if (rowj[j1] < bsize)
                numl++;
            else
                numr++;
        }
        B->nzcount[j] = numl;
        F->nzcount[j] = numr;
        B->ja[j] = (int *)Malloc(numl * sizeof(int), "csSplit4:5");
        B->ma[j] = (double *)Malloc(numl * sizeof(double), "csSplit4:6");
        F->ja[j] = (int *)Malloc(numr * sizeof(int), "csSplit4:7");
        F->ma[j] = (double *)Malloc(numr * sizeof(double), "csSplit4:8");
        numl = numr = 0;
        for (j1 = 0; j1 < rowz; j1++) {
            newj = rowj[j1];
            if (newj < bsize) {
                new1j[numl] = newj;
                new1m[numl] = rowm[j1];
                numl++;
            }
            else {
                new2j[numr] = newj - bsize;
                new2m[numr] = rowm[j1];
                numr++;
            }
        }
        if (numl > 0) {
            memcpy(B->ja[j], new1j, numl * sizeof(int));
            memcpy(B->ma[j], new1m, numl * sizeof(double));
        }
        if (numr > 0) {
            memcpy(F->ja[j], new2j, numr * sizeof(int));
            memcpy(F->ma[j], new2m, numr * sizeof(double));
        }
    }
    /* E and C blocks */
    for (j = 0; j < csize; j++) {
        numl = numr = 0;
        ind = bsize + j;
        rowz = amat->nzcount[ind];
        rowj = amat->ja[ind];
        rowm = amat->ma[ind];
        for (j1 = 0; j1 < rowz; j1++) {
            if (rowj[j1] < bsize)
                numl++;
            else
                numr++;
        }
        E->nzcount[j] = numl;
        C->nzcount[j] = numr;
        E->ja[j] = (int *)Malloc(numl * sizeof(int), "csSplit4:9");
        E->ma[j] = (double *)Malloc(numl * sizeof(double), "csSplit4:10");
        C->ja[j] = (int *)Malloc(numr * sizeof(int), "csSplit4:11");
        C->ma[j] = (double *)Malloc(numr * sizeof(double), "csSplit4:12");
        numl = numr = 0;
        for (j1 = 0; j1 < rowz; j1++) {
            newj = rowj[j1];
            if (newj < bsize) {
                new1j[numl] = newj;
                new1m[numl] = rowm[j1];
                numl++;
            }
            else {
                new2j[numr] = newj - bsize;
                new2m[numr] = rowm[j1];
                numr++;
            }
        }
        if (numl > 0) {
            memcpy(E->ja[j], new1j, numl * sizeof(int));
            memcpy(E->ma[j], new1m, numl * sizeof(double));
        }
        if (numr > 0) {
            memcpy(C->ja[j], new2j, numr * sizeof(int));
            memcpy(C->ma[j], new2m, numr * sizeof(double));
        }
    }

    if (new1j)
        free(new1j);
    if (new2j)
        free(new2j);
    if (new1m)
        free(new1m);
    if (new2m)
        free(new2m);
    return 0;
label111:
    return 1;
}

/*----------------------------------------------------------------------
  | Convert CSR matrix to SpaFmt struct
  |----------------------------------------------------------------------
  | on entry:
  |==========
  | a, ja, ia  = Matrix stored in CSR format (with FORTRAN indexing).
  | rsa        = source file is symmetric HB matrix 
  |
  | On return:
  |===========
  |
  | ( mat )  =  Matrix stored as SpaFmt struct. (C indexing)
  |
  |       integer value returned:
  |             0   --> successful return.
  |             1   --> memory allocation error.
  |--------------------------------------------------------------------*/
int CSRcs(int n, double *a, int *ja, int *ia, csptr mat, int rsa)
{
    int i, j, j1, len, col, nnz;
    double *bra;
    int *bja;

    setupCS(mat, n, 1);

    if (rsa) {                  /* RSA HB matrix */
        for (j = 0; j < n; j++) {
            len = ia[j + 1] - ia[j];
            mat->nzcount[j] = len;
        }
        for (j = 0; j < n; j++) {
            for (j1 = ia[j] - 1; j1 < ia[j + 1] - 1; j1++) {
                col = ja[j1] - 1;
                if (col != j)
                    mat->nzcount[col]++;
            }
        }
        for (j = 0; j < n; j++) {
            nnz = mat->nzcount[j];
            mat->ja[j] = (int *)Malloc(nnz * sizeof(int), "CSRcs");
            mat->ma[j] = (double *)Malloc(nnz * sizeof(double), "CSRcs");
            mat->nzcount[j] = 0;
        }
        for (j = 0; j < n; j++) {
            for (j1 = ia[j] - 1; j1 < ia[j + 1] - 1; j1++) {
                col = ja[j1] - 1;
                mat->ja[j][mat->nzcount[j]] = col;
                mat->ma[j][mat->nzcount[j]] = a[j1];
                mat->nzcount[j]++;
                if (col != j) {
                    mat->ja[col][mat->nzcount[col]] = j;
                    mat->ma[col][mat->nzcount[col]] = a[j1];
                    mat->nzcount[col]++;
                }
            }
        }
        return 0;
    }

    for (j = 0; j < n; j++) {
        len = ia[j + 1] - ia[j];
        mat->nzcount[j] = len;
        if (len > 0) {
            bja = (int *)Malloc(len * sizeof(int), "CSRcs");
            bra = (double *)Malloc(len * sizeof(double), "CSRcs");
            i = 0;
            for (j1 = ia[j] - 1; j1 < ia[j + 1] - 1; j1++) {
                bja[i] = ja[j1] - 1;
                bra[i] = a[j1];
                i++;
            }
            mat->ja[j] = bja;
            mat->ma[j] = bra;
        }
    }
    return 0;
}

/*----------------------------------------------------------------------
  | Convert COO matrix to SpaFmt struct
  |----------------------------------------------------------------------
  | on entry:
  |==========
  | a, ja, ia  = Matrix stored in COO format  -- a =  entries
  |                                             ja = column indices
  |                                             ia = row indices 
  | On return:
  |===========
  |
  | ( bmat )  =  Matrix stored as SpaFmt struct.
  |
  |       integer value returned:
  |             0   --> successful return.
  |             1   --> memory allocation error.
  |--------------------------------------------------------------------*/
int COOcs(int n, int nnz, double *a, int *ja, int *ia, csptr bmat)
{
    int i, k, k1, l, job = 1;
    int *len;
    /*-------------------- setup data structure for bmat (csptr) struct */
    if (setupCS(bmat, n, job)) {
        printf(" ERROR SETTING UP bmat IN SETUPCS \n");
        exit(0);
    }
    /*-------------------- determine lengths */
    len = (int *)Malloc(n * sizeof(int), "COOcs:0");
    for (k = 0; k < n; k++)
        len[k] = 0;
    for (k = 0; k < nnz; k++)
        ++len[ia[k]];
    /*-------------------- allocate          */
    for (k = 0; k < n; k++) {
        l = len[k];
        bmat->nzcount[k] = l;
        if (l > 0) {
            bmat->ja[k] = (int *)Malloc(l * sizeof(int), "COOcs:1");
            bmat->ma[k] = (double *)Malloc(l * sizeof(double), "COOcs:2");
        }
        len[k] = 0;
    }
    /*-------------------- Fill actual entries */
    for (k = 0; k < nnz; k++) {
        i = ia[k];
        k1 = len[i];
        (bmat->ja[i])[k1] = ja[k];
        (bmat->ma[i])[k1] = a[k];
        len[i]++;
    }
    free(len);
    return 0;
}

void coocsc(int n, int nnz, double *val, int *col, int *row, double **a, int **ja, int **ia, int job)
{
    int i, *ir, *jc;

    *a = (double *)Malloc(nnz * sizeof(double), "coocsc");
    *ja = (int *)Malloc(nnz * sizeof(int), "coocsc");
    *ia = (int *)Malloc((n + 1) * sizeof(int), "coocsc");

    if (job == 0) {
        ir = (int *)Malloc(nnz * sizeof(int), "coocsc");
        jc = (int *)Malloc(nnz * sizeof(int), "coocsc");
        for (i = 0; i < nnz; i++) {
            ir[i] = row[i] + 1;
            jc[i] = col[i] + 1;
        }
    }
    else {
        ir = row;
        jc = col;
    }

    coocsr_(&n, &nnz, val, jc, ir, *a, *ja, *ia);

    if (job == 0) {
        free(ir);
        free(jc);
    }
}

/*----------------------------------------------------------------------
 *  Compressed C-style Sparse Row to C-style Various Block Sparse Row
 *----------------------------------------------------------------------
 *
 * This  subroutine converts a matrix stored  in a C-style SpaFmt format
 * into a C-style various block SpaFmt format
 *
 * NOTE: the initial matrix does not have to have a block structure. 
 * zero padding is done for general sparse matrices. 
 *
 *----------------------------------------------------------------------
 * on entry:
 *----------
 * job   = if job == 0 on entry, pattern only is generated. 
 *
 * nBlk  = integer equal to the dimension of block matrix.
 * 
 * nB    = integer array of diagonals' block size
 *
 * csmat = Sparse Row format Matrix
 *
 * on return:
 *-----------
 * 
 * vbmat = Various Block Sparse Row format Matrix
 *
 * ierr  = integer, error code. 
 *              0  -- normal termination
 *             -1  -- error occur
 *
 *---------------------------------------------------------------------*/
int csrvbsrC(int job, int nBlk, int *nB, csptr csmat, vbsptr vbmat)
{
    int n, i, j, k;
    int nnz, szofBlock, ipos, b_row, b_col, br, bc;
    int *iw = NULL;

    n = csmat->n;               /* size of the original matrix          */
    setupVBMat(vbmat, nBlk, nB);
    iw = (int *)Malloc(sizeof(int) * nBlk, "csrvbsrC_1");
    for (i = 0; i < nBlk; i++)
        iw[i] = 0;
    b_row = -1;
    for (i = 0; i < n; i += nB[b_row]) {
        vbmat->nzcount[++b_row] = 0;

        /* calculate nzcount of the (b_row)-th row of the block matrix */
        for (j = i; j < i + nB[b_row]; j++) {
            int nnz_j = csmat->nzcount[j];
            for (k = 0; k < nnz_j; k++) {
                /* get the column ID of block matrix by giving the column ID
                   of the original matrix */
                b_col = col2vbcol(csmat->ja[j][k], vbmat);
                if (iw[b_col] == 0) {
                    iw[b_col] = 1;
                    vbmat->nzcount[b_row]++;
                }
            }
        }
        if (0 == (nnz = vbmat->nzcount[b_row]))
            continue;
        vbmat->ja[b_row] = (int *)Malloc(sizeof(int) * nnz, "csrvbsrC_2");

        /* calculate the pattern of the (b_row)-th row of the block matrix */
        for (j = 0, ipos = 0; j < nBlk; j++) {
            if (iw[j] != 0) {
                vbmat->ja[b_row][ipos] = j;
                iw[j] = ipos;
                ipos++;
            }
        }
        if (job == 0)
            goto NEXT_ROW;      /* stop here if patterns only */

        /* copy data to the (b_row)-th row of the block matrix from the
           original matrix */
        vbmat->ba[b_row] = (BData *) Malloc(sizeof(BData) * nnz, "csrvbsrC_3");
        for (j = 0; j < nnz; j++) {
            szofBlock = sizeof(double) * nB[b_row] * nB[vbmat->ja[b_row][j]];
            vbmat->ba[b_row][j] = (BData) Malloc(szofBlock, "csrvbsrC_4");
            memset(vbmat->ba[b_row][j], 0, szofBlock);
        }
        for (j = i; j < i + nB[b_row]; j++) {
            for (k = 0; k < csmat->nzcount[j]; k++) {
                /* get the column ID of block matrix by giving the column ID
                   of the original matrix */
                b_col = col2vbcol(csmat->ja[j][k], vbmat);
                ipos = iw[b_col];
                br = j - i;
                bc = csmat->ja[j][k] - vbmat->bsz[b_col];
                DATA(vbmat->ba[b_row][ipos], nB[b_row], br, bc) = csmat->ma[j][k];
            }
        }
NEXT_ROW:
        /* reset iw */
        for (j = 0; j < nnz; j++)
            iw[vbmat->ja[b_row][j]] = 0;
    }

    free(iw);
    return 0;
}

/*---------------------------------------------------------------------
 * get the column ID of block matrix by giving the column ID of the original
 * matrix
 *--------------------------------------------------------------------*/
int col2vbcol(int col, vbsptr vbmat)
{
    int *bsz = vbmat->bsz, n = vbmat->n;
    int begin = 0, mid, end = n - 1;
    while (end - begin > 1) {
        mid = (begin + end) / 2;
        if (col < bsz[mid]) {
            end = mid;
        }
        else if (col >= bsz[mid + 1]) {
            begin = mid;
        }
        else {
            return mid;
        }
    }
    if (col >= bsz[end]) {
        return end;
    }
    return begin;
}

int nnz_vbilu(vbiluptr lu)
{
    int *bsz = lu->bsz;
    int nzcount, nnz = 0, i, j, col;
    for (i = 0; i < lu->n; i++) {
        nzcount = 0;
        for (j = 0; j < lu->L->nzcount[i]; j++) {
            col = lu->L->ja[i][j];
            nzcount += B_DIM(bsz, col);
        }
        for (j = 0; j < lu->U->nzcount[i]; j++) {
            col = lu->U->ja[i][j];
            nzcount += B_DIM(bsz, col);
        }
        nzcount += B_DIM(bsz, i);       /* diagonal */
        nzcount *= B_DIM(bsz, i);
        nnz += nzcount;
    }
    return nnz;
}

int nnz_ilu(iluptr lu)
{
    int nnz = 0, i;
    for (i = 0; i < lu->n; i++) {
        nnz += lu->L->nzcount[i];
        nnz += lu->U->nzcount[i];
        nnz++;
    }
    return nnz;
}

int nnz_lev4(p4ptr levmat, int *lev, FILE * ft)
{
    int nnzT, nnzL, nnzU, nnzF, nnzE, nnzDown = 0;
    p4ptr nextmat;

    nnzL = nnz_cs(levmat->L);
    nnzU = nnz_cs(levmat->U);
    nnzF = nnz_cs(levmat->F);
    nnzE = nnz_cs(levmat->E);
    nnzT = nnzL + nnzU + nnzF + nnzE;

    if (ft) {
        if (*lev == 0)
            fprintf(ft, "\nnnz/lev used:      L        U        F        E    subtot\n");
        fprintf(ft, "    Level %2d %8d %8d %8d %8d %8d\n", *lev, nnzL, nnzU, nnzF, nnzE, nnzT);
    }
    (*lev)++;
    nextmat = levmat->next;

    if (nextmat != NULL)
        nnzDown = nnz_lev4(nextmat, lev, ft);
    return (nnzT + nnzDown);
}

int nnz_cs(csptr A)
{
    int i, n = A->n, nnz = 0;
    for (i = 0; i < n; i++)
        nnz += A->nzcount[i];
    return nnz;
}

/*-------------------------------------------------------
  | computes and prints out total number of nonzero elements
  | used in ARMS factorization 
  +--------------------------------------------------------*/
int nnz_arms(arms PreSt, FILE * ft)
{
    p4ptr levmat = PreSt->levmat;
    ilutptr ilschu = PreSt->ilus;
    int nlev = PreSt->nlev;
    int ilev = 0, nnz_lev, nnz_sch, nnz_tot;
    nnz_lev = 0;
    if (nlev)
        nnz_lev += nnz_lev4(levmat, &ilev, ft);
    nnz_sch = nnz_cs(ilschu->L) + nnz_cs(ilschu->U);
    if (nlev)
        nnz_sch += nnz_cs(ilschu->C);
    nnz_tot = nnz_lev + nnz_sch;
    if (ft) {
        fprintf(ft, "\n");
        fprintf(ft, "Total nonzeros for interm. blocks.... =  %10d\n", nnz_lev);
        fprintf(ft, "Total nonzeros for last level ....... =  %10d\n", nnz_sch);
        fprintf(ft, "Grand total.......................... =  %10d\n", nnz_tot);
    }
    return nnz_tot;
}

/*----------------------------------------------------------------------
  | Convert CSC matrix to LUSparMat struct
  |----------------------------------------------------------------------
  | on entry:
  |==========
  | a, ja, ia  = Matrix stored in CSC format (with FORTRAN indexing).
  | rsa        = 0: rua matrix
  |              1: source file is symmetric HB matrix 
  |              2: pattern symmetrization (add zeros)
  |
  | On return:
  |===========
  |
  | ( mat )  =  Matrix stored as LUSparMat struct.
  |
  |       integer value returned:
  |             0   --> successful return.
  |             1   --> memory allocation error.
  |--------------------------------------------------------------------*/
int CSClum(int n, double *a, int *ja, int *ia, iluptr mat, int rsa)
{
    int row, col, i, k, j1, j2, nnz, id;
    double val, *D;
    csptr L, U;
    setupILU(mat, n);

    L = mat->L;
    U = mat->U;
    D = mat->D;
    memset(D, 0, n * sizeof(double));
    for (i = 0; i < n; i++) {
        L->nzcount[i] = 0;
        U->nzcount[i] = 0;
    }
    for (col = 0; col < n; col++) {
        j1 = ia[col] - 1;
        j2 = ia[col + 1] - 1;
        for (k = j1; k < j2; k++) {
            row = ja[k] - 1;
            if (row > col) {
                L->nzcount[col]++;
                if (rsa == 1)
                    U->nzcount[col]++;
            }
            else if (row == col) {
                U->nzcount[row]++;
            }
            else {
                U->nzcount[row]++;
                if (rsa == 1)
                    L->nzcount[row]++;
            }
        }
    }
    for (i = 0; i < n; i++) {
        if (rsa == 2) {
            /* nzcount(A + A^T) <= nzcount(A) + nnzcol(A^T) */
            nnz = L->nzcount[i] + U->nzcount[i];
            L->ja[i] = (int *)Malloc(nnz * sizeof(int), "CSClum1");
            L->ma[i] = (double *)Malloc(nnz * sizeof(double), "CSClum2");
            L->nzcount[i] = 0;
            U->ja[i] = (int *)Malloc(nnz * sizeof(int), "CSClum3");
            U->ma[i] = (double *)Malloc(nnz * sizeof(double), "CSClum4");
            U->nzcount[i] = 0;
        }
        else {
            nnz = L->nzcount[i];
            L->ja[i] = (int *)Malloc(nnz * sizeof(int), "CSClum4");
            L->ma[i] = (double *)Malloc(nnz * sizeof(double), "CSClum5");
            L->nzcount[i] = 0;
            nnz = U->nzcount[i];
            U->ja[i] = (int *)Malloc(nnz * sizeof(int), "CSClum6");
            U->ma[i] = (double *)Malloc(nnz * sizeof(double), "CSClum7");
            U->nzcount[i] = 0;
        }
    }
    for (col = 0; col < n; col++) {
        j1 = ia[col] - 1;
        j2 = ia[col + 1] - 1;
        /*-------------------- */
        for (k = j1; k < j2; k++) {
            row = ja[k] - 1;
            val = a[k];
            if (row > col) {
                id = L->nzcount[col];
                L->ja[col][id] = row;
                L->ma[col][id] = val;
                L->nzcount[col]++;
                if (rsa == 1) {
                    id = U->nzcount[col];
                    U->ja[col][id] = row;
                    U->ma[col][id] = val;
                    U->nzcount[col]++;
                }
            }
            else if (row == col) {
                id = U->nzcount[row];
                U->ja[row][id] = col;
                U->ma[row][id] = val;
                U->nzcount[row]++;
                D[col] = val;
            }
            else {
                id = U->nzcount[row];
                U->ja[row][id] = col;
                U->ma[row][id] = val;
                U->nzcount[row]++;
                if (rsa == 1) {
                    id = L->nzcount[row];
                    L->ja[row][id] = col;
                    L->ma[row][id] = val;
                    L->nzcount[row]++;
                }
            }
        }
    }
    if (rsa == 2) {
        /* add zeros to make pattern symmetrization */
        int *idU, *idL;
        int nzcount, nnzcol, nnzU, nnzL, j;
        idU = (int *)Malloc(n * sizeof(int), "CSClum6");
        idL = (int *)Malloc(n * sizeof(int), "CSClum7");
        for (i = 0; i < n; i++) {
            idU[i] = 0;
            idL[i] = 0;
        }
        for (i = 0; i < n; i++) {
            nzcount = U->nzcount[i];
            for (j = 0; j < nzcount; j++)
                idU[U->ja[i][j]] = 1;
            nnzcol = L->nzcount[i];
            for (j = 0; j < nnzcol; j++)
                idL[L->ja[i][j]] = 1;
            nnzU = nzcount;
            for (j = 0; j < nnzcol; j++) {
                row = L->ja[i][j];
                if (idU[row] == 0) {
                    U->ja[i][nnzU] = row;
                    U->ma[i][nnzU] = 0.0;
                    nnzU++;
                }
            }
            nnzL = nnzcol;
            for (j = 0; j < nzcount; j++) {
                col = U->ja[i][j];
                if (col == i)
                    continue;
                if (idL[col] == 0) {
                    L->ja[i][nnzL] = col;
                    L->ma[i][nnzL] = 0.0;
                    nnzL++;
                }
            }
            for (j = 0; j < nzcount; j++)
                idU[U->ja[i][j]] = 0;
            for (j = 0; j < nnzcol; j++)
                idL[L->ja[i][j]] = 0;
            if (nnzU - nnzL != 1) {
                fprintf(stderr, "error in pattern symmetrization...\n");
                exit(-1);
            }
            L->nzcount[i] = nnzL;
            U->nzcount[i] = nnzU;
        }
        free(idU);
        free(idL);
    }

    return 0;
}

/*----------------------------------------------------------------------
  | Convert cs matrix to LUSparMat struct
  |----------------------------------------------------------------------
  | on entry:  
  |==========
  | amat      = matrix in cs format 
  |
  | On return:
  |===========
  |
  | ( mat )  =  Matrix stored as LUSparMat struct.
  |
  |       integer value returned:
  |             0   --> successful return.
  |             1   --> memory allocation error.
  |--------------------------------------------------------------------*/
int CSClumC(csptr amat, iluptr mat, int rsa)
{
    int row, col, i, k, n, nz, nnz, id, *ja;
    double val, *D, *ma;
    csptr L, U;
    /*-------------------- begin */
    n = amat->n;
    setupILU(mat, n);
    L = mat->L;
    U = mat->U;
    D = mat->D;

    for (i = 0; i < n; i++) {
        L->nzcount[i] = 0;
        U->nzcount[i] = 0;
    }
    for (col = 0; col < n; col++) {
        nz = amat->nzcount[col];
        ja = amat->ja[col];
        for (k = 0; k < nz; k++) {
            row = ja[k];
            if (row > col) {
                L->nzcount[col]++;
                if (rsa == 1)
                    U->nzcount[col]++;
            }
            else if (row == col) {
                U->nzcount[row]++;
            }
            else {
                U->nzcount[row]++;
                if (rsa == 1)
                    L->nzcount[row]++;
            }
        }
    }
    for (i = 0; i < n; i++) {
        if (rsa == 2) {
            /* nzcount(A + A^T) <= nzcount(A) + nnzcol(A^T) */
            nnz = L->nzcount[i] + U->nzcount[i];
            L->ja[i] = (int *)Malloc(nnz * sizeof(int), "CSClum1");
            L->ma[i] = (double *)Malloc(nnz * sizeof(double), "CSClum2");
            L->nzcount[i] = 0;
            U->ja[i] = (int *)Malloc(nnz * sizeof(int), "CSClum3");
            U->ma[i] = (double *)Malloc(nnz * sizeof(double), "CSClum4");
            U->nzcount[i] = 0;
        }
        else {
            nnz = L->nzcount[i];
            L->ja[i] = (int *)Malloc(nnz * sizeof(int), "CSClum4");
            L->ma[i] = (double *)Malloc(nnz * sizeof(double), "CSClum5");
            L->nzcount[i] = 0;
            nnz = U->nzcount[i];
            U->ja[i] = (int *)Malloc(nnz * sizeof(int), "CSClum6");
            U->ma[i] = (double *)Malloc(nnz * sizeof(double), "CSClum7");
            U->nzcount[i] = 0;
        }
    }
    for (col = 0; col < n; col++) {
        nz = amat->nzcount[col];
        ja = amat->ja[col];
        ma = amat->ma[col];
        for (k = 0; k < nz; k++) {
            row = ja[k];
            val = ma[k];
            if (row > col) {
                id = L->nzcount[col];
                L->ja[col][id] = row;
                L->ma[col][id] = val;
                L->nzcount[col]++;
                if (rsa == 1) {
                    id = U->nzcount[col];
                    U->ja[col][id] = row;
                    U->ma[col][id] = val;
                    U->nzcount[col]++;
                }
            }
            else if (row == col) {
                id = U->nzcount[row];
                U->ja[row][id] = col;
                U->ma[row][id] = val;
                U->nzcount[row]++;
                D[col] = val;
            }
            else {
                id = U->nzcount[row];
                U->ja[row][id] = col;
                U->ma[row][id] = val;
                U->nzcount[row]++;
                if (rsa == 1) {
                    id = L->nzcount[row];
                    L->ja[row][id] = col;
                    L->ma[row][id] = val;
                    L->nzcount[row]++;
                }
            }
        }
    }
    if (rsa == 2) {
        /* add zeros to make pattern symmetrization */
        int *idU, *idL;
        int nzcount, nnzcol, nnzU, nnzL, j;
        idU = (int *)Malloc(n * sizeof(int), "CSClum6");
        idL = (int *)Malloc(n * sizeof(int), "CSClum7");
        for (i = 0; i < n; i++) {
            idU[i] = 0;
            idL[i] = 0;
        }
        for (i = 0; i < n; i++) {
            nzcount = U->nzcount[i];
            for (j = 0; j < nzcount; j++)
                idU[U->ja[i][j]] = 1;
            nnzcol = L->nzcount[i];
            for (j = 0; j < nnzcol; j++)
                idL[L->ja[i][j]] = 1;
            nnzU = nzcount;
            for (j = 0; j < nnzcol; j++) {
                row = L->ja[i][j];
                if (idU[row] == 0) {
                    U->ja[i][nnzU] = row;
                    U->ma[i][nnzU] = 0.0;
                    nnzU++;
                }
            }
            nnzL = nnzcol;
            for (j = 0; j < nzcount; j++) {
                col = U->ja[i][j];
                if (col == i)
                    continue;
                if (idL[col] == 0) {
                    L->ja[i][nnzL] = col;
                    L->ma[i][nnzL] = 0.0;
                    nnzL++;
                }
            }
            for (j = 0; j < nzcount; j++)
                idU[U->ja[i][j]] = 0;
            for (j = 0; j < nnzcol; j++)
                idL[L->ja[i][j]] = 0;
            if (nnzU - nnzL != 1) {
                fprintf(stderr, "error in pattern symmetrization...\n");
                exit(-1);
            }
            L->nzcount[i] = nnzL;
            U->nzcount[i] = nnzU;
        }
        free(idU);
        free(idL);
    }
    return 0;
}

/* 
 * Purpose
 * ======= 
 *	Returns the time in seconds used by the process.
 *
 * Note: the timer function call is machine dependent. Use conditional
 *       compilation to choose the appropriate function.
 *
 */

double sys_timer(void)
{
    struct timeval tv;
    double t;

    gettimeofday(&tv, (struct timezone *)0);
    t = tv.tv_sec + (double)tv.tv_usec * 1e-6;

    return t;
}

int dumpCooMat(csptr A, int nglob, int, FILE * ft);

/*----------------------------------------------------------------------
  |     does a quick-sort split of a real array.
  |     on input a[0 : (n-1)] is a real array
  |     on output is permuted such that its elements satisfy:
  |
  |     abs(a[i]) >= abs(a[Ncut-1]) for i < Ncut-1 and
  |     abs(a[i]) <= abs(a[Ncut-1]) for i > Ncut-1
  |
  |     ind[0 : (n-1)] is an integer array permuted in the same way as a.
  |---------------------------------------------------------------------*/
int qsplitC(double *a, int *ind, int n, int Ncut)
{
    double tmp, abskey;
    int j, itmp, first, mid, last, ncut;
    ncut = Ncut - 1;

    first = 0;
    last = n - 1;
    if (ncut < first || ncut > last)
        return 0;
    /* outer loop -- while mid != ncut */
    do {
        mid = first;
        abskey = fabs(a[mid]);
        for (j = first + 1; j <= last; j++) {
            if (fabs(a[j]) > abskey) {
                mid = mid + 1;
                tmp = a[mid];
                itmp = ind[mid];
                a[mid] = a[j];
                ind[mid] = ind[j];
                a[j] = tmp;
                ind[j] = itmp;
            }
        }
        /*-------------------- interchange */
        tmp = a[mid];
        a[mid] = a[first];
        a[first] = tmp;
        itmp = ind[mid];
        ind[mid] = ind[first];
        ind[first] = itmp;
        /*-------------------- test for while loop */
        if (mid == ncut)
            break;
        if (mid > ncut)
            last = mid - 1;
        else
            first = mid + 1;
    } while (mid != ncut);

    return 0;
}

/*----------------------------------------------------------------------
  | Finds the transpose of a matrix stored in SpaFmt format.
  |
  |-----------------------------------------------------------------------
  | on entry:
  |----------
  | (amat) = a matrix stored in SpaFmt format.
  |
  | job    = integer to indicate whether to fill the values (job.eq.1)
  |          of the matrix (bmat) or only the pattern.
  |
  | flag   = integer to indicate whether the matrix has been filled
  |          0 - no filled
  |          1 - filled
  |
  | on return:
  | ----------
  | (bmat) = the transpose of (mata) stored in SpaFmt format.
  |
  | integer value returned:
  |             0   --> successful return.
  |             1   --> memory allocation error.
  |---------------------------------------------------------------------*/
int SparTran(csptr amat, csptr bmat, int job, int flag)
{
    int i, j, *ind, pos, size = amat->n, *aja;
    double *ama = NULL;
    ind = (int *)Malloc(size * sizeof(int), "SparTran:1");
    for (i = 0; i < size; i++)
        ind[i] = 0;
    if (!flag) {
        /*--------------------  compute lengths  */
        for (i = 0; i < size; i++) {
            aja = amat->ja[i];
            for (j = 0; j < amat->nzcount[i]; j++)
                ind[aja[j]]++;
        }
        /*--------------------  allocate space  */
        for (i = 0; i < size; i++) {
            bmat->ja[i] = (int *)Malloc(ind[i] * sizeof(int), "SparTran:2");
            bmat->nzcount[i] = ind[i];
            if (job == 1) {
                bmat->ma[i] = (double *)Malloc(ind[i] * sizeof(double), "SparTran:3");
            }
            ind[i] = 0;
        }
    }
    /*--------------------  now do the actual copying  */
    for (i = 0; i < size; i++) {
        aja = amat->ja[i];
        if (job == 1)
            ama = amat->ma[i];
        for (j = 0; j < amat->nzcount[i]; j++) {
            pos = aja[j];
            bmat->ja[pos][ind[pos]] = i;
            if (job == 1)
                bmat->ma[pos][ind[pos]] = ama[j];
            ind[pos]++;
        }
    }
    free(ind);
    return 0;
}

void swapj(int v[], int i, int j)
{
    int temp;
    temp = v[i];
    v[i] = v[j];
    v[j] = temp;
}

void swapm(double v[], int i, int j)
{
    double temp;
    temp = v[i];
    v[i] = v[j];
    v[j] = temp;
}

/*---------------------------------------------------------------------
  |
  | This routine scales each row of mata so that the norm is 1.
  |
  |----------------------------------------------------------------------
  | on entry:
  | mata  = the matrix (in SpaFmt form)
  | nrm   = type of norm
  |          0 (\infty),  1 or 2
  |
  | on return
  | diag  = diag[j] = 1/norm(row[j])
  |
  |     0 --> normal return
  |     j --> row j is a zero row
  |--------------------------------------------------------------------*/
int roscalC(csptr mata, double *diag, int nrm)
{
    int i, k;
    double *kr, scal;
    for (i = 0; i < mata->n; i++) {
        scal = 0.0;
        kr = mata->ma[i];
        if (nrm == 0) {
            for (k = 0; k < mata->nzcount[i]; k++)
                if (fabs(kr[k]) > scal)
                    scal = fabs(kr[k]);
        }
        else if (nrm == 1) {
            for (k = 0; k < mata->nzcount[i]; k++)
                scal += fabs(kr[k]);
        }
        else {                  /* nrm = 2 */
            for (k = 0; k < mata->nzcount[i]; k++)
                scal += kr[k] * kr[k];
        }
        if (nrm == 2)
            scal = sqrt(scal);
        if (scal == 0.0) {
            scal = 1.0;
            /* YS. return i+1; */
        }
        else
            scal = 1.0 / scal;
        diag[i] = scal;
        for (k = 0; k < mata->nzcount[i]; k++)
            kr[k] = kr[k] * scal;
    }
    return 0;
}

/*---------------------------------------------------------------------
  |
  | This routine scales each column of mata so that the norm is 1.
  |
  |----------------------------------------------------------------------
  | on entry:
  | mata  = the matrix (in SpaFmt form)
  | nrm   = type of norm
  |          0 (\infty),  1 or 2
  |
  | on return
  | diag  = diag[j] = 1/norm(row[j])
  |
  |     0 --> normal return
  |     j --> column j is a zero column
  |--------------------------------------------------------------------*/
int coscalC(csptr mata, double *diag, int nrm)
{
    int i, j, k;
    double *kr;
    int *ki;
    for (i = 0; i < mata->n; i++)
        diag[i] = 0.0;
    /*---------------------------------------
      |   compute the norm of each column
      |--------------------------------------*/
    for (i = 0; i < mata->n; i++) {
        kr = mata->ma[i];
        ki = mata->ja[i];
        if (nrm == 0) {
            for (k = 0; k < mata->nzcount[i]; k++) {
                j = ki[k];
                if (fabs(kr[k]) > diag[j])
                    diag[j] = fabs(kr[k]);
            }
        }
        else if (nrm == 1) {
            for (k = 0; k < mata->nzcount[i]; k++)
                diag[ki[k]] += fabs(kr[k]);
        }
        else {                  /*  nrm = 2 */
            for (k = 0; k < mata->nzcount[i]; k++)
                diag[ki[k]] += kr[k] * kr[k];
        }
    }
    if (nrm == 2) {
        for (i = 0; i < mata->n; i++)
            diag[i] = sqrt(diag[i]);
    }
    /*---------------------------------------
      |   invert
      |--------------------------------------*/
    for (i = 0; i < mata->n; i++) {
        if (diag[i] == 0.0)
            /* return i+1; */
            diag[i] = 1.0;
        else
            diag[i] = 1.0 / diag[i];
    }
    /*---------------------------------------
      |   C = A * D
      |--------------------------------------*/
    for (i = 0; i < mata->n; i++) {
        kr = mata->ma[i];
        ki = mata->ja[i];
        for (k = 0; k < mata->nzcount[i]; k++)
            kr[k] = kr[k] * diag[ki[k]];
    }
    return 0;
}

/* Computes  y == DD * x                               */
/* scales the vector x by the diagonal dd - output in y */
void dscale(int n, double *dd, double *x, double *y)
{
    int k;
    for (k = 0; k < n; k++) y[k] = dd[k] * x[k];
}

/*----------------------------------------------------------------------
  |
  | qqsort: sort ma[left]...ma[right] into decreasing order
  | from Kernighan & Ritchie
  |
  | ja holds the column indices
  | abval = 1: consider absolute values
  |         0: values
  |
  |---------------------------------------------------------------------*/
void qsortC(int *ja, double *ma, int left, int right, int abval)
{
    int i, last;

    if (left >= right)
        return;
    if (abval) {
        swapj(ja, left, (left + right) / 2);
        swapm(ma, left, (left + right) / 2);
        last = left;
        for (i = left + 1; i <= right; i++) {
            if (fabs(ma[i]) > fabs(ma[left])) {
                swapj(ja, ++last, i);
                swapm(ma, last, i);
            }
        }
        swapj(ja, left, last);
        swapm(ma, left, last);
        qsortC(ja, ma, left, last - 1, abval);
        qsortC(ja, ma, last + 1, right, abval);
    }
    else {
        swapj(ja, left, (left + right) / 2);
        swapm(ma, left, (left + right) / 2);
        last = left;
        for (i = left + 1; i <= right; i++) {
            if (ma[i] > ma[left]) {
                swapj(ja, ++last, i);
                swapm(ma, last, i);
            }
        }
        swapj(ja, left, last);
        swapm(ma, left, last);
        qsortC(ja, ma, left, last - 1, abval);
        qsortC(ja, ma, last + 1, right, abval);
    }
}

/*-------------------------------------------------------------+
  | to dump rows i0 to i1 of matrix for debugging purposes       |
  |--------------------------------------------------------------*/
void printmat(FILE * ft, csptr A, int i0, int i1)
{
    int i, k, nzi;
    int *row;
    double *rowm;
    for (i = i0; i < i1; i++) {
        nzi = A->nzcount[i];
        row = A->ja[i];
        rowm = A->ma[i];
        for (k = 0; k < nzi; k++) {
            fprintf(ft, " row %d  a  %e ja %d \n", i + 1, rowm[k], row[k] + 1);
        }
    }
}

/*----------------------------------------------------------------------
  |
  | qqsort: sort wa[left]...wa[right] into decreasing order
  | from Kernighan & Ritchie
  |
  |---------------------------------------------------------------------*/
void qsortR2I(double *wa, int *cor1, int *cor2, int left, int right)
{
    int i, last;

    if (left >= right) return;

    swapm(wa, left, (left + right) / 2);
    swapj(cor1, left, (left + right) / 2);
    swapj(cor2, left, (left + right) / 2);
    last = left;
    for (i = left + 1; i <= right; i++) {
        if (wa[i] > wa[left]) {
            swapm(wa, ++last, i);
            swapj(cor1, last, i);
            swapj(cor2, last, i);
        }
    }
    swapm(wa, left, last);
    swapj(cor1, left, last);
    swapj(cor2, left, last);
    qsortR2I(wa, cor1, cor2, left, last - 1);
    qsortR2I(wa, cor1, cor2, last + 1, right);
}

/*----------------------------------------------------------------------
  |
  | qqsort: sort ma[left]...ma[right] into increasing order
  | from Kernighan & Ritchie
  |
  | ja holds the column indices
  | abval = 1: consider absolute values
  |         0: values
  |
  |---------------------------------------------------------------------*/
void qsort2C(int *ja, double *ma, int left, int right, int abval)
{
    int i, last;
    if (left >= right)
        return;
    if (abval) {
        swapj(ja, left, (left + right) / 2);
        swapm(ma, left, (left + right) / 2);
        last = left;
        for (i = left + 1; i <= right; i++) {
            if (fabs(ma[i]) < fabs(ma[left])) {
                swapj(ja, ++last, i);
                swapm(ma, last, i);
            }
        }
        swapj(ja, left, last);
        swapm(ma, left, last);
        qsort2C(ja, ma, left, last - 1, abval);
        qsort2C(ja, ma, last + 1, right, abval);
    }

    else {
        swapj(ja, left, (left + right) / 2);
        swapm(ma, left, (left + right) / 2);
        last = left;
        for (i = left + 1; i <= right; i++) {
            if (ma[i] < ma[left]) {
                swapj(ja, ++last, i);
                swapm(ma, last, i);
            }
        }
        swapj(ja, left, last);
        swapm(ma, left, last);
        qsort2C(ja, ma, left, last - 1, abval);
        qsort2C(ja, ma, last + 1, right, abval);
    }
}

/*----------------------------------------------------------------------
  |
  | qqsort: sort ja[left]...ja[right] into increasing order
  | from Kernighan & Ritchie
  |
  | ma holds the real values
  |
  |---------------------------------------------------------------------*/
void qqsort(int *ja, double *ma, int left, int right)
{
    int i, last;
    if (left >= right)
        return;
    swapj(ja, left, (left + right) / 2);
    swapm(ma, left, (left + right) / 2);
    last = left;
    for (i = left + 1; i <= right; i++) {
        if (ja[i] < ja[left]) {
            swapj(ja, ++last, i);
            swapm(ma, last, i);
        }
    }
    swapj(ja, left, last);
    swapm(ma, left, last);
    qqsort(ja, ma, left, last - 1);
    qqsort(ja, ma, last + 1, right);
}

/*----------------------------------------------------------------------
  |
  | This routine sorts the entries in each row of a matrix from hi to low.
  |
  |-----------------------------------------------------------------------
  | on entry:
  |----------
  | (mat) = a matrix stored in SpaFmt format.
  |
  | abval =   1: use absolute values of entries
  |           0: use values
  |
  | hilo  =   1: sort in decreasing order
  |           0: sort in increasing order
  |
  |
  | on return:
  | ----------
  | (mat) = (mat) where each row is sorted.
  |
  |---------------------------------------------------------------------*/
void hilosort(csptr mat, int abval, int hilo)
{
    int j, n = mat->n, *nnz = mat->nzcount;

    if (hilo)
        for (j = 0; j < n; j++)
            qsortC(mat->ja[j], mat->ma[j], 0, nnz[j] - 1, abval);

    else
        for (j = 0; j < n; j++)
            qsort2C(mat->ja[j], mat->ma[j], 0, nnz[j] - 1, abval);

    return;
}

/*----------------------------------------------------------------------
  |
  | qqsort: sort wa[left]...wa[right] into increasing order
  | from Kernighan & Ritchie
  |
  |---------------------------------------------------------------------*/
void qsort3i(int *wa, int *cor1, int *cor2, int left, int right)
{
    int i, last;

    if (left >= right)
        return;

    swapj(wa, left, (left + right) / 2);
    swapj(cor1, left, (left + right) / 2);
    swapj(cor2, left, (left + right) / 2);
    last = left;
    for (i = left + 1; i <= right; i++) {
        if (wa[i] < wa[left]) {
            swapj(wa, ++last, i);
            swapj(cor1, last, i);
            swapj(cor2, last, i);
        }
    }
    swapj(wa, left, last);
    swapj(cor1, left, last);
    swapj(cor2, left, last);
    qsort3i(wa, cor1, cor2, left, last - 1);
    qsort3i(wa, cor1, cor2, last + 1, right);
}

int dumpArmsMat(arms PreSt, FILE * ft)
{
    int lev, nnz, nglob = 0, old = 0;
    p4ptr levmat = PreSt->levmat;
    ilutptr ilus = PreSt->ilus;
    int n = levmat->n;
    int nlev = PreSt->nlev;
    FILE *dummy = NULL;

    nnz = nnz_arms(PreSt, dummy) - nnz_cs(ilus->C);

    fprintf(ft, " %d %d %d \n", n, n, nnz);

    old = 0;
    for (lev = 0; lev < nlev; lev++) {
        nglob += levmat->nB;
        dumpCooMat(levmat->L, old, old, ft);
        dumpCooMat(levmat->U, old, old, ft);
        dumpCooMat(levmat->E, nglob, old, ft);
        dumpCooMat(levmat->F, old, nglob, ft);
        levmat = levmat->next;
        if (levmat == NULL)
            break;
        old = nglob;
    }
    dumpCooMat(ilus->L, nglob, nglob, ft);
    dumpCooMat(ilus->U, nglob, nglob, ft);
    return (0);
}

int dumpCooMat(csptr A, int shiftR, int shiftC, FILE * ft)
{
    int n, i, k, nzi;
    int *row;
    double *rowm;

    n = A->n;
    for (i = 0; i < n; i++) {
        nzi = A->nzcount[i];
        row = A->ja[i];
        rowm = A->ma[i];

        for (k = 0; k < nzi; k++) {
            fprintf(ft, " %d  %d  %e \n", row[k] + shiftC, i + shiftR, rowm[k]);
        }
    }

    return (0);
}

/*----------------------------------------------------------------------
  | Output the pattern of L\U, which can be loaded by matlab
  ----------------------------------------------------------------------*/
int outputLU(iluptr lu, char *filename)
{
    FILE *fmatlab = fopen(filename, "w");
    int n = lu->n, i, j, nzcount;
    csptr L = lu->L, U = lu->U;

    if (!fmatlab)
        return -1;
    fprintf(fmatlab, "%d %d 0\n", n, n);
    for (i = 0; i < n; i++) {
        nzcount = L->nzcount[i];
        for (j = 0; j < nzcount; j++)
            fprintf(fmatlab, "%d %d 1\n", i + 1, L->ja[i][j] + 1);
    }
    for (i = 0; i < n; i++) {
        nzcount = U->nzcount[i];
        for (j = 0; j < nzcount; j++)
            fprintf(fmatlab, "%d %d 1\n", i + 1, U->ja[i][j] + 1);
    }
    for (i = 0; i < n; i++)
        fprintf(fmatlab, "%d %d 1\n", i + 1, i + 1);
    fclose(fmatlab);
    return 0;
}

/*-------------------- checks the validity of a permutation [for 
  debugging purposes.] 
  Return codes:
  0  -- permutation is valid
  1  -- a value perm[?] is outside the range 0--(n-1)
  2  -- perm[i] hit a value between 0--(n-1) more than once.
  */
int checkperm(int *p, int n)
{
    int *work;
    int k, i;
    work = Malloc(n * sizeof(int), " check perm:work ");
    for (k = 0; k < n; k++)
        work[k] = -1;
    for (k = 0; k < n; k++) {
        i = p[k];
        if ((i < 0) | (i >= n))
            return (1);
        if (work[i] >= 0)
            return (2);
        work[i] = k;
    }
    free(work);
    return (0);
}

/*----------------------------------------------------------------------
  |
  | qqsort: sort wa[left]...wa[right] into decreasing order
  | from Kernighan & Ritchie
  |
  |---------------------------------------------------------------------*/
void qsortR1I(double *wa, int *cor1, int left, int right)
{
    int i, last;

    if (left >= right)
        return;

    swapm(wa, left, (left + right) / 2);
    swapj(cor1, left, (left + right) / 2);
    last = left;
    for (i = left + 1; i <= right; i++) {
        if (wa[i] > wa[left]) {
            swapm(wa, ++last, i);
            swapj(cor1, last, i);
        }
    }
    swapm(wa, left, last);
    swapj(cor1, left, last);
    qsortR1I(wa, cor1, left, last - 1);
    qsortR1I(wa, cor1, last + 1, right);
}
