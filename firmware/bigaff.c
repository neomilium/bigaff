#include "bigaff.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>

#include "framebuffer.h"
#include "led_panel.h"
#include "uart.h"
#include "shell.h"
#include "timer0.h"

#include "twi.h"
#include "drv_ds1307.h"
#include "rtc.h"

#include "config.h"

#include "memo.h"

/* define CPU frequency in Mhz here if not defined in Makefile */
#ifndef F_CPU
#  define F_CPU 16000000UL
#endif

/* Configure le flux d'entrée sortie uart_stream */
FILE    uart_stream = FDEV_SETUP_STREAM (uart_putchar, uart_getchar, _FDEV_SETUP_RW);

typedef enum {
  BIGAFF_MODE_WAIT,
  BIGAFF_MODE_DATETIME,
  BIGAFF_MODE_MESSAGE,
} bigaff_mode_t;

// Debug mode bit
volatile bool _debug_mode = true;

// BigAff's current mode used in the main state-machine
static bigaff_mode_t _bigaff_mode = BIGAFF_MODE_DATETIME;
// Remaining seconds used in message mode
static uint8_t _bigaff_message_seconds_left = 0;
// Sequence id used while displaying an animated text
static uint16_t _bigaff_sequence = 0;

shell_command_t shell_commands[SHELL_COMMAND_COUNT];

#define DECLARE_CMD(ID, TEXT, DESCRIPTION, DEBUG, FUNCTION) \
  static const char menu##ID##_text[] PROGMEM = TEXT; \
  static const char menu##ID##_description[] PROGMEM = DESCRIPTION; \
  shell_commands[ID].text = menu##ID##_text; \
  shell_commands[ID].description = menu##ID##_description; \
  shell_commands[ID].debug = DEBUG; \
  shell_commands[ID].function = FUNCTION;

int
main (void)
{
  uart_init ();
  framebuffer_init ();
  led_panel_init (framebuffer_get_display);
  led_panel_set_intensity(eeprom_read_byte((const uint8_t*)EEPROM_MEMORY_MAP__INTENSITY));
  timer0_init();

  /* L'entrée et la sortie standard utilisent l'UART */
  stdout = stdin = &uart_stream;

  printf_P (PSTR ("\n" PACKAGE_STRING "\n"));

  twi_init();
  drv_ds1307_init();
  // Make sure that DS1307 is running
  drv_ds1307_start();
  memo_init();

  DECLARE_CMD (0, "help", "this help", false, bigaff_command_help)
  DECLARE_CMD (1, "datetime", "set/get datetime\n\t* datetime [YYYY-MM-DD HH:MM:SS]", false, bigaff_command_datetime);
  DECLARE_CMD (2, "message", "set message during seconds (0-255)\n\t* message [seconds [text]]", false, bigaff_command_message);
  DECLARE_CMD (3, "memo", "list/add/del memo\n\t* memo\n\t* memo add YYYY-MM-DD text\n\t* memo del id", false, bigaff_command_memo);
  DECLARE_CMD (4, "intensity", "set led panel intensity (0-255)\n\t* intensity [level]", false, bigaff_command_intensity);
  DECLARE_CMD (5, "debug", "toogle debug mode", false, bigaff_command_debug);
  DECLARE_CMD (6, "eepr", "eeprom read", true, bigaff_command_debug_eeprom_read);
  DECLARE_CMD (7, "freemem", "display freemem", true, bigaff_command_debug_freemem);
  DECLARE_CMD (8, "reset", "reset MCU", true, bigaff_command_debug_reset);

  sei ();

  for (;;) {
    shell_loop ();
  }
}

