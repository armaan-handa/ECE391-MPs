#ifndef _RTC_H
#define _RTC_H
#endif

#include "types.h"
#define RTC_PORT	0x70
#define CMOS_PORT	0x71
#define RTC_PORTA	0x8A
#define RTC_PORTB	0x8B
#define RTC_PORTC	0xC
#define RTC_IRQ_NUM 8

/* FUNctions */
extern void init_rtc();
extern void cp1_rtc_handler();
extern void rtc_handler();
extern int32_t open_rtc(const uint8_t* filename);
extern int32_t close_rtc(uint32_t fd);
extern int32_t write_rtc(uint32_t fd, const void* buf, uint32_t nbytes);
extern int32_t read_rtc(uint32_t fd, void* buf, uint32_t nbytes);
