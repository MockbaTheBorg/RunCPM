# Main Makefile for RunCPM
#
# Usage:
#	To build:	make <platform> build
#	To clean:	make <platform> clean	

# Default CCP selection (used by platform Makefiles). Override on the command line,
# e.g. `make posix CCP=DR build`
CCP ?= INTERNAL
export CCP

# Default CPU selection (used by platform Makefiles). Override on the command line,
# e.g. `make posix CPU=cpu2.h build`
CPU ?= cpu1
export CPU

PLATS = macosx mingw msys2 posix tdm runvt
CCPS = INTERNAL DR CCPZ ZCPR2 ZCPR3 Z80
CPUS = cpu1 cpu2 cpu3 cpu4

all:	none

dos macosx mingw msys2 posix tdm runvt:
	$(eval SYS=$@)
	@echo "Platform = $@ (type 'make all' for help)"

clean:
	$(MAKE) -f Makefile.$(SYS) clean

build:
	$(MAKE) -f Makefile.$(SYS) all

rebuild:
	$(MAKE) -f Makefile.$(SYS) rebuild

help:	none

none:
	@echo "To build RunCPM issue 'make PLATFORM build'."
	@echo "To clean RunCPM issue 'make PLATFORM clean'."
	@echo "To clean,then build RunCPM issue 'make PLATFORM rebuild'."
	@echo "Valid platforms are: $(PLATS)"
	@echo "----------------------------------------------------------------"
	@echo "To select a CCP, define CCP when invoking make, e.g. 'make posix CCP=DR build'."
	@echo "Valid CCPs are: $(CCPS)"
	@echo "----------------------------------------------------------------"
	@echo "To select a CPU, define CPU to the header filename when invoking make, e.g. 'make posix CPU=cpu2 build'."
	@echo "Valid CPUs are: $(CPUS)"
	@echo "----------------------------------------------------------------"
	@echo "To enable CP/M 3.0 support, define CPM3=1 when invoking make, e.g. 'make posix CPM3=1 build'."

.PHONY: all $(PLATS) clean build rebuild help none
