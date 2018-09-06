// Byteduino lib - papabyte.com
// MIT License
#include "messenger.h"

extern WebSocketsClient webSocketForHub;
extern Byteduino byteduino_device;

extern messengerKeys myMessengerKeys;
extern bufferPackageReceived bufferForPackageReceived;
extern bufferPackageSent bufferForPackageSent;

void encryptAndSendPackage(){

	const char * recipientTempMessengerkey = bufferForPackageSent.recipientTempMessengerkey;  
	
	uint8_t decompressedPubkey[64];
	decodeAndDecompressPubKey(recipientTempMessengerkey,decompressedPubkey);

	uint8_t secret[32];

	uECC_shared_secret(decompressedPubkey, myMessengerKeys.privateKey, secret, uECC_secp256k1());

	uint8_t hashedSecret[16];
	getSHA256(hashedSecret, (const char*)secret, 32, 16);
	
	GCM<AES128> gcm;

	uint8_t authTag[16];

	gcm.setKey(hashedSecret, 16);
	uint8_t iv [12];
	do{
	}
	while (!getRandomNumbersForVector(iv, 12));

	 char ivb64 [17];
	Base64.encode(ivb64, (char *)iv, 12);

	gcm.setIV(iv, 12);
	
	char message[400];
	gcm.encrypt((uint8_t *)message, (uint8_t *)bufferForPackageSent.message ,  strlen(bufferForPackageSent.message));

	gcm.computeTag(authTag,16);
	char authTagB64 [25];
	Base64.encode(authTagB64, (char *)authTag, 16);

	char  messageB64[600];
	Base64.encode(messageB64, message, strlen(bufferForPackageSent.message));

	DynamicJsonBuffer jsonBuffer(900);
	JsonArray & mainArray = jsonBuffer.createArray();

	mainArray.add("request");
	JsonObject & objRequest= jsonBuffer.createObject();

	objRequest["command"] = "hub/deliver";

	JsonObject & encryptedPackage = jsonBuffer.createObject();
	JsonObject & objParams = jsonBuffer.createObject();

	JsonObject & dh= jsonBuffer.createObject();
	dh["sender_ephemeral_pubkey"] = myMessengerKeys.pubKeyB64;
	dh["recipient_ephemeral_pubkey"] = recipientTempMessengerkey;

	encryptedPackage["encrypted_message"] = messageB64;
	encryptedPackage["iv"] = ivb64;
	encryptedPackage["authtag"] = authTagB64;
	encryptedPackage["dh"] = dh;

	char deviceAddress[34];
	getDeviceAddress(bufferForPackageSent.recipientPubkey, deviceAddress);
	objParams["encrypted_package"] = encryptedPackage;
	objParams["to"] = deviceAddress;
	objParams["pubkey"] = byteduino_device.keys.publicKeyM1b64;
	
	uint8_t hash[32];
	getSHA256ForJsonObject (hash, objParams);

	char sigb64[89];
	getB64SignatureForHash(sigb64 ,byteduino_device.keys.privateM1, hash,32);

	objParams["signature"] = sigb64;

	objRequest["params"] = objParams;
	mainArray.add(objRequest);
	mainArray.printTo(bufferForPackageSent.message);
	yield();
#ifdef DEBUG_PRINT
	Serial.println(bufferForPackageSent.message);
#endif
	webSocketForHub.sendTXT(bufferForPackageSent.message);
	bufferForPackageSent.isFree =true;
	
}
void requestMessengerTempKey(){

	char output[256];
	StaticJsonBuffer<200> jsonBuffer;
	JsonArray & mainArray = jsonBuffer.createArray();

	mainArray.add("request");
	JsonObject & objRequest= jsonBuffer.createObject();

	objRequest["command"] = "hub/get_temp_pubkey";
	objRequest["params"] = bufferForPackageSent.recipientPubkey;
	
	char tag[12];
	getTag(tag,GET_RECIPIENT_KEY);
	objRequest["tag"] = (const char*) tag;
	
	mainArray.add(objRequest);
	mainArray.printTo(output);
#ifdef DEBUG_PRINT
	Serial.println(output);
#endif
	webSocketForHub.sendTXT(output);
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



void decryptPackageAndPlaceItInBuffer(JsonObject& message){

	if (strcmp(message["to"],byteduino_device.deviceAddress)!= 0){
#ifdef DEBUG_PRINT
		Serial.println(F("wrong recipient"));
#endif
		return;
	}

	const char * recipient_ephemeral_pubkey = message["encrypted_package"]["dh"]["recipient_ephemeral_pubkey"];
	const char * sender_ephemeral_pubkey_b64 =  message["encrypted_package"]["dh"]["sender_ephemeral_pubkey"];

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

	GCM<AES128> gcm;

	const char * authtagb64 = message["encrypted_package"]["authtag"];
	uint8_t authTag[18];
	Base64.decode(authTag, authtagb64, 25);

	const char* plaintextB64 = message["encrypted_package"]["encrypted_message"];
	int sizeB64 =   strlen(plaintextB64);
	if (sizeB64 < RECEIVED_PACKAGE_BUFFER_SIZE*1.3){
		Base64.decode(bufferForPackageReceived.message, plaintextB64,  sizeB64);
		size_t bufferSize = Base64.decodedLength(plaintextB64, sizeB64);

		const char* pubkey =  message["pubkey"];
		memcpy(bufferForPackageReceived.senderPubkey, pubkey,45);
		gcm.setKey(hashedSecret, 16);
		const char * ivb64 = message["encrypted_package"]["iv"];
		uint8_t iv [12];
		Base64.decode(iv, ivb64, 17);
		gcm.setIV(iv, 12);
		gcm.decrypt(bufferForPackageReceived.message , bufferForPackageReceived.message , bufferSize);

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


void respondToMessage(JsonObject& messageBody){
#ifdef DEBUG_PRINT
	Serial.println(F("respondToMessage"));
#endif

	if (checkMessageBodyStructure(messageBody)){
		
		if (bufferForPackageReceived.isFree){ //we delete message from hub if we have free buffer to treat it
			deleteMessageFromHub(messageBody["message_hash"]);
		} else {
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
				decryptPackageAndPlaceItInBuffer(message);
			} else { 
#ifdef DEBUG_PRINT
				Serial.println(F("Wrong message hash"));
#endif
			}
		} else {
#ifdef DEBUG_PRINT
	Serial.println(F("Wrong message sctructure"));
#endif
		}

	}
}


bool  checkMessageStructure(JsonObject& message){
	if (message["encrypted_package"].is<JsonObject>()) {
		if (message["to"].is<char*>()) {
			if (message["pubkey"].is<char*>()) {	
				if (message["signature"].is<char*>()) {
					if (message["encrypted_package"]["encrypted_message"].is<char*>()) {	
						if (message["encrypted_package"]["iv"].is<char*>()) {
							if (message["encrypted_package"]["authtag"].is<char*>()) {
								if (message["encrypted_package"]["dh"].is<JsonObject>()){
									if (message["encrypted_package"]["dh"]["sender_ephemeral_pubkey"].is<char*>()) {
										if (message["encrypted_package"]["dh"]["recipient_ephemeral_pubkey"].is<char*>()) {
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
					}
					else {
#ifdef DEBUG_PRINT
						Serial.println(F("encrypted_package should contain encrypted_message string"));
#endif
					}
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
	StaticJsonBuffer<200> jsonBuffer;
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


