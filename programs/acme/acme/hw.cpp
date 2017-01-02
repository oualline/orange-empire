#include <iostream>
#include "conf.h"
#include "hw.h"

// Map relays for Head 1
struct head_map h1_map = {
    H1_RED,
    H1_GREEN,
    H1_YELLOW,

    H1_MOTOR_DIR,
    H1_MOTOR_POWER,
    H1_FOLD,

    H1_BELL
};

// Map relays for head 2
struct head_map h2_map = {
    H2_RED,
    H2_GREEN,
    H2_YELLOW,

    H2_MOTOR_DIR,
    H2_MOTOR_POWER,
    H2_FOLD,

    H2_BELL
};

/********************************************************
 * fold the arms of the signal
 ********************************************************/
void head::fold_arms(void)
{
    int sleep_time = acme_config.get_arm_time() * 2;
    // ### todo -- make a config item
    {
	switch (arm_state)
	{
	    case ARM_STATE::ARM_NONE: 
		break;
	    case ARM_STATE::ARM_STOP:
		relay("manual", head_info.fold, RELAY_STATE::RELAY_ON);
		go_arms(sleep_time);
		relay("manual", head_info.fold, RELAY_STATE::RELAY_OFF);
		arm_state = ARM_STATE::ARM_NONE;
		break;
	    case ARM_STATE::ARM_GO:
		relay("manual", head_info.fold, RELAY_STATE::RELAY_ON);
		stop_arms(sleep_time);
		relay("manual", head_info.fold, RELAY_STATE::RELAY_OFF);
		arm_state = ARM_STATE::ARM_NONE;
		break;
	    default:
		std::cerr << "Internal error.  Illegal arm_state\r" << std::endl;
		abort();
	}
    }
}

/********************************************************
 * Make the signal show stop
 *
 * Parameters
 * 	how -- Do we use light, arms, or both
 * 	enabled -- True if the arms are enabled
 *********************************************************/
void head::stop(SIGNAL_HOW how, const bool enabled)
{
    if (how == SIGNAL_HOW::AS_CONF)
    {
	if (enabled)
	    how = SIGNAL_HOW::ARMS_AND_LIGHTS;
	else
	    how = SIGNAL_HOW::LIGHTS_ONLY;
    }

    switch (how)
    {
	case SIGNAL_HOW::ARMS_AND_LIGHTS:
	    relay("manual", head_info.green_light, RELAY_STATE::RELAY_OFF);
	    relay("manual", head_info.red_light, RELAY_STATE::RELAY_ON);
	    stop_arms();
	    break;
	case SIGNAL_HOW::ARMS_ONLY:
	    stop_arms();
	    break;
	case SIGNAL_HOW::LIGHTS_ONLY:
	    relay("manual", head_info.green_light, RELAY_STATE::RELAY_OFF);
	    relay("manual", head_info.red_light, RELAY_STATE::RELAY_ON);
	    break;
	default:
	    die("Internal error: Bad how");
    }
}

/********************************************************
 * Make the signal show go
 *
 * Parameters
 * 	how -- Do we use light, arms, or both
 * 	enabled -- True if the arms are enabled
 *********************************************************/
void head::go(enum SIGNAL_HOW how, const bool enabled)
{
    if (how == SIGNAL_HOW::AS_CONF)
    {
	if (enabled)
	    how = SIGNAL_HOW::ARMS_AND_LIGHTS;
	else
	    how = SIGNAL_HOW::LIGHTS_ONLY;
    }
    switch (how)
    {
	case SIGNAL_HOW::ARMS_AND_LIGHTS:
	    relay("manual", head_info.red_light, RELAY_STATE::RELAY_OFF);
	    relay("manual", head_info.green_light, RELAY_STATE::RELAY_ON);
	    go_arms();
	    break;
	case SIGNAL_HOW::ARMS_ONLY:
	    go_arms();
	    break;
	case SIGNAL_HOW::LIGHTS_ONLY:
	    relay("manual", head_info.red_light, RELAY_STATE::RELAY_OFF);
	    relay("manual", head_info.green_light, RELAY_STATE::RELAY_ON);
	    break;
	default:
	    die("Internal error: Bad how");
    }
}
/********************************************************
 * Turn the lights off
 ********************************************************/
void head::lights_off(void)
{
    relay("manual", head_info.red_light, RELAY_STATE::RELAY_OFF);
    relay("manual", head_info.green_light, RELAY_STATE::RELAY_OFF);
}

class head h1(h1_map);	// Head one controller
class head h2(h2_map);	// Head two controller

/********************************************************
 * Ring both bells and flash both yellow lights
 ********************************************************/
void ding_and_flash_both(void)
{
    relay("manual", h1_map.bell, RELAY_STATE::RELAY_ON);
    relay("manual", h2_map.bell, RELAY_STATE::RELAY_ON);
    usleep(750);
    relay("manual", h1_map.bell, RELAY_STATE::RELAY_OFF);
    relay("manual", h2_map.bell, RELAY_STATE::RELAY_OFF);

    relay("manual", h1_map.yellow_light, RELAY_STATE::RELAY_ON);
    relay("manual", h2_map.yellow_light, RELAY_STATE::RELAY_ON);

    sleep_10(15);

    relay("manual", h1_map.yellow_light, RELAY_STATE::RELAY_OFF);
    relay("manual", h2_map.yellow_light, RELAY_STATE::RELAY_OFF);
}

/********************************************************
 * Ring both bells 
 ********************************************************/
void ding_both(void)
{
    relay("manual", h1_map.bell, RELAY_STATE::RELAY_ON);
    relay("manual", h2_map.bell, RELAY_STATE::RELAY_ON);

    usleep(750);

    relay("manual", h1_map.bell, RELAY_STATE::RELAY_OFF);
    relay("manual", h2_map.bell, RELAY_STATE::RELAY_OFF);
}
/********************************************************
 * Flash both yellows
 ********************************************************/
void flash_both(void)
{
    relay("manual", h1_map.yellow_light, RELAY_STATE::RELAY_ON);
    relay("manual", h2_map.yellow_light, RELAY_STATE::RELAY_ON);

    sleep_10(25);

    relay("manual", h1_map.yellow_light, RELAY_STATE::RELAY_OFF);
    relay("manual", h2_map.yellow_light, RELAY_STATE::RELAY_OFF);
}

