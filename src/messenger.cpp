// Byteduino lib - papabyte.com
// MIT License
#include "messenger.h"

extern Byteduino byteduino_device;

extern messengerKeys myMessengerKeys;
extern bufferPackageReceived bufferForPackageReceived;
extern bufferPackageSent bufferForPackageSent;

extern WebSocketsClient webSocketForHub;
#if !UNIQUE_WEBSOCKET
extern WebSocketsClient secondWebSocket;
#endif

void treatSentPackage(){
	if (bufferForPackageSent.isOnSameHub){
		if (!bufferForPackageSent.isFree && !bufferForPackageSent.isRecipientKeyRequested){
			requestRecipientMessengerTempKey();
		}

		if (!bufferForPackageSent.isFree && bufferForPackageSent.isRecipientTempMessengerKeyKnown){
			encryptAndSendPackage();
			yield(); //we let the wifi stack work since AES encryption may have been long
		}
	} 
#if !UNIQUE_WEBSOCKET
	else {
		if (!byteduino_device.isConnectingToSecondWebSocket){
	#ifdef DEBUG_PRINT
			Serial.println(F("Connect to second websocket"));
	#endif 
			secondWebSocket.disconnect();
			secondWebSocket.beginSSL(getDomain(bufferForPackageSent.recipientHub), byteduino_device.port, getPath(byteduino_device.hub));
			byteduino_device.isConnectingToSecondWebSocket = true;
		} else if (!bufferForPackageSent.isFree && bufferForPackageSent.isRecipientTempMessengerKeyKnown){
			encryptAndSendPackage();
			yield(); //we let the wifi stack work since AES encryption may have been long
		} 
	}
#endif

}


void managePackageSentTimeOut(){
	if (bufferForPackageSent.timeOut > 0){
		bufferForPackageSent.timeOut--;
	}

	if (bufferForPackageSent.timeOut == 0 && !bufferForPackageSent.isFree){
		bufferForPackageSent.isFree = true;
#if !UNIQUE_WEBSOCKET
	//	secondWebSocket.disconnect();
#endif 	
#ifdef DEBUG_PRINT
		Serial.println(F("Recipient key was never received"));
#endif 
	}
};

void encryptPackage(const char * recipientTempMessengerkey, char * messageB64,char * ivb64, char * authTagB64){

	uint8_t recipientDecompressedPubkey[64];
	decodeAndDecompressPubKey(recipientTempMessengerkey,recipientDecompressedPubkey);

	uint8_t secret[32];
	uECC_shared_secret(recipientDecompressedPubkey, myMessengerKeys.privateKey, secret, uECC_secp256k1());

	uint8_t hashedSecret[16];
	getSHA256(hashedSecret, (const char*)secret, 32, 16);
	GCM<AES128BD> gcm;

	uint8_t authTag[16];

	gcm.setKey(hashedSecret, 16);
	uint8_t iv [12];
	do{
	}
	while (!getRandomNumbersForVector(iv, 12));

	Base64.encode(ivb64, (char *)iv, 12);
	gcm.setIV(iv, 12);

	size_t packageMessageLength = strlen(bufferForPackageSent.message);
	gcm.encrypt((uint8_t *)messageB64, (uint8_t *)bufferForPackageSent.message, packageMessageLength); //unlike for decryption, encryption doesn't work well when usng same pointer as input and ouput
	memcpy(bufferForPackageSent.message, messageB64, packageMessageLength);
	gcm.computeTag(authTag,16);
	Base64.encode(authTagB64, (char *)authTag, 16);
	Base64.encode(messageB64, bufferForPackageSent.message, packageMessageLength);
}

