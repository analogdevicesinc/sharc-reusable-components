################################################################################
# Project makefile
################################################################################

# Build tool settings
RM := rm
ARM_CC := arm-none-eabi-gcc
SHARC_CC := cc21k
SHARC_AS := easm21k
LD := elfloader

# Project settings
PROJECT := FreeRTOS-simple
#PROJECT := FreeRTOS-simple-573
LDR = $(PROJECT).ldr

# Hardware configuration
ifeq ($(PROJECT), FreeRTOS-simple)
	INIT_CODE = ezkitSC589_initcode_core0_v10
	BOARD = SAM_V1
	PROC  = ADSP-SC589
	SI_REVISION = 1.0
else
	INIT_CODE = ezkitSC573_initcode_core0
	BOARD = EZKIT_573
	PROC  = ADSP-SC573
	SI_REVISION = any
endif

# Set optimizer flags
#ARM_OPTIMIZE ?= -Og

# retrieve the command-line release flag (default no release)
ifeq ($(RELEASE), yes)
	BUILD_RELEASE =
else
	BUILD_RELEASE = -g
endif

################################################################################
# ARM section
################################################################################

# All of the ARM sources participating in the build are defined here
SRC_PREFIX = ..
ARM_SRC_DIRS = \
	ARM \
	ARM/src \
	ARM/src/oss-services/FreeRTOS-ARM \
	ARM/src/oss-services/FreeRTOS-ARM/portable/MemMang \
	ARM/src/oss-services/FreeRTOS-ARM/portable/CCES/ARM_CA5 \
	ARM/src/oss-services/FreeRTOS-ARM/portable/GCC/ARM_CA9

# Include directories
ARM_INCLUDE_DIRS = \
	-I"../ARM/include" \
	-I"../ARM/src/oss-services/FreeRTOS-ARM/include" \
	-I"../ARM/src/oss-services/FreeRTOS-ARM/portable/GCC/ARM_CA9"

ARM_C_SRC = $(foreach srcdir, $(ARM_SRC_DIRS), $(wildcard $(SRC_PREFIX)/$(srcdir)/*.c))
ARM_C_OBJ = $(subst $(SRC_PREFIX)/,,${ARM_C_SRC:%.c=%.o})

ifeq ($(PROC), ADSP-SC589)
	TMP = $(filter-out ../ARM/src/apt-sc573.c, $(ARM_C_SRC))
	ARM_C_SRC := $(TMP)
else
	TMP = $(filter-out ../ARM/src/apt-sc589.c, $(ARM_C_SRC))
	ARM_C_SRC := $(TMP)
endif

ARM_ASM_SRC = $(foreach srcdir, $(ARM_SRC_DIRS), $(wildcard $(SRC_PREFIX)/$(srcdir)/*.S))
ARM_ASM_OBJ = $(subst $(SRC_PREFIX)/,,${ARM_ASM_SRC:%.S=%.o})

# retrieve dependencies
ARM_DEPS = $(foreach depdir, $(ARM_SRC_DIRS), $(wildcard $(depdir)/*.d))

# define the compiler/assembler flags ARM_OPTIMIZE is meant to be passed in
# on the command-line (i.e. make ARM_OPTIMIZE=-O1)
ARM_CFLAGS = $(ARM_OPTIMIZE) -DFREE_RTOS -D__$(BOARD)__
ARM_CFLAGS += -Wall -Wno-unused-but-set-variable -Wno-unused-function $(BUILD_RELEASE)
ARM_CFLAGS += -march=armv7-a -mcpu=cortex-a5 -gdwarf-2 -ffunction-sections -fdata-sections
ARM_CFLAGS += -DCORE0 $(ARM_INCLUDE_DIRS) -mproc=$(PROC) -msi-revision=$(SI_REVISION)

ARM_AFLAGS = -x assembler-with-cpp -mproc=$(PROC) -msi-revision=$(SI_REVISION) -gdwarf-2 -DCORE0

ARM_OBJS = $(ARM_ASM_OBJ) $(ARM_C_OBJ)

ARM_EXE = $(PROJECT)-ARM.exe

# compile ARM 'C' files
ARM/%.o: ../ARM/%.c
	$(ARM_CC) $(ARM_CFLAGS) -I"$(SRC_PREFIX)/$(dir $@)" -MMD -MP -MF "$(basename $@).d" -o "$@" -c "$<"

# compile ARM assembly files
ARM/%.o: ../ARM/%.S
	$(ARM_CC) $(ARM_AFLAGS) -MMD -MP -MF "$(basename $@).d" -o "$@" -c "$<"

$(ARM_EXE): $(ARM_OBJS)
	$(ARM_CC) -mproc=$(PROC) -msi-revision=$(SI_REVISION) -T"ARM-$(PROJECT).ld" -Wl,--gc-sections -madi-threads -mdebug-libs -o "$@" $^ -lm

################################################################################
# Generic section
################################################################################

.DEFAULT_GOAL = all
all: builddirs $(LDR)

$(LDR):  $(ARM_EXE)
	$(LD) -proc $(PROC) -core0=$(ARM_EXE) -init $(INIT_CODE) -b SPI -f BINARY -Width 8 -bcode 0x0C -verbose -o $@ > /dev/null 2>&1

# Other Targets
clean:
	$(RM) -f *.xml $(PROJECT).* ../*.xml
	$(RM) -rf $(ARM_EXE) $(ARM_SRC_DIRS)

builddirs:
	@mkdir -p $(ARM_SRC_DIRS)

help:
	@echo 'usage:'
	@echo '    make [all|clean] [ARM_OPTIMIZE=<-Og,-O1,etc.>] [RELEASE=<yes/no>]'
	@echo ''
	@echo 'DEBUG:'
	@echo '    make all'
	@echo ''
	@echo 'RELEASE:'
	@echo '    make all ARM_OPTIMIZE=-O1 RELEASE=yes'


.PHONY: all clean help builddirs
.SECONDARY:

# pull in and check dependencies
-include $(ARM_C_OBJ:.o=.d)
-include $(ARM_ASM_OBJ:.o=.d)