int
bigaff_str2rtc (const char* str, rtc_datetime_t *rtc)
{
  unsigned int year;
  unsigned int month;
  unsigned int date;

  unsigned int hour;
  unsigned int minute;
  unsigned int second;
  int res;
  if ((res = sscanf_P(str, PSTR("%*s 20%02x-%02x-%02x %02x:%02x:%02x"), &year, &month, &date, &hour, &minute, &second)) > 0) {
    rtc->year = year;
    if(res>1) rtc->month = month; else rtc->month = 0xff;
    if(res>2) rtc->date = date; else rtc->date = 0xff;
    if(res>3) rtc->hours = hour; else rtc->hours = 0xff;
    if(res>4) rtc->minutes = minute; else rtc->minutes = 0xff;
    if(res>5) rtc->seconds = second; else rtc->seconds = 0xff;
  };
  return res;
}

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)
// Memo
void
bigaff_command_memo(const char * args)
{
  unsigned int year;
  unsigned int month;
  unsigned int date;

  memo m;
  char command[4];
  int res = sscanf_P(args, PSTR("%*s %3s 20%02x-%02x-%02x %"STR(MEMO_TEXT_LEN)"[^\n]"), command, &year, &month, &date, m.text);
  if(_debug_mode) printf_P(PSTR("%d element(s) decoded\n"), res);
  if(res<0) { // No argument
    const uint8_t _memo_count = memo_count();
    if (_memo_count) {
      int8_t res = -1;
      while((res = memo_get_next(res, &m)) >= 0) {
        printf_P(PSTR("%d: 20%02x-%02x-%02x \"%s\"\n"), m.id, m.date_year, m.date_month, m.date_date, m.text);
      }
    }
    printf_P(PSTR("%d/"STR(MEMO_SLOTS_MAX)" memo(s) used.\n"), _memo_count);
    return;
  }else{
    if(strcmp_P(command, PSTR("add")) == 0) {
      if(res==5) {
        m.date_year = year;
        m.date_month = month;
        m.date_date = date;
        int res;
        if((res = memo_add (&m)) < 0)
          printf_P(PSTR("no space left!\n"));
        else {
          if (_debug_mode) printf_P(PSTR("id:%d\n"), res);
          printf_P(PSTR("done.\n"));
        }
        return;
      }
    } else if(strcmp_P(command, PSTR("del")) == 0) {
      uint8_t id;
      res = sscanf_P(args, PSTR("%*s del %" PRIu8), &id);
      if(_debug_mode) printf_P(PSTR("%d element(s) decoded\n"), res);
      if(res==1) {
        if((res = memo_del(id)) < 0) {
          printf_P(PSTR("fail!\n"));
        } else {
          printf_P(PSTR("done.\n"));
        }
        return;
      }
    }
  }
  printf_P(PSTR("invalid argument(s)).\n"));
}

// Daylight saving time (DST)
/*
2012    25 mars         28 octobre
2013    31 mars         27 octobre
2014    30 mars         26 octobre
2015    29 mars         25 octobre
2016    27 mars         30 octobre
2017    26 mars         29 octobre
2018    25 mars         28 octobre
2019    31 mars         27 octobre
2020    29 mars         25 octobre
*/
#define DST_SUMMER  0x00
#define DST_WINTER  0x01
void
bigaff_dst_time_correction(rtc_datetime_t rtc)
{
  static const uint8_t _dst_summer[] PROGMEM = {
    0x25, // 2012
    0x31, // 2013
    0x30, // 2014
    0x29, // 2015
    0x27, // 2016
    0x26, // 2017
    0x25, // 2018
    0x31, // 2019
    0x29, // 2020
  };
  static const uint8_t _dst_winter[] PROGMEM = {
    0x28, // 2012
    0x27, // 2013
    0x26, // 2014
    0x25, // 2015
    0x30, // 2016
    0x29, // 2017
    0x28, // 2018
    0x27, // 2019
    0x25, // 2020
  };
  if(rtc.minutes != 0x00) return; // We only need DST correction on new hour
  if((rtc.year<0x12)||(rtc.year>0x20)) return; // We only know from 2012 to 2020 DST
  switch (rtc.month) {
    case 0x03: // March
    {
      if(rtc.hours!=0x02) return; // In March, change occurs at 1h00 GMT -> 2h00 local time
      uint8_t id = (rtc.year&0x0f) + (((rtc.year&0xf0)>>4)*10) - 12;
      if(rtc.date!=pgm_read_byte(_dst_summer + id)) return; // If we are the right day :)
      if(eeprom_read_byte ((uint8_t *)EEPROM_MEMORY_MAP__DST) != DST_SUMMER) {
        rtc.hours=0x03; // Set hours to 3h AM
        drv_ds1307_write(rtc);
        eeprom_write_byte ((uint8_t *)EEPROM_MEMORY_MAP__DST, DST_SUMMER);
      }
    }
    break;
    case 0x10: // October
    {
      if(rtc.hours!=0x03) return; // In October, change occurs at 1h00 GMT -> 3h00 local time
      uint8_t id = (rtc.year&0x0f) + (((rtc.year&0xf0)>>4)*10) - 12;
      if(rtc.date!=pgm_read_byte(_dst_winter + id)) return; // If we are the right day :)
      if(eeprom_read_byte ((uint8_t *)EEPROM_MEMORY_MAP__DST) != DST_WINTER) {
        rtc.hours=0x02; // Set hours to 2h AM
        drv_ds1307_write(rtc);
        eeprom_write_byte ((uint8_t *)EEPROM_MEMORY_MAP__DST, DST_WINTER);
      }
    }
    break;
  }
}

