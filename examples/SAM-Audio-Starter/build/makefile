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
PROJECT = SAM-Audio-Starter
PROC = ADSP-SC589
SI_REVISION = 1.0
LDR = $(PROJECT).ldr

# Set optimizer flags
#ARM_OPTIMIZE ?= -O1
#SHARC_OPTIMIZE ?= -Ov100
SHARC0_OPTIMIZE ?= $(SHARC_OPTIMIZE)
SHARC1_OPTIMIZE ?= $(SHARC_OPTIMIZE)

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
	ALL \
	ALL/src/sae \
	ARM \
	ARM/src \
	ARM/src/adi-drivers/rsi \
	ARM/src/simple-drivers \
	ARM/src/simple-services/adau1761 \
	ARM/src/simple-services/syslog \
	ARM/src/simple-services/buffer-track \
	ARM/src/simple-services/a2b-xml \
	ARM/src/simple-services/adi-a2b-cmdlist \
	ARM/src/simple-services/FreeRTOS-cpu-load \
	ARM/src/simple-services/adi-osal-minimal \
	ARM/src/simple-services/uac2-soundcard \
	ARM/src/simple-services/fs-dev \
	ARM/src/oss-services/umm_malloc \
	ARM/src/oss-services/shell \
	ARM/src/oss-services/romfs \
	ARM/src/oss-services/crc \
	ARM/src/oss-services/xmodem \
	ARM/src/oss-services/yxml \
	ARM/src/oss-services/pa-ringbuffer \
	ARM/src/oss-services/FreeRTOS-ARM \
	ARM/src/oss-services/FreeRTOS-ARM/portable/MemMang \
	ARM/src/oss-services/FreeRTOS-ARM/portable/CCES/ARM_CA5 \
	ARM/src/oss-services/FreeRTOS-ARM/portable/GCC/ARM_CA9 \
	ARM/src/oss-services/FatFs

# Include directories
ARM_INCLUDE_DIRS = \
	-I. \
	-I"../ALL/src" \
	-I"../ALL/src/sae" \
	-I"../ALL/include" \
	-I"../ARM/include" \
	-I"../ARM/src" \
	-I"../ARM/src/simple-drivers" \
	-I"../ARM/src/simple-services/adau1761" \
	-I"../ARM/src/simple-services/syslog" \
	-I"../ARM/src/simple-services/buffer-track" \
	-I"../ARM/src/simple-services/a2b-xml" \
	-I"../ARM/src/simple-services/FreeRTOS-cpu-load" \
	-I"../ARM/src/simple-services/adi-a2b-cmdlist" \
	-I"../ARM/src/simple-services/fs-dev" \
	-I"../ARM/src/simple-services/uac2-soundcard" \
	-I"../ARM/src/oss-services/umm_malloc" \
	-I"../ARM/src/oss-services/shell" \
	-I"../ARM/src/oss-services/romfs" \
	-I"../ARM/src/oss-services/crc" \
	-I"../ARM/src/oss-services/xmodem" \
	-I"../ARM/src/oss-services/yxml" \
	-I"../ARM/src/oss-services/pa-ringbuffer" \
	-I"../ARM/src/oss-services/FreeRTOS-ARM/include" \
	-I"../ARM/src/oss-services/FreeRTOS-ARM/portable/GCC/ARM_CA9" \
	-I"../ARM/src/oss-services/FatFs"