void encryptAndSendPackage(){

	const char * recipientTempMessengerkey = bufferForPackageSent.recipientTempMessengerkey;  
#if defined(ESP8266) //for ESP8266 we use stack memory since we don't have much heap available
	char messageB64[(const int) SENT_PACKAGE_BUFFER_SIZE*134/100];
#endif
#if defined(ESP32) //for ESP32 we use heap to be able to send big packages
	char * messageB64 = NULL;
	messageB64 = (char *) malloc ((const int) SENT_PACKAGE_BUFFER_SIZE*134/100);
#endif

	char ivb64 [17];
	char authTagB64 [25];
	encryptPackage(recipientTempMessengerkey, messageB64, ivb64, authTagB64);
	
	const size_t bufferSize = JSON_ARRAY_SIZE(2) + 2*JSON_OBJECT_SIZE(2) + 2*JSON_OBJECT_SIZE(4);
	DynamicJsonBuffer jsonBuffer(bufferSize);
	JsonArray & mainArray = jsonBuffer.createArray();

	mainArray.add("request");
	JsonObject & objRequest= jsonBuffer.createObject();

	objRequest["command"] = "hub/deliver";

	JsonObject & encryptedPackage = jsonBuffer.createObject();
	JsonObject & objParams = jsonBuffer.createObject();

	JsonObject & dh= jsonBuffer.createObject();
	dh["sender_ephemeral_pubkey"] = (const char*) myMessengerKeys.pubKeyB64;
	dh["recipient_ephemeral_pubkey"] = (const char*) recipientTempMessengerkey;

	encryptedPackage["encrypted_message"] = (const char*) messageB64;
	encryptedPackage["iv"] = (const char*) ivb64;
	encryptedPackage["authtag"] = (const char*) authTagB64;
	encryptedPackage["dh"] = dh;

	char deviceAddress[34];
	getDeviceAddress(bufferForPackageSent.recipientPubkey, deviceAddress);
	objParams["encrypted_package"] = encryptedPackage;
	objParams["to"] = (const char*) deviceAddress;
	objParams["pubkey"] = (const char*) byteduino_device.keys.publicKeyM1b64;
	
	uint8_t hash[32];
	getSHA256ForJsonObject (hash, objParams);

	char sigb64[89];
	getB64SignatureForHash(sigb64 ,byteduino_device.keys.privateM1, hash,32);

	objParams["signature"] = (const char*) sigb64;

	objRequest["params"] = objParams;
	mainArray.add(objRequest);

	String output;
	mainArray.printTo(output);

#if defined(ESP32)
	free (messageB64);
#endif
#ifdef DEBUG_PRINT
	Serial.println(output);
#endif

#if UNIQUE_WEBSOCKET
	webSocketForHub.sendTXT(output);
#else
	if (bufferForPackageSent.isOnSameHub){
		webSocketForHub.sendTXT(output);
	}
	else{
		secondWebSocket.sendTXT(output);
	}
#endif
	bufferForPackageSent.isFree =true;
}


void loadBufferPackageSent(const char * recipientPubKey, const char *  recipientHub){

	if (strcmp(recipientHub, byteduino_device.hub) != 0){
		bufferForPackageSent.isOnSameHub = false;
#ifdef DEBUG_PRINT
		Serial.println(F("Recipient is not on same hub"));
#endif 
#if UNIQUE_WEBSOCKET
	#ifdef DEBUG_PRINT
		Serial.println(F("Recipient must be on the same hub"));
	#endif 
		bufferForPackageSent.isFree = true;
		return;
#endif
	} else {
	#ifdef DEBUG_PRINT
		Serial.println(F("Recipient is on same hub"));
	#endif 
		bufferForPackageSent.isOnSameHub = true;
	}

	memcpy(bufferForPackageSent.recipientPubkey,recipientPubKey,45);
	strcpy(bufferForPackageSent.recipientHub,recipientHub);
	bufferForPackageSent.timeOut = REQUEST_KEY_TIME_OUT;
	bufferForPackageSent.isRecipientTempMessengerKeyKnown = false;
	bufferForPackageSent.isFree = false;
	bufferForPackageSent.isRecipientKeyRequested = false;
	byteduino_device.isConnectingToSecondWebSocket = false;
}

