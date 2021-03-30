#Makefile for pco_com

VERSION = $(word 3,$(shell head ./VersionNo.h -n50 | awk '/VERSION/&&!/SUB/'))
SUBVERSION = $(word 3,$(shell head ./VersionNo.h -n50 | awk '/SUBVERSION/'))
SUBAGE = $(word 3,$(shell head ./VersionNo.h -n50 | awk '/SUBAGE/'))

VERSTR = "\"$(VERSION).$(SUBVERSION).$(SUBAGE)\""

PCOHDIR = ../../pco_common/pco_include
PCOLIBDIR = ../../pco_common/pco_lib
CPCODIR = ../../pco_common/pco_classes
PCOCLHSDIR = ../pco_clhs_common

REORDERLIB=$(PCOLIBDIR)/libreorderfunc.a
LOGLIB=$(PCOLIBDIR)/libpcolog.a
PCOCLHSLIB = $(PCOLIBDIR)/libpcoclhs.a


CFLAGS := -O2 -Wall -DLINUX -DVERSTR=$(VERSTR) -fPIC -I$(PCOHDIR) -I$(CPCODIR) -I$(PCOCLHSDIR)
LFLAGS += -L$(PCOLIBDIR)

COM_OBJ =  Cpco_com.o Cpco_com_func.o Cpco_com_func_2.o Cpco_com_clhs.o
GRAB_OBJ = Cpco_grab_clhs.o


HEADERS = ./VersionNo.h ./Cpco_com_clhs.h ./Cpco_grab_clhs.h $(CPCODIR)/Cpco_com.h $(CPCODIR)/Cpco_com_func.h $(CPCODIR)/Cpco_com_func_2.h


TARGETS= libpcocom_clhs libpcocam_clhs


all: $(TARGETS) 

clean:
		$(RM) *.o *~ $(PCOHDIR)*~  $(addprefix $(PCOLIBDIR)*/,$(addsuffix *,$(TARGETS)))


%.o:		%.cpp $(HEADERS)
		$(CXX) $(CFLAGS) -c $< -o $@

C%.o:		$(CPCODIR)/C%.cpp $(HEADERS)
		$(CXX) $(CFLAGS) -c $< -o $@


libpcocam_clhs:	$(COM_OBJ) $(GRAB_OBJ) 
		$(LD) -r -s -L$(PCOLIBDIR) $^ -o $(PCOLIBDIR)/$@.a 
		$(CC) -shared -Wl,-soname,$@.so.$(VERSION) -Wl,-L$(PCOLIBDIR) \
		-o $(PCOLIBDIR)dyn/$@.so.$(VERSION).$(SUBVERSION).$(SUBAGE) $^ 


libpcocom_clhs:	$(COM_OBJ) 
		$(LD) -r -s -L$(PCOLIBDIR) $^ -o $(PCOLIBDIR)/$@.a 
		$(CC) -shared -Wl,-soname,$@.so.$(VERSION) -Wl,-L$(PCOLIBDIR) \
		-o $(PCOLIBDIR)dyn/$@.so.$(VERSION).$(SUBVERSION).$(SUBAGE) $^ 



