// Byteduino lib - papabyte.com
// MIT License

#include "hub_challenge.h"

extern WebSocketsClient webSocketForHub;
extern Byteduino byteduino_device;

/*
void getHashToSignForChallenge(uint8_t* hash, const char* challenge, const char* publicKeyM1b64) {
	SHA256  hasher;
	hasher.update("challenge\0s\0", 12);
	hasher.update(challenge,strlen(challenge));
	hasher.update("\0max_message_count\0n\0", 21);
	char str[16];
	sprintf(str, "%d", MAX_MESSAGE_COUNT);
	hasher.update(str,strlen(str));
	hasher.update("\0max_message_length\0n\0", 22);
	sprintf(str, "%d", MAX_MESSAGE_LENGTH);
	hasher.update(str,strlen(str));
	hasher.update("\0pubkey\0s\0", 10);
	hasher.update(publicKeyM1b64, strlen(publicKeyM1b64));
	hasher.finalize(hash,32);
}
*/

void respondToHubChallenge(const char* challenge) {
	
	if (strlen(challenge) != 40){
#ifdef DEBUG_PRINT
		Serial.println(F("challenge must be 40 chars long"));
#endif
		return;
	}
		

	char output[320];
	const int capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + 20;
	StaticJsonBuffer<capacity> jsonBuffer;
	JsonArray & mainArray = jsonBuffer.createArray();

	mainArray.add("justsaying");
	JsonObject & objJustSaying = jsonBuffer.createObject();
	JsonObject & objBody = jsonBuffer.createObject();

	objJustSaying["subject"] = "hub/login";
	objBody["challenge"] = challenge;
	objBody["max_message_count"] = MAX_MESSAGE_COUNT;
	objBody["max_message_length"] = MAX_MESSAGE_LENGTH;
	objBody["pubkey"] = (const char *) byteduino_device.keys.publicKeyM1b64;
	
		char sigb64[89];
	uint8_t hash[32];
	getSHA256ForJsonObject(hash ,objBody);
	//getHashToSignForChallenge(hash, challenge, byteduino_device.keys.publicKeyM1b64);
	getB64SignatureForHash(sigb64 ,byteduino_device.keys.privateM1,hash,32);
	
	objBody["signature"]  = (const char *) sigb64;
	objJustSaying["body"] = objBody;

	mainArray.add(objJustSaying);
	mainArray.printTo(output);
#ifdef DEBUG_PRINT
	Serial.println(output);
#endif
	webSocketForHub.sendTXT(output);

}