void requestRecipientMessengerTempKey(){

	char output[130];
	const size_t bufferSize = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(3);
	StaticJsonBuffer<bufferSize> jsonBuffer;
	JsonArray & mainArray = jsonBuffer.createArray();

	mainArray.add("request");
	JsonObject & objRequest= jsonBuffer.createObject();

	objRequest["command"] = "hub/get_temp_pubkey";
	objRequest["params"] = (const char*) bufferForPackageSent.recipientPubkey;
	
	char tag[12];
	getTag(tag,GET_RECIPIENT_KEY);
	objRequest["tag"] = (const char*) tag;
	
	mainArray.add(objRequest);
	mainArray.printTo(output);
#ifdef DEBUG_PRINT
	Serial.println(output);
#endif
#if UNIQUE_WEBSOCKET
	webSocketForHub.sendTXT(output);
#else
	if (bufferForPackageSent.isOnSameHub)
		webSocketForHub.sendTXT(output);
	else
		secondWebSocket.sendTXT(output);
#endif
	bufferForPackageSent.isRecipientKeyRequested = true;
}

void checkAndUpdateRecipientKey(JsonObject& objResponse){
	
	if (objResponse["response"].is<JsonObject>()) {
		
		const char* temp_pubkeyB64 = objResponse["response"]["temp_pubkey"];
		if (temp_pubkeyB64 != nullptr){
			const char * pubkeyB64 = objResponse["response"]["pubkey"];
			if (pubkeyB64 != nullptr){
				const char * signatureB64 = objResponse["response"]["signature"];
				if (signatureB64 != nullptr){
					
					uint8_t hash[32];
					SHA256 hasher;
					hasher.update("pubkey\0s\0", 9);
					hasher.update(pubkeyB64,44);
					hasher.update("\0temp_pubkey\0s\0", 15);
					hasher.update(temp_pubkeyB64,44);
					hasher.finalize(hash,32);
					
					uint8_t decodedSignature[64];
					Base64.decode(decodedSignature, signatureB64, 89);
					
					uint8_t pubkey[64];
					decodeAndDecompressPubKey(pubkeyB64, pubkey);
					
				 if(uECC_verify(pubkey, hash, 32, decodedSignature, uECC_secp256k1())){
#ifdef DEBUG_PRINT
					Serial.println(F("temp pub key sig verified"));
#endif
					if (strcmp(pubkeyB64, bufferForPackageSent.recipientPubkey)==0){
						 memcpy(bufferForPackageSent.recipientTempMessengerkey,temp_pubkeyB64,45);
						 bufferForPackageSent.isRecipientTempMessengerKeyKnown = true; 
					} else {
#ifdef DEBUG_PRINT
					Serial.println(F("wrong temp pub key received"));
#endif 
					}
					
				 } else{
#ifdef DEBUG_PRINT
					Serial.println(F("temp pub key sig verification failed"));
#endif 
				 }
					
				}else{
#ifdef DEBUG_PRINT
	Serial.println(F("signature must be a char"));
#endif
				}
			
			}else{
#ifdef DEBUG_PRINT
	Serial.println(F("pubkey must be a char"));
#endif
			}
			
		}else{
#ifdef DEBUG_PRINT
	Serial.println(F("temp_pubkey must be a char"));
#endif
		}
		
	}else{
#ifdef DEBUG_PRINT
	Serial.println(F("response must be an object"));
#endif
	}
	
}



