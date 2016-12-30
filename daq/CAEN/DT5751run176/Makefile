UNIC = $(HOME)
NICE = $(HOME)
# Please define CAENSYS if CAEN libs are not installed in standard place
HAS_CAEN = X$(CAENSYS)
CFLAGS = -Wall -g
ifneq ($(HAS_CAEN), X)
  CAENLIB = -L$(CAENSYS)/lib
  CFLAGS += -I$(CAENSYS)/include
endif

OPT2       = -O2

ROOTCONFIG = root-config

ARCH      := $(shell $(ROOTCONFIG) --arch)
ALTCXX    := $(shell $(ROOTCONFIG) --cxx)
ROOTCFLAGS:= $(shell $(ROOTCONFIG) --cflags)
ROOTLIBS  := $(shell $(ROOTCONFIG) --libs)

ARCHOK     = no

ifeq ($(ARCH),linux)
  CXX      = g++
  CXXFLAGS = $(OPT2) -Wall -fPIC
  ARCHOK   = yes
endif

ifeq ($(ARCH),linuxx8664gcc)
  CXX      = g++
  CXXFLAGS = $(OPT2) -Wall -fPIC
  ARCHOK   = yes
endif

# If arch != the above options, a error message is given
ifeq ($(ARCHOK),no)
  $(error $(ARCH) invalid architecture)
endif

# In case that ALTCXX = 0, g++ will be used as CXX
ifneq ($(ALTCXX),)
  CXX = $(ALTCXX)
endif

# Finally, define CXXFLAGS
CXXFLAGS+= $(ROOTCFLAGS) -I$(UNIC)/include -I$(NICE)/include

# Define SOURCES & OBJECTS 
# ==========================================

SOURCES = $(wildcard *.cc)
OBJECTS = $(SOURCES:.cc=.o)
DEPFILE = $(SOURCES:.cc=.d)

SRCS = $(wildcard *.C)
EXES = $(SRCS:.C=.exe)

# own sscanf.c has to be included if glibc < 2.7 is used
GLIBC_MIN_VER = $(shell ldd --version | head -1 | awk -F. '{print $$NF}') 
TOO_OLD = $(shell [ $(GLIBC_MIN_VER) -lt 7 ] && echo true)
ifeq ($(TOO_OLD), true)
  OBJS = $(wildcard *.c)
else
  OBJS = cfg.c daq.c
endif

# Action starts
# =============

all: $(EXES) daq
	@echo
	@echo "* Done!"
	@echo 

daq:$(OBJS)
	@echo
	@echo "* Creating C executable:"
	gcc $(CFLAGS) -o $@.exe $(OBJS) $(CAENLIB) -lCAENDigitizer

# include *.d files, which are makefiles defining dependencies between files
ifeq ($(filter clean,$(MAKECMDGOALS)),)
  -include $(DEPFILE)
endif

# rules to create *.d files
%.d:%.cc
	@echo creating $@
	@set -e; rm -f $@; \
	  $(CXX) -MM $(CPPFLAGS) $< > $@.$$$$; \
	  sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	  rm -f $@.$$$$ 

# An xxx.o file depends on xxx.cc. It is created with the command:
# 	g++ -c xxx.cc -o xxx.o
# Since this is obvious, "make" automatically does it. 
# There is no need to explicitly write down the rules to do it.

$(EXES):%.exe:%.C $(OBJECTS)
	@echo
	@echo "* Creating C++ executables:"
	$(CXX) $(CXXFLAGS) -o $@ $< $(OBJECTS) -L$(NICE)/lib -lNICE $(ROOTLIBS)

clean:
	$(RM) *.exe *.o *.d *.d.* *~

.PHONY: all clean

