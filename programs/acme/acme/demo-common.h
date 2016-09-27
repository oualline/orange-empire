#ifndef __DEMO_COMMON_H__
#define __DEMO_COMMON_H__
// Different for each demo
extern void do_demo(void);

extern const char* const DEMO_NAME;

// Common functions
extern void image(const char* const image);
extern void say(const char* const words);
extern void stop_say(void);
#endif // __DEMO_COMMON_H__

