#ifndef __RTC_H__
#define __RTC_H__

typedef uint8_t bcd_byte;

typedef struct {
	bcd_byte	seconds;
	bcd_byte	minutes;
	bcd_byte	hours;
	bcd_byte	day;
	bcd_byte	date;
	bcd_byte	month;
	bcd_byte	year;
} rtc_datetime_t;

#include "drv_ds1307.h"

#define rtc_init() drv_ds1307_init()
#define rtc_write( X ) drv_ds1307_write( X )
#define rtc_read() drv_ds1307_read()
#define rtc_start() drv_ds1307_start()
#define rtc_stop() drv_ds1307_stop()


#endif