void decryptPackageAndPlaceItInBuffer(JsonObject& encryptedPackage, const char* senderPubkey){

	const char * recipient_ephemeral_pubkey = encryptedPackage["dh"]["recipient_ephemeral_pubkey"];
	const char * sender_ephemeral_pubkey_b64 =  encryptedPackage["dh"]["sender_ephemeral_pubkey"];

	uint8_t decompressedSenderPubKey[64];
	decodeAndDecompressPubKey(sender_ephemeral_pubkey_b64, decompressedSenderPubKey);

	uint8_t secret[32];

	if (strcmp(recipient_ephemeral_pubkey,myMessengerKeys.pubKeyB64) == 0){
#ifdef DEBUG_PRINT
		Serial.println(F("encoded with messengerPubKeyB64"));
#endif
		uECC_shared_secret(decompressedSenderPubKey,  myMessengerKeys.privateKey, secret, uECC_secp256k1());
	} else if (strcmp(recipient_ephemeral_pubkey,myMessengerKeys.previousPubKeyB64) == 0) {
#ifdef DEBUG_PRINT
		Serial.println(F("encoded with previousMessengerPubKeyB64"));
#endif
		uECC_shared_secret(decompressedSenderPubKey, myMessengerKeys.previousPrivateKey, secret, uECC_secp256k1());
	} else if (strcmp(recipient_ephemeral_pubkey,byteduino_device.keys.publicKeyM1b64) == 0) {
#ifdef DEBUG_PRINT
		Serial.println(F("encoded with permanent public key"));
#endif
		uECC_shared_secret(decompressedSenderPubKey, byteduino_device.keys.privateM1, secret, uECC_secp256k1());
	} else {
#ifdef DEBUG_PRINT
		Serial.println(F("Unknown public key"));
#endif
		return;
	}
	
	uint8_t hashedSecret[16];
	getSHA256(hashedSecret ,(const char*)secret, 32,16);

	GCM<AES128BD> gcm;

	const char * authtagb64 = encryptedPackage["authtag"];
	uint8_t authTag[18];
	Base64.decode(authTag, authtagb64, 25);

	const char* plaintextB64 = encryptedPackage["encrypted_message"];
	int sizeB64 = strlen(plaintextB64);
	if (sizeB64 < RECEIVED_PACKAGE_BUFFER_SIZE*1.3){
		Base64.decode(bufferForPackageReceived.message, plaintextB64,  sizeB64);
		size_t bufferSize = Base64.decodedLength(plaintextB64, sizeB64);

		memcpy(bufferForPackageReceived.senderPubkey, senderPubkey,45);
		gcm.setKey(hashedSecret, 16);
		const char * ivb64 = encryptedPackage["iv"];
		uint8_t iv [12];
		Base64.decode(iv, ivb64, 17);
		gcm.setIV(iv, 12);
		gcm.decrypt(bufferForPackageReceived.message, bufferForPackageReceived.message, bufferSize);

		if (!gcm.checkTag(authTag, 16)) {
#ifdef DEBUG_PRINT
			Serial.println(F("Invalid auth tag"));
#endif
			return;
		}
		bufferForPackageReceived.isFree = false;

#ifdef DEBUG_PRINT
		for (int i = 0; i < bufferSize; i++){
			Serial.write(bufferForPackageReceived.message[i]);
		}
#endif
	} else {
#ifdef DEBUG_PRINT
		Serial.println(F("Message too long for buffer"));
#endif
	}

}


void treatInnerPackage(JsonObject& encryptedPackage){
	if (checkEncryptedPackageStructure(encryptedPackage)){
#ifdef DEBUG_PRINT
		Serial.println(F("Going to decrypt inner package"));
#endif
		decryptPackageAndPlaceItInBuffer(encryptedPackage, bufferForPackageReceived.senderPubkey);
	}
}

void treatReceivedMessage(JsonObject& messageBody){
#ifdef DEBUG_PRINT
	Serial.println(F("treatReceivedMessage"));
#endif

	if (checkMessageBodyStructure(messageBody)){
		
		if (bufferForPackageReceived.isFree){ //we delete message from hub if we have free buffer to treat it
			deleteMessageFromHub(messageBody["message_hash"]);
		} else {
			bufferForPackageReceived.hasUnredMessage = true;
#ifdef DEBUG_PRINT
			Serial.println(F("buffer not free to treat received message"));
#endif
			return;
		}
		Serial.println(F("ready to decode message"));

		JsonObject& message = messageBody["message"];
		
		if (checkMessageStructure(message)){

			char hashB64[45];
			getBase64HashForJsonObject(hashB64, message);
#ifdef DEBUG_PRINT
			Serial.println(F("Computed message hash"));
			Serial.println(hashB64);
#endif
			if (strcmp(hashB64,messageBody["message_hash"]) == 0){
				if (strcmp(message["to"],byteduino_device.deviceAddress) == 0){
#ifdef DEBUG_PRINT
			Serial.println(F("Going to decrypt package"));
#endif
					decryptPackageAndPlaceItInBuffer(message["encrypted_package"], message["pubkey"]);
				} else {
#ifdef DEBUG_PRINT
		Serial.println(F("wrong recipient"));
#endif
				}
			} else { 
#ifdef DEBUG_PRINT
				Serial.println(F("Wrong message hash"));
#endif
			}
		} else {
#ifdef DEBUG_PRINT
	Serial.println(F("Wrong message structure"));
#endif
		}

	}
}

