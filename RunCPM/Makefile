# Main Makefile for RunCPM
#
# Usage:
#	To build:	make <platform> build
#	To clean:	make <platform> clean	

PLATS = macosx mingw msys2 posix tdm

all:	none

dos macosx mingw msys2 posix tdm:
	$(eval SYS=$@)
	@echo "Platform = $@ (type 'make all' for help)"

clean:
	$(MAKE) -f Makefile.$(SYS) clean

build:
	$(MAKE) -f Makefile.$(SYS) all

rebuild:
	$(MAKE) -f Makefile.$(SYS) rebuild

none:
	@echo "To build RunCPM issue 'make PLATFORM build'."
	@echo "To clean RunCPM issue 'make PLATFORM clean'."
	@echo "To clean,then build RunCPM issue 'make PLATFORM rebuild'."
	@echo "Valid platforms are: $(PLATS)"

.PHONY: all $(PLATS) clean build none
