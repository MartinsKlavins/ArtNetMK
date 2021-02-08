/* Mārtiņš Kļaviņš */

#include <config.h>
#include <ArtNetMK.h>



#if defined(ARDUINO_SAMD_ZERO) || defined(ESP8266) || defined(ESP32)
    	WiFiUDP Udp;
	WiFiUDP udp_slave1;
	WiFiUDP udp_slave2;
	WiFiUDP udp_slave3;
#else
	EthernetUDP Udp;
	EthernetUDP udp_slave1;
	EthernetUDP udp_slave2;
	EthernetUDP udp_slave3;
#endif


// definitions, declarations ---------------------------------------------------------------------------------

uint32_t addressPoll[4] = {0};

struct artnet_reply ArtPollReply;
struct artnet_dmx ArtDmx;
struct artnet_poll ArtPoll;
struct artnet_nzs ArtNzs;
struct port_address PortAddress;

void (*DMX_frame)( uint16_t, uint8_t*);
void (*NZS_frame)( uint8_t, uint16_t, uint8_t*);


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

void set_Broadcast( uint8_t octet1, uint8_t octet2, uint8_t octet3, uint8_t octet4 ){
	BCaddress[0] = octet1;
	BCaddress[1] = octet2;
	BCaddress[2] = octet3;
	BCaddress[3] = octet4;
}

void set_destIP( uint8_t octet1, uint8_t octet2, uint8_t octet3, uint8_t octet4 ){
	dest_ip[0] = octet1;
	dest_ip[1] = octet2;
	dest_ip[2] = octet3;
	dest_ip[3] = octet4;
}

void ClassA_network( uint8_t IP ){
	CLASS_A_ip = true;
	// get hard-coded MAC from device and store it in MACaddress
	#if defined(ARDUINO_SAMD_ZERO) || defined(ESP8266) || defined(ESP32)
		WiFi.macAddress(MACaddress);
	#elif defined(WIZ550io_WITH_MACADDRESS)
		W5100.getMACAddress(MACaddress);
	#endif
	
	uint16_t octet2 = MACaddress[3] + (ESTA >> 8) + (uint8_t)ESTA;
	
	if ( IP ){
		IPaddress[0] = 2;
		IPaddress[1] = (uint8_t)octet2;
		IPaddress[2] = MACaddress[4];
		IPaddress[3] = MACaddress[5];
	}
	
	BCaddress[0] = 2;
	BCaddress[1] = 255;
	BCaddress[2] = 255;
	BCaddress[3] = 255;
	
	Subnet_Mask[0] = 255;
	Subnet_Mask[1] = 0;
	Subnet_Mask[2] = 0;
	Subnet_Mask[3] = 0;
	
}

uint8_t* get_myIP(){
	return IPaddress;
}



#ifdef _kind_of_ARDUINO

	void ArtNet_beginETH(){
		Ethernet.begin(MACaddress,IPaddress);
		
		local_ip[0] = Ethernet.localIP()[0];
		local_ip[1] = Ethernet.localIP()[1];
		local_ip[2] = Ethernet.localIP()[2];
		local_ip[3] = Ethernet.localIP()[3];

		Udp.begin(ART_NET_PORT);
		if ( STYLE == 1){
			udp_slave1.begin(ART_NET_PORT);
			udp_slave2.begin(ART_NET_PORT);
			udp_slave3.begin(ART_NET_PORT);
		}
		if ( CLASS_A_ip ){
			#ifdef all_WIZnets
				W5100.setSubnetMask(Subnet_Mask);		// for w5500
			#else
				Ethernet.setSubnetMask(Subnet_Mask);		// for w5100
			#endif
		}
	}
	
	void ArtNet_beginETH_dhcp(){
		// to make sure the DHCP lease is renewed, call Ethernet.maintain() regularly
		Ethernet.begin(MACaddress);
		
		local_ip[0] = Ethernet.localIP()[0];
		local_ip[1] = Ethernet.localIP()[1];
		local_ip[2] = Ethernet.localIP()[2];
		local_ip[3] = Ethernet.localIP()[3];

		Udp.begin(ART_NET_PORT);
	}

