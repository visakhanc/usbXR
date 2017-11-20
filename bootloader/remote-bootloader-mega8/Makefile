# Hey Emacs, this is a -*- makefile -*-

.PHONY:	all build elf hex eep lss sym program coff extcoff clean depend

MCU = atmega8
F_CPU = 8000000
FORMAT = ihex
TARGET = main
COMMON_DIR = ../../../common
RFM70_DIR = $(COMMON_DIR)/RFM70_lib
SPI_DIR = $(COMMON_DIR)/avr_spi
SRC = main.c $(RFM70_DIR)/rfm70_lib.c $(SPI_DIR)/avr_spi.c
OPT = s
BOOTLOADER_START = 1800

FUSE_H = 0xc0
FUSE_L = 0xe4
# Fuse high byte:
# 0xc0 = 1 1 0 0   0 0 0 0 <-- BOOTRST (boot reset vector at 0x1800)
#        ^ ^ ^ ^   ^ ^ ^------ BOOTSZ0 
#        | | | |   | +-------- BOOTSZ1
#        | | | |   + --------- EESAVE (preserve EEPROM over chip erase)
#        | | | +-------------- CKOPT (full output swing)
#        | | +---------------- SPIEN (allow serial programming)
#        | +------------------ WDTON (WDT not always on)
#        +-------------------- RSTDISBL (reset pin is enabled)
# Fuse low byte:
# 0xe4 = 1 1 1 0   0 1 0 0 
#        ^ ^ \ /   \--+--/
#        | |  |       +------- CKSEL 3..0 (Internal 8MHz crystal)
#        | |  +--------------- SUT 1..0 (Max startup time)
#        | +------------------ BODEN (BrownOut Detector disabled)
#        +-------------------- BODLEVEL (2.7V)



# List any extra directories to look for include files here.
#     Each directory must be seperated by a space.
#     Use forward slashes for directory separators.
#     For a directory that has spaces, enclose it in quotes.
CINCS = -I$(RFM70_DIR) -I$(SPI_DIR) -I.

# List any extra directories to look for libraries here.
#     Each directory must be seperated by a space.
#     Use forward slashes for directory separators.
#     For a directory that has spaces, enclose it in quotes.
EXTRALIBDIRS = $(RFM70_DIR)


CSTANDARD = -std=gnu99
CDEFS = -DF_CPU=$(F_CPU)UL
CDEBUG = -g
CWARN = -Wall -Wstrict-prototypes
CTUNING = -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -ffunction-sections -fdata-sections
#CEXTRA = -Wa,-adhlns=$(<:.c=.lst)
ALL_CFLAGS = -mmcu=$(MCU) $(CDEBUG) $(CINCS) $(CDEFS) -O$(OPT) $(CWARN) $(CSTANDARD) $(CTUNING)


LDFLAGS =  -Wl,-Map=$(TARGET).map,--cref
LDFLAGS += $(patsubst %,-L%,$(EXTRALIBDIRS))
LDFLAGS += -Wl,--section-start=.text=$(BOOTLOADER_START)
LDFLAGS += -Wl,--gc-sections

####### AVRDUDE #######

AVRDUDE_PROGRAMMER = usbasp
AVRDUDE_PORT = usb
AVRDUDE_WRITE_FLASH = -U flash:w:$(TARGET).hex
#AVRDUDE_WRITE_EEPROM = -U eeprom:w:$(TARGET).eep


AVRDUDE_FLAGS = -p $(MCU) -P $(AVRDUDE_PORT) -c $(AVRDUDE_PROGRAMMER)

###### COMPILE AND LINK ########
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
NM = avr-nm
AVRDUDE = avrdude
REMOVE = rm -f
MV = mv -f
SIZE = avr-size
AR = avr-ar rcs

# Define all object files.
OBJ = $(SRC:.c=.o)  


# Define all listing files.
LST = $(SRC:.c=.lst)


# Default target. (lib for compiling to static library)

all: elf hex eep lss size
#all: lib

elf: $(TARGET).elf
hex: $(TARGET).hex
eep: $(TARGET).eep
lss: $(TARGET).lss 
sym: $(TARGET).sym
LIBNAME=lib$(TARGET).a
lib: $(LIBNAME)

size: 
	$(SIZE) --mcu=$(MCU) --format=avr $(TARGET).elf

# Program the device.  
program: $(TARGET).hex $(TARGET).eep
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH) $(AVRDUDE_WRITE_EEPROM)

fuse:
	$(AVRDUDE) -U hfuse:w:$(FUSE_H):m -U lfuse:w:$(FUSE_L):m
	
.SUFFIXES: .elf .hex .eep .lss .sym

.elf.hex:
	$(OBJCOPY) -O $(FORMAT) -R .eeprom $< $@

.elf.eep:
	-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 -O $(FORMAT) $< $@

# Create extended listing file from ELF output file.
.elf.lss:
	$(OBJDUMP) -h -S $< > $@

# Create a symbol table from ELF output file.
.elf.sym:
	$(NM) -n $< > $@


# Link:
# create ELF output file from object files.
$(TARGET).elf: $(OBJ)
	$(CC) $(ALL_CFLAGS) $(OBJ) --output $@ $(LDFLAGS)

%.a: $(OBJ)
	$(AR) $@ $(OBJ)
	
# Compile: 
# create object files from C source files.
.c.o:
	$(CC) -c $(ALL_CFLAGS) $< -o $@ 

# Target: clean project.
clean:
	$(REMOVE) $(TARGET).hex $(TARGET).eep $(TARGET).cof $(TARGET).elf \
	$(TARGET).map $(TARGET).sym $(TARGET).lss \
	$(OBJ) $(LST) $(SRC:.c=.s) $(SRC:.c=.d)
