/*
 * button_mcp -- Take buttons from the mcp input and send them to the
 * 	garden program
 *
 * Written 2014 by Steve Oualline
 * Placed in the public domain by Steve Oualline
 * oualline@www.oualline.com
 *
 * Usage:
 * 	Run the garden command
 * 	Run this command
 * 	Press buttons -- Watch signals move
 */
/*
 * mcp2200_t -- Class for handling the mcp device
 *
 * Member function
 * 	get_serial_list -- Get a list of the serial numbers of the attached devices.
 * 		(static function so can be called without a class instance)
 *
 * 	mcp2200_t(serial) -- Create a class for this specific device
 * 	configure(config_data) -- Configure the device
 * 	read_all_response_t = read_all() -- Do a read all and get the response
 * 	set_class_all(set_clear_all_t) -- Do a set/clear
 *
 * Embedded classes
 * ================
 *
 * config_cmd_t -- Configuration commmand block
 * 	set_io_bmp -- Set the io_bmp config field
 * 	set_config_alt_pins -- Set the config_alt_pins config field
 * 	set_io_default_val_bmap -- Set the io_default_val_bmap config field
 * 	set_config_alt_options -- Set the config_alt_options config field
 * 	set_baud -- Set the baud rate
 * 	toString -- Produce string from configuration data
 *
 * read_all_response_t -- Response to the read-all command
 * 	get_EEP_Addr -- Return the EEP_Addr value
 * 	get_EEP_Value -- Return the EEP_Value value
 * 	get_IO_bmp -- Return the IO_bmp value
 * 	get_Config_Alt_Pins -- Return the Config_Alt_Pins value
 * 	get_IO_Default-Val_bmap -- Return the IO_Default-Val_bmap value
 * 	get_Config_Alt_Options -- Return the Config_Alt_Options value
 * 	get_Baud -- Return the baud rate divisor
 * 	get_IO_Port_Val_bmap -- Return the IO_Porty_Val_bmap value
 * 	toString -- Produce string value from the result
 *
 * set_clear_all_t -- Set/Clear all command
 * 	set -- Set GPIO bits
 * 	clear -- Clear GPIO bits
 */
#include <iostream>
#include <iomanip>
#include <sstream>

#include <cstdlib>
#include <list>
#include <cstring>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <libusb-1.0/libusb.h>

#include "device.h"

// Timeout for transfers
#define MCP2200_HID_TRANSFER_TIMEOUT 50000

/*
 * mcp2200_error_t -- Throw this when an error occurs
 */
class mcp2200_error_t {
    public:
	const char* const file;	// File name where error occurred
	const int line;		// Line number where error occurred
	const int usb_error;	// USB error message
	const std::string msg;	// Error string from the suer
    public:
	mcp2200_error_t(const char* const _file, const int _line, const int _usb_error, const std::string& _msg):
	    file(_file), line(_line), usb_error(_usb_error), msg(_msg) 
	{}
};

/*
 * mcp2200_t -- Class for handling the mcp device
 */
class mcp2200_t {
    private:
	// Product id for the USB id list
	static const uint16_t MCP2200_VENDOR_ID	= 0x04d8;
	static const uint16_t MCP2200_PRODUCT_ID = 0x00df;

	// Interfaces defined on the device
	static const uint8_t MCP2200_CDC_INTERFACE = 1;
	static const uint8_t MCP2200_HID_INTERFACE = 2;

	static const uint8_t MCP2200_HID_ENDPOINT_IN = 0x81;
	static const uint8_t MCP2200_HID_ENDPOINT_OUT = 0x01;

