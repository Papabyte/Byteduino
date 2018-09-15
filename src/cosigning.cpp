// Byteduino lib - papabyte.com
// MIT License

#include "cosigning.h"

walletCreation newWallet;
extern bufferPackageSent bufferForPackageSent;
extern Byteduino byteduino_device;
waitingConfirmationRoom waitingConfirmationSignature;

cbSignatureToConfirm _cbSignatureToConfirm;

void setCbSignatureToConfirm(cbSignatureToConfirm cbToSet){
	_cbSignatureToConfirm = cbToSet;
}

void removeKeyIfExisting(const char * key, JsonObject& object){
	if (object.containsKey(key)){
		object.remove(key);
#ifdef DEBUG_PRINT
		Serial.println(F("remove key"));
		Serial.println(key);
#endif
	}
	
}

String getOnGoingSignatureJsonString(){

	const size_t bufferSize = JSON_OBJECT_SIZE(3);
	StaticJsonBuffer<bufferSize> jsonBuffer;
	JsonObject & mainObject = jsonBuffer.createObject();
	if (!waitingConfirmationSignature.isFree){
		mainObject["signedText"] = (const char *) waitingConfirmationSignature.signedText;
		mainObject["digest"] = RawJson((const char *) waitingConfirmationSignature.JsonDigest);
		mainObject["isConfirmed"] = (const bool) waitingConfirmationSignature.isConfirmed;
		mainObject["isRefused"] = (const bool) waitingConfirmationSignature.isRefused;
	}
	String returnedString;
	mainObject.printTo(returnedString);
	return returnedString;
}

bool acceptToSign(const char * signedTxt){
	for (int i=0;i<45;i++){
		if (signedTxt[i]!= waitingConfirmationSignature.signedText[i]){
			return false;
		}
	}
	waitingConfirmationSignature.isConfirmed = true;
	return true;
}

bool refuseTosign(const char * signedTxt){
	for (int i=0;i<45;i++){
		if (signedTxt[i]!= waitingConfirmationSignature.signedText[i]){
			return false;
		}
	}
	waitingConfirmationSignature.isRefused = true;
	return true;
}

void treatWaitingSignature(){
	if (!waitingConfirmationSignature.isFree && (waitingConfirmationSignature.isConfirmed || waitingConfirmationSignature.isRefused)){
		if (bufferForPackageSent.isFree){
			waitingConfirmationSignature.isFree = true;
			const size_t bufferSize = 2*JSON_OBJECT_SIZE(4);
			DynamicJsonBuffer jsonBuffer(bufferSize);
			JsonObject & message = jsonBuffer.createObject();
			message["from"] = (const char*) byteduino_device.deviceAddress;
			message["device_hub"] = (const char*) byteduino_device.hub;
			message["subject"] = "signature";

			JsonObject & objBody = jsonBuffer.createObject();
			objBody["signed_text"]= waitingConfirmationSignature.signedText;
			if (waitingConfirmationSignature.isRefused)
				objBody["signature"] = "[refused]";
			else
				objBody["signature"] = (const char*) waitingConfirmationSignature.sigb64;
			objBody["signing_path"] = (const char*) waitingConfirmationSignature.signing_path;
			objBody["address"] = (const char*) waitingConfirmationSignature.address;

			message["body"]= objBody;

			message.printTo(bufferForPackageSent.message);
			loadBufferPackageSent(waitingConfirmationSignature.recipientPubKey, waitingConfirmationSignature.recipientHub);
#ifdef DEBUG_PRINT
			Serial.println(bufferForPackageSent.message);
#endif
		} else {
#ifdef DEBUG_PRINT
			Serial.println(F("Buffer not free to send message"));
#endif
		}
	}
	
}


