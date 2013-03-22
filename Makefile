# Makefile for PXDrum (PSP version)

TARGET = xdrum
OBJS = xdrum.o drumkit.o song.o fontengine.o gui.o joymap.o writewav.o

PSPBIN = $(PSPDEV)/psp/bin

INCDIR = 
CFLAGS = -DPSP -O2 -G0 -Wall $(shell $(PSPBIN)/sdl-config --cflags)
#CFLAGS = -G0 -Wall $(shell $(PSPBIN)/sdl-config --cflags)
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR = 
LDFLAGS = 
#LIBS = -lm $(shell $(PSPBIN)/sdl-config --libs) -logg -lvorbis -lvorbisidec -lSDL_mixer
LIBS = -lm $(shell $(PSPBIN)/sdl-config --libs) -lSDL_mixer -lvorbisidec -lSDLmain -lSDL -lm -lstdc++

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = PXDrum v1.2
PSP_EBOOT_ICON = icon.png
PSP_EBOOT_ICON1 = icon1.png
PSP_EBOOT_PIC1 = pic1.png
#PSP_EBOOT_SND0 = snd0.at3

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