	// Commands are defined by the "MCP2200 HID Interface Command Description" document
	static const uint8_t MCP2200_HID_COMMAND_SET_CLEAR_OUTPUT = 0x08u;	// Set / Clear output bits
	static const uint8_t MCP2200_HID_COMMAND_CONFIGURE = 	    0x10u;	// Configure the device
	static const uint8_t MCP2200_HID_COMMAND_READ_EE = 	    0x20u;	// Read EE prom
	static const uint8_t MCP2200_HID_COMMAND_WRITE_EE = 	    0x40u;	// Write EE prom
	static const uint8_t MCP2200_HID_COMMAND_READ_ALL = 	    0x80u;	// Read all information
    public:
	// From Table 5 of the Interface Command Description
	// Values for config_alt_pins
	static const uint8_t ALT_SSPND = (1<<7);
	static const uint8_t ALT_USBCFG = (1<<6);
	static const uint8_t ALT_RxLED = (1<<3);
	static const uint8_t ALT_TxLED = (1<<2);
	// Value for config_alt_options
	// From table 6 of the Interface Command Description
	static const uint8_t ALT_OPT_RxTGL = (1<<7);
	static const uint8_t ALT_OPT_TxTGL = (1<<6);
	static const uint8_t ALT_OPT_LEDX = (1<<5);
	static const uint8_t ALT_OPT_INVERT = (1<<1);
	static const uint8_t ALT_OPT_HW_FLOW = (1<<0);
    public:

	/*------------------------------------------------------*/
	// Configuration command data
	class config_cmd_t {
	    private:
		// Table 4 in the Command Description document
		struct config_cmd {
		    uint8_t cmd;	// [0] Command (0x10)
		    uint8_t dc1;	// [1] Don't care
		    uint8_t dc2;	// [2] Don't care
		    uint8_t dc3;	// [3] Don't care
		    uint8_t io_bmp;	// [4] GPIO bitmap for pin assignment
		    uint8_t config_alt_pins;	// [5] Alternate configuration pin settings
		    uint8_t io_default_val_bmap;	//[6] Default GPIO value bitmap
		    uint8_t config_alt_options;	//[7] Alternateve function options
		    uint8_t baud_h;	//[8] Baud rate (high)
		    uint8_t baud_l;	//[9] Baud rate (low)
		    uint8_t dc10;	//[10] Don't care
		    uint8_t dc11;	//[11] Don't care
		    uint8_t dc12;	//[12] Don't care
		    uint8_t dc13;	//[13] Don't care
		    uint8_t dc14;	//[14] Don't care
		    uint8_t dc15;	//[15] Don't care
		};
		// The command as a union
		union {
		    struct config_cmd cmd;
		    unsigned char data[16];
		} config_cmd;
	    public:
		config_cmd_t(void) {
		    memset(&config_cmd, '\0', sizeof(config_cmd));
		    config_cmd.cmd.cmd = 0x10;
		    set_baud(9600);
		}
		// Copy constructor defaults
		// Assignement operator defaults
		// Destructor defaults
	    public:
		void set_io_bmp(const uint8_t io_bmp) {
		    config_cmd.cmd.io_bmp = io_bmp;
		}
		void set_config_alt_pins(const uint8_t config_alt_pins) {
		    config_cmd.cmd.config_alt_pins = config_alt_pins;
		}
		void set_io_default_val_bmap(const uint8_t io_default_val_bmap) {
		    config_cmd.cmd.io_default_val_bmap = io_default_val_bmap;
		}
		void set_config_alt_options(const uint8_t config_alt_options) {
		    config_cmd.cmd.config_alt_options = config_alt_options;
		}
		void set_baud(const unsigned int rate) {
		    uint16_t rate_code = (12000000 / rate) - 1;
		    config_cmd.cmd.baud_h = (rate_code >> 8) & 0xFF;
		    config_cmd.cmd.baud_l = rate_code & 0xFF;
		}
		unsigned int get_IO_bmap(void) {
		    return (config_cmd.cmd.io_bmp);
		}
		unsigned int get_Config_Alt_Pins(void) {
		    return (config_cmd.cmd.config_alt_pins);
		}
		unsigned int get_IO_Default_Val_bmap(void) {
		    return (config_cmd.cmd.io_default_val_bmap);
		}
		unsigned int get_Config_Alt_Options(void) {
		    return (config_cmd.cmd.config_alt_options);
		}
		unsigned int get_Baud(void) {
		    return (config_cmd.cmd.baud_h << 8 | config_cmd.cmd.baud_l);
		}
	    private:
		const unsigned char* get_data(void) const {
		    return (config_cmd.data);
		}
		ssize_t get_data_size(void) const {
		    return (sizeof(config_cmd.data));
		}
		// Let our parent in on this
		friend class mcp2200_t;
	    public:
		std::string toString(void);
	};
	/*------------------------------------------------------*/
	/*------------------------------------------------------*/

