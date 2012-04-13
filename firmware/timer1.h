#ifndef __TIMER1_H__
#define __TIMER1_H__

#include <stdint.h>

// Fonctions qui concernent les manipulations hardware (ceci est le driver)
void    timer1_init (void);

void   timer1_microsecond (const uint16_t us);

#endif // __TIMER1_H__
