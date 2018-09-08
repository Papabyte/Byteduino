// Byteduino lib - papabyte.com
// MIT License

#include "communication.h"


extern WebSocketsClient webSocketForHub;
//extern WebSocketsClient secondaryWebSocket;

extern Byteduino byteduino_device;
extern bufferPackageReceived bufferForPackageReceived;
extern bufferPackageSent bufferForPackageSent;
cbMessageReceived _cbMessageReceived;


void setCbTxtMessageReceived(cbMessageReceived cbToSet){
_cbMessageReceived = cbToSet;
}

bool isValidArrayFromHub(JsonArray& arr){
	
	if (arr.size() != 2) {
#ifdef DEBUG_PRINT
		Serial.println(F("Array should have 2 elements"));
#endif
		return false;
	}

	if (!arr[0].is<char*>())  {
#ifdef DEBUG_PRINT
		Serial.println(F("First array element should be a char"));
#endif
		return false;
	}

	if (!arr[1].is<JsonObject>())  {
#ifdef DEBUG_PRINT
		Serial.println(F("Second array should be an object"));
#endif
		return false;
	}
return true;
}


void respondToHub(uint8_t *payload) {
#ifdef DEBUG_PRINT
	Serial.printf("Received: %s\n", payload);
#endif
	DynamicJsonBuffer jb(1000);
	JsonArray& arr = jb.parseArray(payload);
	if (!arr.success()) {
#ifdef DEBUG_PRINT
	Serial.println(F("Deserialization failed"));
#endif
	return;
	}

	if (!isValidArrayFromHub(arr)){
		return;
	}

	if (strcmp(arr[0], "justsaying") == 0) {
		respondToJustSayingFromHub(arr);
	} else if (strcmp(arr[0], "request") == 0) {
		respondToRequestFromHub(arr);
	} else if (strcmp(arr[0], "response") == 0) {
		treatResponseFromHub(arr);
	}

}

String getDomain(const char * hub){
	String returnedString = hub;
	int slashIndex = returnedString.indexOf("/");
	if (slashIndex > 0)
		returnedString.remove(slashIndex);
	return returnedString;
}

String getPath(const char * hub){
	String returnedString = hub;
	int slashIndex = returnedString.indexOf("/");
	if (slashIndex > 0)
		returnedString.remove(0, slashIndex);
	return returnedString;
}

/*
void connectSecondaryWebsocket(){
	
	size_t hubNameLength = strlen(bufferForPackageSent.recipientHub);
	
	char url [40];
	char path [10];
	size_t i = 0;
		
	while (bufferForPackageSent.recipientHub[i] != 0x2F && i < 50){ //while not "/"
	i++;
	}
	memcpy(url,bufferForPackageSent.recipientHub,i);
	url[i] = 0x00;
	memcpy(path,bufferForPackageSent.recipientHub+i,hubNameLength-i);
	path[hubNameLength-i] = 0x00;
	Serial.println(url);
	Serial.println(path);
	secondaryWebSocket.disconnect();
	secondaryWebSocket.beginSSL(url, 443, path);
	secondaryWebSocket.onEvent(secondaryWebSocketEvent);
}*/

//["request",{"command":"hub/get_temp_pubkey","params":"AnU9jftJF3Pn0J02+MhH01jNeStU978vthO9M1B9Yf8f","tag":"c1AUOMMdkFYCxMNvMhSjOgQj9ajXwIrGsksWxW7f5yg="}] 




/*
void treatResponseFromSecondWebsocket(JsonArray& arr){
	if (arr[1].is<JsonObject>()) {
		const char* tag = arr[1]["tag"];
		if (tag != nullptr) {
			if (strncmp(tag,byteduino_device.tagId,TAG_LENGTH)){
			if (tag[10] == GET_RECIPIENT_KEY[1]){
					Serial.println(F("recipient pub key received"));
				secondaryWebSocket.disconnect();

				}
				
			}else{
#ifdef DEBUG_PRINT
			Serial.println(F("wrong tag id for response"));
#endif
			}
			
		} else {
#ifdef DEBUG_PRINT
			Serial.println(F("response should contain a tag"));
#endif
		}
		
	} else {
#ifdef DEBUG_PRINT
		Serial.println(F("Second array of response should contain a object"));
#endif
	}
}*/