	// Response to the read all command
	class read_all_response_t {
	    private:
		// Table 12 in the Command Description document
		struct read_all_struct {
		    uint8_t cmd;	// [0] Command (0x10)
		    uint8_t eep_addr;	// [1] EEProm address
		    uint8_t dc2;	// [2] Don't care
		    uint8_t eep_value;	// [3] EEProm value
		    uint8_t io_bmp;	// [4] GPIO bitmap for pin assignment
		    uint8_t config_alt_pins;	// [5] Alternate configuration pin settings
		    uint8_t io_default_val_bmap;	//[6] Default GPIO value bitmap
		    uint8_t config_alt_options;	//[7] Alternateve function options
		    uint8_t baud_h;	//[8] Baud rate (high)
		    uint8_t baud_l;	//[9] Baud rate (low)
		    uint8_t io_port_val_bmap;	//[10] IP Port value bitmap
		    uint8_t dc11;	//[11] Don't care
		    uint8_t dc12;	//[12] Don't care
		    uint8_t dc13;	//[13] Don't care
		    uint8_t dc14;	//[14] Don't care
		    uint8_t dc15;	//[15] Don't care
		};
		// The command as a union
		union {
		    struct read_all_struct response;
		    unsigned char data[16];
		} read_all_response_data;
	    public:
		unsigned int get_EEP_Addr(void) {
		    return (read_all_response_data.response.eep_addr);
		}
		unsigned int get_EEP_Value(void) {
		    return (read_all_response_data.response.eep_value);
		}
		unsigned int get_IO_bmap(void) {
		    return (read_all_response_data.response.io_bmp);
		}
		unsigned int get_Config_Alt_Pins(void) {
		    return (read_all_response_data.response.config_alt_pins);
		}
		unsigned int get_IO_Default_Val_bmap(void) {
		    return (read_all_response_data.response.io_default_val_bmap);
		}
		unsigned int get_Config_Alt_Options(void) {
		    return (read_all_response_data.response.config_alt_options);
		}
		unsigned int get_Baud(void) {
		    return (read_all_response_data.response.baud_h << 8 | read_all_response_data.response.baud_l);
		}
		unsigned int get_IO_Port_Val_bmap(void) {
		    return (read_all_response_data.response.io_port_val_bmap);
		}
	    public:
		read_all_response_t(void) {
		    memset(&read_all_response_data, '\0', sizeof(read_all_response_data));
		}
		// Copy constructor defaults
		// Assignment operator defaults
		// Destructor defaults
	    private:
		const unsigned char* get_data(void) const {
		    return (read_all_response_data.data);
		}
		ssize_t get_data_size(void) const {
		    return (sizeof(read_all_response_data.data));
		}
		friend class mcp2200_t;
	    public:
		std::string toString(void);
	};
	/*------------------------------------------------------*/
	/*------------------------------------------------------*/

