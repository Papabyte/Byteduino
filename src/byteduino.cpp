// Byteduino lib - papabyte.com
// MIT License
#include <byteduino.h>

#if defined(ESP8266)
extern "C" {
	#include "user_interface.h"
}
os_timer_t baseTimer;
#endif

#if defined(ESP32)
hw_timer_t * baseTimer = NULL;
hw_timer_t * watchdogTimer = NULL;
#endif

WebSocketsClient webSocketForHub;
#if !UNIQUE_WEBSOCKET
WebSocketsClient secondWebSocket;
#endif

static volatile bool baseTickOccured = false;
byte job2Seconds = 0;
bufferPackageReceived bufferForPackageReceived;
bufferPackageSent bufferForPackageSent;
Byteduino byteduino_device; 

#if defined(ESP8266)
void timerCallback(void * pArg) {
	baseTickOccured = true;
}
#endif

#if defined(ESP32)
void IRAM_ATTR timerCallback() {
	baseTickOccured = true;
}

void IRAM_ATTR restartDevice() {
  ESP.restart();
}

#endif

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

#if defined(ESP32)
	//watchdog timer
	watchdogTimer = timerBegin(0, 80, true);                  
	timerAttachInterrupt(watchdogTimer, &restartDevice, true);  
	timerAlarmWrite(watchdogTimer, 3000 * 1000, false); 
	timerAlarmEnable(watchdogTimer);
	FEED_WATCHDOG;
#endif


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
	
#if !UNIQUE_WEBSOCKET
	secondWebSocket.onEvent(secondWebSocketEvent);
#endif
	EEPROM.begin(TOTAL_USED_FLASH);

	uECC_set_rng(&getRandomNumbersForUecc);
	loadPreviousMessengerKeys();

	//set up base timer
#if defined(ESP8266)
	os_timer_setfn(&baseTimer, timerCallback, NULL);
	os_timer_arm(&baseTimer, 1000, true);
#endif

#if defined(ESP32)
	//base timer
	baseTimer = timerBegin(1, 80, true);
	timerAttachInterrupt(baseTimer, &timerCallback, true);
	timerAlarmWrite(baseTimer, 1000000, true);
	timerAlarmEnable(baseTimer);
#endif


	

	byteduino_device.isInitialized = true;

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


String getDeviceInfosJsonString(){
	const size_t bufferSize = JSON_OBJECT_SIZE(4);
	StaticJsonBuffer<bufferSize> jsonBuffer;
	JsonObject & mainObject = jsonBuffer.createObject();

	mainObject["device_address"] = (const char *) byteduino_device.deviceAddress;
	mainObject["device_hub"] = (const char *) byteduino_device.hub;
	mainObject["device_pubkey"] =(const char *) byteduino_device.keys.publicKeyM1b64;
	mainObject["extended_pubkey"] =(const char *) byteduino_device.keys.extPubKey;
	String returnedString;
	mainObject.printTo(returnedString);
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

	FEED_WATCHDOG;
	webSocketForHub.loop();
	yield();
#if !UNIQUE_WEBSOCKET
	secondWebSocket.loop();
	yield();
#endif

	updateRandomPool();

	if (!byteduino_device.isInitialized && isRandomGeneratorReady()){
		byteduino_init ();
	}

	//things we do when device is connected
	if (byteduino_device.isConnected){
		treatReceivedPackage();
		treatNewWalletCreation();
		treatWaitingSignature();
		treatSentPackage();

		if (bufferForPackageReceived.hasUnredMessage && bufferForPackageReceived.isFree && !bufferForPackageReceived.isRequestingNewMessage)
			refreshMessagesFromHub();
	}

	//things we every second
	if (baseTickOccured == true) {
		job2Seconds++;
		if (job2Seconds == 2){
			if (byteduino_device.isConnected)
				sendHeartbeat();
				job2Seconds = 0;
		}


    baseTickOccured = false;
	managePackageSentTimeOut();
	manageMessengerKey();
	}

	
	
}