TC_PATH = /opt/avr8-gnu-toolchain-linux_x86_64

DEVNUM=85

CLOCK=1000000

MCU = attiny$(DEVNUM)
AVRDUDE_DEVICE = attiny$(DEVNUM)
FUSES = -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m

#CFLAGS=-DDEBUG_SPI -g -Wall -Wpedantic -Werror -std=c11 -mmcu=$(MCU) $(DEVICE_SPECIFIC_CFLAGS) -O2 -save-temps -DF_CPU=$(CLOCK) 
CFLAGS=-g -Wall -Wpedantic -Werror -std=c11 -mmcu=$(MCU) $(DEVICE_SPECIFIC_CFLAGS) -O2 -save-temps -DF_CPU=$(CLOCK) 
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

util.o: util.c util.h Makefile
	$(CC) $(CFLAGS) -c util.c -o util.o

control.o: control.c control.h util.h pins.h Makefile
	$(CC) $(CFLAGS) -c control.c -o control.o

button.o: button.c button.h util.h control.h Makefile
	$(CC) $(CFLAGS) -c button.c -o button.o

packet.o: packet.c packet.h button.h control.h util.h pins.h Makefile
	$(CC) $(CFLAGS) -c packet.c -o packet.o

main.o: main.c control.h packet.h Makefile
	$(CC) $(CFLAGS) -c main.c -o main.o

main.i: main.c Makefile
	$(CC) $(CFLAGS) main.c -E -o main.i 

main.obj: util.o control.o button.o packet.o main.o Makefile
	$(CC) $(CFLAGS) util.o control.o button.o packet.o main.o $(LDFLAGS) -o main.obj

install: main.hex Makefile
	$(AVRDUDE) -p $(AVRDUDE_DEVICE) -c avrispmkII -P $(PORT) -U flash:w:main.hex

fuse:
	$(AVRDUDE) -p $(AVRDUDE_DEVICE) -c avrispmkII -P $(PORT) $(FUSES)

get_eeprom:
	$(AVRDUDE) -p $(AVRDUDE_DEVICE) -c avrispmkII -P $(PORT) -U eeprom:r:eeprom.hex:i 

main.dump: main.hex Makefile
	$(OBJDUMP) -m avr -D main.hex > main.dump
