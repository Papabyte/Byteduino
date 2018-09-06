// Byteduino lib - papabyte.com
// MIT License

#include "hub_challenge.h"

extern WebSocketsClient webSocketForHub;
extern Byteduino byteduino_device;


void getHashToSignForChallenge(uint8_t* hash, const char* challenge, const char* publicKeyM1b64) {
	SHA256  hasher;
	hasher.update("challenge\0s\0", 12);
	hasher.update(challenge,strlen(challenge));
	hasher.update("\0pubkey\0s\0", 10);
	hasher.update(publicKeyM1b64, strlen(publicKeyM1b64));
	hasher.finalize(hash,32);
}


void respondToHubChallenge(const char* challenge) {
	
	if (strlen(challenge) != 40){
#ifdef DEBUG_PRINT
		Serial.println(F("challenge must be 40 chars long"));
#endif
		return;
	}
		
	char sigb64[89];
	uint8_t hash[32];
	getHashToSignForChallenge(hash, challenge, byteduino_device.keys.publicKeyM1b64);
	getB64SignatureForHash(sigb64 ,byteduino_device.keys.privateM1,hash,32);

	char output[270];
	const int capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + 20;
	StaticJsonBuffer<capacity> jsonBuffer;
	JsonArray & mainArray = jsonBuffer.createArray();

	mainArray.add("justsaying");
	JsonObject & objJustSaying = jsonBuffer.createObject();
	JsonObject & objBody = jsonBuffer.createObject();

	objJustSaying["subject"] = "hub/login";
	objBody["challenge"] = challenge;
	objBody["pubkey"] = (const char *) byteduino_device.keys.publicKeyM1b64;
	objBody["signature"]  = (const char *) sigb64;
	objJustSaying["body"] = objBody;

	mainArray.add(objJustSaying);
	mainArray.printTo(output);
#ifdef DEBUG_PRINT
	Serial.println(output);
#endif
	webSocketForHub.sendTXT(output);

}