/*
void secondaryWebSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

	switch (type) {
		case WStype_DISCONNECTED:
			Serial.println(F("2nd Wss disconnected\n"));
			secondaryWebSocket.disconnect();
		break;
		case WStype_CONNECTED:
		{
			Serial.printf("2nd Wss connected to: %s\n", payload);
			requestMessengerTempKey();
			
		}
		break;
		case WStype_TEXT:
		Serial.printf("2nd Wss received: %s\n", payload);
		DynamicJsonBuffer jb(1000);
		JsonArray& arr = jb.parseArray(payload);
		if (!arr.success()) {
#ifdef DEBUG_PRINT
			Serial.println(F("Deserialization failed"));
#endif
			return;
		}

	if (!isValidArrayFromHub(arr)){
		return;
	}
		if (strcmp(arr[0], "response") == 0) {
		treatResponseFromSecondWebsocket(arr);
	}
			
			//respondToHub(payload);
	break;
  }
}

*/

bool sendTxtMessage(const char recipientPubkey [45],const char * deviceHub, const char * text){

	if (bufferForPackageSent.isFree){
		if (strlen(deviceHub) < MAX_HUB_STRING_SIZE){
			if (strlen(recipientPubkey) == 44){
				if (strlen(text) < (SENT_PACKAGE_BUFFER_SIZE - 108)){
					const size_t bufferSize = JSON_OBJECT_SIZE(4);
					StaticJsonBuffer<bufferSize> jsonBuffer;
					JsonObject & message = jsonBuffer.createObject();
					message["from"] = (const char*) byteduino_device.deviceAddress;
					message["device_hub"] = (const char*) byteduino_device.hub;
					message["subject"] = "text";

					message["body"]= text;
					bufferForPackageSent.isRecipientTempMessengerKeyKnown = false;
					strcpy(bufferForPackageSent.recipientPubkey,recipientPubkey);
					strcpy(bufferForPackageSent.recipientHub, deviceHub);
					bufferForPackageSent.isFree = false;
					bufferForPackageSent.isRecipientKeyRequested = false;
					message.printTo(bufferForPackageSent.message);
#ifdef DEBUG_PRINT
					Serial.println(bufferForPackageSent.message);
#endif
					return true;
				} else {
#ifdef DEBUG_PRINT
					Serial.println(F("text too long"));
#endif
				}
			} else {
#ifdef DEBUG_PRINT
				Serial.println(F("wrong pub key size"));
#endif
			}
		} else {
#ifdef DEBUG_PRINT
			Serial.println(F("hub url too long"));
#endif
		}

	} else {
#ifdef DEBUG_PRINT
			Serial.println(F("Buffer not free to send message"));
#endif
	}
	return false;
}

void treatReceivedPackage(){
	
	if (!bufferForPackageReceived.isFree){
		
		DynamicJsonBuffer jb(700);
		JsonObject& receivedPackage = jb.parseObject(bufferForPackageReceived.message);
		if (!receivedPackage.success()) {
#ifdef DEBUG_PRINT
			Serial.println(F("Deserialization failed"));
#endif
		bufferForPackageReceived.isFree = true;
		return;
		}

		const char* subject = receivedPackage["subject"];

		if (subject != nullptr) {

			if (strcmp(subject,"pairing") == 0){
				if (receivedPackage["body"].is<JsonObject>()){
#ifdef DEBUG_PRINT
					Serial.println(F("handlePairingRequest"));
#endif
					handlePairingRequest(receivedPackage);
				}

			} else if (strcmp(subject,"text") == 0){
#ifdef DEBUG_PRINT
				Serial.println(F("handle text"));
#endif
				const char * messageDecoded = receivedPackage["body"];
				const char * senderHub = receivedPackage["device_hub"];
				if (messageDecoded != nullptr && senderHub != nullptr) {
					if(_cbMessageReceived){
						_cbMessageReceived(bufferForPackageReceived.senderPubkey, senderHub, messageDecoded);
					}
				} else {
#ifdef DEBUG_PRINT
					Serial.println(F("body and device_hub must be char"));
#endif
				}

			} else if (strcmp(subject,"create_new_wallet") == 0){
				if (receivedPackage["body"].is<JsonObject>()){
#ifdef DEBUG_PRINT
					Serial.println(F("handle new wallet"));
#endif
					handleNewWalletRequest(bufferForPackageReceived.senderPubkey,receivedPackage);
				}

			} else if (strcmp(subject,"sign") == 0){
				if (receivedPackage["body"].is<JsonObject>()){
#ifdef DEBUG_PRINT
					Serial.println(F("handle signature request"));
#endif
					handleSignatureRequest(bufferForPackageReceived.senderPubkey,receivedPackage);
				}

			}

			bufferForPackageReceived.isFree = true;
			return;

		} else {
#ifdef DEBUG_PRINT
			Serial.println(F("no subject for received message"));
#endif
		}
	bufferForPackageReceived.isFree = true;
	}
}


