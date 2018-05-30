TC_PATH = /opt/avr8-gnu-toolchain-linux_x86_64

DEVNUM=85

CLOCK=1000000

MCU = attiny$(DEVNUM)
AVRDUDE_DEVICE = attiny$(DEVNUM)
FUSES = -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m

CFLAGS=-g -Wall -Wpedantic -Werror -std=c11 -mmcu=$(MCU) $(DEVICE_SPECIFIC_CFLAGS) -Os -save-temps -DF_CPU=$(CLOCK) 
# adding -mcall-prologues might may the code smaller and slower
CC=$(TC_PATH)/bin/avr-gcc
OBJ2HEX=$(TC_PATH)/bin/avr-objcopy 
OBJDUMP=$(TC_PATH)/bin/avr-objdump 
LDFLAGS=-Wl,-gc-sections -Wl,-relax

PORT ?= usb
AVRDUDE=avrdude

all: main.hex

clean: Makefile
	rm -f *.o *.hex *.obj *.hex test *.i *.bin

%.hex: %.obj Makefile
	$(OBJ2HEX) -R .eeprom -O ihex $< $@

main.o: main.c Makefile
	$(CC) $(CFLAGS) -c main.c -o main.o

main.i: main.c Makefile
	$(CC) $(CFLAGS) main.c -E -o main.i 

main.obj: main.o Makefile
	$(CC) $(CFLAGS) main.o $(LDFLAGS) -o main.obj

install: main.hex Makefile
	$(AVRDUDE) -p $(AVRDUDE_DEVICE) -c avrispmkII -P $(PORT) -U flash:w:main.hex

fuse:
	$(AVRDUDE) -p $(AVRDUDE_DEVICE) -c avrispmkII -P $(PORT) $(FUSES)

get_eeprom:
	$(AVRDUDE) -p $(AVRDUDE_DEVICE) -c avrispmkII -P $(PORT) -U eeprom:r:eeprom.hex:i 

main.dump: main.hex Makefile
	$(OBJDUMP) -m avr -D main.hex > main.dump
