// Byteduino lib - papabyte.com
// MIT License
#include "key_rotation.h"

messengerKeys myMessengerKeys;

extern WebSocketsClient webSocketForHub;
extern Byteduino byteduino_device;


void getHashToSignForUpdatingMessengerKey(uint8_t* hash) {
	SHA256 hasher;
	hasher.update("pubkey\0s\0", 9);
	hasher.update( byteduino_device.keys.publicKeyM1b64, strlen(byteduino_device.keys.publicKeyM1b64));
	hasher.update("\0temp_pubkey\0s\0", 15);
	hasher.update(myMessengerKeys.pubKeyB64,strlen(myMessengerKeys.pubKeyB64));
	hasher.finalize(hash,32);
}


void createNewMessengerKeysAndSaveInFlash(){

	uint8_t publicKey[64];
	uint8_t publicKeyCompressed[33];

	do {
		do{
		}
		while (!getRandomNumbersForPrivateKey(myMessengerKeys.privateKey, 32));
		} while (!uECC_compute_public_key(myMessengerKeys.privateKey, publicKey, uECC_secp256k1()));
		
		uECC_compress(publicKey, publicKeyCompressed, uECC_secp256k1());
	
		Base64.encode(myMessengerKeys.pubKeyB64, (char *) publicKeyCompressed, 33);
		for (int i=0;i<32;i++){
			EEPROM.write(i + PREVIOUS_PRV_MESSENGER_KEY, myMessengerKeys.privateKey[i]);
		}
		for (int i=0;i<45;i++){
			EEPROM.write(i + PREVIOUS_PUB_MESSENGER_KEY, myMessengerKeys.pubKeyB64[i]);
		}
		
	 EEPROM.commit();	

}

void loadPreviousMessengerKeys(){
	
	for (int i=0;i<32;i++){
		myMessengerKeys.previousPrivateKey[i] = EEPROM.read(i + PREVIOUS_PRV_MESSENGER_KEY);
	}
	
	for (int i=0;i<45;i++){
		myMessengerKeys.previousPubKeyB64[i] = EEPROM.read(i + PREVIOUS_PUB_MESSENGER_KEY);
	}
	
}

void manageMessengerKey(){
	if(byteduino_device.isAuthenticated){
		if (byteduino_device.messengerKeyRotationTimer > 0)
			byteduino_device.messengerKeyRotationTimer--;

		if (byteduino_device.messengerKeyRotationTimer == 0 && byteduino_device.isAuthenticated){
			byteduino_device.messengerKeyRotationTimer = SECONDS_BETWEEN_KEY_ROTATION;
			rotateMessengerKey();
		}

	}
}


void rotateMessengerKey() {
	
	createNewMessengerKeysAndSaveInFlash();
	loadPreviousMessengerKeys();

	uint8_t hash[32];
	getHashToSignForUpdatingMessengerKey(hash);

	char sigb64[89];
	getB64SignatureForHash(sigb64 ,byteduino_device.keys.privateM1,hash,32);
	char output[300];
	const int capacity = JSON_ARRAY_SIZE(2) + 2*JSON_OBJECT_SIZE(3);
	StaticJsonBuffer<capacity> jsonBuffer;
	JsonArray & mainArray = jsonBuffer.createArray();

	mainArray.add("request");
	JsonObject & objRequest = jsonBuffer.createObject();

	objRequest["command"] = "hub/temp_pubkey";

	JsonObject & objParams = jsonBuffer.createObject();
	objParams["temp_pubkey"] = (const char*) myMessengerKeys.pubKeyB64;
	objParams["pubkey"] = (const char*) byteduino_device.keys.publicKeyM1b64;
	objParams["signature"] = (const char*) sigb64;
	objRequest["params"] = objParams;
	
	char tag[12];
	getTag(tag,UPDATE_MESSENGER_KEY);
	objRequest["tag"] = (const char*) tag;

	mainArray.add(objRequest);
	mainArray.printTo(output);
#ifdef DEBUG_PRINT
	Serial.println(output);
#endif
	webSocketForHub.sendTXT(output);
}