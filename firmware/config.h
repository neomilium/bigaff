#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "version.h"

// eeprom memory map
#define EEPROM_MEMORY_MAP__INTENSITY  1   // 1 byte
#define EEPROM_MEMORY_MAP__DST        2   // 1 byte
#define EEPROM_MEMORY_MAP__MEMO       8   // MEMO_SLOTS_MAX * sizeof(memo)

// memo related config
#define MEMO_EEPROM_START_ADDR        EEPROM_MEMORY_MAP__MEMO
#define MEMO_SLOTS_MAX                20
#define MEMO_TEXT_LEN                 40

// message
#define MESSAGE_TEXT_LEN          80

#define SHELL_COMMAND_COUNT       9

#endif