	// Data for the set / clear all command
	class set_clear_all_t {
	    private:
		// Table 3 in the Command Description document
		struct set_clear_struct {
		    uint8_t cmd;	// [0] Command (0x08)
		    uint8_t dc1;	// [1] Don't care
		    uint8_t dc2;	// [2] Don't care
		    uint8_t dc3;	// [3] Don't care
		    uint8_t dc4;	// [4] Don't care
		    uint8_t dc5;	// [5] Don't care
		    uint8_t dc6;	// [6] Don't care
		    uint8_t dc7;	// [7] Don't care
		    uint8_t dc8;	// [8] Don't care
		    uint8_t dc9;	// [9] Don't care
		    uint8_t dc10;	// [10] Don't care
		    uint8_t set_bmap;	// [11] Set bitmap
		    uint8_t clear_bmap;	// [12] Clear bitmap
		    uint8_t dc13;	// [13] Don't care
		    uint8_t dc14;	// [14] Don't care
		    uint8_t dc15;	// [15] Don't care
		};
		// The command as a union
		union {
		    struct set_clear_struct set_clear;
		    unsigned char data[16];
		} set_clear_data;
	    public:
		void set(uint8_t bits) {
		    set_clear_data.set_clear.set_bmap |= bits;
		    set_clear_data.set_clear.clear_bmap &= (~bits);
		}
		void clear(uint8_t bits) {
		    set_clear_data.set_clear.clear_bmap |= bits;
		    set_clear_data.set_clear.set_bmap &= (~bits);
		}
	    public:
		set_clear_all_t(void) {
		    memset(&set_clear_data, '\0', sizeof(set_clear_data));
		    set_clear_data.set_clear.cmd = MCP2200_HID_COMMAND_SET_CLEAR_OUTPUT;
		    set_clear_data.set_clear.clear_bmap = 0xFF;
		    set_clear_data.set_clear.set_bmap = 0x00;
		}
		// Copy constructor defaults
		// Assignment operator defaults
		// Destructor defaults
	    private:
		const unsigned char* get_data(void) const {
		    return (set_clear_data.data);
		}
		ssize_t get_data_size(void) const {
		    return (sizeof(set_clear_data));
		}
		friend class mcp2200_t;
	};
	/*------------------------------------------------------*/
	/*------------------------------------------------------*/

	// Actual class begins here
    private:
	libusb_device_handle *handle;	// Handle of the device we are using
    public:
	static std::list<std::string> get_serial_list(void);
    public:
	mcp2200_t(const std::string& serial);
	~mcp2200_t() {
	    if (handle != NULL)
		libusb_close(handle);
	}
    private:
	mcp2200_t(const mcp2200_t&);	// No copy
	mcp2200_t& operator = (const mcp2200_t&);	// No assignment
    public:
	// Configure the device
	void configure(const config_cmd_t& config_data);
	read_all_response_t read_all(void);
	void set_clear_all(set_clear_all_t& set_clear_param);
};
/*
 * get_serial_list -- Get list of serial numbers for the devices on the system
 */
std::list<std::string> mcp2200_t::get_serial_list(void)
{
    std::list<std::string> result;		// List of serial numbers returned
    libusb_device **devs;			// The devices return by lsusb 

    // Get the devices from the item
    int cnt = libusb_get_device_list(NULL, &devs);
    if (cnt < 0) {
	throw (mcp2200_error_t(__FILE__, __LINE__, cnt, "Unable to get a list of devices"));
    }

    // Loop through each device to see if it is a mcp2200
    for(int i = 0; i < cnt; i++) {
	// Usb device descriptor
	struct libusb_device_descriptor desc;

	// Get the device description
	int usb_result = libusb_get_device_descriptor(devs[i], &desc);
	if (usb_result < 0) {
	    throw (mcp2200_error_t(__FILE__, __LINE__, usb_result, "Unable to get device information"));
	} else {
	    if ((MCP2200_VENDOR_ID == desc.idVendor) &&
		(MCP2200_PRODUCT_ID == desc.idProduct)) {
		if (desc.iSerialNumber > 0) {
		    libusb_device_handle *handle;
		    int res = libusb_open(devs[i], &handle);
		    if (res < 0) {
			throw (mcp2200_error_t(__FILE__, __LINE__, res, "Unable to open device"));
		    }
		    unsigned char serial[100] = {0};
		    ssize_t serial_len = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, serial, sizeof(serial)-1);
		    serial[serial_len] = '\0';
		    result.push_back(reinterpret_cast<char*>(serial));
		    libusb_close(handle);
		}
	    }
	}
    }
    libusb_free_device_list(devs, 1);
    return result;
}
/*
 * mcp2200_t::mcp2200_t -- Open a given mcp2200 device
 *
 * Parameters
 * 	serial -- Serial number to use for opening the device
 */
