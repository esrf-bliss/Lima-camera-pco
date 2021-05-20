#Makefile for libpcocom_me4.a and libpcocam_me4.a


VERSION = $(word 3,$(shell head ./VersionNo.h -n40 | awk '/VERSION/{print $2}'))
SUBVERSION = $(word 3,$(shell head ./VersionNo.h -n40 | awk '/SUBVERSION/{print $2}'))
SUBAGE = $(word 3,$(shell head ./VersionNo.h -n40 | awk '/SUBAGE/{print $2}'))

PCOHDIR = ../../pco_common/pco_include
PCOLIBDIR = ../../pco_common/pco_lib
CPCODIR = ../../pco_common/pco_classes


REORDERLIB=$(PCOLIBDIR)/libreorderfunc.a
LOGLIB=$(PCOLIBDIR)/libpcolog.a

SISOINCLUDEDIR = $(SISODIR5)/include

HEADERS = ./VersionNo.h ./Cpco_com_cl_me4.h $(CPCODIR)/Cpco_com.h $(CPCODIR)/Cpco_com_func.h $(CPCODIR)/Cpco_com_func_2.h

CFLAGS := -O2 -Wall -DLINUX -fPIC -I$(PCOHDIR) -I$(CPCODIR) -I$(SISOINCLUDEDIR) 
LFLAGS += -L$(PCOLIBDIR)


COM_OBJ =  Cpco_com.o Cpco_com_func.o Cpco_com_func_2.o Cpco_com_cl_me4.o
GRAB_OBJ = Cpco_grab_cl_me4.o Cpco_grab_cl_me4_GL.o

TARGETS= libpcocom_me4 libpcocam_me4

all: $(TARGETS)

clean:
		$(RM) *.o *~ $(PCOHDIR)*~ $(addprefix $(PCOLIBDIR)*/,$(addsuffix *,$(TARGETS)))

copy:
		@mv $(LIB_COM) $(PCOLIBDIR)
		@mv $(LIB_CAM) $(PCOLIBDIR)


%.o:		%.cpp $(HEADERS)
		$(CXX) $(CFLAGS) -c $< -o $@

C%.o:		$(CPCODIR)/C%.cpp $(HEADERS)
		$(CXX) $(CFLAGS) -c $< -o $@


libpcocam_me4:	$(COM_OBJ) $(GRAB_OBJ) 
		$(LD) -r -s -L$(PCOLIBDIR) $^ -o $(PCOLIBDIR)/$@.a 
		$(CC) -shared -Wl,-soname,$@.so.$(VERSION) -Wl,-L$(PCOLIBDIR) \
		-o $(PCOLIBDIR)dyn/$@.so.$(VERSION).$(SUBVERSION).$(SUBAGE) $^ 


libpcocom_me4:	$(COM_OBJ) 
		$(LD) -r -s -L$(PCOLIBDIR) $^ -o $(PCOLIBDIR)/$@.a 
		$(CC) -shared -Wl,-soname,$@.so.$(VERSION) -Wl,-L$(PCOLIBDIR) \
		-o $(PCOLIBDIR)dyn/$@.so.$(VERSION).$(SUBVERSION).$(SUBAGE) $^ 


