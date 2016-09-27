# Main Makefile for RunCPM
PLAT= none

PLATS= dos mingw posix tdm 

all:	$(PLAT)

dos mingw posix tdm:
	@echo "Building... $@"
	$(MAKE) -f Makefile.$@

dosclean mingwclean posixclean tdmclean:
	@echo "Building... $@"
	$(MAKE) -f Makefile.$@ clean

none:
	@echo "To build RunCPM issue 'make PLATFORM'."
	@echo "Valid patforms are: $(PLATS)"

clean:
	@echo "To clean use 'make PLATFORMclean'."
	@echo "Valid patforms are: $(PLATS)"

.PHONY: all $(PLATS) dosclean mingwclean posixclean tdmclean none clean
