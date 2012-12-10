#$(GSL_LIB)  $(GSL_INC) 
CC = mpicc
CFLAGS	= -Wall -O2 $(FFTW_INC) -DHAVE_SINCOS -DPREFIX="./candidates" -DDTAPREFIX="." 
LDFLAGS = $(FFTW_LIB) 
#LOADLIBES = -lc

search: search.o JobNAllSky-common.o JobNAllSky.o lvcvirgo.o auxi.o fstat.o gridr.o
doc:
	doxygen ../doc/Doxyfile
clean:
	rm -f *.o

uninstall: clean
	rm -f search
	rm -fr ../doc/html
