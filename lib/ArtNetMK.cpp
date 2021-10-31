/* Mārtiņš Kļaviņš */

#include <FingerWing_config.h>
#include <ArtNetMK.h>


const uint8_t socketCount = 4;
/* MAX_SOCK_NUM */
/* check ethernet.h for socket count and buffer sizes */
#if defined(ARDUINO_SAMD_ZERO) || defined(ESP8266) || defined(ESP32)
    WiFiUDP Udp;
	WiFiUDP udp_slave1;
	WiFiUDP udp_slave2;
	WiFiUDP udp_slave3;
	WiFiUDP* ptr_to_UDP_objects[] = { &Udp, &udp_slave1, &udp_slave2, &udp_slave3 };
#else
	EthernetUDP Udp;
	EthernetUDP udp_slave1;
	EthernetUDP udp_slave2;
	EthernetUDP udp_slave3;
	EthernetUDP* ptr_to_UDP_objects[] = { &Udp, &udp_slave1, &udp_slave2, &udp_slave3 };
#endif



// definitions, declarations ---------------------------------------------------------------------------------

// Art-Net variables
// -----------------
uint8_t artnetPacket[MAX_BUFFER_ARTNET];
uint16_t packetSize;							// byte count in packet
uint16_t opcode;
uint16_t protVersion;
uint8_t sequence;
uint8_t physical;
uint16_t incomingUniverse;
uint16_t dmxDataLength;
uint8_t payload_id;
uint8_t select_id;
uint8_t dmxCallback = 0;
// network variables
// -----------------
uint8_t IPaddress[4];
uint8_t BCaddress[4];							// directed broadcast
uint8_t MACaddress[6];
uint8_t Subnet_Mask[4];
uint8_t local_ip[4];							// device IP, important if IP is assigned with DHCP, not set_IP()
uint8_t remote_ip[4];							// Source IP, who sends ArtNet
uint8_t dest_ip[4];								// destination IP, for unicast mode
uint32_t addressPoll[socketCount] = {0};


typedef enum {
	BROADCAST,
	NETMASK
} NET_address;

struct artnet_reply ArtPollReply;
struct artnet_dmx ArtDmx;
struct artnet_poll ArtPoll;
struct artnet_nzs ArtNzs;
struct port_address PortAddress;
struct artnet_IPreply ArtIpReply;

void (*DMX_frame)( uint16_t, uint8_t* );
void (*NZS_frame)( uint8_t, uint16_t, uint8_t* );
void set_Broadcast( uint8_t octet1, uint8_t octet2, uint8_t octet3, uint8_t octet4 ){
	BCaddress[0] = octet1;
	BCaddress[1] = octet2;
	BCaddress[2] = octet3;
	BCaddress[3] = octet4;
}
void set_Subnet( uint8_t octet1, uint8_t octet2, uint8_t octet3, uint8_t octet4 ){
	Subnet_Mask[0] = octet1;
	Subnet_Mask[1] = octet2;
	Subnet_Mask[2] = octet3;
	Subnet_Mask[3] = octet4;
}
void netCLASS_config( NET_address config, void (*ptr_to_userFunction)( uint8_t, uint8_t, uint8_t, uint8_t ) ){
	uint8_t netID = local_ip[0];
	if ( config == BROADCAST ){
		if ( CLASS_A(netID) ) (*ptr_to_userFunction)( local_ip[0], 255, 255, 255 );
    	else if ( CLASS_B(netID) ) (*ptr_to_userFunction)( local_ip[0], local_ip[1], 255, 255 );
    	else if ( CLASS_C(netID) ) (*ptr_to_userFunction)( local_ip[0], local_ip[1], local_ip[2], 255 );
	}
	if ( config == NETMASK ){
		if ( CLASS_A(netID) ) (*ptr_to_userFunction)( 255, 0, 0, 0 );
    	else if ( CLASS_B(netID) ) (*ptr_to_userFunction)( 255, 255, 0, 0 );
    	else if ( CLASS_C(netID) ) (*ptr_to_userFunction)( 255, 255, 255, 0 );
	}
}


// functions -------------------- Network Setup --------------------------------------------------------------

