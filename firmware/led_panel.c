#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <util/delay.h>

#include "bit_field.h"

#include "timer1.h"

#define LED_PANEL_DDR         DDRA
#define LED_PANEL_RST 				GET_BIT(PORTA).bit5
#define LED_PANEL_CLK 				GET_BIT(PORTA).bit4
#define LED_PANEL_SERIAL_1   			GET_BIT(PORTA).bit6
#define LED_PANEL_SERIAL_2   			GET_BIT(PORTA).bit7


// #define LED_PANEL_DATA_A                       GET_BIT(PORTA).bit0
// #define LED_PANEL_DATA_B                       GET_BIT(PORTA).bit1
// #define LED_PANEL_DATA_C                       GET_BIT(PORTA).bit2
// #define LED_PANEL_DATA_D                       GET_BIT(PORTA).bit3

void    led_panel_display_point (void);
void    led_panel_lines_select (uint8_t line);
static volatile uint8_t _led_panel_pass = 0;
static volatile uint8_t _led_panel_current_line = 0;
volatile uint8_t _led_panel_intensity = 100;

volatile uint8_t *_led_panel_framebuffer;
static volatile uint8_t* (*_framebuffer_get_display) (void);

#define LED_PANEL_REG			PORTA
#define LED_PANEL_LINES_MASK	0x0F

#define LED_PANEL_COLUMNS   95

typedef enum {
  LED_PANEL_MODE_SEND,
  LED_PANEL_MODE_LIGHT,
  LED_PANEL_MODE_RESET
} led_panel_mode_t;

#define LED_PANEL_FREQ_RATIO 7

/**
 * Interruption: Se déclenche à n ms, sachant que n dépends de la value chargée dans le timer1
 * @see timer1_microsecond()
 */
ISR (SIG_OUTPUT_COMPARE1A)
{
//      cli();
  TIMSK = TIMSK & ~(0x10);      //              Timer/counter1 output compare A interrupt disabled
  static led_panel_mode_t _led_panel_mode = LED_PANEL_MODE_RESET;

  switch (_led_panel_mode) {
  case LED_PANEL_MODE_SEND:
    LED_PANEL_RST = 0;            // Desactive le RESET des bascules D
    if (LED_PANEL_CLK) {
      // On fabrique la donn�e � �crire sur les lignes de donn�es
      led_panel_lines_select (8);       // On �teind les ENABLE des lignes visibles
//                              timer1_microsecond(30000);
/*
				if( _led_panel_pass % 2 )
				{
					LED_PANEL_SERIAL_1 = 1;
					LED_PANEL_SERIAL_2 = 1;
				} else {
					LED_PANEL_SERIAL_1 = 0;
					LED_PANEL_SERIAL_2 = 0;
				}
*/
      led_panel_display_point ();
      timer1_microsecond (10);

      LED_PANEL_CLK = 0;
    } else {
      _led_panel_pass++;
      if (_led_panel_pass > 48) {
        _led_panel_mode = LED_PANEL_MODE_LIGHT;
      }
      timer1_microsecond (10);
      LED_PANEL_CLK = 1;
    }
    break;
  case LED_PANEL_MODE_LIGHT:
    // On selectionne la ligne
    led_panel_lines_select (_led_panel_current_line);

    _led_panel_current_line++;
    if (_led_panel_current_line > 7) {
      _led_panel_current_line = 0;
    }
    timer1_microsecond (100 + (_led_panel_intensity * LED_PANEL_FREQ_RATIO));
    _led_panel_mode = LED_PANEL_MODE_RESET;
    break;

  case LED_PANEL_MODE_RESET:
    LED_PANEL_RST = 1;            // Vide le contenu des bascules D
    _led_panel_pass = 0;
    timer1_microsecond (100 + ((255 * LED_PANEL_FREQ_RATIO) - (_led_panel_intensity * LED_PANEL_FREQ_RATIO)));
    _led_panel_mode = LED_PANEL_MODE_SEND;
    if(_led_panel_current_line == 0) _led_panel_framebuffer = (_framebuffer_get_display)();
    break;
  }
//      sei();
  TIMSK = TIMSK | 0x10;         //              Timer/counter1 output compare A interrupt enable
}

void
led_panel_set_intensity (const uint8_t intensity)
{
  _led_panel_intensity = intensity;
}

uint8_t
led_panel_get_intensity (void)
{
  return _led_panel_intensity;
}

void
led_panel_display_point (void)
{
//      const char ascii = 'A';

//      uint8_t data[5];
//      memcpy_P(data, ascii_table[ascii - 32], 5);
  uint8_t framebuffer_col = 0;
  uint8_t led_panel_current_col = 48 - _led_panel_pass;
  if (led_panel_current_col < 23) {
    framebuffer_col = (44 - (2 * led_panel_current_col)) - 1;
  } else if (led_panel_current_col < 25) {
    framebuffer_col = (47 - (led_panel_current_col - 23)) - 1;
  } else if (led_panel_current_col < 48) {
    framebuffer_col = (led_panel_current_col - 25) * 2;
  }

  if (_led_panel_framebuffer[framebuffer_col] & (1 << (_led_panel_current_line))) {
    LED_PANEL_SERIAL_1 = 1;
  } else {
    LED_PANEL_SERIAL_1 = 0;
  }
//      if (_led_panel_current_line == 6) LED_PANEL_SERIAL_1 = 1;

  if (led_panel_current_col < 24) {
    framebuffer_col = (49 + (2 * led_panel_current_col)) - 1;
  } else if (led_panel_current_col < 49) {
    framebuffer_col = (94 - (2 * (led_panel_current_col - 24))) - 1;
  }

  if (_led_panel_framebuffer[framebuffer_col] & (1 << (_led_panel_current_line))) {
    LED_PANEL_SERIAL_2 = 1;
  } else {
    LED_PANEL_SERIAL_2 = 0;
  }
/*

	if(led_panel_current_col == 1) {
		LED_PANEL_SERIAL_2 = 1;
	} else {
		LED_PANEL_SERIAL_2 = 0;
	}
*/
}

void
led_panel_lines_select (uint8_t line)
{
  register_set (LED_PANEL_REG, line, LED_PANEL_LINES_MASK);
}


/* Fonction d'initialisation de l'afficheur */
void
led_panel_init (volatile uint8_t* (*framebuffer_get_display) (void))
{
  LED_PANEL_DDR = 0xFF;                  //met le port en sortie
  LED_PANEL_RST = 1;
  _delay_ms (2);
  LED_PANEL_RST = 0;

  LED_PANEL_SERIAL_1 = 0;
  LED_PANEL_SERIAL_2 = 0;

  timer1_init ();
  timer1_microsecond (500);
  _framebuffer_get_display = framebuffer_get_display;
}
