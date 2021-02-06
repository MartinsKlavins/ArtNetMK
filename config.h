/* Mārtiņš Kļaviņš */

#ifndef CONFIG_H
#define CONFIG_H



// --------------------------- MCU related -------------------------
// uncomment your choice for devices:

//#define _kind_of_ARDUINO
#define all_WIZnets
//#define _kind_of_ESP


// --------------------------- Art-Net related ---------------------
// correct for your device:

// ---- device ID
#define SHORT_NAME    	" "			// device name (max 17 bytes)
#define LONG_NAME   	" " 		// device description (max 63 bytes)
#define ESTA      		0xFFFF		// ESTA code
#define VERSION			1			// device firmware version
#define OEM				0xffff		// OEM code for ArtNet product (OEMunknown=0x00FF)
// ---- device configuration
#define DMX_CHANNELS	24			// dmx512 channel count in one frame
#define en_read_DMX		1			// enable to receive dmx512
#define en_reply_msg	0			// enable to replay to discovery messages
#define PAYLOAD			8			// NZS channel count
#define STATUS1			0xd0		// 11-01-0-0-0-0 = 0xd0 (indicators normalMode - set by frontPanel - 0 - normalBoot - RDMon - UBEAoff)
#define PORTSc			0x0001		// port count
#define PORT1			0xC5		// 1-1-000101 = c5 (out_yes - in_yes - ArtNet data)
//#define PORT2
//#define PORT3
//#define PORT4
#define STYLE			0			// 0 - node, 1 - controller
#define STATUS2			0x08		// 000-0-1-0-0-0 = 0x08 (not squawking - no ACN - ArtNet4 - noDHCP - IP set manualy - no web config)
#define discoveryCONFIG	0			// 000-0-0-0-0-0 = 0x00	(unused - ignore VLC - (diag_msg broadcast)- no diag_msg - artPollReply after artPoll - uused)





#endif