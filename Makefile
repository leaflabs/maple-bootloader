CROSS_COMPILE?=arm-ncrmnt-eabi-
SRCDIR=.
OBJDIR=./build

MCU      = cortex-m3
SUBMDL   = stm32f103
THUMB    = -mthumb
THUMB_IW = -mthumb-interwork

TARGET=$(OBJDIR)/ucboot-$(BOARD)

CFLAGS += -O$(OPT)
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -Wall -Wimplicit
CFLAGS += -Wpointer-arith -Wswitch
CFLAGS += -Wredundant-decls -Wreturn-type -Wshadow -Wunused
CFLAGS += -Wa,-adhlns=$(OBJDIR)/$(subst $(suffix $<),.lst,$<)
CFLAGS += -I$(OBJDIR)/

GENDEPFLAGS = -MD -MP -MF $(@).d

CFLAGS += -mcpu=$(MCU) $(THUMB_IW) $(GENDEPFLAGS)
ASFLAGS = -Wa,-adhlns=$(OBJDIR)/$(<:.s=.lst)#,--g$(DEBUG)
ASFLAGS += -mcpu=$(MCU) $(THUMB_IW) -x assembler-with-cpp
ASFLAGS += $(GENDEPFLAGS)
LDFLAGS = -nostartfiles -Wl,-Map=$(TARGET).map,--cref,--gc-sections
LDFLAGS += -lc -lgcc
#TODO:...
LDFLAGS +=-T$(SRCDIR)/src/startup/c_only_md.ld

CROSS_CC=$(CROSS_COMPILE)gcc
CROSS_AS=$(CROSS_COMPILE)as
CROSS_LD=$(CROSS_COMPILE)ld
CROSS_OBJCOPY=$(CROSS_COMPILE)objcopy
CROSS_OBJDUMP=$(CROSS_COMPILE)objdump

# Pretty colors
tb_ylw=$(shell echo -e '\e[1;33m')
tb_blu=$(shell echo -e '\e[1;34m')
tb_pur=$(shell echo -e '\e[1;35m')
col_rst=$(shell echo -e '\e[0m')

ifneq ($(V),y)
   SILENT_CC       = @echo '  $(tb_ylw)[CC]$(col_rst)       ' $(@F);
   SILENT_AS       = @echo '  $(tb_blu)[AS]$(col_rst)       ' $(@F);
   SILENT_LD       = @echo '  $(tb_pur)[LD]$(col_rst)       ' $(@F);
   SILENT_OBJCOPY  = @echo '  $(tb_pur)[OBJCOPY]$(col_rst)  ' $(@F);
   SILENT_OBJDUMP  = @echo '  $(tb_pur)[OBJDUMP]$(col_rst)  ' $(OBJDUMP);
   #Shut up this crap
   MAKEFLAGS+=--no-print-directory
endif

# This is a dirty hack, since I don't want to
# recurse properly into dirs with recusive make invocation
# and the only method of reading submakes descending into subdirs,
# without recusive invocation I came up with looks like shit
# Anyway, this would be a bit too much for such a project

define suck_directory
-include $(SRCDIR)/$(1)/Makefile
mkdirs+=$(OBJDIR)/$(1)
objects+=$$(addprefix $(OBJDIR)/$(1)/,$$(objects-$(2)))
endef

$(eval $(call suck_directory,src/common,common))
$(eval $(call suck_directory,src/usb_lib,usb_lib))
$(eval $(call suck_directory,src/startup,startup))
$(eval $(call suck_directory,boards/$(BOARD),y))

all:
	@$(foreach d,$(shell ls $(SRCDIR)/boards/),make BOARD=$(d) build;)

-include $(addsuffix .d,$(objects))

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(SILENT_CC) $(CROSS_CC) $(CFLAGS)  -c  -o $(@) $<

$(OBJDIR)/%.o: $(SRCDIR)/%.S
	$(SILENT_AS) $(CROSS_CC) $(ASFLAGS) -c -o $(@) $<

$(OBJDIR)/%.o: $(SRCDIR)/%.s
	$(SILENT_AS) $(CROSS_CC) $(ASFLAGS) -c -o $(@) $<

$(OBJDIR)/%.elf: $(objects)
	$(SILENT_LD) $(CROSS_CC) $(CFLAGS) --output $(@) $(LDFLAGS) $(objects)

$(OBJDIR)/%.bin: $(OBJDIR)/%.elf
	$(SILENT_OBJCOPY) $(CROSS_OBJCOPY) -O binary $< $@

#config.h symlink voodoo ensures we'll get stuff properly rebuilt
warmup:
	@mkdir -p $(OBJDIR)
	@$(foreach d,$(mkdirs),mkdir -p $(d);)
	@ln -sf $(abspath $(SRCDIR)/boards/$(BOARD)/config.h) $(OBJDIR)/config.h
	@touch $(OBJDIR)/config.h

build: warmup $(OBJDIR)/ucboot-$(BOARD).bin
	@echo "Build for board $(BOARD) complete"

clean:
	@rm -Rfv $(OBJDIR)
help:
	@echo "To build for a board, run 'make BOARD=boardname build'"
	@echo "To build for all avaliable boards run make"
	@echo "Use CROSS_COMPILE to set toolchain prefix"
	@echo "USE V=y for verbose build"
	@echo "Have fun"

.PRECIOUS: $(objects) $(TARGET).bin $(TARGET).elf
.PHONY: warmup