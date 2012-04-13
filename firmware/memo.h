#ifndef __MEMO_H__
#define __MEMO_H__

#include "config.h"

#include <stdint.h>

#ifndef MEMO_SLOTS_MAX
#error MEMO_SLOTS_MAX is not defined, please define slot array size
#endif
#ifndef MEMO_EEPROM_START_ADDR
#error MEMO_EEPROM_START_ADDR is not defined, please define start address in eeprom to store memos
#endif
#ifndef MEMO_TEXT_LEN
#error MEMO_TEXT_LEN is not defined, please max characters count in memos text
#endif

typedef struct {
  uint8_t id;
  uint8_t date_year;
  uint8_t date_month;
  uint8_t date_date;
  char text[MEMO_TEXT_LEN+1];
} memo;

void memo_init(void);
int8_t memo_add(memo *const m);
int8_t memo_del(const uint8_t id);

uint8_t memo_count(void);
int8_t memo_get_next (const int8_t previous_id, memo *const m);
int8_t memo_get_next_at(const int8_t previous_id, const uint8_t year, const uint8_t month, const uint8_t, memo *const m);

#endif