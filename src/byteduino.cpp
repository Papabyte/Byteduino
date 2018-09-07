// Byteduino lib - papabyte.com
// MIT License
#include <byteduino.h>


extern "C" {
#include "user_interface.h"
}

WebSocketsClient webSocketForHub;
//WebSocketsClient secondaryWebSocket;

bufferPackageReceived bufferForPackageReceived;
bufferPackageSent bufferForPackageSent;
os_timer_t baseTimer;
bool baseTickOccured = false;
byte job2Seconds = 0;
bool isByteduinoInitialized = false;
bool isMessengerKeyTobeRotated = true;
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
	Serial.println(getDeviceInfos());
	
	//start websocket
	webSocketForHub.beginSSL("byteball.org", byteduino_device.port, "/bb-test"); //tbd: get url and path from config
	webSocketForHub.onEvent(webSocketEvent);
	
	//set up base timer
	os_timer_setfn(&baseTimer, timerCallback, NULL);
	os_timer_arm(&baseTimer, 10, true);
	EEPROM.begin(TOTAL_USED_FLASH);
	
	uECC_set_rng(&getRandomNumbersForUecc);

	//	secondaryWebSocket.beginSSL(byteduino_device.hub, 443,  "/bb-test");
	//secondaryWebSocket.onEvent(secondaryWebSocketEvent);
}

String getDeviceInfos(){
	String returnedString = "Device address: ";
	returnedString.concat(byteduino_device.deviceAddress);
	returnedString.concat("\n");
	returnedString.concat("Device name: ");
	returnedString.concat(byteduino_device.deviceName);
	returnedString.concat("\n");
	returnedString.concat("Pairing code: ");
	returnedString.concat(byteduino_device.keys.publicKeyM1b64);
	returnedString.concat("@");
	returnedString.concat(byteduino_device.hub);
	returnedString.concat("#0000");
	returnedString.concat("\nExtended Pub Key:\n");
	returnedString.concat(byteduino_device.keys.extPubKey);
	return returnedString;
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
	
	if (isByteduinoInitialized){
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

	if (!isByteduinoInitialized && isRandomGeneratorReady()){
		isByteduinoInitialized = true;
		byteduino_init ();
	}
	
	if (isMessengerKeyTobeRotated && byteduino_device.isAuthenticated){
		isMessengerKeyTobeRotated = false;
		setAndSendNewMessengerKey();
	}
	
}

void timerCallback(void * pArg) {
	baseTickOccured = true;
}

