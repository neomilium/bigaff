#ifndef _DS1307_H_
#define _DS1307_H_

#include <stdint.h>
#include "rtc.h"

void		drv_ds1307_init(void);
rtc_datetime_t	drv_ds1307_read(void);
void		drv_ds1307_write(const rtc_datetime_t rtc_datetime);

/* void ds1307_write_data( X, Y ) */
void		drv_ds1307_start(void);
void		drv_ds1307_stop(void);
/* ds1307_read_data( X ) */
/* ds1307_read_datetime() */


#endif
