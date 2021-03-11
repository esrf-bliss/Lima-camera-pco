# Makefile for programs in this directory

PCODIR      = ../../pco_common/pco_include
PCOLIBDIR   = ../../pco_common/pco_lib
CPCODIR     = ../../pco_common/pco_classes
CPCODIRCLHS = ../pco_classes
PCOCLHSDIR  = ../pco_clhs_common

CFLAGS += -O2 -Wall -DLINUX -I$(PCODIR) -I$(CPCODIR) -I$(CPCODIRCLHS) -I$(PCOCLHSDIR)
LFLAGS += -L$(PCOLIBDIR)

DISPLIB    = $(PCOLIBDIR)/libpcodisp.a
LOGLIB     = $(PCOLIBDIR)/libpcolog.a
FILELIB    = $(PCOLIBDIR)/libpcofile.a
REORDERLIB = $(PCOLIBDIR)/libreorderfunc.a
CAMLIB     = $(PCOLIBDIR)/libpcocam_clhs.a

HEADERS = $(CPCODIR)/Cpco_com.h $(CPCODIRCLHS)/VersionNo.h $(CPCODIRCLHS)/Cpco_grab_clhs.h $(CPCODIRCLHS)/Cpco_com_clhs.h

PCOLIB  =-lpcolog  -lpcofile -lpcocam_clhs -lpcoclhs
PCOLIBX =-lpcodisp -lpcofile -lpcocam_clhs -lpcoclhs

LIBADD  = -lpthread -lrt -ldl
LIBADDX = -lpthread -lrt -ldl -lX11 -lXext



TARGETS = pco_camera_grab pcox_camera_grab


all:		$(TARGETS) copy

clean:
		$(RM) *~ $(TARGETS)  $(CPCODIR)/*~  *.log *.o

copy:	
		@mv $(TARGETS) ../bin

%.o:		%.cpp
		$(CXX) $(CFLAGS) -c $< -o $@

pco_%:		pco_%.cpp $(CAMLIB) $(HEADERS) 
		$(CXX) $(CFLAGS) $@.cpp -o $@ $(LFLAGS) $(PCOLIB) $(LIBADD)
		$(CXX) $(CFLAGS) -rdynamic -L../bindyn $@.cpp -o ../bindyn/$@  $(PCOLIB) $(LIBADD)


pcox_%:		pcox_%.cpp $(CAMLIB) $(HEADERS) 
		$(CXX) $(CFLAGS) $@.cpp -o $@ $(LFLAGS) $(PCOLIBX) $(LIBADDX)
		$(CXX) $(CFLAGS) -rdynamic -L../bindyn $@.cpp -o ../bindyn/$@  $(PCOLIBX) $(LIBADDX)