#endif

	void ArtNet_beginWiFi(){
		#if defined(ARDUINO_SAMD_ZERO) || defined(ESP8266) || defined(ESP32)
			local_ip[0] = WiFi.localIP()[0];
			local_ip[1] = WiFi.localIP()[1];
			local_ip[2] = WiFi.localIP()[2];
			local_ip[3] = WiFi.localIP()[3];
		#endif
			
		Udp.begin(ART_NET_PORT);
	}
	

// functions -------------------- Art-Net --------------------------------------------------------------------
/* returns OpCode from received ArtNet packet or 0 if its not ArtNet.										*/

	
	void set_Port( uint8_t port, uint16_t Universe ){
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
		ArtPollReply.porttypes[port] 	= 0x85;
		ArtPollReply.goodinput[port] 	= 0x80;
		ArtPollReply.goodoutput[port] 	= 0x80;
		ArtPollReply.swin[port]      	= PortAddress.U;
		ArtPollReply.swout[port]     	= PortAddress.U;
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
			for( uint8_t i = 0 ; i < 8 ; i++ ){
				if ( artnetPacket[i] != ART_NET_ID[i] ){
					return 0;
				}
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
				if ( remote_ip[0] < 255 ){
					// broadcast packet to all hosts on a defined network
					// (makes it discoverable outside LAN)
					Udp.beginPacket(BCaddress, ART_NET_PORT);
					Udp.write((uint8_t *)&ArtPollReply, sizeof(ArtPollReply));
					Udp.endPacket();
				}
				else {
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
				
				return ART_ADDRESS;
			}
			if ( opcode == ART_IP_PROG ){
				uint8_t command = artnetPacket[14];
				if ( command != 0x84 ){
					return 0;
				}
				IPaddress[0] = artnetPacket[16];
				IPaddress[1] = artnetPacket[17];
				IPaddress[2] = artnetPacket[18];
				IPaddress[3] = artnetPacket[19];
				
				return ART_IP_PROG;
			}
			if ( opcode == ART_POLL_REPLY && STYLE ){
				uint8_t empty = 0;
				uint32_t IP;
				
				IP = remote_ip[0];
				IP = (IP << 8) | remote_ip[1];
				IP = (IP << 8) | remote_ip[2];
				IP = (IP << 8) | remote_ip[3];
				
				for ( uint8_t i = 0; i < 4; i++ ){
					/* stop if IP already exist */
					if ( addressPoll[i] == IP ) break;
					/* renember position if empty space */
					if ( addressPoll[i] == 0 ) empty = i + 1;
					/* write IP in empty space */
					if ( i == 3 && empty ){
						addressPoll[empty-1] = IP;
						empty = 0;
					}
				}
				
				return ART_POLL_REPLY;
			}
			
		}
		else {
			return 0;
		}
	
	}
	void ArtNet_write_DMX( uint8_t DmxFrame[DMX_CHANNELS] ){
		for( uint8_t i = 0; i < 8; i++ ){
			ArtDmx.id[i]	= ART_NET_ID[i];
		}
		ArtDmx.opCode		= ART_DMX;
		ArtDmx.protVersion 	= prot_version << 8;
		ArtDmx.sequence 	= 0;
		ArtDmx.physical 	= 0;
		ArtDmx.portAddress 	= PortAddress.NNSU;
		ArtDmx.dmxDataLenght = DMX_CHANNELS << 8; 	// 0x0018 << 8 (legal until channel count smaller than 255)
		for( uint8_t i = 0; i < DMX_CHANNELS; i++ ){
			ArtDmx.dmxData[i] = DmxFrame[i];
		}
		
		uint8_t loop = 0;
		for( uint8_t i = 0; i < 4; i++ ){
			if ( addressPoll[i] == 0 ) continue;
			
			uint8_t IP[4];
			IP[0] = addressPoll[i] >> 24;
			IP[1] = addressPoll[i] >> 16;
			IP[2] = addressPoll[i] >> 8;
			IP[3] = addressPoll[i];
			uint8_t ok;
			switch(loop){
				case 0:
					Udp.beginPacket(IP, ART_NET_PORT);
					Udp.write((uint8_t *)&ArtDmx, sizeof(ArtDmx));
					ok = Udp.endPacket();
					if ( !ok ) addressPoll[i] = 0;
					break;
				case 1:
					udp_slave1.beginPacket(IP, ART_NET_PORT);
					udp_slave1.write((uint8_t *)&ArtDmx, sizeof(ArtDmx));
					ok = udp_slave1.endPacket();
					if ( !ok ) addressPoll[i] = 0;
					break;
				case 2:
					udp_slave2.beginPacket(IP, ART_NET_PORT);
					udp_slave2.write((uint8_t *)&ArtDmx, sizeof(ArtDmx));
					ok = udp_slave2.endPacket();
					if ( !ok ) addressPoll[i] = 0;
					break;
				case 3:
					udp_slave3.beginPacket(IP, ART_NET_PORT);
					udp_slave3.write((uint8_t *)&ArtDmx, sizeof(ArtDmx));
					ok = udp_slave3.endPacket();
					if ( !ok ) addressPoll[i] = 0;
					break;
				default:
					return;
			}
			loop++;	
		}
	}
	uint8_t ArtNet_direct_DMX( uint8_t DmxFrame[DMX_CHANNELS] ){
		for( uint8_t i = 0; i < 8; i++ ){
			ArtDmx.id[i]	= ART_NET_ID[i];
		}
		ArtDmx.opCode		= ART_DMX;
		ArtDmx.protVersion 	= prot_version << 8;
		ArtDmx.sequence 	= 0;
		ArtDmx.physical 	= 0;
		ArtDmx.portAddress 	= PortAddress.NNSU;
		ArtDmx.dmxDataLenght = DMX_CHANNELS << 8; 	// 0x0018 << 8 (legal until channel count smaller than 255)
		for( uint8_t i = 0; i < DMX_CHANNELS; i++ ){
			ArtDmx.dmxData[i] = DmxFrame[i];
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
		memcpy( ArtNzs.nzsData, nzsFrame, sizeof(nzsFrame) );
		
		Udp.beginPacket(dest_ip, ART_NET_PORT);
		Udp.write((uint8_t *)&ArtNzs, sizeof(ArtNzs));
		Udp.endPacket();
		
		return 0;
	}
	uint8_t ArtNet_discover(){
		for( uint8_t i = 0; i < 8; i++ ){
			ArtPoll.id[i]	= ART_NET_ID[i];
		}
		ArtPoll.opCode		= ART_POLL;
		ArtPoll.protVersion = prot_version << 8;
		ArtPoll.talkToMe	= discoveryCONFIG;
		ArtPoll.priority	= 0;
		
		// broadcast packet in LAN (makes it discoverable regardless to IP)
		Udp.beginPacket(limited_BC, ART_NET_PORT);
		Udp.write((uint8_t *)&ArtPoll, sizeof(ArtPoll));
		Udp.endPacket();
        
		Udp.beginPacket(limited_BC, ART_NET_PORT);
		Udp.write((uint8_t *)&ArtPollReply, sizeof(ArtPollReply));
		Udp.endPacket();
		
		return 0;
	}
	


// returns a pointer to the start of the DMX data
uint8_t* get_DmxFrame(){
	/* returns artnetPacket array first element + offset = ART_DMX_START	*/
    return artnetPacket + ART_DMX_START;
}

// returns incomingUniverse
uint16_t get_Universe(){
    return incomingUniverse;
}

uint8_t* get_hostIP(){
	return remote_ip;
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
	CLASS_A_ip = false;
	Subnet_Mask[0] = 255;
	Subnet_Mask[1] = 255;
	Subnet_Mask[2] = 255;
	Subnet_Mask[3] = 0;
	
#ifdef _kind_of_ARDUINO
	#ifdef all_WIZnets
		W5100.setSubnetMask(Subnet_Mask);		// for w5500
	#else
		Ethernet.setSubnetMask(Subnet_Mask);		// for w5100
	#endif
#endif

	Udp.stop();
	if ( STYLE == 1){
		udp_slave1.stop();
		udp_slave2.stop();
		udp_slave3.stop();
	}
}