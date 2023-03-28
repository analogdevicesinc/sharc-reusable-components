################################################################################
# Project makefile
################################################################################

# Build tool settings
RM := rm
ARM_CC := arm-none-eabi-gcc
SHARC_CC := cc21k.exe
SHARC_AS := easm21k.exe
LD := elfloader.exe

# Project settings
PROJECT = FreeRTOS-multi-core
PROC = ADSP-SC589
SI_REVISION = 1.0
LDR = $(PROJECT).ldr

# retrieve the compiler build release flag (default no release)
ifeq ($(RELEASE), yes)
	BUILD_RELEASE =
else
	BUILD_RELEASE = -g -D_DEBUG
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
	-I"../ALL/include" \
	-I"../ARM/include" \
	-I"../ARM/src/oss-services/FreeRTOS-ARM/include" \
	-I"../ARM/src/oss-services/FreeRTOS-ARM/portable/GCC/ARM_CA9"

ARM_C_SRC = $(foreach srcdir, $(ARM_SRC_DIRS), $(wildcard $(SRC_PREFIX)/$(srcdir)/*.c))
ARM_C_OBJ = $(subst $(SRC_PREFIX)/,,${ARM_C_SRC:%.c=%.o})

ARM_ASM_SRC = $(foreach srcdir, $(ARM_SRC_DIRS), $(wildcard $(SRC_PREFIX)/$(srcdir)/*.S))
ARM_ASM_OBJ = $(subst $(SRC_PREFIX)/,,${ARM_ASM_SRC:%.S=%.o})

# retrieve dependencies
ARM_DEPS = $(foreach depdir, $(ARM_SRC_DIRS), $(wildcard $(depdir)/*.d))

# define the compiler/assembler flags OPTIMIZE is meant to be passed in
# on the command-line (i.e. make OPTIMIZE=-O3)
ARM_CFLAGS = $(OPTIMIZE) -DFREE_RTOS -D__SAM_V1__ -Wall -Wno-unused-but-set-variable -Wno-unused-function $(BUILD_RELEASE) $(BUILD_RTOS) -march=armv7-a -mcpu=cortex-a5 -gdwarf-2 -ffunction-sections -fdata-sections -DCORE0 $(ARM_INCLUDE_DIRS) -mproc=$(PROC) -msi-revision=$(SI_REVISION)
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
# SHARC0 section
################################################################################

# All of the ARM sources participating in the build are defined here
SHARC0_SRC_DIRS = \
	SHARC0 \
	SHARC0/src \
	SHARC0/startup_ldf

# Include directories
SHARC0_INCLUDE_DIRS = \
	-I"../ALL/include" \
	-I"../SHARC0/include" \
	-I"../SHARC0/src"

SHARC0_EXE = $(PROJECT)-SHARC0.exe

SHARC0_C_SRC = $(foreach srcdir, $(SHARC0_SRC_DIRS), $(wildcard $(SRC_PREFIX)/$(srcdir)/*.c))
SHARC0_C_OBJ = $(subst $(SRC_PREFIX)/,,${SHARC0_C_SRC:%.c=%.doj})

SHARC0_ASM_SRC = $(foreach srcdir, $(SHARC0_SRC_DIRS), $(wildcard $(SRC_PREFIX)/$(srcdir)/*.s))
SHARC0_ASM_OBJ = $(subst $(SRC_PREFIX)/,,${SHARC0_ASM_SRC:%.s=%.doj})

# retrieve dependencies
SHARC0_DEPS = $(foreach depdir, $(SHARC0_SRC_DIRS), $(wildcard $(depdir)/*.d))

# define the compiler/assembler flags
SHARC0_CFLAGS = $(BUILD_RELEASE) -proc $(PROC) $(SHARC0_INCLUDE_DIRS) -si-revision $(SI_REVISION) -DCORE1 -structs-do-not-overlap -no-const-strings -no-multiline -warn-protos -double-size-32 -char-size-8 -swc
SHARC0_AFLAGS = $(BUILD_RELEASE) -proc $(PROC) -si-revision $(SI_REVISION) -DCORE1 -swc -char-size-8

SHARC0_OBJS = $(SHARC0_ASM_OBJ) $(SHARC0_C_OBJ)

SHARC0_EXE = $(PROJECT)-SHARC0.dxe

# compile SHARC0 'C' files
SHARC0/%.doj: ../SHARC0/%.c
	$(SHARC_CC) $(SHARC0_CFLAGS) -I"$(SRC_PREFIX)/$(dir $@)" -gnu-style-dependencies -MD -Mo "$(basename $@).d" -o "$@" -c "$<"

# compile SHARC0 assembly files
SHARC0/%.doj: ../SHARC0/%.s
	$(SHARC_AS) $(SHARC0_AFLAGS) -gnu-style-dependencies -MM -Mo "$(basename $@).d" -o "$@" "$<"

$(SHARC0_EXE): $(SHARC0_OBJS)
	$(SHARC_CC) -proc $(PROC) -si-revision $(SI_REVISION) -T"SHARC0-$(PROJECT).ldf" -no-mem -map linker_log_SHARC0.xml -flags-link -ip -flags-link -MDCORE1,-MDOTHERCORE,-MDDEBUG -o "$@" $^
	@$(RM) -f $@.doj
	@$(RM) -f $@.asm

################################################################################
# SHARC1 section
################################################################################

# All of the ARM sources participating in the build are defined here
SHARC1_SRC_DIRS = \
	SHARC1 \
	SHARC1/src \
	SHARC1/startup_ldf

# Include directories
SHARC1_INCLUDE_DIRS = \
	-I"../ALL/include" \
	-I"../SHARC1/include" \
	-I"../SHARC1/src"

SHARC1_EXE = $(PROJECT)-SHARC1.exe

SHARC1_C_SRC = $(foreach srcdir, $(SHARC1_SRC_DIRS), $(wildcard $(SRC_PREFIX)/$(srcdir)/*.c))
SHARC1_C_OBJ = $(subst $(SRC_PREFIX)/,,${SHARC1_C_SRC:%.c=%.doj})

SHARC1_ASM_SRC = $(foreach srcdir, $(SHARC1_SRC_DIRS), $(wildcard $(SRC_PREFIX)/$(srcdir)/*.s))
SHARC1_ASM_OBJ = $(subst $(SRC_PREFIX)/,,${SHARC1_ASM_SRC:%.s=%.doj})

# retrieve dependencies
SHARC1_DEPS = $(foreach depdir, $(SHARC1_SRC_DIRS), $(wildcard $(depdir)/*.d))

# define the compiler/assembler flags
SHARC1_CFLAGS = $(BUILD_RELEASE) -proc $(PROC) $(SHARC1_INCLUDE_DIRS) -si-revision $(SI_REVISION) -DCORE1 -structs-do-not-overlap -no-const-strings -no-multiline -warn-protos -double-size-32 -char-size-8 -swc
SHARC1_AFLAGS = $(BUILD_RELEASE) -proc $(PROC) -si-revision $(SI_REVISION) -DCORE1 -swc -char-size-8

SHARC1_OBJS = $(SHARC1_ASM_OBJ) $(SHARC1_C_OBJ)

SHARC1_EXE = $(PROJECT)-SHARC1.dxe

# compile SHARC1 'C' files
SHARC1/%.doj: ../SHARC1/%.c
	$(SHARC_CC) $(SHARC1_CFLAGS) -I"$(SRC_PREFIX)/$(dir $@)" -gnu-style-dependencies -MD -Mo "$(basename $@).d" -o "$@" -c "$<"

# compile SHARC1 assembly files
SHARC1/%.doj: ../SHARC1/%.s
	$(SHARC_AS) $(SHARC1_AFLAGS) -gnu-style-dependencies -MM -Mo "$(basename $@).d" -o "$@" "$<"

$(SHARC1_EXE): $(SHARC1_OBJS)
	$(SHARC_CC) -proc $(PROC) -si-revision $(SI_REVISION) -T"SHARC1-$(PROJECT).ldf" -no-mem -map linker_log_SHARC1.xml -flags-link -ip -flags-link -MDCORE1,-MDOTHERCORE,-MDDEBUG -o "$@" $^
	@$(RM) -f $@.doj
	@$(RM) -f $@.asm

################################################################################
# Generic section
################################################################################

.DEFAULT_GOAL = all
all: builddirs $(LDR)

$(LDR):  $(ARM_EXE) $(SHARC0_EXE) $(SHARC1_EXE)
	$(LD) -proc ADSP-SC589 -core0=$(ARM_EXE) -init ezkitSC589_initcode_core0_v10 -core1=$(SHARC0_EXE) -core2=$(SHARC1_EXE) -NoFinalTag=$(ARM_EXE) -NoFinalTag=$(SHARC0_EXE) -b SPI -f BINARY -Width 8 -bcode 0x0C -verbose -o $@ > /dev/null 2>&1

# Other Targets
clean:
	$(RM) -f *.xml $(PROJECT).* ../*.xml
	$(RM) -rf $(ARM_EXE) $(ARM_SRC_DIRS)
	$(RM) -rf $(SHARC0_EXE) $(SHARC0_SRC_DIRS)
	$(RM) -rf $(SHARC1_EXE) $(SHARC1_SRC_DIRS)

builddirs:
	@mkdir -p $(ARM_SRC_DIRS)
	@mkdir -p $(SHARC0_SRC_DIRS)
	@mkdir -p $(SHARC1_SRC_DIRS)

help:
	@echo 'usage:'
	@echo '    make [all|clean] [OPTIMIZE_LEVEL=<0,s,fast,g>] [RELEASE=<yes/no>]'
	@echo ''
	@echo 'DEBUG:'
	@echo '    make all'
	@echo ''
	@echo 'RELEASE:'
	@echo '    make all OPTIMIZE_LEVEL=g RELEASE=yes'

.PHONY: all clean help builddirs
.SECONDARY:

# pull in and check dependencies
-include $(ARM_C_OBJ:.o=.d)
-include $(ARM_ASM_OBJ:.o=.d)

-include $(SHARC0_C_OBJ:.doj=.d)
-include $(SHARC0_ASM_OBJ:.doj=.d)

-include $(SHARC1_C_OBJ:.doj=.d)
-include $(SHARC1_ASM_OBJ:.doj=.d)
