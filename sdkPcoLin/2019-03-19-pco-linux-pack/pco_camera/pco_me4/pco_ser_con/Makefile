# Makefile for programs in this directory

PCODIR     = ../../pco_common/pco_include
PCOLIBDIR  = ../../pco_common/pco_lib
CPCODIR    = ../../pco_common/pco_classes
CPCODIRME4 = ../pco_classes
SISOLIBDIR = $(SISODIR5)/lib
SISOINCLUDEDIR = $(SISODIR5)/include


CFLAGS += -O2 -Wall -DLINUX -I$(PCODIR) -I$(CPCODIR) -I$(CPCODIRME4) -I$(SISOINCLUDEDIR)
LFLAGS += -L$(SISOLIBDIR) -L$(PCOLIBDIR)

LOGLIB    = $(PCOLIBDIR)/libpcolog.a
PCOCOMLIB = $(PCOLIBDIR)/libpcocom_me4.a
CAMLIB = $(PCOLIBDIR)/libpcocam_me4.a

HEADERS = $(CPCODIRME4)/VersionNo.h $(CPCODIR)/Cpco_com.h $(CPCODIRME4)/Cpco_com_cl_me4.h

PCOLIB  = -lpcolog  -lpcofile -lpcocom_me4
PCOLIBX = -lpcodisp -lpcofile -lpcocom_me4

LIBADD  = -lclsersis -lpthread -lrt 
LIBADDX = -lclsersis -lpthread -lrt -lX11 -lXext

TARGETS = pco_ser_con pco_getversions pco_switch_edge


all:		$(TARGETS) copy

clean:
		$(RM)  *~ $(TARGETS) $(CPCODIR)/*~  *.log *.o

copy:
		@mv $(TARGETS) ../bin

%.o:		%.cpp
		$(CXX) $(CFLAGS) -c $< -o $@


pco_%:		pco_%.cpp $(PCOCOMLIB) $(HEADERS)
		$(CXX) $(CFLAGS) $@.cpp -o $@ $(LFLAGS) $(LIBADD) $(PCOLIB)
		$(CXX) $(CFLAGS) -rdynamic -L../bindyn -L$(SISOLIBDIR) $@.cpp -o ../bindyn/$@  $(LIBADD) $(PCOLIB)


