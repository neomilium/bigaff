CC=avr-gcc
OBJCOPY=avr-objcopy

MCU=atmega32
CFLAGS=-W -Wall -ansi -pedantic -std=c99 -funsigned-bitfields -fpack-struct -fshort-enums -mmcu=$(MCU) -fno-jump-tables -mcall-prologues -Os -DF_CPU=16000000UL
LDFLAGS=-mmcu=$(MCU)

#CFLAGS+=-DSIMULATE_TEMP=1

SRCS=	\
	bigaff.c \
	drv_ds1307.c \
	framebuffer.c \
	led_panel.c \
	memo.c \
	shell.c \
	timer0.c \
	timer1.c \
	twi.c \
	uart.c

OBJS=	$(SRCS:.c=.o)

HEX=	bigaff.hex

all: $(HEX)


bigaff.out: version.h $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ 

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $< 

%.hex: %.out
	$(OBJCOPY) -R .eeprom -O ihex $< $@

version.h: update-version

clean:
	rm -rf *.out $(OBJS) $(HEX)

PACKAGE_VERSION=$(shell date +"%Y-%m-%d %H:%M:%S")
PACKAGE_STRING="BigAff $(PACKAGE_VERSION)"

update-version:
	echo "#define PACKAGE_STRING \"$(PACKAGE_STRING)\"" > version.h

AVRDUDE_PROGRAMMER=stk500v2
AVRDUDE_MCU=m32
SUDO=

flash: $(HEX)
	$(SUDO) avrdude -p $(AVRDUDE_MCU) -c $(AVRDUDE_PROGRAMMER) -P /dev/ttyACM0 -U flash:w:$(HEX):i

reset:
	$(SUDO) avrdude -p $(AVRDUDE_MCU) -c $(AVRDUDE_PROGRAMMER) -P /dev/ttyACM0

eeprom-read: $(HEX)
	$(SUDO) avrdude -p $(AVRDUDE_MCU) -c $(AVRDUDE_PROGRAMMER) -P /dev/ttyACM0 -U eeprom:r:eeprom.bin:r

eeprom-write: $(HEX)
	$(SUDO) avrdude -p $(AVRDUDE_MCU) -c $(AVRDUDE_PROGRAMMER) -P /dev/ttyACM0 -U eeprom:w:eeprom.bin:r

mrproper: clean
	rm -rf *.o *~

size:
	avr-size -C --mcu=$(MCU) bigaff.out

INDENT = indent

INDENT_ARGS = --blank-before-sizeof \
                --blank-lines-after-commas \
                --blank-lines-after-procedures \
                --braces-after-func-def-line \
                --braces-on-if-line \
                --braces-on-struct-decl-line \
                --continue-at-parentheses \
                --cuddle-else \
                --declaration-indentation 8 \
                --indent-level 2 \
                --line-length 120 \
                --no-blank-lines-after-declarations \
                --no-tabs \
                --preprocessor-indentation 2 \
                --procnames-start-lines \
                --space-after-cast \
                --space-after-procedure-calls

indent:
	@for f in $$(find ./ -name "*.[hc]"); do \
	echo "  INDENT $$f"; \
		$(INDENT) $(INDENT_ARGS) "$$f" || exit 1; \
	done

DATETIME_CMD = date +"%Y-%m-%d %H:%M:%S"
DATE_CMD = date +"%Y-%m-%d"
UART_PORT = /dev/ttyUSB0

datetime-sync:
	$(DATETIME_CMD)
	echo "datetime `$(DATETIME_CMD)`" > $(UART_PORT)

datetime-view:
	sleep 1
	echo "datetime" > /dev/ttyUSB0
	$(DATETIME_CMD)

test-dst-summer:
	echo "datetime 2012-03-25 01:59:55" > $(UART_PORT)

test-dst-winter:
	echo "datetime 2020-10-25 02:59:55" > $(UART_PORT)

test-memo-autodelete:
	echo "datetime 2012-03-25 01:59:59" > $(UART_PORT)
	sleep 1
	echo "memo add 2012-03-26 test1" > $(UART_PORT)
	sleep 1
	echo "memo add 2012-03-25 test2" > $(UART_PORT)
	sleep 1
	echo "memo add 2012-03-26 test3" > $(UART_PORT)
	sleep 1
	echo "memo add 2012-03-25 test4" > $(UART_PORT)
	sleep 1
	echo "memo" > $(UART_PORT)
	sleep 1
	echo "datetime 2012-03-26 01:59:59" > $(UART_PORT)
	sleep 1
	echo "memo" > $(UART_PORT)
