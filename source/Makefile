DEBUG = FALSE

GCC ?= gcc
GXX ?= g++
GCCFLAGS = -O1 -g3 -Wall -I/usr/include/SDL -fomit-frame-pointer
LDFLAGS = -lSDL -lm 

# Disable Assembly Routines
#ifeq ($(DEBUG),FALSE)
#	GCCFLAGS += -O2 -fomit-frame-pointer -DFORNSPIRE
#else
#	GCCFLAGS += -O0 -g -fomit-frame-pointer -DFORNSPIRE
#endif

OBJS = $(patsubst %.c, %.o, $(shell find . -name \*.c))
OBJS += $(patsubst %.cpp, %.o, $(shell find . -name \*.cpp))
# OBJS += $(patsubst %.S, %.o, $(shell find . -name \*.S))
EXE = shQuake
DISTDIR = .
# path %.tns $(DISTDIR)
# vpath %.elf $(DISTDIR)

all: $(EXE)

%.o: %.c
	$(GCC) $(GCCFLAGS) -c $< -o $@

%.o: %.cpp
	$(GXX) $(GCCFLAGS) -c $< -o $@
	
$(EXE): $(OBJS)
	mkdir -p $(DISTDIR)
	# $(LD) $^ -o $@ $(LDFLAGS)
	$(GCC) $(GCCFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJS) $(DISTDIR)/$(EXE).tns $(DISTDIR)/$(EXE).elf $(DISTDIR)/$(EXE).zehn
