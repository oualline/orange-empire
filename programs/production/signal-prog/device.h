#ifndef __DEVICE_H__
#define __DEVICE_H__

// Device name of the wireless keypad 
static const char* const KEYPAD = 
    "/dev/input/by-id/usb-G-Tech_CHINA_USB_Wireless_Mouse___Keypad_V1.02-event-kbd";

// Device name of the prokey 55 reader
static const char* const PROKEY55 = 
    "/dev/input/by-id/usb-PoLabs_PoKeys56U_2.34126-if02-event-kbd";

// Socket from input handler to garden
static const char* const INPUT_PIPE = "/tmp/garden.input";

static const char* const AVR_KBD = "/dev/input/by-id/usb-MfgName_Keyboard-event-kbd";

#endif // __DEVICE_H__
