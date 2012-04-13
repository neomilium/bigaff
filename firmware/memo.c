#include "memo.h"

#include <stdbool.h>
#include <string.h>

#include <avr/eeprom.h>

static uint8_t _memo_slot_flags[MEMO_SLOTS_MAX];

#define _MEMO_SLOT_FLAG_VALID 0xff
#define _MEMO_SLOT_FLAG_INVALID 0x00

bool memo_is_valid(const memo* m);
uint8_t memo_crc(const memo *const m);

void memo_read(const uint8_t id, memo *const m);
void memo_write(const uint8_t id, memo *const m);

void
memo_init(void)
{
  for (uint8_t i=0; i<MEMO_SLOTS_MAX; i++) {
    memo m;
    memo_read(i,&m);
    _memo_slot_flags[i] = memo_is_valid(&m)?_MEMO_SLOT_FLAG_VALID:_MEMO_SLOT_FLAG_INVALID;
  }
}

int8_t
memo_add(memo *const m)
{
  uint8_t i;
  for (i=0; i<MEMO_SLOTS_MAX; i++) {
    if(_memo_slot_flags[i]==_MEMO_SLOT_FLAG_INVALID) {
      break;
    }
  }
  if(i<MEMO_SLOTS_MAX) {
    memo_write(i,m);
    _memo_slot_flags[i] = _MEMO_SLOT_FLAG_VALID;
  } else return -1;
  return i;
}

int8_t memo_del(const uint8_t id)
{
  if(_memo_slot_flags[id]==_MEMO_SLOT_FLAG_VALID) {
    uint8_t *eeprom_addr = (uint8_t *)MEMO_EEPROM_START_ADDR + id * sizeof(memo);
    uint8_t crc = eeprom_read_byte (eeprom_addr);
    eeprom_write_byte (eeprom_addr, crc+1); // We waste CRC to set slot as invalid
    _memo_slot_flags[id] = _MEMO_SLOT_FLAG_INVALID;
  } else {
    return -1;
  }
  return id;
}

uint8_t
memo_count(void)
{
  uint8_t count=0;
  for (uint8_t i=0; i<MEMO_SLOTS_MAX; i++) {
    if(_memo_slot_flags[i]==_MEMO_SLOT_FLAG_VALID) {
      count++;
    }
  }
  return count;
}

int8_t
memo_get_next (const int8_t previous_id, memo *const m)
{
  for(int8_t id=previous_id+1; id<MEMO_SLOTS_MAX; id++) {
    if(_memo_slot_flags[id]==_MEMO_SLOT_FLAG_VALID) {
      memo_read(id,m);
      m->id = id;
      return id;
    }
  }
  return -1;
}

int8_t
memo_get_next_at(const int8_t previous_id, const uint8_t year, const uint8_t month, const uint8_t date, memo *const m)
{
  int8_t res = previous_id;
  memo _m;

  for (;;) {
    if((res = memo_get_next(res, &_m)) < 0) {
      return res;
    }
    if((_m.date_year == year)&&(_m.date_month==month)&&(_m.date_date==date)) {
      memcpy(m, &_m, sizeof(memo));
      return res;
    }
  }
  return -1;
}

uint8_t
memo_crc(const memo *const m)
{
  uint8_t *_p_crc = (uint8_t*)m;
  uint8_t *_p_end = _p_crc + sizeof(memo);
  uint8_t crc = 0x55;

  for(_p_crc+=1;_p_crc<_p_end;_p_crc++) {
    crc ^= *_p_crc;
  }
  return crc;
}

bool
memo_is_valid(const memo *const m)
{
  return (memo_crc(m) == m->id);
}

void
memo_read(const uint8_t id, memo *const m)
{
  const uint8_t *eeprom_addr = (const uint8_t *)MEMO_EEPROM_START_ADDR + id * sizeof(memo);
  eeprom_read_block((void*)m, eeprom_addr, sizeof(memo));
}

void
memo_write(const uint8_t id, memo *const m)
{
  m->id = memo_crc(m);
  const uint8_t *eeprom_addr = (const uint8_t *)MEMO_EEPROM_START_ADDR + id * sizeof(memo);
  eeprom_update_block((const void*)m, (void*)eeprom_addr, sizeof(memo));
}
