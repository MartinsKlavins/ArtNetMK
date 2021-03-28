/* Mārtiņš Kļaviņš */

#ifndef ARTNETMK_H
#define ARTNETMK_H



#if defined(ARDUINO_SAMD_ZERO)
    #include <WiFi101.h>
    #include <WiFiUdp.h>
#elif defined(ESP8266)
    #include <ESP8266WiFi.h>
    #include <WiFiUdp.h>
#elif defined(ESP32)
    #include <WiFi.h>
    #include <WiFiUdp.h>
#else
	#include <Ethernet.h>
	#include <EthernetUdp.h>
#endif

// UDP
// ---
#define ART_NET_PORT 		6454
// network
// -------
const uint8_t limited_BC[4] = { 255, 255, 255, 255 };
#define CLASS_A(x) 	((1 <= x) && (x <= 127))
#define CLASS_B(x)	((128 <= x) && (x <= 191))
#define CLASS_C(x)	((192 <= x) && (x <= 223))
// OpCodes
// -------
#define ART_POLL 			0x2000
#define ART_POLL_REPLY 		0x2100
#define ART_DMX 			0x5000
#define ART_NZS				0x5100
#define ART_ADDRESS			0x6000
#define ART_IP_PROG			0xF800
#define ART_IP_PROG_REPLY 	0xF900
// Buffers
// -------
#define MAX_BUFFER_ARTNET 	530
// Packet
// ------
#define ART_NET_ID 			"Art-Net\0"
#define ART_DMX_START 		18
#define prot_version		0x000e



typedef enum {
	NO_DHCP,
	DHCP
} DHCP_mode;
typedef enum {
	port1,
	port2,
	port3,
	port4
} NODE_port;
typedef enum {
	myIP,
	mySUBNET,
	hostIP
} NODE_network;


struct artnet_dmx {
	uint8_t  id[8];						// ART_NET_ID	(0x41 0x72 0x74 0x2d 0x4e 0x65 0x74 0x00)
	uint16_t opCode;					// ART_DMX (0x5000)
	uint16_t protVersion;				// 0x000e = 14
	uint8_t	 sequence;					// use for tracking ArtDMX packet order
	uint8_t	 physical;					// port that sent data (informative use only)
	uint16_t portAddress;				// 0b0NNNNNNNSSSSUUUU ; where N - NET, S - SubNet, U - universe
	uint16_t dmxDataLenght;				// 0x0200 = 512, 0x0018 = 24
	uint8_t  dmxData[DMX_CHANNELS];		// (512) channels
};

struct artnet_nzs {
	uint8_t  id[8];						// ART_NET_ID	(0x41 0x72 0x74 0x2d 0x4e 0x65 0x74 0x00)
	uint16_t opCode;					// ART_NZS (0x5100)
	uint16_t protVersion;				// 0x000e = 14
	uint8_t	 sequence;					// use for tracking ArtDMX packet order
	uint8_t	 startCode;					// dmx start code
	uint16_t portAddress;				// 0b0NNNNNNNSSSSUUUU ; where N - NET, S - SubNet, U - universe
	uint16_t nzsDataLenght;				// 0x0200 = 512, 0x0018 = 24
	uint8_t  nzsData[PAYLOAD];			// (512) channels
};

struct artnet_poll {
	uint8_t  id[8];						// ART_NET_ID	(0x41 0x72 0x74 0x2d 0x4e 0x65 0x74 0x00)
	uint16_t opCode;					// ART_POLL
	uint16_t protVersion;				// 0x000e = 14
	uint8_t  talkToMe;					// config
	uint8_t  priority;					// diagnostic messages not implemented
};