ARM_C_SRC = $(foreach srcdir, $(ARM_SRC_DIRS), $(wildcard $(SRC_PREFIX)/$(srcdir)/*.c))
ARM_C_OBJ = $(subst $(SRC_PREFIX)/,,${ARM_C_SRC:%.c=%.o})

ARM_ASM_SRC = $(foreach srcdir, $(ARM_SRC_DIRS), $(wildcard $(SRC_PREFIX)/$(srcdir)/*.S))
ARM_ASM_OBJ = $(subst $(SRC_PREFIX)/,,${ARM_ASM_SRC:%.S=%.o})

# Can't do this automatically due to library linking order dependency.
# When the GNU linker sees a library, it discards all symbols that it
# doesn't need.
ARM_STATIC_LIBS = ../ARM/lib/cld_sc58x_audio_2_0_w_cdc_lib_Core0.a

# retrieve dependencies
ARM_DEPS = $(foreach depdir, $(ARM_SRC_DIRS), $(wildcard $(depdir)/*.d))

# define the compiler/assembler flags ARM_OPTIMIZE is meant to be passed in
# on the command-line (i.e. make ARM_OPTIMIZE=-O1)
ARM_CFLAGS = $(ARM_OPTIMIZE) $(BUILD_RELEASE) $(BUILD_RTOS) $(ARM_INCLUDE_DIRS)
ARM_CFLAGS += -Wall -Wno-unused-but-set-variable -Wno-unused-function
ARM_CFLAGS += -march=armv7-a -mcpu=cortex-a5 -gdwarf-2 -ffunction-sections -fdata-sections
ARM_CFLAGS += -mproc=$(PROC) -msi-revision=$(SI_REVISION) -DCORE0
ARM_CFLAGS += -DSAE_IPC -DFREE_RTOS -D__SAM_V1__ -DUSB_CDC_STDIO

ARM_AFLAGS = -x assembler-with-cpp -mproc=$(PROC) -msi-revision=$(SI_REVISION) -gdwarf-2 -DCORE0

ARM_OBJS = $(ARM_ASM_OBJ) $(ARM_C_OBJ)

ARM_EXE = $(PROJECT)-ARM.exe

# compile the common files
ALL/%.o: ../ALL/%.c
	$(ARM_CC) $(ARM_CFLAGS) -I"$(SRC_PREFIX)/$(dir $@)" -MMD -MP -MF "$(basename $@).d" -o "$@" -c "$<"

# compile ARM 'C' files
ARM/%.o: ../ARM/%.c
	$(ARM_CC) $(ARM_CFLAGS) -I"$(SRC_PREFIX)/$(dir $@)" -MMD -MP -MF "$(basename $@).d" -o "$@" -c "$<"

# compile ARM assembly files
ARM/%.o: ../ARM/%.S
	$(ARM_CC) $(ARM_AFLAGS) -MMD -MP -MF "$(basename $@).d" -o "$@" -c "$<"

# Create the ARM executable
$(ARM_EXE): $(ARM_OBJS) $(ARM_STATIC_LIBS)
	$(ARM_CC) -mproc=$(PROC) -msi-revision=$(SI_REVISION) -T"ARM-$(PROJECT).ld" -Wl,-Map=$(PROJECT)-ARM.map -Wl,--gc-sections -madi-threads -mdebug-libs -o "$@" $^ -lm

################################################################################
# SHARC0 section
################################################################################

# All of the ARM sources participating in the build are defined here
SHARC0_SRC_DIRS = \
	ALL \
	ALL/src/sae \
	SHARC0 \
	SHARC0/src \
	SHARC0/startup_ldf

# Include directories
SHARC0_INCLUDE_DIRS = \
	-I"../ALL/src/sae" \
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
SHARC0_CFLAGS = $(SHARC0_OPTIMIZE) $(BUILD_RELEASE) -proc $(PROC) $(SHARC0_INCLUDE_DIRS) -si-revision $(SI_REVISION) -DCORE1 -structs-do-not-overlap -no-const-strings -no-multiline -warn-protos -double-size-32 -char-size-8 -swc
SHARC0_AFLAGS = $(BUILD_RELEASE) -proc $(PROC) -si-revision $(SI_REVISION) -DCORE1 -swc -char-size-8

SHARC0_OBJS = $(SHARC0_ASM_OBJ) $(SHARC0_C_OBJ)

SHARC0_EXE = $(PROJECT)-SHARC0.dxe

# compile the common files
ALL/%.doj: ../ALL/%.c
	$(SHARC_CC) $(SHARC0_CFLAGS) -I"$(SRC_PREFIX)/$(dir $@)" -gnu-style-dependencies -MD -Mo "$(basename $@).d" -o "$@" -c "$<"

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
	ALL \
	ALL/src/sae \
	SHARC1 \
	SHARC1/src \
	SHARC1/startup_ldf

# Include directories
SHARC1_INCLUDE_DIRS = \
	-I"../ALL/src/sae" \
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
SHARC1_CFLAGS = $(SHARC1_OPTIMIZE) $(BUILD_RELEASE) -proc $(PROC) $(SHARC1_INCLUDE_DIRS) -si-revision $(SI_REVISION) -DCORE2 -structs-do-not-overlap -no-const-strings -no-multiline -warn-protos -double-size-32 -char-size-8 -swc
SHARC1_AFLAGS = $(BUILD_RELEASE) -proc $(PROC) -si-revision $(SI_REVISION) -DCORE1 -swc -char-size-8

SHARC1_OBJS = $(SHARC1_ASM_OBJ) $(SHARC1_C_OBJ)

SHARC1_EXE = $(PROJECT)-SHARC1.dxe

# compile the common files
ALL/%.doj: ../ALL/%.c
	$(SHARC_CC) $(SHARC1_CFLAGS) -I"$(SRC_PREFIX)/$(dir $@)" -gnu-style-dependencies -MD -Mo "$(basename $@).d" -o "$@" -c "$<"

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
	$(LD) -proc $(PROC) -core0=$(ARM_EXE) -init ezkitSC589_initcode_core0_v10 -core1=$(SHARC0_EXE) -core2=$(SHARC1_EXE) -NoFinalTag=$(ARM_EXE) -NoFinalTag=$(SHARC0_EXE) -b SPI -f BINARY -Width 8 -bcode 0x0C -o $@

# Other Targets
clean:
	$(RM) -f *.xml $(PROJECT).* ../*.xml *.map
	$(RM) -rf $(ARM_EXE) $(ARM_SRC_DIRS)
	$(RM) -rf $(SHARC0_EXE) $(SHARC0_SRC_DIRS)
	$(RM) -rf $(SHARC1_EXE) $(SHARC1_SRC_DIRS)

builddirs:
	@mkdir -p $(ARM_SRC_DIRS)
	@mkdir -p $(SHARC0_SRC_DIRS)
	@mkdir -p $(SHARC1_SRC_DIRS)

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

-include $(SHARC0_C_OBJ:.doj=.d)
-include $(SHARC0_ASM_OBJ:.doj=.d)

-include $(SHARC1_C_OBJ:.doj=.d)
-include $(SHARC1_ASM_OBJ:.doj=.d)
