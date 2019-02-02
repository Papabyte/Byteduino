// Byteduino lib - papabyte.com
// MIT License

#include "communication.h"


extern WebSocketsClient webSocketForHub;
#if !UNIQUE_WEBSOCKET
extern WebSocketsClient secondWebSocket;
#endif

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
	JsonArray& arr = jb.parseArray(payload, 10);
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
		respondToJustSayingFromHub(arr[1]);
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

#if !UNIQUE_WEBSOCKET

void treatResponseFromSecondWebsocket(JsonArray& arr){
	if (arr[1].is<JsonObject>()) {
		const char* tag = arr[1]["tag"];
		if (tag != nullptr) {
			if  (tag[9] == GET_RECIPIENT_KEY[1]){
				checkAndUpdateRecipientKey(arr[1]);
			}
		}
	} else {
#ifdef DEBUG_PRINT
		Serial.println(F("Second array second websocket should contain an object"));
#endif
	}	
}


void secondWebSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

	switch (type) {
		case WStype_DISCONNECTED:
#ifdef DEBUG_PRINT
		Serial.println(F("2nd Wss disconnected\n"));
#endif
		break;
		case WStype_CONNECTED:
		{
#ifdef DEBUG_PRINT
			Serial.printf("2nd Wss connected to: %s\n", payload);
#endif
			if (!bufferForPackageSent.isFree && !bufferForPackageSent.isRecipientKeyRequested)
				requestRecipientMessengerTempKey();
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
	break;
  }
}

#endif

int sendTxtMessage(const char recipientPubKey [45],const char * recipientHub, const char * text){

	if (bufferForPackageSent.isFree){
		if (strlen(recipientHub) < MAX_HUB_STRING_SIZE){
			if (strlen(recipientPubKey) == 44){
				if (strlen(text) < (SENT_PACKAGE_BUFFER_SIZE - 108)){
					const size_t bufferSize = JSON_OBJECT_SIZE(4);
					StaticJsonBuffer<bufferSize> jsonBuffer;
					JsonObject & message = jsonBuffer.createObject();
					message["from"] = (const char*) byteduino_device.deviceAddress;
					message["device_hub"] = (const char*) byteduino_device.hub;
					message["subject"] = "text";

					message["body"]= text;

					loadBufferPackageSent(recipientPubKey, recipientHub);
					message.printTo(bufferForPackageSent.message);
#ifdef DEBUG_PRINT
					Serial.println(bufferForPackageSent.message);
#endif
					return SUCCESS;
				} else {
#ifdef DEBUG_PRINT
					Serial.println(F("text too long"));
#endif
					return TEXT_TOO_LONG;
				}
			} else {
#ifdef DEBUG_PRINT
				Serial.println(F("wrong pub key size"));
#endif
				return WRONG_PUBKEY_SIZE;
			}
		} else {
#ifdef DEBUG_PRINT
			Serial.println(F("hub url too long"));
#endif
			return HUB_URL_TOO_LONG;
		}

	} else {
#ifdef DEBUG_PRINT
			Serial.println(F("Buffer not free to send message"));
#endif
		return BUFFER_NOT_FREE;
	}
}

void treatReceivedPackage(){
	
	if (!bufferForPackageReceived.isFree){
		
		DynamicJsonBuffer jb(JSON_BUFFER_SIZE_FOR_RECEIVED_PACKAGE);
		JsonObject& receivedPackage = jb.parseObject(bufferForPackageReceived.message);
		if (receivedPackage.success()) {
			
			if (receivedPackage["encrypted_package"].is<JsonObject>()){
				Serial.println(F("inner encrypted package"));
				treatInnerPackage(receivedPackage["encrypted_package"]);
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

				}
#ifndef REMOVE_COSIGNING
				
				else if (strcmp(subject,"create_new_wallet") == 0){
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
#endif
			} else {
#ifdef DEBUG_PRINT
				Serial.println(F("no subject for received message"));
#endif
			}
	} else {
#ifdef DEBUG_PRINT
			Serial.println(F("Deserialization failed"));
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
					Serial.println(ESP.getFreeHeap());
					Serial.println(F("Heartbeat acknowledged by hub"));
#endif
				} else if (tag[9] == UPDATE_MESSENGER_KEY[1]){
#ifdef DEBUG_PRINT
					Serial.println(F("Ephemeral key updated"));
#endif
				} else if (tag[9] == GET_RECIPIENT_KEY[1]){
#ifdef DEBUG_PRINT
					Serial.println(F("recipient pub key received"));
#endif
					checkAndUpdateRecipientKey(arr[1]);
				} else if (tag[9] == GET_ADDRESS_DEFINITION[1]){
#ifdef DEBUG_PRINT
					Serial.println(F("definition received"));
#endif
					handleDefinition(arr[1]);
				} else if (tag[9] == GET_INPUTS_FOR_AMOUNT[1]){
#ifdef DEBUG_PRINT
					Serial.println(F("inputs received"));
#endif
					handleInputsForAmount(arr[1], tag);
				}else if (tag[9] == GET_PARENTS_BALL_WITNESSES[1]){
#ifdef DEBUG_PRINT
					Serial.println(F("units props received"));
#endif
					handleUnitProps(arr[1]);
				} else if (tag[9] == POST_JOINT[1]){
#ifdef DEBUG_PRINT
					Serial.println(F("post joint result received"));
#endif
					handlePostJointResponse(arr[1], tag);
				} else if (tag[9] == GET_BALANCE[1]){
#ifdef DEBUG_PRINT
					Serial.println(F("balance received"));
#endif
					handleBalanceResponse(arr[1]);
				} else {
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
	} else if (strcmp(command, "heartbeat") == 0) {
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
	}

}

void respondToJustSayingFromHub(JsonObject& justSayingObject) {

	const char* subject = justSayingObject["subject"];

	if (subject != nullptr) {

		if (strcmp(subject, "hub/challenge") == 0) {
			const char* body = justSayingObject["body"];
			if (body != nullptr) {
				respondToHubChallenge(body);
			} else {
#ifdef DEBUG_PRINT
				Serial.println(F("justSayingObject should contain a body"));
#endif
			}
		} else if (strcmp(subject, "hub/push_project_number") == 0) {
			byteduino_device.isAuthenticated = true;
			Serial.println(F("Authenticated by hub"));
			return;
		} else if (strcmp(subject, "hub/message") == 0) {
			if (justSayingObject["body"].is<JsonObject>()) {
				JsonObject& messageBody = justSayingObject["body"];
				treatReceivedMessage(messageBody);
			}
		} else if (strcmp(subject, "hub/message_box_status") == 0) {
			bufferForPackageReceived.isRequestingNewMessage = false;
			const char* body = justSayingObject["body"];
			if (body != nullptr) {
				if (strcmp(body, "empty") != 0){
					bufferForPackageReceived.hasUnredMessage = true;
#ifdef DEBUG_PRINT
					Serial.println(F("Message box not empty"));
#endif
				} else{
					bufferForPackageReceived.hasUnredMessage = false;
#ifdef DEBUG_PRINT
					Serial.println(F("Empty message box"));
#endif
				}
			} else {
#ifdef DEBUG_PRINT
				Serial.println(F("justSayingObject should contain a body"));
#endif
			}
		} 
	} else {
#ifdef DEBUG_PRINT
		Serial.println(F("Second array should contain a subject"));
#endif
	}
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



