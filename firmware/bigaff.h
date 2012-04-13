#ifndef __BIGAFF_H__
#define __BIGAFF_H__

#include <stdint.h>
#include <stdbool.h>
extern volatile bool _debug_mode;

void bigaff_message(const uint8_t seconds, const char * text);

void bigaff_command_help(const char * args);
void bigaff_command_datetime(const char * args);
void bigaff_command_memo(const char * args);
void bigaff_command_message(const char * args);
void bigaff_command_intensity(const char * args);

uint16_t bigaff_animate(uint16_t sequence, const char * text);

void bigaff_command_debug(const char * args);
void bigaff_command_debug_eeprom_read(const char * args);
void bigaff_command_debug_freemem(const char *args);
void bigaff_command_debug_reset(const char *args);

#endif // __BIGAFF_H__