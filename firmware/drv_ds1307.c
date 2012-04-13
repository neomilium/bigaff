#include "drv_ds1307.h"

#include "rtc.h"

#include <stdio.h>

#include "twi.h"
#define DS1307_I2C_ADDR   0xD0

void
drv_ds1307_init(void)
{
  uint8_t 		data      [2];

  data[0] = 0x07; // Register address in DS1307
  data[1] = 0x00;

  /* Clear OUT, SQWE, RS1, RS0 */
  if(2 != twi_write_bytes(DS1307_I2C_ADDR, 2, (uint8_t *)&data)) {
    printf("DS1307 error\n");
  }
}

void
drv_ds1307_start(void)
{
	uint8_t 		data      [2];
	data[0] = 0x00;
	/* Timekeeper Registers address */
	twi_write_bytes(DS1307_I2C_ADDR, 1, &data[0]);
	twi_read_bytes(DS1307_I2C_ADDR, 1, &data[1]);
	data[1] &= 0x7F;
	/* Clear CH (enable clock generator) */
	twi_write_bytes(DS1307_I2C_ADDR, 2, (uint8_t *)&data);
}

void
drv_ds1307_stop(void)
{
	uint8_t 		data      [2];
	data[0] = 0x00;
	/* Timekeeper Registers address */
	twi_write_bytes(DS1307_I2C_ADDR, 1, &data[0]);
	twi_read_bytes(DS1307_I2C_ADDR, 1, &data[1]);
	data[1] |= 0x80;
	/* Set CH (disable clock generator) */
	twi_write_bytes(DS1307_I2C_ADDR, 2, (uint8_t *)&data);
}

rtc_datetime_t
drv_ds1307_read(void)
{
	uint8_t 		ds1307_addr[] = {0x00};
	twi_write_bytes(DS1307_I2C_ADDR, 1, ds1307_addr);
	rtc_datetime_t	data;
	twi_read_bytes(DS1307_I2C_ADDR, sizeof(rtc_datetime_t), (uint8_t *) (&data));
	data.seconds &= 0x7F;
	data.hours &= 0x3F;
	return data;
}

void
drv_ds1307_write(const rtc_datetime_t rtc_datetime)
{
  struct {
    uint8_t address;
    rtc_datetime_t datetime;
  } data;

  data.address = 0x00;
  data.datetime = rtc_datetime;

  /* Keep CH (clock enable) bit as it set */
  uint8_t ch;
  /* Timekeeper Registers address */
  uint8_t ds1307_addr[] = { 0x00 };
  twi_write_bytes(DS1307_I2C_ADDR, 1, ds1307_addr);
  twi_read_bytes(DS1307_I2C_ADDR, 1, &ch);
  ch &= 0x80;
  data.datetime.seconds |= ch;

  twi_write_bytes(DS1307_I2C_ADDR, sizeof(data), (uint8_t *)&data);
}
