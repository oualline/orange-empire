/********************************************************
 * Handle the hardware layer of the signal 
 ********************************************************/
#ifndef __HW_H__
#define __HW_H__

#include <unistd.h>

#include "relay.h"
#include "conf.h"
#include "common.h"

// Turn relays into something we can use for common code
struct head_map {
    RELAY_NAME red_light;
    RELAY_NAME green_light;
    RELAY_NAME yellow_light;

    RELAY_NAME direction;
    RELAY_NAME motor;
    RELAY_NAME fold;

    RELAY_NAME bell;
};

// Map relays for Head 1
extern struct head_map h1_map;

// Map relays for head 2
extern struct head_map h2_map;


// Class for the manipulation of a head
class head {
    private:
	enum class ARM_STATE {ARM_NONE, ARM_STOP, ARM_GO};

	ARM_STATE arm_state;		// Where's our arm
	const head_map& head_info;	// How do we map things

    public:
	// How do we change the stop/go aspect of the signal
	enum class SIGNAL_HOW {ARMS_ONLY, LIGHTS_ONLY, ARMS_AND_LIGHTS, AS_CONF};
    public:
	head(const struct head_map& _head_info): arm_state(ARM_STATE::ARM_NONE), head_info(_head_info) {};
	head(const head&) = default;
	head& operator = (const head&) = default;
	~head(void)
	{
	    fold_arms();
	}
    private:
	void stop_arms(int sleep_time = -1) {
	    if (arm_state != ARM_STATE::ARM_STOP)
	    {
		if (sleep_time < 0)
		    sleep_time = acme_config.get_arm_time();

		sleep_10(acme_config.get_arm_time());
		relay("manual", head_info.direction, RELAY_STATE::RELAY_OFF);
		relay("manual", head_info.motor, RELAY_STATE::RELAY_ON);
		sleep_10(sleep_time);
		relay("manual", head_info.motor, RELAY_STATE::RELAY_OFF);
		arm_state = ARM_STATE::ARM_STOP;
	    }
	}
	void go_arms(int sleep_time = -1) {
	    if (arm_state != ARM_STATE::ARM_GO) {
		if (sleep_time < 0)
		    sleep_time = acme_config.get_arm_time();

		relay("manual", head_info.direction, RELAY_STATE::RELAY_ON);
		relay("manual", head_info.motor, RELAY_STATE::RELAY_ON);
		sleep_10(sleep_time);
		relay("manual", head_info.motor, RELAY_STATE::RELAY_OFF);
		relay("manual", head_info.direction, RELAY_STATE::RELAY_OFF);
		arm_state = ARM_STATE::ARM_GO;
	    }
	}
    public:
	void fold_arms(void);
	void stop(SIGNAL_HOW how, const bool enabled);
	void go(SIGNAL_HOW how, const bool enabled);
	void lights_off(void);
	void bell()
	{
	    relay("manual", head_info.bell, RELAY_STATE::RELAY_ON);
	    sleep_10(100);
	    relay("manual", head_info.bell, RELAY_STATE::RELAY_OFF);
	}
	bool is_go(void) {
	    return (arm_state == ARM_STATE::ARM_GO);
	}
};

extern class head h1;	// Head one controller
extern class head h2;	// Head two controller

/********************************************************
 * Ring both bells and flash both yellow lights
 ********************************************************/
extern void ding_and_flash_both(void);
extern void ding_both(void);
extern void flash_both(void);
#endif // __HW_H__