// Debug commands
void
bigaff_command_debug(const char * args)
{
  (void)args;

  if(!_debug_mode) {
    _debug_mode = true;
    printf_P(PSTR("debug mode on\n"));
  } else {
    _debug_mode = false;
    printf_P(PSTR("debug mode off\n"));
  }
}

void
bigaff_command_debug_eeprom_read(const char * args)
{
  (void)args;
  for (uint8_t i=0; i<128; i++) {
    const uint8_t *const eeprom_addr = (const uint8_t *const)(int)i;
    printf_P(PSTR("0x%02x "), eeprom_read_byte(eeprom_addr));
    if ((i%8) == 7) printf_P(PSTR("\n"));
  }
}

void
bigaff_command_debug_freemem(const char *args)
{
  (void)args;
  extern int __heap_start, *__brkval;
  int v;
  printf_P(PSTR("freemem %d bytes\n"), (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval));
}

void
bigaff_command_debug_reset(const char *args)
{
  (void)args;
  printf_P(PSTR("resetting...\n"));
  wdt_enable(WDTO_15MS);
  for(;;) { }
}

// Date Time mode
void
bigaff_command_datetime(const char * args)
{
  rtc_datetime_t rtc;
  int res = bigaff_str2rtc (args, &rtc);
  char datetime[17];
  static const char datetime_fmt[] PROGMEM = "20%02x-%02x-%02x %02x:%02x:%02x";
  
  if (res < 0) {
    // No args then display current rtc datetime
    rtc = drv_ds1307_read();
    sprintf_P(datetime, datetime_fmt, rtc.year, rtc.month, rtc.date, rtc.hours, rtc.minutes, rtc.seconds);
    printf_P(PSTR("%s "), datetime);
    
    switch(eeprom_read_byte ((uint8_t *)EEPROM_MEMORY_MAP__DST)) {
      case DST_SUMMER:
        printf_P(PSTR("(GMT+2)"));
        break;
      case DST_WINTER:
        printf_P(PSTR("(GMT+1)"));
        break;
      default:
        printf_P(PSTR("(GMT+?)"));
        break;
    };
    printf_P(PSTR("\n"));
  } else if (res == 6) {
    drv_ds1307_write(rtc);
    sprintf_P(datetime, datetime_fmt, rtc.year, rtc.month, rtc.date, rtc.hours, rtc.minutes, rtc.seconds);
    printf_P(PSTR("set rtc to %s\n"), datetime);
    if(_debug_mode) {
      rtc = drv_ds1307_read();
      sprintf_P(datetime, datetime_fmt, rtc.year, rtc.month, rtc.date, rtc.hours, rtc.minutes, rtc.seconds);
      printf_P(PSTR("verify rtc: %s\n"), datetime);
    }
  } else {
    printf_P(PSTR("invalid argument (%d)\n"), res);
  }
}

// Message mode
static volatile char _bigaff_message_text[MESSAGE_TEXT_LEN+1] = "no current message";

void
bigaff_command_message(const char * args)
{
  char s[MESSAGE_TEXT_LEN+1];
  uint8_t d;
  int res;
  if ((res = sscanf_P(args, PSTR("%*s %" PRIu8 " %"STR(MESSAGE_TEXT_LEN)"[^\n]"), &d, s)) > 0) {
//     if(_debug_mode) printf_P(PSTR("res=%d,d=%d\n"), res, d);
    char* ps = NULL;
    if (res == 2) {
      if(_debug_mode) printf_P(PSTR("s: %s\n"), s);
      ps = s;
    }
    bigaff_message (d, ps);
    printf_P(PSTR("done.\n"));
  } else {
    printf_P(PSTR("current: %s\n"), _bigaff_message_text);
  }
}

void
bigaff_message(const uint8_t seconds, const char * text)
{
  _bigaff_mode = BIGAFF_MODE_MESSAGE;
  if (text != NULL) strcpy((char*)_bigaff_message_text, text);
  if((strlen((const char*)_bigaff_message_text) * 6) <= 95) {
    framebuffer_display_string_center((const char*)_bigaff_message_text);
    framebuffer_swap();
    _bigaff_sequence = 0;
  } else {
    _bigaff_sequence = 1; // Start animation
  }
  _bigaff_message_seconds_left = seconds;
}

// Intensity command
void
bigaff_command_intensity(const char * args)
{
  uint8_t d;
  if (sscanf_P(args, PSTR("%*s %" PRIu8), &d) > 0) {
    led_panel_set_intensity (d);
    eeprom_update_byte((uint8_t*)EEPROM_MEMORY_MAP__INTENSITY, d);
    printf_P(PSTR("done.\n"));
  } else {
    printf_P(PSTR("%" PRIu8 "\n"), led_panel_get_intensity ());
  }
}