struct artnet_reply {
	uint8_t  id[8];                 	// "Art-Net "
	uint16_t opCode;                  	// ART_POLL_REPLY
	uint8_t  ip[4];                 	// my device IP
	uint16_t port;                  	// ART_NET_PORT
	uint8_t  verH;                 		// device firmware version			
	uint8_t	 ver;						// ---
	uint8_t  net;                  		// NetSwitch
	uint8_t  sub;                   	// SubSwitch
	uint8_t  oemH;                  	// OEM code							
	uint8_t  oem;                   	// ---
	uint8_t  ubea;                  	// UserBiosExtensionArea				not used = 0
	uint8_t  status;                  	// config 
	uint16_t etsa;              		// ESTA code 						
	uint8_t  shortname[18];          	// device name (max 17 bytes)
	uint8_t  longname[64];            	// device description (max 63 bytes)
	uint8_t  nodereport[64];          	// device report for status 			not used = 0
	uint8_t  numbportsH;              	// port count (1-4)					
	uint8_t  numbports;               	// ---------------
	uint8_t  porttypes[4];            	// port types - one byte for each port
	uint8_t  goodinput[4];            	// config for input ports 				1-0-0-0-0-0-0-0 = 0x80
	uint8_t  goodoutput[4];           	// config for output ports 				1-0-0-0-0-0-0-0 = 0x80
	uint8_t  swin[4];                 	// low 4 bits form portAddress (universe part)
	uint8_t  swout[4];                	// low 4 bits form portAddress (universe part)
	uint8_t  swvideo;                 	// select video							not used = 0
	uint8_t  swmacro;                 	// macro								not used = 0
	uint8_t  swremote;                	// trigger								not used = 0
	uint8_t  spare[3];                	// not used, set to 0
	uint8_t  style;						// 0x00 = StNode, 0x01 = StController
	uint8_t  mac[6];                  	// MAC address of this device
	uint8_t  bindip[4];               	// IP of root device
	uint8_t  bindindex;               	// 1 - root device
	uint8_t  status2;                 	// config
	uint8_t  filler[26];              	// empty space
};

struct artnet_IPreply {
	uint8_t  id[8];                 	// "Art-Net "
	uint16_t opCode;                  	// ART_IP_PROG_REPLY
	uint16_t protVersion;				// 0x000e = 14
	uint8_t	 filler[4];					// unused
	uint8_t  ip[4];						// my device IP, most significant byte is transmitted first
	uint8_t  subnet[4];					// my device subnet, most significant byte is transmitted first
	uint16_t progPort;					// deprecated
	uint8_t  status;					// 0-0-000000 = 0x00 (unused - no DHCP - unused)
	uint8_t  spare[8];					// not used, set to 0
};

struct port_address {					// 0b0NNNNNNNSSSSUUUU = 0xNNSU ; where N - NET, S - SubNet, U - universe
	uint16_t NNSU;
	uint8_t  NN;
	uint8_t  S;
	uint8_t  U;
};



// function declarations
// ---------------------
void set_IP( uint8_t octet1, uint8_t octet2, uint8_t octet3, uint8_t octet4 );
void set_MAC( uint8_t oui1, uint8_t oui2, uint8_t oui3, uint8_t nic1, uint8_t nic2, uint8_t nic3 );
void set_destIP( uint8_t octet1, uint8_t octet2, uint8_t octet3, uint8_t octet4 );
void ClassA_network();
uint8_t* get( NODE_network );

void ArtNet_beginETH( DHCP_mode );
void ArtNet_beginWiFi();
void ArtNet_stop();

uint16_t ArtNet_read();
void ArtNet_discover();
void set_Port( NODE_port, uint16_t Universe );

void set_DMXcallback( void (*pointer_to_function)( uint16_t, uint8_t* ) );
void set_NZScallback( uint8_t triggToID, void (*pointer_to_function1)( uint8_t* ) );
void ArtNet_write_DMX( uint8_t* DmxFrame );
uint8_t ArtNet_direct_DMX( uint8_t* DmxFrame );
uint8_t ArtNet_write_nzs( uint8_t stamp, uint8_t frame_ID, uint8_t nzsFrame[] );
uint8_t* get_DmxFrame();
uint16_t get_Universe();



#endif
