#include "timer1.h"

#include <avr/io.h>

void
timer1_init (void) 
{
  TCCR1A = 0x00;
  TCCR1B = 0x0A;                //      clock select /8         clk = 16Mhz prescal = 8 donc resolution de 0,5 us
                                //              MICROSECOND / 0,5 us = VALUE OCR1A ou MICROSECOND x 2 = VALUE OCR1A
  TCNT1 = 0x0000;
  OCR1A = 2 * 2;               //              Donc au min 1 us et au max 32768 us ou 32,768ms

  TIMSK = TIMSK | 0x10;         //              Timer/counter1 output compare A interrupt enable
} 

void
timer1_microsecond (const uint16_t us) 
{
  //      clock select /8         clk = 16Mhz prescal = 8 donc resolution de 0,5 us
  //              MICROSECOND / 0,5 us = VALUE OCR1A ou MICROSECOND x 2 = VALUE OCR1A
  TCNT1 = 0x0000;
  OCR1A = us * 2;              //              Donc au min 1 us et au max 32768 us ou 32,768ms
} 
