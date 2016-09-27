#ifndef __RELAY__H__
#define __RELAY__H__

// Except thrown when an error occurs
class relay_error {
    public:
	const char* error;	// Error message
    public:
	relay_error(const char* const _error):error(_error) {};
	// Copy constructor defaults
	// Destructor defaults
	// Assignment operator defaults
};

// On/off state
enum RELAY_STATE {RELAY_OFF, RELAY_ON};

#ifndef ACME
#define RELAY_LIST 	                                                             \
    D(FUTURE_0, "Future 0"),		/* [0] Future relay 0                     */ \
    D(FUTURE_1, "Future 1"),		/* [1] Future relay 1                     */ \
    D(BELL, "Bell"),			/* [2] Crossing bell                      */ \
    D(TRACK_SEM_R, "Track Sem. L"),	/* [3] Track car semaphore right          */ \
    D(TRACK_SEM_L, "Track Sem. L"),	/* [4] Track car semaphore left           */ \
                                                                                     \
    D(UPPER_WW, "Upper WW"),		/* [5] Upper wig wag                      */ \
    D(FUTURE_6, "Future 6"),		/* [6] Future relay 6                     */ \
    D(LOWER_WW, "Lower WW"),		/* [7] Lower wig wag                      */ \
                                                                                     \
    D(H2_RELAY,   "H2"), 		/* [8] H2 Signal                          */ \
                                                                                     \
    D(W4_RED,    "4W/Red"),		/* [9] 4 White lights / red indicator     */ \
    D(W4_YELLOW, "4W/Yellow"),		/* [10] 4 White lights / yellow indicator */ \
    D(W4_GREEN,  "4W/Green"),		/* [11] 4 White lights / green indicator  */ \
                                                                                     \
    D(C3_RED,    "C3/Red"),		/* [12] 3 color red                       */ \
    D(C3_YELLOW, "C3/Yellow"),		/* [13] 3 color yellow                    */ \
    D(C3_GREEN,  "C3/Green"),		/* [14] 3 color green                     */ \
                                                                                     \
    D(TRACK_CAR, "Track Car"),		/* [15] Track car (dots, light)           */ 
#else // ACME
#define RELAY_LIST 	                                                             \
    D(H1_MOTOR_POWER, "H1: Motor Power"),/* [0] H1: Arm Motor Power                */ \
    D(H1_MOTOR_DIR,   "H1: Motor Dir."),/* [1] H1: Motor Power                    */ \
    D(H1_FOLD,        "H1: Fold"),	/* [2] H1: Fold                           */ \
										     \
    D(H1_GREEN,       "H1: Green"),	/* [3] H1: Green                          */ \
    D(H1_BELL,        "H1: Bell"),	/* [4] H1: Bell                           */ \
                                                                                     \
    D(H1_YELLOW,      "H1: Yellow"),	/* [5] H1: Yellow                         */ \
    D(H1_RED,         "H1: Red"),	/* [6] H1: Red                            */ \
                                                                                     \
    D(FUTURE_7,       "Future 7"),	/* [7] Future 7                           */ \
    D(FUTURE_8,       "Future 8"),	/* [8] Future 8                           */ \
                                                                                     \
    D(H2_RED,         "H2: Red"),	/* [9] H2: Red                            */ \
    D(H2_YELLOW,      "H2: Yellow"),	/* [10] H2: Yellow                        */ \
    D(H2_BELL,        "H2: Bell"),	/* [11] H2: Bell                          */ \
    D(H2_GREEN,       "H2: Green"),	/* [12] H2: Green                         */ \
										     \
    D(H2_FOLD,        "H2: Fold"),	/* [13] H2: Fold                          */ \
    D(H2_MOTOR_DIR,   "H2: Motor Dir."),/* [14] H2: Motor Power                   */ \
    D(H2_MOTOR_POWER, "H2: Motor Power"),/* [15] H2: Arm Motor Power              */ 
#endif // ACME

// Relay assignments
enum RELAY_NAME {
#define D(X, Y) X
    RELAY_LIST
#undef D
};

// Last relay in existance
#ifndef ACME
static const RELAY_NAME LAST_RELAY = H2_RELAY;
#else // ACME
static const RELAY_NAME LAST_RELAY = H2_MOTOR_POWER;
#endif // ACME


extern void relay_setup(void);
extern std::string relay_status(const enum RELAY_NAME relay_number);
extern std::string gpio_status(const int gpio_number);
extern void relay(
	const char* const thread_name,	// Name of the thread doing the change
	const enum RELAY_NAME relay_name, // The name of the relay
	const enum RELAY_STATE state	// The state of the relay
);
extern void relay_reset(void);
extern bool verbose;	// Do we chatter
extern bool simulate;	// Simulate relay information
#endif // __RELAY__H__
