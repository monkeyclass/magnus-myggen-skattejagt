MODULE := engines/ivanoff

MODULE_OBJS = \
	console.o \
	emu/cpu.o \
	emu/memory.o \
	ivanoff.o \
	machine.o \
	machine_files.o \
	metaengine.o \
	resources.o \
	sprite.o \
	world.o

# This module can be built as a plugin
ifeq ($(ENABLE_IVANOFF), DYNAMIC_PLUGIN)
PLUGIN := 1
endif

# Include common rules
include $(srcdir)/rules.mk

# Detection objects
DETECT_OBJS += $(MODULE)/detection.o
