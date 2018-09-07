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
		json[PAIRED_DEVICES_FLASH_SIZE]=0x00;

	}else{
		json[0] = 0x7B;//{
		json[1] = 0x7D;//}
		json[2] = 0x00;//null
	}
	
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

	if (package["body"]["reverse_pairing_secret"].is<char*>()&&package["body"]["device_name"].is<char*>()){
		char correspondentAddress[34];
	//	getDeviceAddress(bufferForPackageReceived.senderPubkey,correspondentAddress);

		if (package["device_hub"].is<char*>()){
			acknowledgePairingRequest(bufferForPackageReceived.senderPubkey, package["device_hub"], package["body"]["reverse_pairing_secret"]);
			savePeerInFlash(bufferForPackageReceived.senderPubkey, package["device_hub"], package["body"]["device_name"]);
			//	connectSecondaryWebsocket();
		} else {
#ifdef DEBUG_PRINT
			Serial.println(F("device_hub and device_name must be a char"));
#endif
		}
	
	}else{
#ifdef DEBUG_PRINT
			Serial.println(F("Reverse secret must be a char"));
#endif
	}

}



void acknowledgePairingRequest(char senderPubkey [45],const char * deviceHub, const char * reversePairingSecret){

	char output[150];
	StaticJsonBuffer<400> jsonBuffer;
	JsonObject & message = jsonBuffer.createObject();
	JsonObject & body = jsonBuffer.createObject();
	message["from"] = byteduino_device.deviceAddress;
	message["device_hub"] = "byteball.org/bb-test";
	message["subject"] = "pairing";

	body["pairing_secret"] = reversePairingSecret;
	body["device_name"] = byteduino_device.deviceName;
	message["body"]= body;
	bufferForPackageSent.isRecipientTempMessengerKeyKnown = false;
	memcpy(bufferForPackageSent.recipientPubkey,senderPubkey,45);
	memcpy(bufferForPackageSent.recipientHub,deviceHub,strlen(deviceHub));
	bufferForPackageSent.isFree = false;
	bufferForPackageSent.isRecipientKeyRequested = false;
	message.printTo(bufferForPackageSent.message);
#ifdef DEBUG_PRINT
	Serial.println(bufferForPackageSent.message);
#endif
}