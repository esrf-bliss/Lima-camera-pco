# Makefile for programs in this directory

PCODIR     = ../../pco_common/pco_include
PCOLIBDIR  = ../../pco_common/pco_lib
CPCODIR    = ../../pco_common/pco_classes
CPCODIRME4 = ../pco_classes
SISOLIBDIR = $(SISODIR5)/lib
SISOINCLUDEDIR = $(SISODIR5)/include

CFLAGS += -O2 -Wall -DLINUX -I$(PCODIR) -I$(CPCODIR) -I$(CPCODIRME4) -I$(SISOINCLUDEDIR)
LFLAGS += -L$(SISOLIBDIR) -L$(PCOLIBDIR) -L../bindyn


DISPLIB    = $(PCOLIBDIR)/libpcodisp.a
LOGLIB     = $(PCOLIBDIR)/libpcolog.a
FILELIB    = $(PCOLIBDIR)/libpcofile.a
REORDERLIB = $(PCOLIBDIR)/libreorderfunc.a
CAMLIB     = $(PCOLIBDIR)/libpcocam_me4.a

HEADERS =  $(CPCODIR)/Cpco_com.h $(CPCODIRME4)/VersionNo.h $(CPCODIRME4)/Cpco_com_cl_me4.h $(CPCODIRME4)/Cpco_grab_cl_me4.h $(PCODIR)/reorderfunc.h

PCOLIB  = -lpcolog  -lpcofile -lreorderfunc -lpcocam_me4
PCOLIBX = -lpcodisp -lpcofile -lreorderfunc -lpcocam_me4

LIBADD  = -lfglib5 -lclsersis -lpthread 
LIBADDX = -lfglib5 -lclsersis -lpthread -lX11 -lXext



#TARGETS = pco_camera_grab pcox_camera_grab 
TARGETS = pco_camera_grab  
#pco_camera_record


all:		$(TARGETS) copy

clean:
		$(RM) *~ $(TARGETS)  $(CPCODIR)/*~  *.log *.o

copy:	
		@mv $(TARGETS) ../bin

%.o:		%.cpp
		$(CXX) $(CFLAGS) -c $< -o $@

pco_%:		pco_%.cpp $(CAMLIB) $(HEADERS)
		$(CXX) $(CFLAGS) $@.cpp -o $@ $(LFLAGS) $(LIBADD) $(PCOLIB)
		$(CXX) $(CFLAGS) -rdynamic -L../bindyn -L$(SISOLIBDIR) $@.cpp -o ../bindyn/$@  $(LIBADD) $(PCOLIB)


pcox_%:		pcox_%.cpp $(CAMLIB) $(HEADERS)
		$(CXX) $(CFLAGS) $@.cpp -o $@ $(LFLAGS) $(LIBADDX) $(PCOLIBX) 
		$(CXX) $(CFLAGS) -rdynamic -L../bindyn -L$(SISOLIBDIR) $@.cpp -o ../bindyn/$@  $(LIBADDX) $(PCOLIBX)