bool checkEncryptedPackageStructure(JsonObject& encryptedPackage){

	if (encryptedPackage["encrypted_message"].is<char*>()) {	
		if (encryptedPackage["iv"].is<char*>()) {
			if (encryptedPackage["authtag"].is<char*>()) {
				if (encryptedPackage["dh"].is<JsonObject>()){
					if (encryptedPackage["dh"]["sender_ephemeral_pubkey"].is<char*>()) {
						if (encryptedPackage["dh"]["recipient_ephemeral_pubkey"].is<char*>()) {
							return true;
						} else {
#ifdef DEBUG_PRINT
							Serial.println(F("encrypted_package should contain recipient_ephemeral_pubkey"));
#endif
						}
					} else {
#ifdef DEBUG_PRINT
						Serial.println(F("encrypted_package should contain sender_ephemeral_pubkey"));
#endif
					}
				} else {
#ifdef DEBUG_PRINT
					Serial.println(F("encrypted_package should contain dh object"));
#endif
				}
			} else {
#ifdef DEBUG_PRINT
				Serial.println(F("encrypted_package should contain authtag string"));
#endif
			}
		} else {
#ifdef DEBUG_PRINT
			Serial.println(F("encrypted_package should contain iv string"));
#endif
		}
	} else {
#ifdef DEBUG_PRINT
		Serial.println(F("encrypted_package should contain encrypted_message string"));
#endif
	}
return false;
}

bool checkMessageStructure(JsonObject& message){
	if (message["encrypted_package"].is<JsonObject>()) {
		if (message["to"].is<char*>()) {
			if (message["pubkey"].is<char*>()) {	
				if (message["signature"].is<char*>()) {
					return checkEncryptedPackageStructure(message["encrypted_package"]);
				} else {
#ifdef DEBUG_PRINT
				Serial.println(F("message body should contain signature string "));
#endif
				}
			} else {
#ifdef DEBUG_PRINT
					Serial.println(F("message body should contain pubkey string "));
#endif
				}
			} else {
#ifdef DEBUG_PRINT
			Serial.println(F("message body should contain <to> string "));
#endif
			}
	} else {
#ifdef DEBUG_PRINT
			Serial.println(F("message should contain an encrypted package object"));
#endif
	}
return false;
}


bool checkMessageBodyStructure(JsonObject& messageBody){
	if (messageBody["message_hash"].is<char*>()){
		if (messageBody["message"].is<JsonObject>()) {
			return true;
		} else {
#ifdef DEBUG_PRINT
		Serial.println(F("message body should contains a message object"));
#endif
			}
	} else {
#ifdef DEBUG_PRINT
	Serial.println(F("message body should contains a hash"));
#endif
	}
return false;
}



void deleteMessageFromHub(const char* messageHash) {
	char output[100];
	const size_t bufferSize = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(3);
	StaticJsonBuffer<bufferSize> jsonBuffer;
	JsonArray & mainArray = jsonBuffer.createArray();

	mainArray.add("justsaying");
	JsonObject & objJustSaying = jsonBuffer.createObject();
	JsonObject & objBody = jsonBuffer.createObject();

	objJustSaying["subject"] = "hub/delete";

	objJustSaying["body"] = messageHash;

	mainArray.add(objJustSaying);
	mainArray.printTo(output);
#ifdef DEBUG_PRINT
	Serial.println(output);
#endif
	webSocketForHub.sendTXT(output);

}

void refreshMessagesFromHub() {
	char output[100];
	const size_t bufferSize = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(2);
	StaticJsonBuffer<bufferSize> jsonBuffer;
	JsonArray & mainArray = jsonBuffer.createArray();

	mainArray.add("justsaying");
	JsonObject & objJustSaying = jsonBuffer.createObject();
	JsonObject & objBody = jsonBuffer.createObject();

	objJustSaying["subject"] = "hub/refresh";

	mainArray.add(objJustSaying);
	mainArray.printTo(output);
#ifdef DEBUG_PRINT
	Serial.println(output);
#endif
	bufferForPackageReceived.isRequestingNewMessage = true;
	webSocketForHub.sendTXT(output);
}

