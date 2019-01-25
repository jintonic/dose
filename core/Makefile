# Dependence
ifeq ($(UNIC),)
  UNIC=$(HOME)
endif

# Configurable variables
ifeq ($(PREFIX),)
  PREFIX=$(HOME)
endif
LIBNAME=NICE

# Define basic variables
ROOTCONFIG = root-config
CXXFLAGS = -O2 -Wall -fPIC
CXXFLAGS+= $(shell $(ROOTCONFIG) --cflags)
CXXFLAGS+= -I$(UNIC)/include
LIBS += $(shell $(ROOTCONFIG) --libs)

# Define things related to ROOT dictionary
LINKDEF = LinkDef.h
DICTSRC = $(LIBNAME).cc
DICTOBJ = $(DICTSRC:.cc=.o)

# Define SOURCES, HEADERS & OBJECTS 
SOURCES = $(filter-out $(DICTSRC), $(wildcard *.cc))
HEADERS = $(SOURCES:.cc=.h)
OBJECTS = $(SOURCES:.cc=.o)
DEPFILE = $(SOURCES:.cc=.d)

# Define LIBRARY, ROOTMAP & variables to create them
LIBRARY = lib$(LIBNAME).so
ROOTMAP = $(LIBRARY:.so=.rootmap)

ROOTVERSION = $(firstword $(subst ., ,$(shell $(ROOTCONFIG) --version)))
ifeq ($(ROOTVERSION), 6)
  CREATEDIC = rootcling -f $(DICTSRC) $(HEADERS) $(LINKDEF)
  CREATEMAP = rootcling -f $(DICTSRC) -rmf $@ -rml $^ $(HEADERS) $(LINKDEF)
else
  CREATEDIC = rootcint $(DICTSRC) -c -p $(CXXFLAGS) $(HEADERS) $(LINKDEF)
  SYMBOLS = `nm -CPu $(LIBRARY) |\
	    awk -F: '/^T/{printf("(%s)\n",$$1)}' |\
	    sort -u | tr '\n' '|'`
  ALLLIBS = $(wildcard $(shell $(ROOTCONFIG) --libdir)/*.so)
  DEPENDS = $(shell symbols=$(SYMBOLS); \
	    for so in $(ALLLIBS); \
	    do nm -CD --defined-only $$so |\
	    grep -E 'T ('$$symbols')::' > /dev/null &&\
	    echo $$so;\
	    done | sort -u | tr '\n' ' ')
  CREATEMAP = rlibmap -o $@ -l $^ -d $(DEPENDS) -c $(LINKDEF)
endif

# Action starts
# ==============================================================================

# the first target is the default target, it depends on $(ROOTMAP)
# before "make all", make will include all other makefiles specified
# by the include command
install: $(ROOTMAP)
	mkdir -p $(PREFIX)/lib/
	cp lib$(LIBNAME).* $(PREFIX)/lib/
	if [ -f *.pcm ]; then cp *.pcm $(PREFIX)/lib/; fi
	mkdir -p $(PREFIX)/include/$(LIBNAME)/
	cp *.h $(PREFIX)/include/$(LIBNAME)/

# include *.d files, which are makefiles defining dependencies between files
ifeq ($(filter info clean tags,$(MAKECMDGOALS)),)
  -include $(DEPFILE) # - tells make to continue if dependence files do not exist
endif

# rules to create *.d files
%.d:%.cc
	@echo creating $@
	@set -e; rm -f $@; \
	  $(CXX) -MM $(CPPFLAGS) $< > $@.$$$$; \
	  sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	  rm -f $@.$$$$ 

# lib$(LIBNAME).rootmap can only be created after the creation of
# lib$(LIBNAME).so. It tells ROOT the dependence among libraries. Putting it
# along with the corresponding library allows one to use in a ROOT interactive
# session functions defined in the library without gSystem->Load("lib.so")
$(ROOTMAP): $(LIBRARY)
	@echo
	@echo "* Creating rootmap file:"
	$(CREATEMAP)
	@echo 

# lib$(LIBNAME).so depends on all *.o files.
#  The flag "-shared" is used to create shared libs
#  $@ represents the target, that is, lib$(LIBNAME).so
#  $^ represents all the prerequisites, i.e., all *.o files
$(LIBRARY): $(DICTOBJ) $(OBJECTS)
	@echo
	@echo "* Creating shared library:"
	$(CXX) $^ $(CXXFLAGS) $(LIBS) -shared -o $@

# An xxx.o file depends on xxx.cc. It is created with the command:
# 	g++ -c xxx.cc -o xxx.o
# Since this is obvious, "make" automatically does it. 
# There is no need to explicitly write down the rules to do it.

# To use classes & functions directly in ROOT. One has to do the following
$(DICTSRC): $(HEADERS) $(LINKDEF)
	@echo 
	@echo "* Create ROOT dictionary:" 
	$(CREATEDIC)
	@echo 
	@echo "* Creating object files:" 

info: 
	@echo
	@echo "target:   $(LIBRARY) $(ROOTMAP)"
	@echo "sources:  $(SOURCES)"
	@echo "headers:  $(HEADERS)"
	@echo "objects:  $(OBJECTS)"
	@echo 
	@echo "compiler: $(CXX)"
	@echo "flags:    $(CXXFLAGS)"
	@echo "libs:     $(LIBS)"
	@echo

clean:
	$(RM) *.o *.d *.d.* *~ *$(LIBNAME)* $(ROOTMAP) $(LIBRARY)

tags:
	ctags --c-kinds=+p $(HEADERS) $(SOURCES)

uninstall:
	$(RM) -r $(PREFIX)/include/$(LIBNAME)
	$(RM) $(PREFIX)/lib/*$(LIBNAME)*

.PHONY: info tags clean install uninstall
