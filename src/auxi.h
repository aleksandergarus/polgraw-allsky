/* auxi.h */

#ifndef AUXI_H
#define AUXI_H

#include <complex.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define TOSTRA(x) #x
#define TOSTR(x) TOSTRA(x)

#define TINY 1.0e-20
#define NINTERP 3       /* degree of the interpolation polynomial */
  /* Do not change this value!!! */
#define NAV 4096
#define sqr(x) ((x)*(x))
#define round(x) floor((x)+0.5)
#ifdef HAVE_SINCOS
  void sincos (double, double *, double *);
#endif

  int ludcmp (double *, int, int *, double *);
  int lubksb (double *, int, int *, double *);

  void spline(complex double *, int, complex double *);
  complex double splint (complex double *, complex double *, int, double);
  void splintpad (complex double *, double *, int, int, complex double*);
  double var (double *, int);
#ifdef __cplusplus
}
#endif

#endif                          /* AUXI_H */
