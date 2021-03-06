PRG            = ws2812_sprites
#PRG            = ws2812_pwm
BURNER         = avr109
BURNERDEV      = /dev/ttyACM0
OBJ            = $(PRG).o
# define ATMEGA328P 0
# MCU_TARGET     = atmega32u4
# define ATMEGA328P 1
MCU_TARGET     = atmega328p

OPTIMIZE       = -O2

INCLUDES       = 
DEFS           =
LIBS           = 

# You should not have to change anything below here.

CC             = avr-gcc

# Override is only needed by avr-lib build system.

override CFLAGS        = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS) $(INCLUDES)
override LDFLAGS       = -Wl,-Map,$(PRG).map

OBJCOPY        = avr-objcopy
OBJDUMP        = avr-objdump

all: $(PRG).elf lst text eeprom

upload burn: all
	@echo PRESS RESET
	@until lsusb | grep -q 'ID 2341:0036'; do sleep 1; echo -n . ; done
	@python -c 'import time; time.sleep(.1)'
	avrdude -c $(BURNER) -u -p $(MCU_TARGET) -P ${BURNERDEV} -U flash:w:$(PRG).hex

arduino: all
	avrdude -c arduino -u -p $(MCU_TARGET) -P /dev/ttyUSB0 -b 115200 -D -U flash:w:$(PRG).hex

$(PRG).elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

#demo.o: demo.c iocompat.h

clean:
	rm -rf *.o $(PRG).elf *.eps *.png *.pdf *.bak 
	rm -rf *.lst *.map $(EXTRA_CLEAN_FILES)

lst:  $(PRG).lst

%.lst: %.elf
	@$(OBJDUMP) -h -S $< > $@

# Rules for building the .text rom images

text: hex bin srec

hex:  $(PRG).hex
bin:  $(PRG).bin
srec: $(PRG).srec

%.hex: %.elf
	@$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.srec: %.elf
	@$(OBJCOPY) -j .text -j .data -O srec $< $@

%.bin: %.elf
	@$(OBJCOPY) -j .text -j .data -O binary $< $@

# Rules for building the .eeprom rom images

eeprom: ehex ebin esrec

ehex:  $(PRG)_eeprom.hex
ebin:  $(PRG)_eeprom.bin
esrec: $(PRG)_eeprom.srec

%_eeprom.hex: %.elf
	@$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@ \
	|| { echo empty $@ not generated; exit 0; }

%_eeprom.srec: %.elf
	@$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O srec $< $@ \
	|| { echo empty $@ not generated; exit 0; }

%_eeprom.bin: %.elf
	@$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O binary $< $@ \
	|| { echo empty $@ not generated; exit 0; }

# Every thing below here is used by avr-libc's build system and can be ignored
# by the casual user.

FIG2DEV                 = fig2dev
EXTRA_CLEAN_FILES       = *.hex *.bin *.srec

dox: eps png pdf

eps: $(PRG).eps
png: $(PRG).png
pdf: $(PRG).pdf

%.eps: %.fig
	$(FIG2DEV) -L eps $< $@

%.pdf: %.fig
	$(FIG2DEV) -L pdf $< $@

%.png: %.fig
	$(FIG2DEV) -L png $< $@

