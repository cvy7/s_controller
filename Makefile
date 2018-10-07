###############################################################################
# Makefile for the project svz
###############################################################################

## General Flags
PROJECT = svz
MCU = atmega128
TARGET = svz.elf
CC = avr-gcc

CPP = avr-g++

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 -std=gnu99 -D__AVR_LIBC_DEPRECATED_ENABLE__   -DF_CPU=8000000UL -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS += -Wl,-u,vfprintf -Wl,--defsym=__stack=0x1000 -Wl,-Map=svz.map,--section-start=.bootloader=1f806,--section-start=.bootloader_start=1f800
#129024 0x1f800 =0xfc00 words 2048 
#126976 0x1f000 =0xf800 words 4096


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature
BIN_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature -R .bootloader -R .bootloader_start

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Include Directories
INCLUDES = -I"../port" -I"../modbus" -I"../modbus/ascii" -I"../modbus/functions" -I"../modbus/include" -I"../modbus/rtu" 

## Libraries
LIBS = -lm -lprintf_flt -lscanf_flt 

## Objects that must be built in order to link
OBJECTS = svzw.o mbcrc.o portevent.o portserial.o porttimer.o mbascii.o mbfunccoils.o mbfuncdiag.o mbfuncdisc.o mbfuncholding.o mbfuncinput.o mbfuncother.o mbutils.o mbrtu.o mb.o aut.o ser.o 
OBJECTS+= bootloader.o
#OBJECTS+= hmin.o 
OBJECTS+= pmodbus.o pdu_ser_interf.o kbd1408.o 
#OBJECTS+= lcd1408.o 
OBJECTS+= conv_320x8pdu.o pin1408.o aut_pult.o 

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Build
all: $(TARGET) svz.hex svz.eep svz.lss svz.bin size

## Compile
svzw.o: ../svzw.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbcrc.o: ../port/mbcrc.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

portevent.o: ../port/portevent.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

portserial.o: ../port/portserial.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

porttimer.o: ../port/porttimer.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbascii.o: ../modbus/ascii/mbascii.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbfunccoils.o: ../modbus/functions/mbfunccoils.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbfuncdiag.o: ../modbus/functions/mbfuncdiag.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbfuncdisc.o: ../modbus/functions/mbfuncdisc.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbfuncholding.o: ../modbus/functions/mbfuncholding.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbfuncinput.o: ../modbus/functions/mbfuncinput.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbfuncother.o: ../modbus/functions/mbfuncother.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbutils.o: ../modbus/functions/mbutils.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mbrtu.o: ../modbus/rtu/mbrtu.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

mb.o: ../modbus/mb.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

aut.o: ../aut.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

ser.o: ../ser.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

#hmin.o: ../hmin.c
#	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

pmodbus.o: ../pmodbus.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

pdu_ser_interf.o: ../pdu_ser_interf.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

kbd1408.o: ../kbd1408.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

#lcd1408.o: ../lcd1408.c
#	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

conv_320x8pdu.o: ../conv_320x8pdu.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

pin1408.o: ../pin1408.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

aut_pult.o: ../aut_pult.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<
	
bootloader.o: ../bootloader.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@
	
%.bin: $(TARGET)
	avr-objcopy -O binary $(BIN_FLASH_FLAGS)  $< $@

%.eep: $(TARGET)
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

size: ${TARGET}
	@echo
	@avr-size -C --mcu=${MCU} ${TARGET}

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) svz.elf dep/* svz.eep svz.lss svz.map


## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

