#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <complex.h>
#include <fftw3.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>

#include <time.h>

#include "auxi.h"
#include "lvcvirgo.h"
#include <mpi.h>

static int white_flag=0; 							//  white noise flag
static int s0_flag=0;								// no spin-down flag
static int help_flag=0; 
static int no_out=1;								// Switch off/on the debugging outputs

/* Default output and data directories */

#ifndef PREFIX
#define PREFIX .
#endif

#ifndef DTAPREFIX
#define DTAPREFIX ../data
#endif

double *cosmodf, *sinmodf, *xDat, *DetSSB, *t2;

FILE *joblist;
char jobname[64];

int
JobNAllSky (int argc, char *argv[]) {
  int i, pm, mm, nn, pst, mst, nst, sst, fd, hemi=0, Nzeros=0, Ninterp, FNum, nmin, nmax, spndr[2], nr[65536], mr[65536], pmr[2], c, nopig, ident=0, band=0, nfftf, sgnlc,
	fftinterp=INT; // default value
  char hostname[32], wfilename[96], filename[64], outname[64], slavefiles[64], prefix[64], dtaprefix[64], label[64], range[64], *wd=NULL;
  double *signal, omrt, coft, epsm, sepsm, cepsm, phir, sphir, cphir,  
    *M, // not used in current search *gamrn, *Mn, 
	trl=20., // default value for the F-statistic threshold
	fpo, sig2;
  fftw_complex *xa, *xb, *xao, *xbo, *xDa, *xDb, *rDa, *rDb;
  fftw_plan plan, pl_int, pl_inv;
  FILE *wisdom, *data, *slavejobs;
  struct flock lck;
  struct stat buffer;
  
  clock_t uptime, downtime ;
  double time_in_seconds ; 	
  int mpiid, numtasks,stepsize;
  
  MPI_Init (&argc, &argv);
  MPI_Comm_size (MPI_COMM_WORLD, &numtasks);
  MPI_Comm_rank (MPI_COMM_WORLD, &mpiid);
  
  strcpy (prefix, TOSTR(PREFIX));
  strcpy (dtaprefix, TOSTR(DTAPREFIX));
  label[0] = '\0';
  range[0] = '\0';

  while (1) {
    static struct option long_options[] = {
      {"help", no_argument, &help_flag, 1},	
      {"whitenoise", no_argument, &white_flag, 1},
      {"nospindown", no_argument, &s0_flag, 1},
      // frame number
      {"ident", required_argument, 0, 'i'},
      // frequency band number 
      {"band", required_argument, 0, 'b'},
      // output directory
      {"output", required_argument, 0, 'o'},
      // input data directory 
      {"data", required_argument, 0, 'd'},
      // non-standard label for naming files 
      {"label", required_argument, 0, 'l'},
      // narrower grid range parameter file
      {"range", required_argument, 0, 'r'},
      // change directory parameter
      {"cwd", required_argument, 0, 'c'},
      // interpolation method
      {"int/fft", required_argument, 0, 'f'},
      // interpolation method
      {"threshold", required_argument, 0, 't'},
      // hemisphere
      {"hemisphere", required_argument, 0, 'h'},
      {0, 0, 0, 0}
    };

  if(help_flag) {

     printf("*** Continuous GW search code using the F-statistic ***\n"); 
     printf("Usage: ./search -[switch1] <value1> -[switch2] <value2> ...\n") ;
     printf("Switches are:\n\n"); 
     printf("-d		Data directory (default is .)\n"); 
     printf("-o		Output directory (default is ./candidates)\n"); 
     printf("-i		Frame number\n"); 
     printf("-b		Band number\n"); 
     printf("-l		Custom label for the input and output files\n"); 
     printf("-r		Grid range filename\n"); 
     printf("-c		Change to directory <dir>\n"); 
     printf("-f		Intepolation method (INT [default] or FFT)\n"); 
     printf("-t		Threshold for the F-statistic (default is 20)\n\n");
     printf("-h         Hemisphere (default is 0 - does both)\n\n");
     printf("Also:\n"); 
     printf("--whitenoise	Will assume white Gaussian noise\n"); 
     printf("--nospindown	Will assume that spindown is not important\n"); 
     printf("--help		This help\n"); 		

     exit (0);

  }

    int option_index = 0;
    c = getopt_long (argc, argv, "i:b:o:d:l:r:c:t:f:h:", long_options,	\
		     &option_index);
    if (c == -1)
      break;

    switch (c) {
    case 'i':
      ident = atoi (optarg);
      break;
    case 'f': 
      if(!strcmp(optarg, "FFT")) fftinterp=FFT;
      break;
    case 't':
      trl = atof(optarg);
      break;
    case 'h':
      hemi = atoi(optarg);
      break;
    case 'b':
      band = atoi (optarg);
      break;
    case 'o':
      strcpy (prefix, optarg);
      break;
    case 'd':
      strcpy (dtaprefix, optarg);
      break;
    case 'l':
      label[0] = '_';
      strcpy (1+label, optarg);
      break;
    case 'r':
      strcpy (range, optarg);
      break;
    case 'c':
      wd = (char *) malloc (1+strlen(optarg));
      strcpy (wd, optarg);
      break;
    case '?':
      break;
    default: break ; 
    } /* switch c */
  } /* while 1 */
  
if(no_out) {
  sprintf(slavefiles, "joblist_%d", mpiid);
  slavejobs = fopen(slavefiles,"w");
  fprintf (slavejobs,"#Interation step mpiid mm nn time_of_loop\n");
  fclose (slavejobs);

  sprintf(jobname, "slave_%d_output", mpiid);
  joblist = fopen(jobname,"w");
  fprintf (joblist,"Data directory is %s\n", dtaprefix);
  fprintf (joblist,"Output directory is %s\n", prefix);
  fprintf (joblist,"Frame number is %d\n", ident);
  fprintf (joblist,"Band number is %d\n", band);    
  if (white_flag)
    fprintf (joblist,"Will assume white Gaussian noise\n");
  if (fftinterp==INT)
    fprintf (joblist,"Will use fftinterp=INT (FFT interpolation by interbinning)\n");
  else 
    fprintf (joblist,"Will use fftinterp=FFT (FFT interpolation by zero-padding)\n");
  if(trl!=20)
    fprintf (joblist,"Threshold for the F-statistic is %lf\n", trl);
  if(hemi)
    fprintf (joblist,"Search for hemisphere %d\n", hemi);
  if (s0_flag)
    fprintf (joblist,"Will assume s_1 = 0.\n");
  if (strlen(label))
    fprintf (joblist,"Will use '%s' as datat label\n", label);
  if (strlen(range))
    fprintf (joblist,"Will read grid range from '%s'\n", range);
  if (wd) {
    fprintf (joblist,"Changing working directory to %s\n", wd);
    if (chdir(wd)) {
      perror (wd);
      abort ();
    }
  }
  fclose (joblist);
}
  // Output data handling 
  if (stat(prefix, &buffer) == -1) {
    if (errno == ENOENT) {
      /* Output directory apparently does not exist, try to create one */
      if (mkdir(prefix, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH	\
		| S_IXOTH) == -1) {
	perror (prefix);
	return 1;
      }
    } else { /* can't access output directory */
      perror (prefix);
      return 1;
    }
  }

  M = (double *) calloc (16, sizeof (double));
  //gamrn = (double *) calloc (16, sizeof (double));
  //Mn = (double *) calloc (16, sizeof (double));
  sprintf (filename, "%s/%02d/grid.bin", dtaprefix, ident);
  if ((data=fopen (filename, "r")) != NULL) {
    // fftpad: used to zero padding to fftpad*nfft data points 
    fread ((void *)&fftpad, sizeof (int), 1, data);
    // M: vector of 16 components consisting of 4 rows 
    // of 4x4 grid-generating matrix  
    fread ((void *)M, sizeof (double), 16, data);
    // Following two matrices are not used in current search 
    // gamrn: normalised Fisher matrix 
    //fread ((void *)gamrn, sizeof (double), 16, data);
    // Mn: normalised grid-generating matrix 
    //fread ((void *)Mn, sizeof (double), 16, data);
    fclose (data);
  } else {
    perror (filename);
    return 1;
  }

  // Starting band frequency
  fpo = 100. + 0.96875 * band;
  // Detector, ephemerides etc. constants 
  lvcvirgo (fpo);
  // Amplitude modulation functions coefficients
  rogcvir ();
  // Establish grid range in which the search will be performed 
  // with the use of the M matrix from grid.bin  
  nopig=gridr (M, spndr, nr, mr);
if(no_out) {
  joblist = fopen(jobname,"a");
  fprintf(joblist,"Parameter ranges: Spin Down [%d %d] Sky position 1 [%d %d] Sky position 2 [%d %d]\n", spndr[0], spndr[1], nr[0], nr[1], mr[0], mr[1]) ; 
  fclose (joblist);
  }
  // Hemispheres (with respect to the ecliptic)
  if(hemi) { 

	pmr[0] = hemi; pmr[1] = hemi; 

  } else { 

  	pmr[0] = 1;
  	pmr[1] = 2;

  }

  // If the parameter range is invoked, the search is performed 
  // within the range of grid parameters from an ascii file 
  // ("-r range_file" from the command line) 
  if (strlen (range)) {
    if ((data=fopen (range, "r")) != NULL) {
      fscanf (data, "%d %d", spndr, 1+spndr);
      fscanf (data, "%d %d", nr, 1+nr);
      fscanf (data, "%d %d", mr, 1+mr);
      fscanf (data, "%d %d", pmr, 1+pmr);
      fclose (data);
    } else {
      perror (range);
      return 1;
    }
  }

  // Allocates and initializes to zero the data and the detector ephemeris 
  xDat = (double *) calloc (N, sizeof (double));
  DetSSB = (double *) calloc (3*N, sizeof (double));
  //# mb now defined locally in JobCore()
  //F = (double *) calloc (2*nfft, sizeof (double));

  // Input time-domain data handling 
  sprintf (filename, "%s/%02d/xdat_%02d_%03d%s.bin", dtaprefix, ident, ident, band, label);
  if ((data = fopen (filename, "r")) != NULL) {
    fread ((void *)(xDat), sizeof (double), N, data);
    fclose (data);
  } else {
    perror (filename);
    return 1;
  }

  // Checking for null values in the data 
  for(i=0; i<N; i++)
	if(!xDat[i]) Nzeros++;
  
  if (white_flag)
    // factor N/(N - Nzeros) to account for null values in the data 
    sig2 = N*var (xDat, N)*N/(N - Nzeros);
  else
    sig2 = -1.;

  // Ephemeris file handling 
  sprintf (filename, "%s/%02d/DetSSB.bin", dtaprefix, ident);
  if ((data = fopen (filename, "r")) != NULL) {
    // Detector position w.r.t solar system baricenter
    // for every datapoint
    fread ((void *)(DetSSB), sizeof (double), 3*N, data);
    // Deterministic phase defining the position of the Earth 
    // in its diurnal motion at t=0  
    fread ((void *)(&phir), sizeof (double), 1, data);
    // Earth's axis inclination to the ecliptic at t=0  
    fread ((void *)(&epsm), sizeof (double), 1, data);
    fclose (data);
  } else {
    perror (filename);
    return 1;
  }

#ifdef HAVE_SINCOS
  sincos (phir, &sphir, &cphir);
  sincos (epsm, &sepsm, &cepsm);
#else
  sphir = sin (phir);
  cphir = cos (phir);
  sepsm = sin (epsm);
  cepsm = cos (epsm);
#endif
     
  cosmodf = (double *) calloc (N, sizeof (double));
  sinmodf = (double *) calloc (N, sizeof (double));
  t2	  = (double *) calloc (N, sizeof (double));

  for (i=0; i<N; i++) {
    omrt = omr*i;
    t2[i] = sqr((double)i); 
    cosmodf[i] = cos (omrt);
    sinmodf[i] = sin (omrt);
  }

  // Because of frequency-domain filters, we search 
  // F-statistic in range (nmin+1, nmax) of data points 
  nmin = 2*NAV;
  nmax = nfft-2*NAV;

  coft = oms; 

  // Imports a "wisdom file" containing information about how to optimally 
  // compute Fourier transforms on a given machine. If such file is not 
  // present, it will be created after the measure runs of the fft_plans 
  // are performed below 
  // (more info at http://www.fftw.org/fftw3_doc/Wisdom.html) 
  gethostname (hostname, 32);
  sprintf (wfilename, "wisdom-%s.dat", hostname);
  if ((wisdom = fopen (wfilename, "r")) != NULL) {
    fftw_import_wisdom_from_file (wisdom);
    fclose (wisdom);
  }

  xa = fftw_malloc (4 * fftpad * nfft * sizeof (fftw_complex));

  // FFT plans vary w.r.t the method of calculation: 
  // case INT (simple interpolation [interbinning] of a shorter Fourier transform)
  if(fftinterp==INT) { 

	xb = xa + nfft;
	xao = fftw_malloc (2 * nfft * sizeof (fftw_complex));
	xbo = xao + nfft;
 
	// Plans a multidimensional DFT, where the input variables are:
	plan = fftw_plan_many_dft 
			(1,	// dimension of the transform (rank) 
			&nfft,	// size of the transform 
			2,	// how many transforms (the k-th transform 
				// is of the array starting at xa + k*idist
				// and xao + k*odist) 
			xa,	// input array
			NULL,	// inembed (defines auxilary array 
				// of size nfft)
			1,	// input stride (the j-th element of 
				// the input array is located at j*istride)
			nfft,	// idist (needed if many transforms) 
			xao,	// output array
			NULL,	// onembed (defines auxilary array 
                                // of size nfft)
			1,	// output stride (the j-th element of 
                                // the output array is located at j*ostride)
			nfft,	// odist (needed if many transforms)
			FFTW_FORWARD,	// sign of the transform 
					// (-1 in the exponent)
			FFTW_MEASURE);	// to get the optimal plan
	// (more info at http://www.fftw.org/fftw3_doc/Advanced-Complex-DFTs.html)

  // Case FFT (longer Fourier transforms, interplation by zero padding)
  } else { 

	nfftf = fftpad*nfft ; 
	xb = xa + nfftf ; 

	// Plans a multidimensional DFT, where the input variables are:
	plan = fftw_plan_many_dft 
                        (1,     // dimension of the transform (rank) 
                        &nfftf, // size of the transform 
                        2,      // how many transforms (the k-th transform 
                                // is of the array starting at xa + k*idist
                                // and xao + k*odist) 
                        xa,     // input array
                        NULL,   // inembed (defines auxilary array 
                                // of size nfft)
                        1,      // input stride (the j-th element of 
                                // the input array is located at j*istride)
                        nfftf,  // idist (needed if many transforms) 
                        xa,     // output array
                        NULL,   // onembed (defines auxilary array 
                                // of size nfft)
                        1,      // output stride (the j-th element of 
                                // the output array is located at j*ostride)
                        nfftf,  // odist (needed if many transforms)
                        FFTW_FORWARD,   // sign of the transform 
                                        // (-1 in the exponent)
                        FFTW_MEASURE);  // to get the optimal plan
  	// (more info at http://www.fftw.org/fftw3_doc/Advanced-Complex-DFTs.html)

 } 


  // These two plans below are used in the resampling 
  // procedure in JobCore() 
  xDa = xa;
  xDb = xDa + nfft;
  pl_int = fftw_plan_many_dft (1, &nfft, 2, xDa, NULL, 1, nfft, xDa,	\
			       NULL, 1, nfft, FFTW_FORWARD,		\
			       FFTW_MEASURE);

  rDa = xa;
  Ninterp = interpftpad*nfft;
  rDb = rDa + Ninterp;
  pl_inv = fftw_plan_many_dft (1, &Ninterp, 2, rDa, NULL, 1, Ninterp,	\
			       rDa, NULL, 1, Ninterp, FFTW_BACKWARD,	\
			       FFTW_MEASURE);

  // Generates a 'wisdom' FFT file if there is none  
  if ((wisdom = fopen (wfilename, "r")) == NULL) {

	wisdom = fopen (wfilename, "w");
	fftw_export_wisdom_to_file (wisdom);
  }
  fclose (wisdom);

  //if(hemi)
  	//sprintf (qname, "state_%02d_%03d%s_%d.dat", ident, band, label, hemi);
  //else 
	//sprintf (qname, "state_%02d_%03d%s.dat", ident, band, label);

  //// Checkpointing 
  //if ((state = fopen (qname, "r")) != NULL) {

    //// Scan the state file to get last recorded parameters
    //if ((fscanf (state, "%d %d %d %d %d", &pst, &mst, &nst, &sst, &FNum) 
	 //) == EOF) {

	//// This means that state file is empty (=end of the calculations)
	//fprintf (stderr, "state file empty: ending...\n") ;
	//fclose (state);
	//return 0;

    //} fclose (state);

  //// No state file - start from the beginning
  //} else {

    pst = pmr[0];
    mst = mr[0];
    nst = nr[0];
    sst = spndr[0];
    FNum = 0;
	signal = (double *) malloc (8192*sizeof(double));
	sgnlc = 0;
  
//  } /* if state */

  // Main loops 
  for (pm=pst; pm<=pmr[1]; pm++) 
	{	//loop over hemispheres 
		int iterstep;
		sprintf (outname, "%s/triggers_%02d_%03d%s_%d_%d.bin", prefix, ident, band, label, pm, mpiid);
		stepsize = ceil((double)nopig / numtasks);

		if(no_out) {
		joblist = fopen(jobname,"a");
		fprintf (joblist,"Number of tasks=%d My mpiid=%d Number of skypoints=%d stepsize=%d\n",numtasks,mpiid,nopig,stepsize);
		fclose (joblist);
		}
				//for (mm=mst; mm<=mr[1]; mm++) {	// 2 loops over 
				//for (nn=nst; nn<=nr[1]; nn++) {	// sky positions 

		for (iterstep=0; iterstep<=stepsize; iterstep++ ) 
		{int loop_id;
			loop_id    = mpiid + iterstep*numtasks;
			mm=mr[loop_id]; 
			nn=nr[loop_id]; 
			if (loop_id<=nopig) 	
			{
				if(no_out) {
				slavejobs = fopen(slavefiles,"a");
				fprintf (slavejobs,"%d %d %d %d ",iterstep,mpiid,mm,nn);
				fclose (slavejobs);
				}

				//state = fopen (qname, "w");
				//fprintf (state, "%d %d %d %d %d\n", pm, mm, nn, sst, FNum);
				//fclose (state);

				uptime = clock() / (CLOCKS_PER_SEC / 1000 ) ; 
			
				int err;
				err=JobCore(pm,		// hemisphere 
					mm,		// grid 'sky position' 		
					nn,		// other grid 'sky position' 
					sst,		// grid spindown coordinate
					spndr[1],	// spindown range limit 	
					M,		// grid generating matrix 
					DetSSB,		// ephemerides array 
					xDat,		// time-domain input data array 
					N,		// Number of data points
					Ninterp,	// interpftpad*nfft (for resampling)
					nfft,		// size of the FFT transform 
					xDa,		// Array for resampling
					rDa,		// Array for resampling
					xa,		// input array for plan
					xao,		// output array for plan
					pl_int,		// fftw_plan needed for resampling
					pl_inv,		// fftw_plan needed for resampling
							// (inverse transformation)
					plan,		// main fftw_plan 
					nmin,		// nmin+1: first point of 
							// the F-statistic to search for a signal
					nmax,		// nmax: last point of 
							// the F-statistic to search for a signal
					sepsm,		// sin(epsm)
					cepsm,		// cos(epsm)
					sphir,		// sin(phi_r)
					cphir,		// cos(phi_r)
					signal,	// reference to array with the parameters 
					&sgnlc,		// of the candidate signal 
							// (used below to write to the file) 
					1,		// std output writing flag
					fftinterp,	// interpolation flag 
							// (INT of FFT, see lvcvirgo.h) 
					&FNum,		// Candidate signal number 
					coft,		// = oms 
					trl,		// F-statistic threshold
					sig2,		// N*(variance of xDat) if white_flag
							// else sig2=-1 
					s0_flag		// No-spindown flag
					);

				sst = spndr[0];

 				downtime = clock() / (CLOCKS_PER_SEC / 1000 ) ; 
				
				time_in_seconds = (double)(downtime - uptime) / 1000 ;  

				if(no_out) {
					slavejobs = fopen(slavefiles,"a");
					fprintf (slavejobs,"%.3lf\n", time_in_seconds);
					fclose (slavejobs);
					if(time_in_seconds)
					{
						joblist = fopen(jobname,"a");
						fprintf(joblist,"\nTime in JobCore [s]: %.3lf ", time_in_seconds) ; 
						fprintf(joblist,"Interation step= %d mpiid= %d mm= %d nn= %d Amount of signals=%d\n",iterstep,mpiid,mm,nn,sgnlc);
						fclose (joblist);
					}
				}		
			} // for nn
			//nst = nr[0];
			else
			{
			if(no_out) {
			joblist = fopen(jobname,"a");
			fprintf (joblist,"Out of range nn=%d Loop_id=%d mpiid=%d Number of Skypoints=%d\n",nn,loop_id,mpiid,nopig);
			fclose (joblist);
			}
			}
		} // for 
		//mst = mr[0];
	} // for pm


				
				// Add trigger parameters to a file
				if (sgnlc) 
				{
				if ((fd = open (outname, O_WRONLY|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0) 
					{
						perror (outname);
						return 1;
					}
				lck.l_type = F_WRLCK;
				lck.l_whence = 0;
				lck.l_start = 0L;
				lck.l_len = 0L;
				if (fcntl (fd, F_SETLKW, &lck) < 0) perror ("fcntl()");

				write (fd, (void *)(signal), sgnlc*NPAR*sizeof (double));

				if (close (fd) < 0) perror ("close()");

				} // if sgnlc
				


    //state = fopen (qname, "w");
    //fclose (state);
	
	
  //# mb these arrays are now defined and freed locally in JobCore()	
  //free (aa);
  //free (bb);
  //free (xDatma);
  //free (xDatmb);
  //free (shft);
  //free (shftf);

  free (t2); 
  free (xDat);
  free (DetSSB);
  
  free (cosmodf);
  free (sinmodf);
  fftw_free (xa);
  // This array is defined only in the case of fftinterp==INT
  if(fftinterp==INT) fftw_free (xao);
  free (M);
  MPI_Finalize (); //inventing MPI
  return 0;
} /* JobNAllSky() */
