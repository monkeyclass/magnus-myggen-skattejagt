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
	script.o \
	vars.o \
	scripts/base.o \
	scripts/batch_a.o \
	scripts/batch_b.o \
	scripts/batch_c.o \
	scripts/batch_d.o \
	scripts/batch_e.o \
	scripts/batch_f.o \
	scripts/batch_g.o \
	scripts/batch_h.o \
	scripts/items.o \
	scripts/registry.o \
	scripts/setup.o \
	scripts/setupbuilder.o \
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
