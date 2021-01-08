#
# compilation requires avr-gcc 5.4.0 - later versions generate bloated binaries
#
# get the AVR 8-bit Toolchain 3.6.2 from
# https://www.microchip.com/en-us/development-tools-tools-and-software/gcc-compilers-avr-and-arm
#

PROJ = VideoGame
MCU = attiny10

OBJS = $(PROJ)/main.o
LIBS = -lm

all:		$(PROJ).elf

$(PROJ)/main.o:	$(PROJ)/main.c
		avr-gcc -Os -x c -funsigned-char -funsigned-bitfields -DNDEBUG -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -Wall -mmcu=$(MCU) -std=gnu99 -c -o $(@) $(<)

$(PROJ).elf:	$(OBJS)
	avr-gcc -o $(@) $(OBJS) $(LIBS) -Wl,-Map=$(PROJ).map -Wl,--start-group -Wl,-lm  -Wl,--end-group -Wl,--gc-sections -mmcu=$(MCU)
	avr-objcopy -O ihex -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures  $(@) $(PROJ).hex
	avr-objcopy -j .eeprom  --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0  --no-change-warnings -O ihex $(PROJ).elf $(PROJ).eep || exit 0
	avr-objdump -h -S $(@) > $(PROJ).lss
	avr-objcopy -O srec -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures $(@) $(PROJ).srec
	avr-size $(@)

clean:
	-/bin/rm -rf *.elf *.lss *.eep *.hex *.map *.srec $(OBJS)
