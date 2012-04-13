#include "timer0.h"

#include <avr/io.h>

void
timer0_init (void)
{
  TCCR0 = 0b00001101;         // clk = 16Mhz, prescal / 1024, Normal mode
  TCNT0 = 0x00;
  OCR0 = 0xFF;                // (16 000 000 / 1024) / 255 = 61,2745098039 hz => 16,32 ms

  TIMSK |= 0x02;              // Timer/counter0 output compare interrupt enabled
} 

// void
// timer0_microsecond (const uint16_t us)
// {
//   //      clock select /8         clk = 16Mhz prescal = 8 donc resolution de 0,5 us
//   //              MICROSECOND / 0,5 us = VALUE OCR1A ou MICROSECOND x 2 = VALUE OCR1A
//   TCNT1 = 0x0000;
//   OCR1A = us * 2;              //              Donc au min 1 us et au max 32768 us ou 32,768ms
// } 