// Help command
void
bigaff_command_help(const char * args)
{
  (void)args;
  printf_P(PSTR("supported commands:\n"));
  for(size_t n = 0; n < SHELL_COMMAND_COUNT; n++) {
    if ((_debug_mode) || (shell_commands[n].debug == false)) {
      printf_P(PSTR("  %S - %S\n"), shell_commands[n].text, shell_commands[n].description);
    }
  }
}

uint16_t
bigaff_animate(uint16_t sequence, const char * text)
{
  char * s = (char*)text;
  if(sequence <= 95) {
    framebuffer_blank_column(96 - sequence);
  } else {
    s += (sequence - 96) / 6;
    framebuffer_display_partial_char( s[0], (sequence - 96) % 6 );
    s++;
  }
  framebuffer_display_line(s);
  framebuffer_swap();

  sequence++;
  if ( sequence > ((strlen(text) * 6) + 95)) {
    sequence = 1;
  }
  return sequence;
}

// Processing
void bigaff_process(void);
// Timer0 is lanched in main() at each timer interrupt (comparator):
ISR (SIG_OUTPUT_COMPARE0)
{
  // Logical scaler (div by 4)
  static uint8_t downcounter = 4;
  downcounter--;
  if( downcounter == 0 ) {
    bigaff_process();
    downcounter = 4;
  }
}

static int8_t _bigaff_memo_id = -1;

void
bigaff_process(void)
{
  // Logical scaler but used at different time
  static uint8_t downcounter = 16;
  downcounter--;

  static uint8_t _bigaff_date_minutes_current = 0xff;
  static uint8_t _bigaff_date_date_current = 0xff;
  switch(_bigaff_mode) {
    case BIGAFF_MODE_WAIT:
    {
      if(downcounter == 0) {
        const rtc_datetime_t rtc = drv_ds1307_read();
        if (rtc.minutes != _bigaff_date_minutes_current) {
          _bigaff_date_minutes_current = rtc.minutes;
          // Each minute
          bigaff_dst_time_correction(rtc); // Check if we need to change DST

          memo m;
          if(rtc.date != _bigaff_date_date_current) {
            // Each day
            _bigaff_memo_id = -1; // Current memo is not valid anymore
            // Delete old memo(s)
            int8_t res = -1;
            while ((res = memo_get_next_at(res, rtc.year, rtc.month, _bigaff_date_date_current, &m)) >= 0) {
              memo_del(res);
            }
            _bigaff_date_date_current = rtc.date;
         }
          int8_t res;
          if ((res = memo_get_next_at (_bigaff_memo_id, rtc.year, rtc.month, rtc.date, &m)) >= 0) {
//             if(_debug_mode) printf_P(PSTR("id:%d\n"), res);
            _bigaff_memo_id = res;
            _bigaff_message_seconds_left = 30;
            strcpy((char*)_bigaff_message_text, m.text);
            _bigaff_sequence = 1; // Animated text
            _bigaff_mode = BIGAFF_MODE_MESSAGE;
            downcounter++;
          } else {
            _bigaff_memo_id = -1;
//             if(_debug_mode) printf_P(PSTR("res:%d\n"), res);
            _bigaff_mode = BIGAFF_MODE_DATETIME;
            downcounter++;
          }
        }
      }
    }
    break;
    case BIGAFF_MODE_DATETIME:
    {
      char datetime[17];
      // f(bigaff_process) / 16
      if(downcounter == 0) {
        const rtc_datetime_t rtc = drv_ds1307_read();
        sprintf_P(datetime, PSTR("20%02x-%02x-%02x %02xh%02x"), rtc.year, rtc.month, rtc.date, rtc.hours, rtc.minutes);
        framebuffer_display_string_center(datetime);
        framebuffer_swap();
        _bigaff_mode = BIGAFF_MODE_WAIT;
      }
    }
    break;
    case BIGAFF_MODE_MESSAGE:
      if ( _bigaff_sequence != 0 ) {
        // _bigaff_sequence == 0 means unanimated text
        if(downcounter % 2 == 0) {
          _bigaff_sequence = bigaff_animate(_bigaff_sequence, (const char*)_bigaff_message_text);
          if((_bigaff_sequence == 1)&&(_bigaff_message_seconds_left < 25)) {
            _bigaff_mode = BIGAFF_MODE_DATETIME;
          }
        }
      }
      if(downcounter == 0) {
        if(_bigaff_message_seconds_left > 0) _bigaff_message_seconds_left--;
        if ((_bigaff_sequence == 0)&&(_bigaff_message_seconds_left == 0)) {
          _bigaff_mode = BIGAFF_MODE_DATETIME;
        }
      }
      break;
  }
  if( downcounter == 0 ) {
    downcounter = 16;
  }
}