void stripSignAndAddToConfirmationRoom(const char recipientPubKey[45],const char * recipientHub, JsonObject& body){

	JsonObject& unsignedUnit = body["unsigned_unit"];
	int authorsSize = unsignedUnit["authors"].size();

	for (int i=0; i < authorsSize; i++){
		JsonObject& objAuthentifier = unsignedUnit["authors"][i];
		objAuthentifier.remove("authentifiers");
#ifdef DEBUG_PRINT
		Serial.println(F("remove authentifier"));
#endif
	}
	
	//these unit properties aren't to be signed
	removeKeyIfExisting("unit", unsignedUnit);
	removeKeyIfExisting("headers_commission", unsignedUnit);
	removeKeyIfExisting("payload_commission", unsignedUnit);
	removeKeyIfExisting("main_chain_index", unsignedUnit);
	removeKeyIfExisting("timestamp", unsignedUnit);
	size_t messagesSize = unsignedUnit["messages"].size();
	
	//we create a JSON digest about what will signed
	DynamicJsonBuffer jsonBuffer(1000);
	JsonArray & arrayDigest = jsonBuffer.createArray();
	for (size_t i=0; i < messagesSize; i++){
		const char * app = unsignedUnit["messages"][i]["app"];
		JsonObject & objApp = jsonBuffer.createObject();

		if (strcmp(app,"payment") == 0){
			objApp["type"] = "payment";
			if (unsignedUnit["messages"][i]["payload"].is<JsonObject>()){
				JsonObject & payload = unsignedUnit["messages"][i]["payload"];
				const char* payload_asset = payload["asset"];
				if (payload_asset == nullptr)
					objApp["asset"] = (char*) "byte";
				else
					objApp["asset"] = (char *) payload_asset;
					
				if (payload["outputs"].is<JsonArray>()){
					if (!objApp.containsKey("outputs"))
						objApp.createNestedObject("outputs");
					size_t outputsSize = payload["outputs"].size();
					for (size_t j = 0; j<outputsSize; j++){
						const char* address = payload["outputs"][j]["address"];
						if (address != nullptr && strlen(address) == 32) {
							if (payload["outputs"][j]["amount"].is<int>()){
								int amountToAdd = payload["outputs"][j]["amount"];
								JsonObject & objAppOutputs =	objApp["outputs"];
								if (objAppOutputs.containsKey(address)){
									int previousAmount = objAppOutputs[address];
									objAppOutputs[address] = previousAmount + amountToAdd;
								} else {
									objAppOutputs[(char *)address] = amountToAdd;
								}
							}
						}
					}
				}
				arrayDigest.add(objApp);
			}
		} else {
			if (unsignedUnit["messages"][i]["payload"].is<JsonObject>()){
				arrayDigest.add(unsignedUnit["messages"][i]["payload"]);
			}
		}
		
		removeKeyIfExisting("payload", unsignedUnit["messages"][i]);
		removeKeyIfExisting("payload_uri", unsignedUnit["messages"][i]);
	}
	

	uint8_t hash[32];
	getSHA256ForJsonObject(hash, unsignedUnit);
	char sigb64 [89];
	getB64SignatureForHash(sigb64 ,byteduino_device.keys.privateM4400, hash,32);

	const char * signing_path= body["signing_path"];
	const char * address= body["address"];
	if (signing_path != nullptr){
		if(strlen(signing_path) < MAX_SIGNING_PATH_SIZE){
			if (address != nullptr){
				if (strlen(address) == 32){
					arrayDigest.printTo(waitingConfirmationSignature.JsonDigest);
					Serial.println(waitingConfirmationSignature.JsonDigest);
					memcpy(waitingConfirmationSignature.recipientPubKey,recipientPubKey, 45);
					memcpy(waitingConfirmationSignature.hash, hash, 32);
					memcpy(waitingConfirmationSignature.sigb64, sigb64, 89);
					strcpy(waitingConfirmationSignature.signing_path, signing_path);
					memcpy(waitingConfirmationSignature.address, address, 33);
					strcpy(waitingConfirmationSignature.recipientHub, recipientHub);
					Base64.encode(waitingConfirmationSignature.signedText,(char *)hash, 32);
					waitingConfirmationSignature.isConfirmed = false;
					waitingConfirmationSignature.isRefused = false;
					waitingConfirmationSignature.isFree = false;
					if (_cbSignatureToConfirm){
						_cbSignatureToConfirm(waitingConfirmationSignature.signedText, waitingConfirmationSignature.JsonDigest);
					}
				} else {
#ifdef DEBUG_PRINT
					Serial.println(F("wrong address size"));
#endif
				}
				
			} else {
#ifdef DEBUG_PRINT
				Serial.println(F("address must be a string"));
#endif
				}
		}else{
#ifdef DEBUG_PRINT
			Serial.println(F("signing_path too long"));
#endif
		}
	}else{
#ifdef DEBUG_PRINT
		Serial.println(F("signing_path must be a string"));
#endif
	}
}


void handleSignatureRequest(const char senderPubkey[45],JsonObject& receivedPackage){
	if (receivedPackage["body"]["unsigned_unit"].is<JsonObject>()){

		if (receivedPackage["body"]["unsigned_unit"]["messages"].is<JsonArray>()){
			const char* device_hub = receivedPackage["device_hub"];
			if (device_hub != nullptr){
				if(strlen(device_hub) < MAX_HUB_STRING_SIZE){
					int arraySize = receivedPackage["body"]["unsigned_unit"]["messages"].size();
					if (arraySize > 0){
						for (int i = 0;i<arraySize;i++){
							if(receivedPackage["body"]["unsigned_unit"]["messages"][i].is<JsonObject>()){
								if(receivedPackage["body"]["unsigned_unit"]["messages"][i]["payload"].is<JsonObject>()){
									const char * payloadHash = receivedPackage["body"]["unsigned_unit"]["messages"][i]["payload_hash"];
									char hashB64[45];
									getBase64HashForJsonObject (hashB64, receivedPackage["body"]["unsigned_unit"]["messages"][i]["payload"]);
									if (strcmp(hashB64,payloadHash) != 0){
#ifdef DEBUG_PRINT
									Serial.println(F("payload hash does not match"));
									Serial.println(hashB64);
									Serial.println(payloadHash);
#endif
									return;
									}
					

								}else{
#ifdef DEBUG_PRINT
									Serial.println(F("payload must be an object"));
#endif
									return;
								}
							}else{
#ifdef DEBUG_PRINT
								Serial.println(F("message must be an object"));
#endif
								return;
							}

						}
						stripSignAndAddToConfirmationRoom(senderPubkey,device_hub, receivedPackage["body"]);

					} else {
#ifdef DEBUG_PRINT
						Serial.println(F("arraySize must be >0"));
#endif
					}
				}
			}

		} else {
#ifdef DEBUG_PRINT
			Serial.println(F("messages must be an array"));
#endif
		}
	} else {
#ifdef DEBUG_PRINT
	Serial.println(F("unsigned_unit must be object"));
#endif
	}

}