void set_IP( uint8_t octet1, uint8_t octet2, uint8_t octet3, uint8_t octet4 ){
	IPaddress[0] = octet1;
	IPaddress[1] = octet2;
	IPaddress[2] = octet3;
	IPaddress[3] = octet4;
}

void set_MAC( uint8_t oui1, uint8_t oui2, uint8_t oui3, uint8_t nic1, uint8_t nic2, uint8_t nic3 ){
	MACaddress[0] = oui1;
	MACaddress[1] = oui2;
	MACaddress[2] = oui3;
	MACaddress[3] = nic1;
	MACaddress[4] = nic2;
	MACaddress[5] = nic3;
}

void set_destIP( uint8_t octet1, uint8_t octet2, uint8_t octet3, uint8_t octet4 ){
	dest_ip[0] = octet1;
	dest_ip[1] = octet2;
	dest_ip[2] = octet3;
	dest_ip[3] = octet4;
}

void ClassA_network(){
	uint16_t octet2 = MACaddress[3] + (ESTA >> 8) + (uint8_t)ESTA;
	IPaddress[0] = 2;
	IPaddress[1] = (uint8_t)octet2;
	IPaddress[2] = MACaddress[4];
	IPaddress[3] = MACaddress[5];
}

uint8_t* get( NODE_network find ){
	if ( find == myIP ) return IPaddress;
	else if ( find == mySUBNET ) return Subnet_Mask;
	else if ( find == hostIP ) return remote_ip;

	return NULL;
}


#ifdef _kind_of_ARDUINO

	void ArtNet_beginETH( DHCP_mode config ){
		if ( config == DHCP ){
			/* to make sure the DHCP lease is renewed, call Ethernet.maintain() regularly */
			Ethernet.begin(MACaddress);
		}
		else {
			Ethernet.begin(MACaddress, IPaddress);
		}
		
		local_ip[0] = Ethernet.localIP()[0];
		local_ip[1] = Ethernet.localIP()[1];
		local_ip[2] = Ethernet.localIP()[2];
		local_ip[3] = Ethernet.localIP()[3];

		Udp.begin(ART_NET_PORT);
		if ( STYLE == 1 ){
			udp_slave1.begin(ART_NET_PORT);
			udp_slave2.begin(ART_NET_PORT);
			udp_slave3.begin(ART_NET_PORT);
		}

		netCLASS_config(BROADCAST, &set_Broadcast);
		netCLASS_config(NETMASK, &set_Subnet);
		Ethernet.setSubnetMask(Subnet_Mask);
	}

#endif

void ArtNet_beginWiFi(){
	#if defined(ARDUINO_SAMD_ZERO) || defined(ESP8266) || defined(ESP32)
		WiFi.macAddress(MACaddress);
		local_ip[0] = WiFi.localIP()[0];
		local_ip[1] = WiFi.localIP()[1];
		local_ip[2] = WiFi.localIP()[2];
		local_ip[3] = WiFi.localIP()[3];
	#endif
			
	Udp.begin(ART_NET_PORT);
}



// functions -------------------- Art-Net --------------------------------------------------------------------
/* returns OpCode from received ArtNet packet or 0 if its not ArtNet.										*/
	