void treatResponseFromHub(JsonArray& arr){
	if (arr[1].is<JsonObject>()) {
		const char* tag = arr[1]["tag"];
		if (tag != nullptr) {
				if (tag[9] == HEARTBEAT[1]){
#ifdef DEBUG_PRINT
					Serial.println(F("Heartbeat acknowledged by hub"));
#endif
				} else if (tag[9] == UPDATE_MESSENGER_KEY[1]){
#ifdef DEBUG_PRINT
					Serial.println(F("Ephemeral key updated"));
#endif
				} else if  (tag[9] == GET_RECIPIENT_KEY[1]){
					checkAndUpdateRecipientKey(arr[1]);
#ifdef DEBUG_PRINT
					Serial.println(F("recipient pub key received"));
#endif
				}else{
#ifdef DEBUG_PRINT
			Serial.println(F("wrong tag id for response"));
#endif
				}
		
		} else {
#ifdef DEBUG_PRINT
		Serial.println(F("Second array of response should contain a object"));
#endif
		}
	}
}

void respondToRequestFromHub(JsonArray& arr) {

	const char* command = arr[1]["command"];

	if (command != nullptr) {

	if (strcmp(command, "subscribe") == 0) {
		const char* tag = arr[1]["tag"];
		if (tag != nullptr) {
			sendErrorResponse(tag, "I'm a microdevice, cannot subscribe you to updates");
		} else {
#ifdef DEBUG_PRINT
			Serial.println(F("Second array should contain a tag"));
#endif
		}
	return;
	}

	if (strcmp(command, "heartbeat") == 0) {
		const char* tag = arr[1]["tag"];
		if (tag != nullptr) {
		//   sendHeartbeatResponse(tag);
		} else {
	#ifdef DEBUG_PRINT
			Serial.println(F("Second array should contain a tag"));
	#endif
		}
	}

	} else {
#ifdef DEBUG_PRINT
		Serial.println(F("Second array should contain a command"));
#endif
	return;
	}

}



void respondToJustSayingFromHub(JsonArray& arr) {

	const char* subject = arr[1]["subject"];

	if (subject != nullptr) {

		if (strcmp(subject, "hub/challenge") == 0) {
			const char* body = arr[1]["body"];
			if (body != nullptr) {
				respondToHubChallenge(body);
			} else {
#ifdef DEBUG_PRINT
				Serial.println(F("Second array should contain a body"));
#endif
			}
			return;
		} else if (strcmp(subject, "hub/push_project_number") == 0) {
			byteduino_device.isAuthenticated = true;
			Serial.println(F("Authenticated by hub"));
			return;
		} else if (strcmp(subject, "hub/message") == 0) {
			if (arr[1]["body"].is<JsonObject>()) {
				JsonObject& object = arr[1]["body"];
				respondToMessage(object);
			} else {
#ifdef DEBUG_PRINT
				Serial.println(F("Second array should contain an object"));
#endif
			}
		}

	} else {
#ifdef DEBUG_PRINT
		Serial.println(F("Second array should contain a subject"));
#endif
	}
		return;
}


void sendErrorResponse(const char* tag, const char* error) {

	char output[256];
	StaticJsonBuffer<200> jsonBuffer;
	JsonArray & mainArray = jsonBuffer.createArray();

	mainArray.add("response");
	JsonObject & objResponse = jsonBuffer.createObject();
	JsonObject & objError = jsonBuffer.createObject();

	objError["error"] = error;
	objResponse["tag"] = tag;
	objResponse["response"] = objError;

	mainArray.add(objResponse);
	mainArray.printTo(output);
#ifdef DEBUG_PRINT
	Serial.println(output);
#endif
	webSocketForHub.sendTXT(output);

}

void getTag(char * tag, const char * extension){
	uint8_t random[5];
	getRandomNumbersForTag(random,5);
	Base64.encode(tag, (char *) random, 5);
	memcpy(tag+8,extension,2);
	tag[10] = 0x00;
	
}

void sendHeartbeat(void) {
	
	char output[60];
	const int capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(2);
	StaticJsonBuffer<capacity> jsonBuffer;
	JsonArray & mainArray = jsonBuffer.createArray();

	mainArray.add("request");
	JsonObject & objRequest = jsonBuffer.createObject();

	objRequest["command"] = "heartbeat";
	
	char tag[12];
	getTag(tag,HEARTBEAT);
	objRequest["tag"] = (const char*) tag;
	
	mainArray.add(objRequest);
	mainArray.printTo(output);
#ifdef DEBUG_PRINT
	Serial.println(output);
#endif
	webSocketForHub.sendTXT(output);

}



