# MIXIT makefile
# 91111 - Jim Petrick
#

INCS = -I.

A1 = asm.o dio.o dld.o getfile.o getform.o hexutl.o 
A2 = image.o img.o intel.o lda.o mac.o mixit.o 
A3 = mot.o parser.o port2.o putfile.o qa.o rom.o 
A4 = tekhex.o varargs.o vlda.o coff.o cpe.o elf.o

AC1 = asm.c dld.c dio.c getfile.c getform.c hexutl.c image.c
AC2 = img.c intel.c lda.c mac.c mixit.c mot.c parser.c
AC3 = port2.c putfile.c qa.c rom.c tekhex.c varargs.c vlda.c coff.c cpe.c elf.c

DEFINES = -DLINUX -D_LANGUAGE_C -D_ISOC99_SOURCE -D_XOPEN_SOURCE

#MODE = -Ml2
#MODE = -Ml2 -dos
MODE = -m32

OPT = -O
CFLAGS = $(DEFINES) $(MODE) $(INCS) -g $(OPT) -Wall -pedantic -ansi -Wno-char-subscripts -std=c99
LFLAGS = $(MODE)
#CFLAGS = $(DEFINES) $(MODE) $(INCS) -g -Wall -pedantic -Dsun
#CFLAGS = $(DEFINES) $(MODE) $(INCS) -O -Zi -i 
#CFLAGS = $(DEFINES) $(MODE) $(INCS) -g  -Wall
#CFLAGS = $(DEFINES) $(MODE) $(INCS) -O2
#CFLAGS = $(DEFINES) $(MODE) $(INCS) -g  
#D = rcc -c -i $(MODE)
#L = rcc -O -Zi -i $(MODE)
CC = gcc 

#CC = cc 
#CC = rcc 

ECHO = /bin/echo -e
SHELL = /bin/sh
CO = co

C = $(CC) -c $(CFLAGS)
D = $(CC) -c $(CFLAGS)
L = $(CC) $(CFLAGS)

.SILENT:

#		*Explicit Rules*

CHECKOUT = 0 

define FROM_RCS
 if [ -w $(@F) ];\
 then \
        $(ECHO) "Warning: $@ is writeable but older than version in RCS"; \
        exit 0; \
 fi; \
 if [ -r $(@F) ]; \
 then \
        if [ $(CHECKOUT) -eq 0 ]; \
        then \
                rcsdiff $(@F) > /dev/null 2>&1;\
                if [ $$? -ne 0 ]; \
                then \
                        $(ECHO)  "FYI: There's a newer version of $@ in RCS"; \
                fi; \
                exit 0; \
        fi; \
 fi; \
 $(ECHO)  "\tChecking out $@ ...";\
 $(CO) $(COFLAGS) $(@F)
endef

%.c : RCS/%.c,v
	$(FROM_RCS)

%.h : RCS/%.h,v
	$(FROM_RCS)

define CC_RULE
        $(ECHO) "\tCompiling $<..."
        rm -f $(basename $(@F)).err $(basename $(@F)).warn
        $(CC) $(CFLAGS) -c $< > $(basename $(@F)).err 2>&1;\
        sts=$$?; \
        if [ ! -s $(basename $(@F)).err ];\
        then\
          rm $(basename $(@F)).err;\
        else\
          if [ $$sts -eq 0 ]; \
          then \
            mv $(basename $(@F)).err $(basename $(@F)).warn; \
            $(ECHO) 'Warnings in "$<"';\
          else \
            $(ECHO) 'Errors in "$<"';\
          fi \
        fi
endef

%.o : %.c
	$(CC_RULE)

%.d : %.c
	@$(ECHO)  "Making dependencies on $< ..."
	@$(CC) $(CFLAGS) -M $< >> depends

ALLDS = $(patsubst %.o,%.d,$(A1) $(A2) $(A3) $(A4))

mixit: $(A1) $(A2) $(A3) $(A4) Makefile
	$(ECHO)  "\tlinking..."
	$L $(LFLAGS) -o mixit $(A1) $(A2) $(A3) $(A4)
#	$L $(LFLAGS) -m mixit.map -o mixit $(A1) $(A2) $(A3) $(A4)

test:
	@$(ECHO)  "ALLDS = $(ALLDS)"