void set_Port( NODE_port selected, uint16_t Universe ){
	PortAddress.NNSU	= Universe;
	PortAddress.NN		= (Universe >> 8) & 0x7F;
	PortAddress.S		= ((uint8_t)Universe >> 4) & 0x0F; 
	PortAddress.U		= (uint8_t)Universe & 0x0F;
		
	for( uint8_t i = 0; i < 8; i++ ){
		ArtPollReply.id[i]  	= ART_NET_ID[i];
	}
	ArtPollReply.opCode 		= ART_POLL_REPLY;
	for( uint8_t i = 0; i < 4; i++ ){
		ArtPollReply.ip[i]  	= local_ip[i];
	}
	ArtPollReply.port       	= ART_NET_PORT;
	ArtPollReply.verH	       	= VERSION >> 8;
	ArtPollReply.ver			= (uint8_t)VERSION;
	ArtPollReply.net	       	= PortAddress.NN;
	ArtPollReply.sub        	= PortAddress.S;
	ArtPollReply.oemH       	= OEM >> 8;
	ArtPollReply.oem        	= (uint8_t)OEM;
	ArtPollReply.ubea       	= 0;
	ArtPollReply.status     	= STATUS1;
	ArtPollReply.etsa 			= ESTA;
	memcpy( ArtPollReply.shortname, SHORT_NAME, sizeof(SHORT_NAME) );
	memcpy( ArtPollReply.longname, LONG_NAME, sizeof(LONG_NAME) );
	ArtPollReply.nodereport[0] 	= 0;
	ArtPollReply.numbportsH 	= PORTSc >> 8;
	ArtPollReply.numbports  	= (uint8_t)PORTSc;
	// --- port
	ArtPollReply.porttypes[selected] 	= PORT_1;
	ArtPollReply.goodinput[selected] 	= 0x80;
	ArtPollReply.goodoutput[selected] 	= 0x80;
	ArtPollReply.swin[selected]      	= PortAddress.U;
	ArtPollReply.swout[selected]     	= PortAddress.U;
	// ---
	ArtPollReply.swvideo    	= 0;
	ArtPollReply.swmacro    	= 0;
	ArtPollReply.swremote   	= 0;
	ArtPollReply.style      	= STYLE;
	for( uint8_t i = 0; i < 6; i++ ){
		ArtPollReply.mac[i] 	= MACaddress[i];
	}
	for( uint8_t i = 0; i < 4; i++ ){
		ArtPollReply.bindip[i] 	= local_ip[i];
	}
	ArtPollReply.bindindex		= 1;
	ArtPollReply.status2		= STATUS2;
}
uint16_t ArtNet_read(){
	packetSize = Udp.parsePacket();
	remote_ip[0] = Udp.remoteIP()[0];
	remote_ip[1] = Udp.remoteIP()[1];
	remote_ip[2] = Udp.remoteIP()[2];
	remote_ip[3] = Udp.remoteIP()[3];
		
	if ( (packetSize <= MAX_BUFFER_ARTNET) && (packetSize > 0) ){
		
		Udp.read(artnetPacket, MAX_BUFFER_ARTNET);
		// Check that packetID is "Art-Net" else ignore
		for ( uint8_t i = 0 ; i < 8 ; i++ ){
			if ( artnetPacket[i] != ART_NET_ID[i] ) return 0;
		}
		/* precedence - first goes 9th packet bitshift and then OR	*/
		/*         xxxxxxxx											*/
		/* yyyyyyyy00000000											*/
		opcode = artnetPacket[8] | (artnetPacket[9] << 8);
		protVersion = (artnetPacket[10] << 8) | artnetPacket[11];

		if ( opcode == ART_DMX && en_read_DMX ){
			sequence = artnetPacket[12];
			physical = artnetPacket[13];
			incomingUniverse = artnetPacket[14] | (artnetPacket[15] << 8);
			dmxDataLength = artnetPacket[17] | (artnetPacket[16] << 8);
			
			if ( dmxCallback && (incomingUniverse == PortAddress.NNSU) ){
				// calling user function through explicit dereference
				(*DMX_frame)( dmxDataLength, artnetPacket + ART_DMX_START );
			}
			
			return ART_DMX;
		}
		if ( opcode == ART_POLL && en_reply_msg ){
			if ( 	CLASS_A(local_ip[0]) == CLASS_A(remote_ip[0]) ||
					CLASS_B(local_ip[0]) == CLASS_B(remote_ip[0]) ||
					CLASS_C(local_ip[0]) == CLASS_C(remote_ip[0])	){
				/* broadcast packet to all hosts on a defined network (makes it discoverable outside LAN) */
				Udp.beginPacket(BCaddress, ART_NET_PORT);
				Udp.write((uint8_t *)&ArtPollReply, sizeof(ArtPollReply));
				Udp.endPacket();
			}
			else {
				/* broadcast packet in LAN (makes it discoverable regardless to IP) */
				Udp.beginPacket(limited_BC, ART_NET_PORT);
				Udp.write((uint8_t *)&ArtPollReply, sizeof(ArtPollReply));
				Udp.endPacket();
			}

			return ART_POLL;
		}
		if ( opcode == ART_NZS ){
			sequence = artnetPacket[12];
			payload_id = artnetPacket[13];
			incomingUniverse = artnetPacket[14] | (artnetPacket[15] << 8);
			dmxDataLength = artnetPacket[17] | (artnetPacket[16] << 8);
				
			if ( payload_id == select_id ){
				(*NZS_frame)( sequence, dmxDataLength, artnetPacket + ART_DMX_START );
			}
			
			return ART_NZS;
		}
		if ( opcode == ART_ADDRESS ){
			/*
			If universe is updated, changes will not show in pollreply
			Updated IP will show, as device needs to be restared to reload network
			*/
			uint8_t en_artAddress = 0;
			uint8_t netswitch = artnetPacket[12] & 0x7F;
			uint8_t universe = artnetPacket[100];
			en_artAddress = universe & 0x80;
			if ( !en_artAddress ){
				return 0;
			}
			universe = universe & 0x7F;
			uint8_t subswitch = artnetPacket[104] << 4;
			// BindIndex, ShortName, LongName, Command are ignored
			incomingUniverse = (netswitch << 8) | (subswitch | universe);
			
			Udp.beginPacket(BCaddress, ART_NET_PORT);
			Udp.write((uint8_t *)&ArtPollReply, sizeof(ArtPollReply));
			Udp.endPacket();

			return ART_ADDRESS;
		}
		if ( opcode == ART_IP_PROG ){
			uint8_t command = artnetPacket[14];
			if ( command == 0x84 ){
				IPaddress[0] = artnetPacket[16];
				IPaddress[1] = artnetPacket[17];
				IPaddress[2] = artnetPacket[18];
				IPaddress[3] = artnetPacket[19];

				return ART_IP_PROG;
			}
			if ( command == 0 ){
				for ( uint8_t i = 0; i < 8; i++ ){
					ArtIpReply.id[i]  	= ART_NET_ID[i];
				}
				ArtIpReply.opCode 		= ART_IP_PROG_REPLY;
				ArtIpReply.protVersion 	= prot_version << 8;
				for ( uint8_t i = 0; i < 4; i++ ){
					ArtIpReply.ip[i] 	= local_ip[i];
				}
				for ( uint8_t i = 0; i < 4; i++ ){
					ArtIpReply.subnet[i] = Subnet_Mask[i];
				}
				ArtIpReply.status		= 0;

				Udp.beginPacket(remote_ip, ART_NET_PORT);
				Udp.write((uint8_t *)&ArtIpReply, sizeof(ArtIpReply));
				Udp.endPacket();
			}

			return 0;
		}
		if ( opcode == ART_POLL_REPLY && STYLE == 1 ){
			uint8_t empty = 0;
			uint32_t IP;
				
			IP = remote_ip[0];
			IP = (IP << 8) | remote_ip[1];
			IP = (IP << 8) | remote_ip[2];
			IP = (IP << 8) | remote_ip[3];
				
			for ( uint8_t i = 0; i < socketCount; i++ ){
				/* stop if IP already exist */
				if ( addressPoll[i] == IP ) break;
				/* renember position if empty space */
				if ( addressPoll[i] == 0 ) empty = i + 1;			// i + 1 is on purpose, even if 0th possition is empty, variable 'empty' will be true
				/* at the end of loop, write IP in empty space */
				if ( i == (socketCount-1) && empty ) addressPoll[empty-1] = IP;
			}
			
			return ART_POLL_REPLY;
		}
			
	}
	
	return 0;
}
void ArtNet_write_DMX( uint8_t* DmxFrame ){
	for( uint8_t i = 0; i < 8; i++ ){
		ArtDmx.id[i]	= ART_NET_ID[i];
	}
	ArtDmx.opCode		= ART_DMX;
	ArtDmx.protVersion 	= prot_version << 8;
	ArtDmx.sequence 	= 0;
	ArtDmx.physical 	= 0;
	ArtDmx.portAddress 	= PortAddress.NNSU;
	ArtDmx.dmxDataLenght = ((uint8_t)DMX_CHANNELS << 8) | (DMX_CHANNELS >> 8);
	for ( uint16_t i = 0; i < DMX_CHANNELS; i++ ){
		ArtDmx.dmxData[i] = *(DmxFrame+i);
	}
	
	for ( uint8_t i = 0; i < socketCount; i++ ){
		if ( addressPoll[i] == 0 ) continue;
		
		uint8_t IP[4];
		IP[0] = addressPoll[i] >> 24;
		IP[1] = addressPoll[i] >> 16;
		IP[2] = addressPoll[i] >> 8;
		IP[3] = addressPoll[i];

		ptr_to_UDP_objects[i]->beginPacket(IP, ART_NET_PORT);
		ptr_to_UDP_objects[i]->write((uint8_t *)&ArtDmx, sizeof(ArtDmx));
		uint8_t ok = ptr_to_UDP_objects[i]->endPacket();
		if ( !ok ) addressPoll[i] = 0;
	}
}
uint8_t ArtNet_direct_DMX( uint8_t* DmxFrame ){
	for ( uint8_t i = 0; i < 8; i++ ){
		ArtDmx.id[i]	= ART_NET_ID[i];
	}
	ArtDmx.opCode		= ART_DMX;
	ArtDmx.protVersion 	= prot_version << 8;
	ArtDmx.sequence 	= 0;
	ArtDmx.physical 	= 0;
	ArtDmx.portAddress 	= PortAddress.NNSU;
	ArtDmx.dmxDataLenght = ((uint8_t)DMX_CHANNELS << 8) | (DMX_CHANNELS >> 8);
	for ( uint16_t i = 0; i < DMX_CHANNELS; i++ ){
		ArtDmx.dmxData[i] = *(DmxFrame+i);
	}
		
	uint8_t ok = Udp.beginPacket(dest_ip, ART_NET_PORT);
	if ( ok ){
		Udp.write((uint8_t *)&ArtDmx, sizeof(ArtDmx));
		ok = Udp.endPacket();
	}
		
	return ok;
}
uint8_t ArtNet_write_nzs( uint8_t stamp, uint8_t frame_ID, uint8_t nzsFrame[PAYLOAD] ){
	memcpy( ArtNzs.id, ART_NET_ID, sizeof(ART_NET_ID) );
	ArtNzs.opCode		= ART_NZS;
	ArtNzs.protVersion 	= prot_version << 8;
	ArtNzs.sequence 	= stamp;
	ArtNzs.startCode 	= frame_ID;
	ArtNzs.portAddress 	= 0;
	ArtNzs.nzsDataLenght = PAYLOAD << 8;
	for( uint8_t i = 0; i < PAYLOAD; i++ ){
		ArtNzs.nzsData[i] = nzsFrame[i];
	}

	Udp.beginPacket(dest_ip, ART_NET_PORT);
	Udp.write((uint8_t *)&ArtNzs, sizeof(ArtNzs));
	Udp.endPacket();
		
	return 0;
}
void ArtNet_discover(){
	if ( STYLE == 1){
		for ( uint8_t i = 0; i < 8; i++ ){
			ArtPoll.id[i]	= ART_NET_ID[i];
		}
		ArtPoll.opCode		= ART_POLL;
		ArtPoll.protVersion = prot_version << 8;
		ArtPoll.talkToMe	= discoveryCONFIG;
		ArtPoll.priority	= 0;
		
		Udp.beginPacket(limited_BC, ART_NET_PORT);
		Udp.write((uint8_t *)&ArtPoll, sizeof(ArtPoll));
		Udp.endPacket();
			
		udp_slave1.beginPacket(limited_BC, ART_NET_PORT);
		udp_slave1.write((uint8_t *)&ArtPollReply, sizeof(ArtPollReply));
		udp_slave1.endPacket();
	}
}

// returns a pointer to the start of the DMX data
uint8_t* get_DmxFrame(){
	/* returns artnetPacket array first element + offset = ART_DMX_START	*/
    return artnetPacket + ART_DMX_START;
}

uint16_t get_Universe(){
    return incomingUniverse;
}

// setup function for calling another function at DMX data arrival
void set_DMXcallback( void (*pointer_to_function)( uint16_t, uint8_t* ) ){
	DMX_frame = pointer_to_function;			// asigning DMX_frame to pointer_to_function's address
	dmxCallback = 1;
}

void set_NZScallback( uint8_t triggToID, void (*pointer_to_function1)( uint8_t, uint16_t, uint8_t* ) ){
	select_id = triggToID;
	NZS_frame = pointer_to_function1;
}

void ArtNet_stop(){
	Udp.stop();
	if ( STYLE == 1){
		udp_slave1.stop();
		udp_slave2.stop();
		udp_slave3.stop();
	}
}
