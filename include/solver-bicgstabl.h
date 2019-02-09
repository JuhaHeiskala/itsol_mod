
#ifndef __ITSOL_BICGSTABL_H__
#define __ITSOL_BICGSTABL_H__

#include "mat-utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------
|                 *** Preconditioned BiCGSTAB(l) ***                  
+-----------------------------------------------------------------------
| This is a simple version of the ARMS preconditioned FGMRES algorithm. 
+-----------------------------------------------------------------------
| Y. S. Dec. 2000. -- Apr. 2008   -- Jul. 2009 
+-----------------------------------------------------------------------
| on entry:
|---------- 
|
|(Amat)   = matrix struct. the matvec operation is Amat->matvec.
|(lu)     = preconditioner struct.. the preconditioner is lu->precon
|           if (lu == NULL) the no-preconditioning option is invoked.
| rhs     = real vector of length n containing the right hand side.
| sol     = real vector of length n containing an initial guess to the
|           solution on input.
| bgsl    = l  parameter
| tol     = tolerance for stopping iteration
| (maxits) = max number of iterations allowed. 
| fp      = NULL: no output
|        != NULL: file handle to output " resid vs time and its" 
|
| on return:
|---------- 
| fgmr      int =  0 --> successful return.
|           int =  1 --> convergence not achieved in itmax iterations.
| sol     = contains an approximate solution (upon successful return).
| nits    = has changed. It now contains the number of steps required
|           to converge -- 
| res     = relative residual.
+-----------------------------------------------------------------------
| subroutines called :
|     matvec and
|     preconditionning operation 
+---------------------------------------------------------------------*/
int itsol_solver_bicgstabl(ITS_SMat *Amat, ITS_PC *lu, double *rhs, double *sol, int bgsl, double tol,
        int maxits, int *nits, double *res, FILE *fp);

#ifdef __cplusplus
}
#endif
#endif