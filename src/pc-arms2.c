
#include "pc-arms2.h"

#define  PERMTOL  0.99          /*  0 --> no permutation 0.01 to 0.1 good  */
#define ITS_MAX_NUM_LEV 10

/*---------------------------------------------------------------------
  | MULTI-LEVEL BLOCK ILUT PRECONDITIONER.
  | ealier  version:  June 23, 1999  BJS -- 
  | version2: Dec. 07th, 2000, YS  [reorganized ]
  | version 3 (latest) Aug. 2005.  [reorganized + includes ddpq]
  +---------------------------------------------------------------------- 
  | ON ENTRY:
  | ========= 
  | ( Amat ) = original matrix A stored in C-style Compressed Sparse
  |            Row (CSR) format -- 
  |            see LIB/heads.h for the formal definition of this format.
  |
  | ipar[0:17]  = integer array to store parameters for 
  |       arms construction (arms2) 
  |
  |       ipar[0]:=nlev.  number of levels (reduction processes). 
  |                       see also "on return" below. 
  | 
  |       ipar[1]:= level-reordering option to be used.  
  |                 if ipar[1]==0 ARMS uses a block independent set ordering
  |                  with a sort of reverse cutill Mc Kee ordering to build 
  |                  the blocks. This yields a symmetric ordering. 
  |                  in this case the reordering routine called is indsetC
  |                 if ipar[1] == 1, then a nonsymmetric ordering is used.
  |                  In this case, the B matrix is constructed to be as
  |                  diagonally dominant as possible and as sparse as possble.
  |                  in this case the reordering routine called is ddPQ.
  |                 
  |       ipar[2]:=bsize. for indset  Dimension of the blocks. 
  |                  bsize is only a target block size. The size of 
  |                  each block can vary and is >= bsize. 
  |                  for ddPQ: this is only the smallest size of the 
  |                  last level. arms will stop when either the number 
  |                  of levels reaches nlev (ipar[0]) or the size of the
  |                  next level (C block) is less than bsize.
  |
  |       ipar[3]:=iout   if (iout > 0) statistics on the run are 
  |                       printed to FILE *ft
  |
  |       ipar[4-9] NOT used [reserved for later use] - set to zero.
  | 
  | The following set method options for arms2. Their default values can
  | all be set to zero if desired. 
  |
  |       ipar[10-13] == meth[0:3] = method flags for interlevel blocks
  |       ipar[14-17] == meth[0:3] = method flags for last level block - 
  |       with the following meaning in both cases:
  |            meth[0] nonsummetric permutations of  1: yes. affects rperm
  |                    USED FOR LAST SCHUR COMPLEMENT 
  |            meth[1] permutations of columns 0:no 1: yes. So far this is
  |                    USED ONLY FOR LAST BLOCK [ILUTP instead of ILUT]. 
  |                    (so ipar[11] does no matter - enter zero). If 
  |                    ipar[15] is one then ILUTP will be used instead 
  |                    of ILUT. Permutation data stored in: perm2. 
  |            meth[2] diag. row scaling. 0:no 1:yes. Data: D1
  |            meth[3] diag. column scaling. 0:no 1:yes. Data: D2
  |       all transformations related to parametres in meth[*] (permutation, 
  |       scaling,..) are applied to the matrix before processing it 
  | 
  | ft       =  file for printing statistics on run
  |
  | droptol  = Threshold parameters for dropping elements in ILU 
  |            factorization.
  |            droptol[0:4] = related to the multilevel  block factorization
  |            droptol[5:6] = related to ILU factorization of last block.
  |            This flexibility is more than is really needed. one can use
  |            a single parameter for all. it is preferable to use one value
  |            for droptol[0:4] and another (smaller) for droptol[5:6]
  |            droptol[0] = threshold for dropping  in L [B]. See piluNEW.c:
  |            droptol[1] = threshold for dropping  in U [B].
  |            droptol[2] = threshold for dropping  in L^{-1} F 
|            droptol[3] = threshold for dropping  in E U^{-1} 
|            droptol[4] = threshold for dropping  in Schur complement
|            droptol[5] = threshold for dropping  in L in last block
|              [see ilutpC.c]
|            droptol[6] = threshold for dropping  in U in last block
|              [see ilutpC.c]
|             This provides a rich selection - though in practice only 4
|             parameters are needed [which can be set to be the same 
actually] -- indeed it makes sense to take
|             droptol[0] = droptol[1],  droptol[2] = droptol[3], 
    |             and droptol[4] = droptol[5]
    |
    | lfil     = lfil[0:6] is an array containing the fill-in parameters.
    |            similar explanations as above, namely:
    |            lfil[0] = amount of fill-in kept  in L [B]. 
    |            lfil[1] = amount of fill-in kept  in U [B].
    |            lfil[2] = amount of fill-in kept  in E L\inv 
    |            lfil[3] = amount of fill-in kept  in U \inv F
    |            lfil[4] = amount of fill-in kept  in S    .
    |            lfil[5] = amount of fill-in kept  in L_S  .
    |            lfil[6] = amount of fill-in kept  in U_S 
    |             
    | tolind   = tolerance parameter used by the indset function. 
    |            a row is not accepted into the independent set if 
    |            the *relative* diagonal tolerance is below tolind.
    |            see indset function for details. Good values are 
    |            between 0.05 and 0.5 -- larger values tend to be better
    |            for harder problems.
    | 
    | ON RETURN:
    |=============
    |
    | (PreMat)  = arms data structure which consists of two parts:
    |             levmat and ilsch. 
    |
    | ++(levmat)= permuted and sorted matrices for each level of the block 
    |             factorization stored in PerMat4 struct. Specifically
    |             each level in levmat contains the 4 matrices in:
    |
    |
    |            |\         |       |
    |            |  \   U   |       |
    |            |    \     |   F   |
    |            |  L   \   |       |
    |            |        \ |       |
    |            |----------+-------|
    |            |          |       |
    |            |    E     |       |
    |            |          |       |
    |            
    |            plus a few other things. See LIB/heads.h for details.
    |
    | ++(ilsch) = IluSpar struct. If the block of the last level is:
    |
    |                        |  B    F |
    |                  A_l = |         | 
    |                        |  E    C |
    |
    |             then IluSpar contains the block C and an ILU
    |             factorization (matrices L and U) for the last 
    |             Schur complement [S ~ C - E inv(B) F ]
    |             (modulo dropping) see LIB/heads.h for details.
    |
| ipar[0]   = number of levels found (may differ from input value) 
    |
    +---------------------------------------------------------------------*/
