// Byteduino lib - papabyte.com
// MIT License
#include <byteduino.h>


extern "C" {
#include "user_interface.h"
}

WebSocketsClient webSocketForHub;
//WebSocketsClient secondaryWebSocket;

os_timer_t baseTimer;
bool baseTickOccured = false;
byte job2Seconds = 0;
bufferPackageReceived bufferForPackageReceived;
bufferPackageSent bufferForPackageSent;
Byteduino byteduino_device; 


void setHub(const char * hub){
	if(strlen(hub) < MAX_HUB_STRING_SIZE){
		strcpy(byteduino_device.hub,hub);
	}
}

void setDeviceName(const char * deviceName){
	if(strlen(deviceName) < MAX_DEVICE_NAME_STRING_SIZE){
		strcpy(byteduino_device.deviceName, deviceName);
	}
}

void setExtPubKey(const char * extPubKey){
	if(strlen(extPubKey) == 111){
		strcpy(byteduino_device.keys.extPubKey, extPubKey);
	}
}

void setPrivateKeyM1(const char * privKeyB64){
	decodeAndCopyPrivateKey(byteduino_device.keys.privateM1, privKeyB64);
}

void setPrivateKeyM4400(const char * privKeyB64){
	decodeAndCopyPrivateKey(byteduino_device.keys.privateM4400, privKeyB64);
}


void byteduino_init (){

	//calculate device pub key
	getCompressAndEncodePubKey(byteduino_device.keys.privateM1, byteduino_device.keys.publicKeyM1b64);
	
	//calculate wallet pub key
	getCompressAndEncodePubKey(byteduino_device.keys.privateM4400, byteduino_device.keys.publicKeyM4400b64);

	//determine device address
	getDeviceAddress(byteduino_device.keys.publicKeyM1b64, byteduino_device.deviceAddress);
	
	//send device infos to serial
	printDeviceInfos();
	
	//start websocket
	webSocketForHub.beginSSL(getDomain(byteduino_device.hub), byteduino_device.port, getPath(byteduino_device.hub));
	webSocketForHub.onEvent(webSocketEvent);
	
	//set up base timer
	os_timer_setfn(&baseTimer, timerCallback, NULL);
	os_timer_arm(&baseTimer, 10, true);
	EEPROM.begin(TOTAL_USED_FLASH);
	
	uECC_set_rng(&getRandomNumbersForUecc);
	
	byteduino_device.isInitialized = true;

	//	secondaryWebSocket.beginSSL(byteduino_device.hub, 443,  "/bb-test");
	//secondaryWebSocket.onEvent(secondaryWebSocketEvent);
}

void printDeviceInfos(){
	Serial.println("Device address: ");
	Serial.println(byteduino_device.deviceAddress);
	Serial.println("Device name: ");
	Serial.println(byteduino_device.deviceName);
	Serial.println("Pairing code: ");
	Serial.println(byteduino_device.keys.publicKeyM1b64);
	Serial.println("@");
	Serial.println(byteduino_device.hub);
	Serial.println("#0000");
	Serial.println("Extended Pub Key:");
	Serial.println(byteduino_device.keys.extPubKey);
}


void getDeviceInfosJson(char * json){
	const size_t bufferSize = JSON_OBJECT_SIZE(4);
	StaticJsonBuffer<bufferSize> jsonBuffer;
	JsonObject & mainObject = jsonBuffer.createObject();

	mainObject["device_address"] = (const char *) byteduino_device.deviceAddress;
	mainObject["device_hub"] = (const char *) byteduino_device.hub;
	mainObject["device_pubkey"] =(const char *) byteduino_device.keys.publicKeyM1b64;
	mainObject["extended_pubkey"] =(const char *) byteduino_device.keys.extPubKey;
	char output[300];
	mainObject.printTo(output);
	strcpy(json,output);
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

	switch (type) {
		case WStype_DISCONNECTED:
			byteduino_device.isConnected = false;
			Serial.println(F("Wss disconnected\n"));
		break;
		case WStype_CONNECTED:
		{
			byteduino_device.isConnected = true;
			Serial.printf("Wss connected to: %s\n", payload);
		}
		break;
		case WStype_TEXT:
			respondToHub(payload);
	break;
  }
}

void byteduino_loop(){
	
	webSocketForHub.loop();
	yield();
	//secondaryWebSocket.loop();
	
	if (byteduino_device.isInitialized){
		treatReceivedPackage();
		treatNewWalletCreation();
		treatWaitingSignature();
	}
	
	if (!bufferForPackageSent.isFree && !bufferForPackageSent.isRecipientKeyRequested){
		requestRecipientMessengerTempKey();
	}
	
	if (!bufferForPackageSent.isFree && bufferForPackageSent.isRecipientTempMessengerKeyKnown){
		encryptAndSendPackage();
		yield();
	}
	
	if (baseTickOccured == true) {
		job2Seconds++;
		if (job2Seconds == 200){
			if (byteduino_device.isConnected)
				sendHeartbeat();
				job2Seconds = 0;
		}
		baseTickOccured = false;
	}
	
	updateRandomPool();

	if (!byteduino_device.isInitialized && isRandomGeneratorReady()){
		byteduino_init ();
	}
	
	if (byteduino_device.isMessengerKeyTobeRotated && byteduino_device.isAuthenticated){
		byteduino_device.isMessengerKeyTobeRotated = false;
		setAndSendNewMessengerKey();
	}
	
}

void timerCallback(void * pArg) {
	baseTickOccured = true;
}

