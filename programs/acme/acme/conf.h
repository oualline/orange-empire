#ifndef   __CONF_H__
#define  __CONF_H__
#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>

// Location of the configuration file
static const char* const GLOBAL_NAME = "/home/garden/acme.conf";
static const char* const LOCAL_NAME = "./acme.conf";

class config {
    private:
	
	std::string password;	// Password
	std::string demo_name;	// Name of the demo
	uint32_t arm_time;	// Time to raise and lower arm
	bool h1_arms;		// Is the H1 arms on
	bool h2_arms;		// Is the H2 arms on
    public:
	config(void)
	{
	    get_config();
	}
	~config() = default;
	config(const config&) = default;
	config& operator = (const config&) = default;
    private:
	std::string get_config_name(void) {
	    if (access(GLOBAL_NAME, W_OK) == 0)
		return(GLOBAL_NAME);

	    if (access(LOCAL_NAME, W_OK) == 0)
		return(LOCAL_NAME);

	    std::cerr << "ERROR: Unable to access config file" << std::endl;
	    exit(8);
	}


	void get_config(void) {
	    std::ifstream config_file(get_config_name());

	    if (config_file.bad()) {
		std::cerr << "ERROR: Unable to open config file" << std::endl;
		exit(8);
	    }
	    std::getline(config_file, password);
	    std::getline(config_file, demo_name);

	    std::string line;	// Line from the inpute
	    std::getline(config_file, line);
	    arm_time = atol(line.c_str());

	    std::getline(config_file, line);
	    h1_arms = tolower(line[0]) == 't';

	    std::getline(config_file, line);
	    h2_arms = tolower(line[0]) == 't';
	    
	}
	void set_config(void)
	{
	    std::ofstream config_file(get_config_name());

	    if (config_file.bad()) {
		std::cerr << "ERROR: Unable to open config file" << std::endl;
		exit(8);
	    }
	    config_file << password << std::endl;
	    config_file << demo_name << std::endl;
	    config_file << arm_time << "	# Sleep time in 1/0 seconds " << std::endl;
	    if (h1_arms)
		config_file << "true" << std::endl;
	    else
		config_file << "false" << std::endl;

	    if (h2_arms)
		config_file << "true" << std::endl;
	    else
		config_file << "false" << std::endl;
	}
    public:
	uint32_t get_arm_time(void) {
	    return (arm_time);
	}
	void set_arm_time(const uint32_t _arm_time) {
	    arm_time = _arm_time;
	    set_config();
	} 
	std::string get_demo_name(void) { 
	    return (demo_name);
	}
	void set_demo_name(const std::string& _demo_name) {
	    demo_name = _demo_name;
	    set_config();
	}
	std::string get_password(void) {
	    return(password);
	}
	bool get_h1_arms(void)
	{
	    return (h1_arms);
	}
	bool get_h2_arms(void)
	{
	    return (h2_arms);
	}
	void set_h1_arms(const bool arm)
	{
	    h1_arms = arm;
	    set_config();
	}
	void set_h2_arms(const bool arm)
	{
	    h2_arms = arm;
	    set_config();
	}
};
extern config acme_config;	// The configuraiton
#endif // __CONF_H__
