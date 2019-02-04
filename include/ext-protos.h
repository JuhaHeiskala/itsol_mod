#ifndef __ITSOL_EXT_PROTOS_H__
#define __ITSOL_EXT_PROTOS_H__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef min
#define min(a,b) (((a)>(b))?(b):(a))
#endif
#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif

#if defined(_IBM)
#define itsol_ddot(n,x,incx,y,incy)        FC_FUNC(ddot,DDOT)((n), (x), (incx), (y), (incy)) 
#define itsol_dscal(n,alpha,x,incx)        FC_FUNC(dscal,DSCAL)((n), (alpha), (x), (incx)) 
#define itsol_daxpy(n,alpha,x,incx,y,incy) FC_FUNC(daxpy,DAXPPY)((n), (alpha), (x), (incx), (y), (incy)) 
#define itsol_dnrm2(n,x,incx)              FC_FUNC(dnrm2,DNRM2)((n), (x), (incx))

#define itsol_dgemv(transa,m,n,alpha,a,lda,x,incx,beta,y,incy)		\
  FC_FUNC(dgemv,DGEMV)((transa), (m), (n),						\
	(alpha), (a), (lda), (x), (incx),				\
	(beta), (y), (incy))

#define itsol_dgemm(transa,transb,l,n,m,alpha,a,lda,b,ldb,beta,c,ldc)		\
  FC_FUNC(dgemm,DGEMM)((transa),(transb),						\
	(l),(n),(m),(alpha),(a),					\
	(lda),(b),(ldb),(beta),(c),(ldc))
#define itsol_dgetrf(m, n, a, lda, ipvt, info)  \
  FC_FUNC(dgetrf,DGETRF)((m), (n), (a), (lda), (ipvt), (info))
#define itsol_dgetri(n, a, lda, ipvt, work, lwork, info)		\
  FC_FUNC(dgetri,DGETRI)((n), (a), (lda), (ipvt), (work), (lwork), (info))
#else
#define itsol_ddot(n,x,incx,y,incy)        FC_FUNC(ddot,DDOT)(&(n),(x),&(incx),(y),&(incy))
#define itsol_dscal(n,alpha,x,incx)        FC_FUNC(dscal,DSCAL)(&(n),&(alpha),(x), &(incx))
#define itsol_daxpy(n,alpha,x,incx,y,incy) FC_FUNC(daxpy,DAXPPY)(&(n), &(alpha), (x), &(incx), y, &(incy))
#define itsol_dnrm2(n, x, incx)            FC_FUNC(dnrm2,DNRM2)(&(n), (x), &(incx))
#define itsol_dgemv(transa, m, n, alpha, a, lda, x, incx, beta, y, incy)  \
  FC_FUNC(dgemv,DGEMV)((transa), &(m), &(n), &(alpha), (a), &(lda), (x), &(incx), \
	 &(beta), (y), &(incy))

#define itsol_dgemm(transa,transb,l,n,m,alpha,a,lda,b,ldb,beta,c,ldc)	\
  FC_FUNC(dgemm,DGEMM)((transa), (transb), &(l), &(n), &(m), &(alpha), (a),	\
	 &(lda), b, &(ldb), &(beta), (c), &(ldc)) 

#define itsol_dgetrf(m, n, a, lda, ipvt, info)		\
  FC_FUNC(dgetrf,DGETRF)(&(m), &(n), (a), &(lda), (ipvt), (info))

#define itsol_dgetri(n, a, lda, ipvt, work, lwork, info)			\
  FC_FUNC(dgetri,DGETRI)(&(n), (a), &(lda), (ipvt), (work), &(lwork), (info))

/* FORTRAN routines */
void FC_FUNC(readmtc,READMTC)(int*,  int*,  int*,  char*,  double*,  int*,
        int*,  double*, int*,  char*,  int*,  int*,  int*,
        char*,  char*, char*,  int*) ;
void FC_FUNC(csrcsc,CSRCSC)(int*, int*, int*, double*, int*, int*, double*,
        int*, int*) ; 
void FC_FUNC(qsplit,QSPLIT)(double *a, int *ind, int *n, int *ncut);	
void FC_FUNC(dgesvd,DGESVD)(char*, char*, int*, int*, double*, int*, double*,
        double *, int*, double*, int*, double*, int*,
        int*); 
void FC_FUNC(csrcoo,CSRCOO)(int *, int *, int *, double *, int *, int *, int *,
        double *, int *, int *, int *);    

double FC_FUNC(ddot,DDOT)(int *n, double *x, int *incx, double *y, int *incy);  
void FC_FUNC(dscal,DSCAL)(int *n, double *alpha, double *x, int *incx);
void FC_FUNC(daxpy,DAXPPY)(int *n, double *alpha, double *x, int *incx, double *y, int *incy);
double FC_FUNC(dnrm2,DNRM2)(int *n, double *x, int *incx);
void FC_FUNC(dgemv,DGEMV)(char *transa, int *m, int *n, double *alpha, double *a, int *lda, double *x, int *incx,
        double *beta, double *y, int *incy);

void FC_FUNC(dgemm,DGEMM)(char *transa, char *transb, int *l, int *m, int *n, double *alpha, double *a, int *lda,
        double *b, int *ldb, double *beta, double *c, int *ldc);       

void FC_FUNC(dgetrf,DGETRF)(int *m, int *n, double *a, int *lda, int *ipvt, int *info); 
void FC_FUNC(dgetri,DGETRI)(int *n, double *a, int *lda, int *ipvt, double *work, int *lwork, int *info);

void FC_FUNC(gauss,GAUSS)(int *, double *, int *);
void FC_FUNC(bxinv,BXINV)(int *, int *, double *, double *, double *);

#endif 

#ifdef __cplusplus
}
#endif

#endif 