int itsol_pc_arms2(ITS_SparMat *Amat, int *ipar, double *droptol, int *lfil, double tolind, ITS_ARMSpar *PreMat, FILE * ft)
{
    /*-------------------- function  prototyping  done in LIB/itsol.h    */
    /*-------------------- move above to itsol.h */
    ITS_Per4Mat *levp, *levc, *levn, *levmat = PreMat->levmat;
    ITS_SparMat *schur, *B, *F, *E, *C = NULL;
    ITS_ILUTSpar *ilsch = PreMat->ilus;
    /*-------------------- local variables  (initialized)   */
    double *dd1, *dd2;
    int nlev = ipar[0], bsize = ipar[2], iout = ipar[3], ierr = 0;
    int methL[4], methS[4];
    /*--------------------  local variables  (not initialized)   */
    int nA, nB, nC, j, n, ilev, symperm;
    /*--------------------    work arrays:    */
    int *iwork, *uwork;
    /*   timer arrays:  */
    /*   double *symtime, *unstime, *factime, *tottime; */
    /*---------------------------BEGIN ARMS-------------------------------*/
    /*   schur matrix starts being original A */

    /*-------------------- begin                                         */
    schur = (ITS_SparMat *) itsol_malloc(sizeof(ITS_SparMat), "arms2:1");
    /*---------------------------------------------------------------------
      | The matrix (a,ja,ia) plays role of Schur compl. from the 0th level.
      +--------------------------------------------------------------------*/
    nC = nA = n = Amat->n;
    if (bsize >= n)
        bsize = n - 1;
    levmat->n = n;
    levmat->nB = 0;
    itsol_setupCS(schur, n, 1);
    itsol_cscpy(Amat, schur);
    levc = levmat;
    /*--------------------------------------- */
    levc->prev = levc->next = levp = NULL;
    levc->n = 0;
    memcpy(methL, &ipar[10], 4 * sizeof(int));
    memcpy(methS, &ipar[14], 4 * sizeof(int));
    /*---------------------------------------------------------------------
      | The preconditioner construction is divided into two parts:
      |   1st part: construct and store multi-level L and U factors;
      |   2nd part: construct the ILUT factorization for the coarsest level
      +--------------------------------------------------------------------*/
    if ((iout > 0) && (nlev > 0)) {
        fprintf(ft, "  \n");
        fprintf(ft, "Level   Total Unknowns    B-block   Coarse set\n");
        fprintf(ft, "=====   ==============    =======   ==========\n");
    }
    /*---------------------------------------------------------------------
      | main loop to construct multi-level LU preconditioner. Loop is on the
      | level ilev. nA is the dimension of matrix A_l at each level.
      +--------------------------------------------------------------------*/
    for (ilev = 0; ilev < nlev; ilev++) {
        /*-------------------- new nA is old nC -- from previous level */
        nA = nC;
        if (nA <= bsize)
            goto label1000;
        /*-------------------- allocate work space                        */
        iwork = (int *)itsol_malloc(nA * sizeof(int), "arms2:2.5");
        symperm = 0;            /* 0nly needed in cleanP4 */
        if (ipar[1] == 1)
            uwork = (int *)itsol_malloc(nA * sizeof(int), "arms2:2.5");
        else {
            symperm = 1;
            uwork = iwork;
        }
        /*-------------------- SCALING*/
        dd1 = NULL;
        dd2 = NULL;
        if (methL[2]) {
            dd1 = (double *)itsol_malloc(nA * sizeof(double), "arms2:3");
            j = itsol_roscalC(schur, dd1, 1);
            if (j)
                printf("ERROR in roscalC -  row %d  is a zero row\n", j);
        }

        if (methL[3]) {
            dd2 = (double *)itsol_malloc(nA * sizeof(double), "arms2:4");
            j = itsol_coscalC(schur, dd2, 1);

            if (j) printf("ERROR in coscalC - column %d is a zero column\n", j);
        }
        /*--------------------independent-sets-permutation-------------------
          |  do reordering -- The matrix and its transpose are used.
          +--------------------------------------------------------------------*/
        /* if (SHIFTTOL > 0.0) shiftsD(schur,SHIFTTOL);    */
        //     printf("  ipar1 = %d \n", ipar[1]);
        if (ipar[1] == 1)
            itsol_PQperm(schur, bsize, uwork, iwork, &nB, tolind);
        else
            itsol_indsetC(schur, bsize, iwork, &nB, tolind);
        /*---------------------------------------------------------------------
          | nB is the total number of nodes in the independent set.
          | nC : nA - nB = the size of the reduced system.
          +--------------------------------------------------------------------*/
        nC = nA - nB;
        /*   if the size of B or C is zero , exit the main loop  */
        /*   printf ("  nB %d nC %d \n",nB, nC); */
        if (nB == 0 || nC == 0)
            goto label1000;
        /*---------------------------------------------------------------------
          | The matrix for the current level is in (schur).
          | The permutations arrays are in iwork and uwork (row).
          | The routines rpermC, cpermC permute the matrix in place.
         *-----------------------------------------------------------------------*/
        /*   DEBUG : SHOULD THIS GO BEFORE GOTO LABEL1000 ?? */
        itsol_rpermC(schur, uwork);
        itsol_cpermC(schur, iwork);
        /*   prtC(schur, ilev) ;   print matrix - debugging */
        /*-----------------------------------------------------------------------
          | If this is the first level, the permuted matrix is stored in 
          | (levc) = (levmat).  Otherwise, the next level is created in (levc).
          +--------------------------------------------------------------------*/
        if (ilev > 0) {
            /*-   delete C matrix of any level except last one (no longer needed) */
            itsol_cleanCS(C);

            /*-------------------- create the next level */
            levn = (ITS_Per4Mat *) itsol_malloc(sizeof(ITS_Per4Mat), "arms2:6");
            /* levc->prev = levp; */
            levc->next = levn;
            levp = levc;
            levc = levn;
            levc->prev = levp;
        }
        /*-------------------- ITS_Per4Mat struct for current schur complement */
        B = (ITS_SparMat *) itsol_malloc(sizeof(ITS_SparMat), "arms2:7");
        E = (ITS_SparMat *) itsol_malloc(sizeof(ITS_SparMat), "arms2:8");
        F = (ITS_SparMat *) itsol_malloc(sizeof(ITS_SparMat), "arms2:9");
        C = (ITS_SparMat *) itsol_malloc(sizeof(ITS_SparMat), "arms2:10");
        itsol_csSplit4(schur, nB, nC, B, F, E, C);
        itsol_setupP4(levc, nB, nC, F, E);

        /*--------------------     copy a few pointers       ---- */
        levc->perm = iwork;
        levc->rperm = uwork;
        levc->symperm = symperm;
        levc->D1 = dd1;
        levc->D2 = dd2;
        /*---------------------------------------------------------------------
          | a copy of the matrix (schur) has been permuted. Now perform the 
          | block factorization: 
          |
          | | B   F |       | L       0 |     | U  L^-1 F |
          | |       |   =   |           |  X  |           | = L x U
          | | E   C |       | E U^-1  I |     | 0    A1   |
          |   
          | The factors E U^-1 and L^-1 F are discarded after the factorization.
          |
          +--------------------------------------------------------------------*/
        if (iout > 0)
            fprintf(ft, "%3d %13d %13d %10d\n", ilev + 1, nA, nB, nC);
        /*---------------------------------------------------------------------
          | PILUT constructs one level of the block ILU fact.  The permuted matrix
          | is in (levc).  The L and U factors will be stored in the p4mat struct.
          | destroy current Schur  complement - no longer needed  - and set-up new
          | one for next level...
          +--------------------------------------------------------------------*/
        itsol_cleanCS(schur);
        schur = (ITS_SparMat *) itsol_malloc(sizeof(ITS_SparMat), "arms2:11");
        itsol_setupCS(schur, nC, 1);

        /*----------------------------------------------------------------------
          | calling PILU to construct this level block factorization
          | ! core dump in extreme case of empty matrices.
          +----------------------------------------------------------------------*/
        ierr = itsol_pc_pilu(levc, B, C, droptol, lfil, schur);
        /* prtC(levc->L, ilev) ; */
        if (ierr) {
            fprintf(ft, " ERROR IN  PILU  -- IERR = %d\n", ierr);
            return (1);
        }

        itsol_cleanCS(B);
    }

    /*---------------------------------------------------------------------
      |   done with the reduction. Record the number of levels in ipar[0] 
      |**********************************************************************
      +--------------------------------------------------------------------*/
 label1000:
    /* printf (" nnz_Schur %d \n",cs_nnz (schur)); */
    levc->next = NULL;
    ipar[0] = ilev;
    PreMat->nlev = ilev;
    PreMat->n = n;
    nC = schur->n;
    itsol_setupILUT(ilsch, nC);

    /*--------------------------------------------------------------------*/
    /* define C-matrix (member of ilsch) to be last C matrix */
    if (ilev > 0)
        ilsch->C = C;
    /*-------------------- for ilut fact of schur matrix */
    /*  SCALING  */

    ilsch->D1 = NULL;
    if (methS[2]) {
        ilsch->D1 = (double *)itsol_malloc(nC * sizeof(double), "arms2:iluschD1");
        j = itsol_roscalC(schur, ilsch->D1, 1);

        if (j) printf("ERROR in roscalC - row %d is a zero row\n", j);
    }

    ilsch->D2 = NULL;
    if (methS[3]) {
        ilsch->D2 = (double *)itsol_malloc(nC * sizeof(double), "arms2:iluschD1");
        j = itsol_coscalC(schur, ilsch->D2, 1);

        if (j) printf("ERROR in coscalC - column %d is a zero column\n", j);
    }

    /*---------------------------------------------------------------------
      |     get ILUT factorization for the last reduced system.
      +--------------------------------------------------------------------*/
    uwork = NULL;
    iwork = NULL;
    if (methS[0]) {
        iwork = (int *)itsol_malloc(nC * sizeof(int), "arms2:3");
        uwork = (int *)itsol_malloc(nC * sizeof(int), "arms2:3.5");
        tolind = 0.0;
        itsol_PQperm(schur, bsize, uwork, iwork, &nB, tolind);
        itsol_rpermC(schur, uwork);
        itsol_cpermC(schur, iwork);
    }
    ilsch->rperm = uwork;
    ilsch->perm = iwork;

    ilsch->perm2 = NULL;

    if (methS[1] == 0)
        ierr = itsol_pc_ilutD(schur, droptol, lfil, ilsch);
    else {
        ilsch->perm2 = (int *)itsol_malloc(nC * sizeof(int), "arms2:ilutpC");
        for (j = 0; j < nC; j++)
            ilsch->perm2[j] = j;
        ierr = itsol_pc_ilutpC(schur, droptol, lfil, PERMTOL, nC, ilsch, 0);
    }

    /*---------- OPTIMIZATION: NEED TO COMPOUND THE TWO
      RIGHT PERMUTATIONS -- CHANGES HERE AND IN 
      USCHUR SOLVE ==  compound permutations */
    if (ierr) {
        fprintf(ft, " ERROR IN  ILUT -- IERR = %d\n", ierr);
        return (1);
    }

    /*-------------------- Last Schur complement no longer needed */
    itsol_cleanCS(schur);

    return 0;
}

