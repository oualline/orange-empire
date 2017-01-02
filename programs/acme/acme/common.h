/********************************************************
 * Handle the hardware layer of the signal 
 ********************************************************/
#ifndef __COMMON_H__
#define __COMMON_H__

#include <unistd.h>

#include "relay.h"
#include "conf.h"

extern void die(const char* const msg) __attribute__((noreturn));
extern void sleep_10(unsigned int sleep_time);

extern void tv_on(void);
extern void tv_off(void);

extern void do_system(const char* const command);
#endif // __COMMON_H__