depends.d : $(ALLDS)
	
clean:
	/bin/rm -f $(A1) $(A2) $(A3) $(A4) mixit.map mixit *.warn *.err

mixit.lnt : $(AC1) $(AC2) $(AC3)
	sh lint -LARGE -abx -UM_XENIX -DM_I86 -DM_I286 -DMS_DOS $(AC1) $(AC2) $(AC3)

mixit.vms : $(AC1) $(AC2) $(AC3)
	sh lint -LARGE -abx -UM_XENIX -UM_I86 -UM_I286 -UM_I386 -DVMS $(AC1) $(AC2) $(AC3)

mixit.ln : makefile
	mixit /OUT=mixit /MAP=mixit.tmap /err/rel/deb mixitst /opt

#
# Automatically genetrated my a make depends
#
asm.o:	asm.c
asm.o:	mixit.h
asm.o:	image.h
asm.o:	mixit.h
asm.o:	gpf.h
asm.o:	mixit.h
asm.o:	formats.h
asm.o:	hexutl.h
asm.o:	mixit.h
asm.o:	prototyp.h
cpe.o:	cpe.c
cpe.o:	mixit.h
cpe.o:	image.h
cpe.o:	mixit.h
cpe.o:	gpf.h
cpe.o:	mixit.h
cpe.o:	formats.h
cpe.o:	hexutl.h
cpe.o:	mixit.h
cpe.o:	prototyp.h
dio.o:	dio.c
dio.o:	mixit.h
dio.o:	image.h
dio.o:	mixit.h
dio.o:	gpf.h
dio.o:	mixit.h
dio.o:	formats.h
dio.o:	hexutl.h
dio.o:	mixit.h
dio.o:	prototyp.h
dld.o:	dld.c
dld.o:	mixit.h
dld.o:	image.h
dld.o:	mixit.h
dld.o:	gpf.h
dld.o:	mixit.h
dld.o:	formats.h
dld.o:	hexutl.h
dld.o:	mixit.h
dld.o:	prototyp.h
getfile.o:	getfile.c
getfile.o:	mixit.h
getfile.o:	image.h
getfile.o:	mixit.h
getfile.o:	gpf.h
getfile.o:	mixit.h
getfile.o:	formats.h
getfile.o:	hexutl.h
getfile.o:	mixit.h
getfile.o:	prototyp.h
getform.o:	getform.c
getform.o:	mixit.h
getform.o:	image.h
getform.o:	mixit.h
getform.o:	gpf.h
getform.o:	mixit.h
getform.o:	formats.h
getform.o:	hexutl.h
getform.o:	mixit.h
getform.o:	prototyp.h
hexutl.o:	hexutl.c
hexutl.o:	mixit.h
hexutl.o:	image.h
hexutl.o:	mixit.h
hexutl.o:	gpf.h
hexutl.o:	mixit.h
hexutl.o:	formats.h
hexutl.o:	hexutl.h
hexutl.o:	mixit.h
hexutl.o:	prototyp.h
image.o:	image.c
image.o:	mixit.h
image.o:	image.h
image.o:	mixit.h
image.o:	gpf.h
image.o:	mixit.h
image.o:	formats.h
image.o:	hexutl.h
image.o:	mixit.h
image.o:	prototyp.h
img.o:	img.c
img.o:	mixit.h
img.o:	image.h
img.o:	mixit.h
img.o:	gpf.h
img.o:	mixit.h
img.o:	formats.h
img.o:	hexutl.h
img.o:	mixit.h
img.o:	prototyp.h
intel.o:	intel.c
intel.o:	mixit.h
intel.o:	image.h
intel.o:	mixit.h
intel.o:	gpf.h
intel.o:	mixit.h
intel.o:	formats.h
intel.o:	hexutl.h
intel.o:	mixit.h
intel.o:	prototyp.h
lda.o:	lda.c
lda.o:	mixit.h
lda.o:	image.h
lda.o:	mixit.h
lda.o:	gpf.h
lda.o:	mixit.h
lda.o:	formats.h
lda.o:	hexutl.h
lda.o:	mixit.h
lda.o:	prototyp.h
mac.o:	mac.c
mac.o:	mixit.h
mac.o:	image.h
mac.o:	mixit.h
mac.o:	gpf.h
mac.o:	mixit.h
mac.o:	formats.h
mac.o:	hexutl.h
mac.o:	mixit.h
mac.o:	prototyp.h
mixit.o:	mixit.c
mixit.o:	mixit.h
mixit.o:	image.h
mixit.o:	mixit.h
mixit.o:	gpf.h
mixit.o:	mixit.h
mixit.o:	formats.h
mixit.o:	hexutl.h
mixit.o:	mixit.h
mixit.o:	prototyp.h
mixit.o:	port.h
mixit.o:	qa.h
mot.o:	mot.c
mot.o:	mixit.h
mot.o:	image.h
mot.o:	mixit.h
mot.o:	gpf.h
mot.o:	mixit.h
mot.o:	formats.h
mot.o:	hexutl.h
mot.o:	mixit.h
mot.o:	prototyp.h
parser.o:	parser.c
parser.o:	mixit.h
parser.o:	image.h
parser.o:	mixit.h
parser.o:	gpf.h
parser.o:	mixit.h
parser.o:	formats.h
parser.o:	hexutl.h
parser.o:	mixit.h
parser.o:	prototyp.h
parser.o:	port.h
port2.o:	port2.c
port2.o:	mixit.h
port2.o:	image.h
port2.o:	mixit.h
port2.o:	gpf.h
port2.o:	mixit.h
port2.o:	formats.h
port2.o:	hexutl.h
port2.o:	mixit.h
port2.o:	prototyp.h
port2.o:	port.h
putfile.o:	putfile.c
putfile.o:	mixit.h
putfile.o:	image.h
putfile.o:	mixit.h
putfile.o:	gpf.h
putfile.o:	mixit.h
putfile.o:	formats.h
putfile.o:	hexutl.h
putfile.o:	mixit.h
putfile.o:	prototyp.h
qa.o:	qa.c
qa.o:	mixit.h
qa.o:	image.h
qa.o:	mixit.h
qa.o:	gpf.h
qa.o:	mixit.h
qa.o:	formats.h
qa.o:	hexutl.h
qa.o:	mixit.h
qa.o:	prototyp.h
qa.o:	port.h
rom.o:	rom.c
rom.o:	mixit.h
rom.o:	image.h
rom.o:	mixit.h
rom.o:	gpf.h
rom.o:	mixit.h
rom.o:	formats.h
rom.o:	hexutl.h
rom.o:	mixit.h
rom.o:	prototyp.h
tekhex.o:	tekhex.c
tekhex.o:	mixit.h
tekhex.o:	image.h
tekhex.o:	mixit.h
tekhex.o:	gpf.h
tekhex.o:	mixit.h
tekhex.o:	formats.h
tekhex.o:	hexutl.h
tekhex.o:	mixit.h
tekhex.o:	prototyp.h
varargs.o:	varargs.c
varargs.o:	mixit.h
varargs.o:	image.h
varargs.o:	mixit.h
varargs.o:	gpf.h
varargs.o:	mixit.h
varargs.o:	formats.h
varargs.o:	hexutl.h
varargs.o:	mixit.h
varargs.o:	prototyp.h
vlda.o:	vlda.c
vlda.o:	mixit.h
vlda.o:	image.h
vlda.o:	mixit.h
vlda.o:	gpf.h
vlda.o:	mixit.h
vlda.o:	formats.h
vlda.o:	hexutl.h
vlda.o:	mixit.h
vlda.o:	prototyp.h
coff.o:	coff.c
coff.o:	mixit.h
coff.o:	image.h
coff.o:	mixit.h
coff.o:	gpf.h
coff.o:	mixit.h
coff.o:	formats.h
coff.o:	hexutl.h
coff.o:	mixit.h
coff.o:	prototyp.h
coff.o:	coff-m68k.h
coff.o:	coff-internal.h
coff.o:	ecoff.h
elf.o:	elf.c
#elf.o:	image.h
#elf.o:	mixit.h
#elf.o:	gpf.h
#elf.o:	mixit.h
#elf.o:	formats.h
#elf.o:	hexutl.h
#elf.o:	mixit.h
#elf.o:	prototyp.h
#elf.o:	elf.h
#elf.o:	elftypes.h
#elf.o:	syself.h