/*-------------------------------------------------*/
/* sets parameters required by arms preconditioner */
/* input ITS_IOT, Dscale                            */
/* output ipar tolcoef, lfil                       */
/*-------------------- trigger an error if not set */
void itsol_set_arms_pars(ITS_PARS *io, int Dscale, int *ipar, double *dropcoef, int *lfil)
{
    int j;

    for (j = 0; j < 17; j++)
        ipar[j] = 0;

    /*-------------------- */
    ipar[0] = ITS_MAX_NUM_LEV;      /* max number of levels allowed */
    ipar[1] = io->perm_type;    /* Indset (0) / PQ (1)    permutation   */

    /* note that these refer to completely  */
    /* different methods for reordering A   */
    /* 0 = standard ARMS independent sets   */
    /* 1 = arms with ddPQ ordering          */
    /* 2 = acoarsening-based ordering [new] */

    ipar[2] = io->Bsize;        /* smallest size allowed for last schur comp. */
    ipar[3] = 1;                /* whether or not to print statistics */

    /*-------------------- interlevel methods */
    ipar[10] = 0;               /* Always do permutations - currently not used  */
    ipar[11] = 0;               /* ILUT or ILUTP - currently only ILUT is implemented */
    ipar[12] = Dscale;          /* diagonal row scaling before PILUT 0:no 1:yes */
    ipar[13] = Dscale;          /* diagonal column scaling before PILUT 0:no 1:yes */

    /*-------------------- last level methods */
    ipar[14] = 1;               /* Always do permutations at last level */
    ipar[15] = 1;               /* ILUTP for last level(0 = ILUT at last level) */
    ipar[16] = Dscale;          /* diagonal row scaling  0:no 1:yes */
    ipar[17] = Dscale;          /* diagonal column scaling  0:no 1:yes */

    /*-------------------- set lfil */
    for (j = 0; j < 7; j++) {
        lfil[j] = io->ilut_p;
    }

    /*--------- dropcoef (droptol[k] = tol0*dropcoef[k]) ----- */
    dropcoef[0] = 1.6;          /* dropcoef for L of B block */
    dropcoef[1] = 1.6;          /* dropcoef for U of B block */
    dropcoef[2] = 1.6;          /* dropcoef for L\ inv F */
    dropcoef[3] = 1.6;          /* dropcoef for U\ inv E */
    dropcoef[4] = 0.004;        /* dropcoef for forming schur comple. */
    dropcoef[5] = 0.004;        /* dropcoef for last level L */
    dropcoef[6] = 0.004;        /* dropcoef for last level U */
}

