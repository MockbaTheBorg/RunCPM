# Main Makefile for RunCPM
#
# Usage:
#	To build:	make <platform> build
#	To clean:	make <platform> clean	

PLATS = dos macosx mingw posix tdm

all:	none

dos macosx mingw posix tdm:
	$(eval SYS=$@)
	@echo "Platform = $@ (type 'make all' for help)"

clean:
	$(MAKE) -f Makefile.$(SYS) clean

build:
	$(MAKE) -f Makefile.$(SYS) all

none:
	@echo "To build RunCPM issue 'make PLATFORM build'."
	@echo "To clean RunCPM issue 'make PLATFORM clean'."
	@echo "Valid platforms are: $(PLATS)"

.PHONY: all $(PLATS) clean build none
