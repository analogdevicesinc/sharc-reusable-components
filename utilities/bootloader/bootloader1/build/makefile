################################################################################
# Project makefile
################################################################################

# Build tool settings
RM := rm
CC := arm-none-eabi-gcc
LD := elfloader

# Project settings
PROJECT = bare-metal-bootloader1
PROC = ADSP-SC589
SI_REVISION = any

# retrieve the compiler optimization level.  Default is -Og
OPT_LEVEL = -Og
ifneq ($(OPTIMIZE_LEVEL), )
	OPT_LEVEL = -O$(OPTIMIZE_LEVEL)
endif

# retrieve the compiler build release flag
ifeq ($(RELEASE), yes)
	BUILD_RELEASE =
else
	BUILD_RELEASE = -g -D_DEBUG
endif

# All of the sources participating in the build are defined here
SRC_PREFIX = ..
SRC_DIRS = \
	ARM \
	ARM/src \
	ARM/src/drivers \
	ARM/src/drivers/flash \
	ARM/src/console \
	ARM/src/cdc

# Include directories
INCLUDE_DIRS = \
	-I"../ARM/include" \
	-I"../ARM/src/drivers" \
	-I"../ARM/src/drivers/flash" \
	-I"../ARM/src"

C_SRC = $(foreach srcdir, $(SRC_DIRS), $(wildcard $(SRC_PREFIX)/$(srcdir)/*.c))
C_OBJ = $(subst $(SRC_PREFIX)/,,${C_SRC:%.c=%.o})

ASM_SRC = $(foreach srcdir, $(SRC_DIRS), $(wildcard $(SRC_PREFIX)/$(srcdir)/*.S))
ASM_OBJ = $(subst $(SRC_PREFIX)/,,${ASM_SRC:%.S=%.o})

# retrieve dependencies
DEPS = $(foreach depdir, $(SRC_DIRS), $(wildcard $(depdir)/*.d))

# define the compiler/assembler flags
CFLAGS = $(BUILD_RELEASE) $(BUILD_RTOS) -Wall -Wno-unused-but-set-variable -Wno-unused-function -gdwarf-2 -ffunction-sections -fdata-sections -DCORE0 $(INCLUDE_DIRS) -mproc=$(PROC) -msi-revision=$(SI_REVISION)
AFLAGS = -x assembler-with-cpp -mproc=$(PROC) -msi-revision=$(SI_REVISION) -gdwarf-2 -DCORE0

OBJS = $(ASM_OBJ) $(C_OBJ)

EXE = $(PROJECT).exe
LDR = $(PROJECT).ldr

all: builddirs $(LDR)

# compile 'C' files
ARM/%.o: ../ARM/%.c
	$(CC) $(CFLAGS) -I"$(SRC_PREFIX)/$(dir $@)" -MMD -MP -MF "$(basename $@).d" -o "$@" -c "$<"

# Link everything
$(EXE): $(OBJS) ../ARM/lib/cld_sc58x_cdc_lib_Core0.a
	$(CC) -mproc=$(PROC) -msi-revision=$(SI_REVISION) -T"ARM-$(PROJECT).ld" -Wl,--gc-sections -mdebug-libs -o "$@" $^ -lm

# Load everything
$(LDR): $(EXE)
	$(LD) -proc $(PROC) -core0=$^ -init ezkitSC589_initcode_core0_v10 -b SPI -f BINARY -Width 8 -bcode 0x3 -o $@

# Other Targets
clean:
	$(RM) -f $(EXE) *.xml $(PROJECT).* ../*.xml
	$(RM) -rf $(SRC_DIRS)

builddirs:
	@mkdir -p $(SRC_DIRS)

help:
	@echo 'usage:'
	@echo '    make [all|clean] [OPTIMIZE_LEVEL=<0,s,fast,g>] [RELEASE=<yes/no>]'
	@echo ''
	@echo 'DEBUG:'
	@echo '    make all'
	@echo ''
	@echo 'RELEASE:'
	@echo '    make all MAKE_LEVEL=100 RELEASE=yes'

.PHONY: all clean help builddirs
.SECONDARY:

# pull in and check dependencies
-include $(C_OBJ:.o=.d)
-include $(ASM_OBJ:.o=.d)