mcp2200_t::mcp2200_t(const std::string& dev_serial)
{
    //TODO: Need to make sure libusb is initialized
    libusb_device **devs;	// The devices return by lsusb

    // Get the devices from the item
    int cnt = libusb_get_device_list(NULL, &devs);
    if (cnt < 0) {
	throw (mcp2200_error_t(__FILE__, __LINE__, cnt, "Unable to get a list of devices"));
    }

    for(int i = 0; i < cnt; i++){
	// Usb device descriptor
	struct libusb_device_descriptor desc;

	// Get the device description
	int result = libusb_get_device_descriptor(devs[i], &desc);
	if (result < 0) {
	    throw (mcp2200_error_t(__FILE__, __LINE__, result, "Unable to get device information"));
	} else {
	    if ((MCP2200_VENDOR_ID == desc.idVendor) &&
		(MCP2200_PRODUCT_ID == desc.idProduct)) {
		if (desc.iSerialNumber > 0) {
		    int res = libusb_open(devs[i], &handle);
		    if (res < 0) {
			throw (mcp2200_error_t(__FILE__, __LINE__, result, "Unable to open device"));
		    }
		    unsigned char serial[100] = {0};	// Serial number as a bunch of characters
		    // Get the serial number of the device (and it's length)
		    ssize_t serial_len = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, serial, sizeof(serial)-1);
		    serial[serial_len] = '\0';

		    // See if this is our serial number
		    if (strcmp(reinterpret_cast<char*>(serial), dev_serial.c_str()) == 0) {
			break;
		    }
		    // Not the one we wanted
		    libusb_close(handle);
		}
	    }
	}
    }
    libusb_free_device_list(devs, 1);

    libusb_detach_kernel_driver(handle, MCP2200_HID_INTERFACE);
    // Claim HID interface
    int result = libusb_claim_interface(handle, MCP2200_HID_INTERFACE);
    if (result != 0){
	throw (mcp2200_error_t(__FILE__, __LINE__, 0, "No such serial number"));
    }
    // We don't claim the CDC interface as this is just about the HID interface
}
/*
 * mcp2200_t::configure -- Configure the device
 *
 * Parameters
 * 	config_cmd_t -- The configuration to use
 */
void mcp2200_t::configure(const mcp2200_t::config_cmd_t& config_data)
{
    int write_size = 0;	// Number of bytes transfered

    int result = libusb_claim_interface(handle, MCP2200_HID_INTERFACE);
    if (result != 0) {
	syslog(LOG_ERR, "CONFIGURE claim interface result %d", result);
	throw(mcp2200_error_t(__FILE__, __LINE__, result, "Claim of interface failure"));
    }
    // Send the configuration command.   Get the result of the transfer
    result = libusb_interrupt_transfer(
	    handle, 
	    MCP2200_HID_ENDPOINT_OUT, 
	    const_cast<unsigned char*>(config_data.get_data()), 
	    config_data.get_data_size(), 
	    &write_size, 
	    MCP2200_HID_TRANSFER_TIMEOUT);

    libusb_release_interface(handle, MCP2200_HID_INTERFACE);

    if (result != 0) {
	throw(mcp2200_error_t(__FILE__, __LINE__, result, "USB write error with configuration"));
    }
	
    if (write_size != config_data.get_data_size()) {
	throw(mcp2200_error_t(__FILE__, __LINE__, 0, "Write of configuration data failed"));
    }
}
/*
 * mcp2200_t::read_all -- Do a read_all on the device
 *
 */
