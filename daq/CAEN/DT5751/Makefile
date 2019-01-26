# dependences
ifeq ($(PREFIX),)
  PREFIX=$(HOME)
endif
UNIC = $(PREFIX)
NICE = $(PREFIX)
DAQ = $(PREFIX)

LIBS = -L$(NICE)/lib -lNICE
CXXFLAGS = -Wall -O2 -I$(UNIC)/include -I$(NICE)/include -I$(DAQ)/include

# check if root-config exists
ifneq ($(shell hash root-config 2> /dev/null && echo true), true)
  $(error cannot find root-config)
endif

# check if in linux
ARCH = $(shell root-config --arch)
ifneq ($(ARCH),linux)
  ifneq ($(ARCH),linuxx8664gcc)
    $(error invalid architecture: $(ARCH))
  endif
endif

# add root libs
CXX = g++
LIBS += $(shell root-config --libs)
CXXFLAGS += $(shell root-config --cflags)

# define targets
CCS = $(wildcard *.cc)
OBJ = $(CCS:.cc=.o)
SRC = $(wildcard *.C)
EXE = $(SRC:.C=.exe)

# Action starts here
all: $(EXE)

# include *.d files
ifeq ($(filter clean,$(MAKECMDGOALS)),)
  -include $(CCS:.cc=.d)
endif

# rules to create *.d files
%.d:%.cc
	@echo $@ created
	@set -e; rm -f $@; \
	  $(CXX) -MM $(CPPFLAGS) $< > $@.$$$$; \
	  sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	  rm -f $@.$$$$ 

# An xxx.o file depends on xxx.cc. It is created with the command:
# 	g++ -c xxx.cc -o xxx.o
# Since this is obvious, "make" automatically does it. 
# There is no need to explicitly write down the rules to do it.

$(EXE):%.exe:%.C $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $< $(OBJ) $(LIBS)

clean:
	$(RM) *.exe *.o *.d *.d.* *~

.PHONY: all clean

