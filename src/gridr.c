#include <math.h>

#include "auxi.h"
#include "lvcvirgo.h"

int 
gridr (double *M, int *spndr, int *nr, int *mr) {

  double cof, Mp[16], smx[64], d, Ob, al1, al2;
  int i, j, nr0, nr1, mr0, mr1, k, indx[4];

  /* Grid range */

// input: 
// *M - pointer to the array that generates the grid
// *spndr - pointer to the range of spindowns in grid units 
// i.e., integer numbers    
// *nr and *mr - pointers to the range of sky positions 
// in grid units i.e., integer numbers
 
// from lvcvirgo() :
// maximal value of the spindown:
// Smax = 2.*M_PI*(fpo+B)*dt*dt/(2.*tau_min)   
// 
// oms equals 2.*M_PI*fpo*dt 

  Ob = M_PI;
  cof = oms + Ob;

  for (i=0; i<4; i++)
    for (j=0; j<4; j++)
      Mp[4*i+j] = M[4*j+i];
  ludcmp (Mp, 4, indx, &d);

  for (i=0; i<8; i++) {
    smx[8*i+2] = cof;
    smx[8*i+6] = -cof;
  }
  for (i=0; i<4; i++) {
    smx[16*i+3] = smx[16*i+7] = cof;
    smx[16*i+11] = smx[16*i+15] = -cof;
  }
  for (i=0; i<8; i++) {
    smx[4*i] = Ob;
    smx[4*i+32] = -Ob;
  }
  for (i=0; i<2; i++)
    for (j=0; j<4; j++) {
      smx[32*i+4*j+1] = -Smax;
      smx[32*i+4*j+17] = 0.;
    }
  for (i=0; i<16; i++)
    lubksb (Mp, 4, indx, smx+4*i);

  spndr[0] = nr0 = mr0 = 16384;
  spndr[1] = nr1 = mr1 = -16384;

  for (i=0; i<16; i++) {
    if (floor(smx[4*i+1]) < spndr[0])
      spndr[0] = floor(smx[4*i+1]);
    if (ceil(smx[4*i+1]) > spndr[1])
      spndr[1] = ceil(smx[4*i+1]);

    if (floor(smx[4*i+2]) < nr0)
      nr0 = floor(smx[4*i+2]);
    if (ceil(smx[4*i+2]) > nr1)
      nr1 = ceil(smx[4*i+2]);

    if (floor(smx[4*i+3]) < mr0)
      mr0 = floor(smx[4*i+3]);
    if (ceil(smx[4*i+3]) > mr1)
      mr1 = ceil(smx[4*i+3]);
  }


  k=0; 
  for(i=nr0; i<nr1; i++)
	for(j=mr0; j<mr1; j++) { 

		// Checking if we are inside the sky ellipse
		al1 = i*M[10] + j*M[14];
		al2 = i*M[11] + j*M[15];
		if((sqr(al1)+sqr(al2))/sqr(oms) <= 1.) { 
			nr[k] = i ; 
			mr[k] = j ;  
			k++; 
		}
	}

  nr[0] = nr0 ; 
  nr[1] = nr1 ; 

  mr[0] = mr0 ; 
  mr[1] = mr1 ;   
  
  // number of points in the sky ellipse
  return k ; 	

} /* gridr() */