mcp2200_t::read_all_response_t mcp2200_t::read_all()
{
    int result = libusb_claim_interface(handle, MCP2200_HID_INTERFACE);
    if (result != 0) {
	syslog(LOG_ERR, "READ_ALL claim interface result %d", result);
	throw(mcp2200_error_t(__FILE__, __LINE__, result, "Claim of interface failure"));
    }

    // Command to read all values
    static const unsigned char read_all_cmd[16] = {
	MCP2200_HID_COMMAND_READ_ALL,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    int write_size = 0;	// Number of bytes transfered

    // Send the read all command -- get the result of the transfer
    result = libusb_interrupt_transfer(
	    handle, 
	    MCP2200_HID_ENDPOINT_OUT, 
	    const_cast<unsigned char*>(read_all_cmd), 
	    sizeof(read_all_cmd), 
	    &write_size, 
	    MCP2200_HID_TRANSFER_TIMEOUT);

    if (result != 0) {
	throw(mcp2200_error_t(__FILE__, __LINE__, result, "USB write error with readall command"));
    }
	
    if (write_size != sizeof(read_all_cmd)) {
	throw(mcp2200_error_t(__FILE__, __LINE__, 0, "Write of readall command failed"));
    }
    mcp2200_t::read_all_response_t response;	// The response 

    // Read the response and check result
    result = libusb_interrupt_transfer(
	    handle, 
	    MCP2200_HID_ENDPOINT_IN, 
	    const_cast<unsigned char*>(response.get_data()), 
	    response.get_data_size(), 
	    &write_size, 
	    MCP2200_HID_TRANSFER_TIMEOUT
	);

    if (result != 0) {
	throw(mcp2200_error_t(__FILE__, __LINE__, result, "USB read error with readall command"));
    }
    libusb_release_interface(handle, MCP2200_HID_INTERFACE);
	
    if (write_size != response.get_data_size()) {
	throw(mcp2200_error_t(__FILE__, __LINE__, 0, "Read of readall command failed"));
    }
    return (response);
}
/*
 * set_clear_all -- Do a set clear all command
 *
 * Parameters
 * 	set_clear_param -- Bits to set or clear
 */
void mcp2200_t::set_clear_all(set_clear_all_t& set_clear_param)
{
    int write_size = 0;	// Number of bytes transfered

    int result = libusb_claim_interface(handle, MCP2200_HID_INTERFACE);
    if (result != 0) {
	syslog(LOG_ERR, "SET_CLEAR_ALL claim interface result %d", result);
	throw(mcp2200_error_t(__FILE__, __LINE__, result, "Claim of interface failure"));
    }

    result = libusb_interrupt_transfer(
	    handle, 
	    MCP2200_HID_ENDPOINT_OUT, 
	    const_cast<unsigned char*>(set_clear_param.get_data()), 
	    set_clear_param.get_data_size(), 
	    &write_size, 
	    MCP2200_HID_TRANSFER_TIMEOUT);

    libusb_release_interface(handle, MCP2200_HID_INTERFACE);

    if (result != 0) {
	throw(mcp2200_error_t(__FILE__, __LINE__, result, "USB write error with set_clear_all command"));
    }
	
    if (write_size != set_clear_param.get_data_size()) {
	throw(mcp2200_error_t(__FILE__, __LINE__, 0, "Write of set_clear_all command failed"));
    }
}
/*
 * mcp2200_config::toString -- Turn configuration into a string
 */
std::string mcp2200_t::read_all_response_t::toString(void)
{
    std::ostringstream output;	// Output string
    for (unsigned i = 0; i < sizeof(read_all_response_data.data); ++i) {
	output << std::setw(2) << i << ": " << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(read_all_response_data.data[i]) << " ";
	output << std::setw(0) << std::setfill(' ') << std::dec;

	switch (i) {
	    case 0: 
		output << "Read all";
		break;
	    case 1:
		output << "EEPROM Location";
		break;
	    case 3:
		output << "EEPROM Value";
		break;
	    case 4:
		output << "IO Bitmap";
		break;
	    case 5:
		output << "Config_Alt_Pins (";
		if ((read_all_response_data.response.config_alt_pins & ALT_SSPND) != 0)
		    output << "SSPND ";
		if ((read_all_response_data.response.config_alt_pins & ALT_USBCFG) != 0)
		    output << "USBCFG ";
		if ((read_all_response_data.response.config_alt_pins & ALT_RxLED) != 0)
		    output << "RxLED ";
		if ((read_all_response_data.response.config_alt_pins & ALT_TxLED) != 0)
		    output << "TxLED ";
		output << ")";
		break;
	    case 6:
		output << "Default IO Bitmap";
		break;
	    case 7:
		output << "Config Alt Options (";
		if ((read_all_response_data.response.config_alt_options & ALT_OPT_RxTGL) != 0) 
		    output << "RxTGL ";
		if ((read_all_response_data.response.config_alt_options & ALT_OPT_TxTGL) != 0) 
		    output << "TxTGL ";
		if ((read_all_response_data.response.config_alt_options & ALT_OPT_LEDX) != 0) 
		    output << "LEDX ";
		if ((read_all_response_data.response.config_alt_options & ALT_OPT_INVERT) != 0) 
		    output << "INVERT ";
		if ((read_all_response_data.response.config_alt_options & ALT_OPT_HW_FLOW) != 0) 
		    output << "HW_FLOW ";
		output << ")";
		break;
	    case 8:
		output << "Baud H";
		break;
	    case 9:
		output << "Baud L";
		break;
	    case 10:
		output << "GPIO Bitmap";
		break;
	    case 2:
	    case 11:
	    case 12:
	    case 13:
	    case 14:
	    case 15:
		output << "Don't care";
		break;
	    default:
		output << "Internal error";
		break;
	}
	output << std::endl;
    }
    int baud_rate_divisor = read_all_response_data.response.baud_h << 8 | read_all_response_data.response.baud_l;
    int baud = 12000000 / (baud_rate_divisor + 1);
    output << "Baud Rate Divisor " << baud_rate_divisor << " Rate: " << baud << std::endl;
    return (output.str());
}

/*
 * mcp2200_config::toString -- Turn configuration into a string
 */
std::string mcp2200_t::config_cmd_t::toString(void)
{
    std::ostringstream output;	// Output string
    for (unsigned i = 0; i < sizeof(config_cmd.data); ++i) {
	output << std::setw(2) << i << ": " << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(config_cmd.data[i]) << " ";
	output << std::setw(0) << std::setfill(' ') << std::dec;

	switch (i) {
	    case 0: 
		output << "CONFIGURE command";
		break;
	    case 1:
	    case 2:
	    case 3:
	    case 10:
	    case 11:
	    case 12:
	    case 13:
	    case 14:
	    case 15:
		output << "Don't care";
		break;
	    case 4:
		output << "IO Bitmap";
		break;
	    case 5:
		output << "Config_Alt_Pins (";
		if ((config_cmd.cmd.config_alt_pins & ALT_SSPND) != 0)
		    output << "SSPND ";
		if ((config_cmd.cmd.config_alt_pins & ALT_USBCFG) != 0)
		    output << "USBCFG ";
		if ((config_cmd.cmd.config_alt_pins & ALT_RxLED) != 0)
		    output << "RxLED ";
		if ((config_cmd.cmd.config_alt_pins & ALT_TxLED) != 0)
		    output << "TxLED ";
		output << ")";
		break;
	    case 6:
		output << "Default IO Bitmap";
		break;
	    case 7:
		output << "Config Alt Options (";
		if ((config_cmd.cmd.config_alt_options & ALT_OPT_RxTGL) != 0) 
		    output << "RxTGL ";
		if ((config_cmd.cmd.config_alt_options & ALT_OPT_TxTGL) != 0) 
		    output << "TxTGL ";
		if ((config_cmd.cmd.config_alt_options & ALT_OPT_LEDX) != 0) 
		    output << "LEDX ";
		if ((config_cmd.cmd.config_alt_options & ALT_OPT_INVERT) != 0) 
		    output << "INVERT ";
		if ((config_cmd.cmd.config_alt_options & ALT_OPT_HW_FLOW) != 0) 
		    output << "HW_FLOW ";
		output << ")";
		break;
	    case 8:
		output << "Baud H";
		break;
	    case 9:
		output << "Baud L";
		break;
	    default:
		output << "Internal error";
		break;
	}
	output << std::endl;
    }
    // Compute the baud rate divisor
    int baud_rate_divisor = config_cmd.cmd.baud_h << 8 | config_cmd.cmd.baud_l;

    // Now get the baud rate
    int baud = 12000000 / (baud_rate_divisor + 1);
    output << "Baud Rate Divisor " << baud_rate_divisor << " Rate: " << baud << std::endl;
    return (output.str());
}

int main(int argc, char* argv[])
{
    bool stdout_log = false;	// Send log messages to stdout

    int opt;	// Option we are looking
    while ((opt = getopt(argc, argv, "s")) != -1) {
	switch (opt) {
	    case 's':
		stdout_log = true;
		break;
	    default:
		std::cerr << "Usage " << argv[0] << " [-s]" << std::endl;
		exit(EXIT_FAILURE);
	}
    }
    if (optind < argc) {
	std::cerr << "Extra arguements on the command line" << std::endl;
	exit(EXIT_FAILURE);
    }
    // Open up the syslog system
    openlog("button_mcp", stdout_log ? LOG_PERROR : 0, LOG_USER); 

    int fd;	// FD for the output fifo
    while (true) {
	// Open the socket for this process
	fd = open(INPUT_PIPE, O_WRONLY);
	if (fd < 0) {
	    syslog(LOG_ERR, "ERROR: Could not open input pipe");
	    sleep(30);
	} else {
	    break;
	}
    }

    while (true) {
	try {
	    // Init the libusb system.   Must be doen first
	    int usb_init =  libusb_init(NULL);
	    if (usb_init != 0) {
		std::cout << "ERROR: libusb_init failed with code " << usb_init << std::endl;
		sleep(30);
		continue;
	    }
	    libusb_set_debug(NULL, 5);//##

	    // Get the list of serail numbers for the devices
	    std::list<std::string> serial_list = mcp2200_t::get_serial_list();
	    if (serial_list.empty()) {
		syslog(LOG_ERR, "No devices seen");
		sleep(30);
		continue;
	    }
	    mcp2200_t::config_cmd_t config;	// Get the configuration
	    config.set_io_bmp(0xFF);	// Set all bits to input

	    if (serial_list.empty()) {
		syslog(LOG_ERR, "No devices found");
		sleep(30);
		continue;
	    }
	    // Get the serial number of the first device
	    std::string serial = serial_list.front();

	    // Create a device for it
	    mcp2200_t device(serial);

	    // Configure it
	    device.configure(config);

	    uint8_t old_bits = 0x00;	// Bits previous pressed
	    while (true) {
		mcp2200_t::read_all_response_t response = device.read_all();

		if ((response.get_IO_bmap() != config.get_IO_bmap()) || 
			(response.get_Config_Alt_Pins() != config.get_Config_Alt_Pins()) ||
			(response.get_Config_Alt_Options() != config.get_Config_Alt_Options())) {
		    syslog(LOG_ERR, "Config / read-all response mismatch ");
		    syslog(LOG_ERR, "Read all respone");
		    syslog(LOG_ERR, "%s", response.toString().c_str());
		    syslog(LOG_ERR, "config");
		    syslog(LOG_ERR, "%s", response.toString().c_str());
		}
		// Current value of the I/O pins
		uint8_t current = response.get_IO_Port_Val_bmap();
		if (current != old_bits) {
		    // current old -> delta
		    // 0       0   -> 0
		    // 0       1   -> 1
		    // 1       0   -> 0
		    // 1       1   -> 0
		    uint8_t delta = (~current) & old_bits;	// Get the new bits

		    for (int i = 0; i < 8; ++i) {
			if ((delta & (1 << i)) != 0) {
			    char input = '0' + i;
			    write(fd, &input, 1);
			}
		    }
		}
		old_bits = current;

		// Sleep 1/5 second
		static const struct timespec sleep_time = {0, 1000000000 / 5};
		nanosleep(&sleep_time, NULL);
	    }
	}
	catch (mcp2200_error_t &error) {
	    syslog(LOG_ERR, "ERROR: %s:%d usb error: %d:%s",
		    error.file, error.line, error.usb_error, error.msg.c_str());
	    exit(EXIT_FAILURE);
	}
    }
    return 0;
}
