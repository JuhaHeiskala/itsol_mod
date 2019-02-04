#ifndef __ITSOL_EXT_PROTOS_H__
#define __ITSOL_EXT_PROTOS_H__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_SGI) || defined(_LINUX)
#define dnrm2   dnrm2_
#define ddot    ddot_
#define daxpy   daxpy_
#define qsplit  qsplit_
#define dscal   dscal_
#define dgemv   dgemv_
#define dgemm   dgemm_
#define dgetrf  dgetrf_
#define dgetri  dgetri_
#define dgesvd  dgesvd_
#define readmtc readmtc_
#define csrcsc  csrcsc_
#define roscal  roscal_
#define coscal  coscal_
#define qsplit  qsplit_
#define bxinv   bxinv_
#define gauss   gauss_
#elif defined(_IBM)
#include <essl.h>
#define dnrm2   dnrm2
#define ddot    ddot
#define daxpy   daxpy
#define qsplit  qsplit
#define dscal   dscal
#define dgemv   dgemv
#define dgemm   dgemm
#define dgetrf  dgetrf
#define dgetri  dgetri
#define dgesvd  dgesvd
#define readmtc readmtc
#define csrcsc  csrcsc 
#define roscal  roscal
#define coscal  coscal
#define qsplit  qsplit
#define bxinv   bxinv
#define gauss   gauss
#else
#define dnrm2   dnrm2_
#define ddot    ddot_
#define daxpy   daxpy_
#define qsplit  qsplit_
#define dscal   dscal_
#define dgemv   dgemv_
#define dgemm   dgemm_
#define dgetrf  dgetrf_
#define dgetri  dgetri_
#define dgesvd  dgesvd_
#define readmtc readmtc_
#define csrcsc  csrcsc_
#define roscal  roscal_
#define coscal  coscal_
#define qsplit  qsplit_
#define bxinv   bxinv_
#define gauss   gauss_
#endif

#ifndef min
#define min(a,b) (((a)>(b))?(b):(a))
#endif
#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif

#if defined(_IBM)
#define itsol_ddot(n,x,incx,y,incy)        ddot((n), (x), (incx), (y), (incy)) 
#define itsol_dscal(n,alpha,x,incx)        dscal((n), (alpha), (x), (incx)) 
#define itsol_daxpy(n,alpha,x,incx,y,incy) daxpy((n), (alpha), (x), (incx), (y), (incy)) 
#define itsol_dnrm2(n,x,incx)              dnrm2((n), (x), (incx))

#define itsol_dgemv(transa,m,n,alpha,a,lda,x,incx,beta,y,incy)		\
  dgemv((transa), (m), (n),						\
	(alpha), (a), (lda), (x), (incx),				\
	(beta), (y), (incy))

#define itsol_dgemm(transa,transb,l,n,m,alpha,a,lda,b,ldb,beta,c,ldc)		\
  dgemm((transa),(transb),						\
	(l),(n),(m),(alpha),(a),					\
	(lda),(b),(ldb),(beta),(c),(ldc))
#define itsol_dgetrf(m, n, a, lda, ipvt, info)  \
  dgetrf((m), (n), (a), (lda), (ipvt), (info))
#define itsol_dgetri(n, a, lda, ipvt, work, lwork, info)		\
  dgetri((n), (a), (lda), (ipvt), (work), (lwork), (info))
#else
#define itsol_ddot(n,x,incx,y,incy)        ddot(&(n),(x),&(incx),(y),&(incy))
#define itsol_dscal(n,alpha,x,incx)        dscal(&(n),&(alpha),(x), &(incx))
#define itsol_daxpy(n,alpha,x,incx,y,incy) daxpy(&(n), &(alpha), (x), &(incx), y, &(incy))
#define itsol_dnrm2(n, x, incx)            dnrm2(&(n), (x), &(incx))
#define itsol_dgemv(transa, m, n, alpha, a, lda, x, incx, beta, y, incy)  \
  dgemv((transa), &(m), &(n), &(alpha), (a), &(lda), (x), &(incx), \
	 &(beta), (y), &(incy))
#define itsol_dgemm(transa,transb,l,n,m,alpha,a,lda,b,ldb,beta,c,ldc)	\
  dgemm((transa), (transb), &(l), &(n), &(m), &(alpha), (a),	\
	 &(lda), b, &(ldb), &(beta), (c), &(ldc)) 
#define itsol_dgetrf(m, n, a, lda, ipvt, info)		\
  dgetrf(&(m), &(n), (a), &(lda), (ipvt), (info))
#define itsol_dgetri(n, a, lda, ipvt, work, lwork, info)			\
  dgetri(&(n), (a), &(lda), (ipvt), (work), &(lwork), (info))

/* FORTRAN routines */
void readmtc(int*,  int*,  int*,  char*,  double*,  int*,
        int*,  double*, int*,  char*,  int*,  int*,  int*,
        char*,  char*, char*,  int*) ;
void csrcsc(int*, int*, int*, double*, int*, int*, double*,
        int*, int*) ; 
void qsplit(double *a, int *ind, int *n, int *ncut);	
void dgesvd(char*, char*, int*, int*, double*, int*, double*,
        double *, int*, double*, int*, double*, int*,
        int*); 
void csrcoo(int *, int *, int *, double *, int *, int *, int *,
        double *, int *, int *, int *);    

double ddot(int *n, double *x, int *incx, double *y, int *incy);  
void dscal(int *n, double *alpha, double *x, int *incx);
void daxpy(int *n, double *alpha, double *x, int *incx, double *y, int *incy);
double dnrm2(int *n, double *x, int *incx);
void dgemv(char *transa, int *m, int *n, double *alpha, double *a, int *lda, double *x, int *incx,
        double *beta, double *y, int *incy);

void dgemm(char *transa, char *transb, int *l, int *m, int *n, double *alpha, double *a, int *lda,
        double *b, int *ldb, double *beta, double *c, int *ldc);       

void dgetrf(int *m, int *n, double *a, int *lda, int *ipvt, int *info); 
void dgetri(int *n, double *a, int *lda, int *ipvt, double *work, int *lwork, int *info);
#endif 

#ifdef __cplusplus
}
#endif

#endif 
