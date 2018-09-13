// Byteduino lib - papabyte.com
// MIT License
#include "pairing.h"

extern Byteduino byteduino_device;
extern bufferPackageReceived bufferForPackageReceived;
extern bufferPackageSent bufferForPackageSent;

void readPairedDevicesJson(char * json){
	const char firstChar = EEPROM.read(PAIRED_DEVICES);
	if (firstChar == 0x7B){
		int i = -1; 
		do {
			i++;
			json[i] = EEPROM.read(PAIRED_DEVICES+i);
		}
		while (json[i] != 0x00 && i < (PAIRED_DEVICES_FLASH_SIZE));
		json[PAIRED_DEVICES_FLASH_SIZE-1]=0x00;

	}else{
		json[0] = 0x7B;//{
		json[1] = 0x7D;//}
		json[2] = 0x00;//null
	}
	
}

String getDevicesJsonString(){
	const char firstChar = EEPROM.read(PAIRED_DEVICES);
	char lastCharRead;
	String returnedString;

	if (firstChar == 0x7B){
		int i = -1; 
		do {
			i++;
			lastCharRead = EEPROM.read(PAIRED_DEVICES+i);
			if (lastCharRead!= 0x00)
				returnedString += lastCharRead;
		}
		while (lastCharRead != 0x00 && i < (PAIRED_DEVICES_FLASH_SIZE));

	}else{
		returnedString += 0x7B;
		returnedString += 0x7D;
	}
	return returnedString;
}

void savePeerInFlash(char peerPubkey[45],const char * peerHub, const char * peerName){
	char output[PAIRED_DEVICES_FLASH_SIZE];
	char input[PAIRED_DEVICES_FLASH_SIZE];
	DynamicJsonBuffer jb((int) PAIRED_DEVICES_FLASH_SIZE*0.65); //this JSON needs a buffer of around x0.65 the size of its raw size
	readPairedDevicesJson(input);
#ifdef DEBUG_PRINT
	Serial.println(input);
#endif
	JsonObject& objectPeers = jb.parseObject(input);
	if (objectPeers.success()){
		DynamicJsonBuffer jb2(250);
		JsonObject& objectPeer = jb2.createObject();

		objectPeer["hub"] = peerHub;
		objectPeer["name"] = peerName;

		objectPeers[peerPubkey] = objectPeer;

		if (objectPeers.measureLength() < PAIRED_DEVICES_FLASH_SIZE){
			objectPeers.printTo(output);
#ifdef DEBUG_PRINT
			Serial.println(F("Save peer in flash"));
			Serial.println(output);
#endif
		} else {
			return;
#ifdef DEBUG_PRINT
		Serial.println(F("No flash available to store peer"));
#endif
		}
		int i = -1; 
		do {
			i++;
			EEPROM.write(PAIRED_DEVICES+i, output[i]);
		}
		while (output[i]!= 0x00 && i < (PAIRED_DEVICES+PAIRED_DEVICES_FLASH_SIZE));
		EEPROM.commit();
	} else{
#ifdef DEBUG_PRINT
	Serial.println(F("Impossible to parse objectPeers"));
#endif
	}
}


void handlePairingRequest(JsonObject& package){
	const char * reverse_pairing_secret = package["body"]["reverse_pairing_secret"];
	const char * device_hub = package["device_hub"];
	const char * device_name = package["body"]["device_name"];
	if (reverse_pairing_secret != nullptr){
		if(strlen(reverse_pairing_secret) < MAX_PAIRING_SECRET_STRING_SIZE){
			if (device_hub != nullptr){
				if(strlen(device_hub) < MAX_HUB_STRING_SIZE){
					if (device_name != nullptr){
						if(strlen(device_name) < MAX_DEVICE_NAME_STRING_SIZE){
							acknowledgePairingRequest(bufferForPackageReceived.senderPubkey, device_hub, reverse_pairing_secret);
							savePeerInFlash(bufferForPackageReceived.senderPubkey, device_hub, device_name);
						} else {
#ifdef DEBUG_PRINT
							Serial.println(F("device_hub is too long"));
#endif
						}
					} else {
#ifdef DEBUG_PRINT
						Serial.println(F("device_name must be a char"));
#endif
						}
				} else {
#ifdef DEBUG_PRINT
					Serial.println(F("device_hub is too long"));
#endif
				}
			} else {
#ifdef DEBUG_PRINT
				Serial.println(F("device_hub must be a char"));
#endif
			}
		}else {
#ifdef DEBUG_PRINT
			Serial.println(F("reverse_pairing_secret is too long"));
#endif
		}

	} else {
#ifdef DEBUG_PRINT
		Serial.println(F("reverse_pairing_secret must be a char"));
#endif
	}

}


void acknowledgePairingRequest(char recipientPubKey [45],const char * recipientHub, const char * reversePairingSecret){

	char output[130 + MAX_HUB_STRING_SIZE + MAX_DEVICE_NAME_STRING_SIZE + MAX_PAIRING_SECRET_STRING_SIZE];
	const size_t bufferSize = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4);
	StaticJsonBuffer<bufferSize> jsonBuffer;
	JsonObject & message = jsonBuffer.createObject();
	JsonObject & body = jsonBuffer.createObject();
	message["from"] = (const char *) byteduino_device.deviceAddress;
	message["device_hub"] = (const char *) byteduino_device.hub;
	message["subject"] = "pairing";

	body["pairing_secret"] = reversePairingSecret;
	body["device_name"] = (const char *) byteduino_device.deviceName;
	message["body"]= body;

	message.printTo(bufferForPackageSent.message);
	loadBufferPackageSent(recipientPubKey, recipientHub);

#ifdef DEBUG_PRINT
	Serial.println(bufferForPackageSent.message);
#endif
}