// Byteduino lib - papabyte.com
// MIT License

#include "wallet.h"

extern walletCreation newWallet;
extern Byteduino byteduino_device;
extern bufferPackageSent bufferForPackageSent;

void readWalletsJson(char * json){
	const char firstChar = EEPROM.read(WALLETS_CREATED);
	if (firstChar == 0x7B){
		int i = -1; 
		do {
			i++;
			json[i] = EEPROM.read(WALLETS_CREATED+i);
		}
		while (json[i] != 0x00 && i < (WALLETS_CREATED_FLASH_SIZE));
		json[WALLETS_CREATED_FLASH_SIZE-1]=0x00;
	}else{
		json[0] = 0x7B;
		json[1] = 0x7D;
		json[2] = 0x00;
	}
	
}

String getWalletsJsonString(){
	const char firstChar = EEPROM.read(WALLETS_CREATED);
	char lastCharRead;
	String returnedString;

	if (firstChar == 0x7B){
		int i = -1; 
		do {
			i++;
			lastCharRead = EEPROM.read(WALLETS_CREATED+i);
			if (lastCharRead!= 0x00)
				returnedString += lastCharRead;
		}
		while (lastCharRead != 0x00 && i < (WALLETS_CREATED_FLASH_SIZE));
	}else{
		returnedString += 0x7B;
		returnedString += 0x7D;
	}
	return returnedString;
}

void saveWalletDefinitionInFlash(const char* wallet,const char* wallet_name, JsonArray& wallet_definition_template){
	char output[WALLETS_CREATED_FLASH_SIZE];
	char input[WALLETS_CREATED_FLASH_SIZE];
	char templateString[(int)WALLETS_CREATED_FLASH_SIZE*7/10];//templates definition size shouldn't take more than 0.7x the size of raw JSON
	DynamicJsonBuffer jb((int)WALLETS_CREATED_FLASH_SIZE*0.5); //this JSON needs a buffer of around half the size of its raw size
	wallet_definition_template.printTo(templateString);
	readWalletsJson(input);
#ifdef DEBUG_PRINT
	Serial.println(input);
#endif
	JsonObject& objectWallets = jb.parseObject(input);
	
	if (objectWallets.success()){
		if (objectWallets.containsKey(wallet)){
			if (objectWallets["wallet"].is<JsonObject>()){
				objectWallets["wallet"]["name"] = wallet_name;
				objectWallets["wallet"]["definition"] = (const char*) templateString;
			}
		} else{
			DynamicJsonBuffer jb2(250);
			JsonObject& objectWallet = jb2.createObject();
			objectWallet["name"] = wallet_name;
			objectWallet["definition"] = (const char*) templateString;
			objectWallets[wallet] = objectWallet;
		}

		if (objectWallets.measureLength() < WALLETS_CREATED_FLASH_SIZE){
			objectWallets.printTo(output);
		} else {
			return;
	#ifdef DEBUG_PRINT
		Serial.println(F("No flash available to store wallet"));
	#endif
		}
	#ifdef DEBUG_PRINT
		Serial.println(F("Save wallet in flash"));
		Serial.println(output);
	#endif

		int i = -1; 
		do {
			i++;
			EEPROM.write(WALLETS_CREATED+i, output[i]);
		}
		while (output[i]!= 0x00 && i < (WALLETS_CREATED+WALLETS_CREATED_FLASH_SIZE));
		EEPROM.commit();
	}else{
#ifdef DEBUG_PRINT
	Serial.println(F("Impossible to parse objectWallets"));
#endif
	}
}


void handleNewWalletRequest(char initiatiorPubKey [45], JsonObject& package){

	const char* wallet = package["body"]["wallet"];
	if (wallet != nullptr){
		if (package["body"]["is_single_address"].is<bool>() && package["body"]["is_single_address"]){
			if(package["body"]["other_cosigners"].is<JsonArray>()){
				int otherCosignersSize = package["body"]["other_cosigners"].size();
				if (otherCosignersSize > 0){
					const char * initiator_device_hub = package["device_hub"];
					if (initiator_device_hub != nullptr && strlen(initiator_device_hub) < MAX_HUB_STRING_SIZE){
						newWallet.isCreating = true;
						strcpy(newWallet.initiatorHub, initiator_device_hub);
						memcpy(newWallet.initiatorPubKey,initiatiorPubKey,45);
						memcpy(newWallet.id,wallet,45);

						
						for (int i; i < otherCosignersSize;i++){
							const  char* device_address = package["body"]["other_cosigners"][i]["device_address"];
							const  char* pubkey = package["body"]["other_cosigners"][i]["pubkey"];
							const  char* device_hub = package["body"]["other_cosigners"][i]["device_hub"];

							if (pubkey != nullptr && device_address != nullptr && device_hub != nullptr){
								if (strlen(device_hub) < MAX_HUB_STRING_SIZE){
									if (strcmp(device_address, byteduino_device.deviceAddress) != 0){
										newWallet.xPubKeyQueue[i].isFree = false;
										memcpy(newWallet.xPubKeyQueue[i].recipientPubKey, pubkey,45);
										strcpy(newWallet.xPubKeyQueue[i].recipientHub, device_hub);
									}
								}

							}
						}
					}
					newWallet.xPubKeyQueue[otherCosignersSize+1].isFree = false;
					memcpy(newWallet.xPubKeyQueue[otherCosignersSize+1].recipientPubKey, initiatiorPubKey, 45);
					strcpy(newWallet.xPubKeyQueue[otherCosignersSize+1].recipientHub, initiator_device_hub);

					const char* wallet_name = package["body"]["wallet_name"];
					if (wallet_name != nullptr && package["body"]["wallet_definition_template"].is<JsonArray>()){
						saveWalletDefinitionInFlash(wallet, wallet_name, package["body"]["wallet_definition_template"]);
					} else {
#ifdef DEBUG_PRINT
				Serial.println(F("wallet_definition_template and wallet_name must be char"));
#endif
				}
				} else {
#ifdef DEBUG_PRINT
				Serial.println(F("other_cosigners cannot be empty"));
#endif
				}
			} else {
#ifdef DEBUG_PRINT
				Serial.println(F("other_cosigners must be an array"));
#endif
			}
		} else {
		Serial.println(F("Wallet must single address wallet"));
		}
	} else {
#ifdef DEBUG_PRINT
			Serial.println(F("Wallet must be a char"));
#endif
	}

}


void treatNewWalletCreation(){

	if (newWallet.isCreating){
		bool isQueueEmpty = true;
		
		//we send our extended pubkey to every wallet cosigners
		for (int i=0;i<MAX_COSIGNERS;i++){
			
			if (!newWallet.xPubKeyQueue[i].isFree){
				isQueueEmpty = false;
				if(sendXpubkeyTodevice(newWallet.xPubKeyQueue[i].recipientPubKey, newWallet.xPubKeyQueue[i].recipientHub)){
					newWallet.xPubKeyQueue[i].isFree = true;
				}
			}
		}
		if (isQueueEmpty){
			if (sendWalletFullyApproved(newWallet.initiatorPubKey, newWallet.initiatorHub)){
				newWallet.isCreating = false;
			}
		}
	}
}
	
bool sendWalletFullyApproved(const char recipientPubKey[45], const char * recipientHub){

	if (bufferForPackageSent.isFree){
		const size_t bufferSize = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(4);
		StaticJsonBuffer<bufferSize> jsonBuffer;
		JsonObject & message = jsonBuffer.createObject();
		message["from"] = (const char*) byteduino_device.deviceAddress;
		message["device_hub"] = (const char*) byteduino_device.hub;
		message["subject"] = "wallet_fully_approved";

		JsonObject & objBody = jsonBuffer.createObject();
		objBody["wallet"]= (const char*) newWallet.id;
		message["body"]= objBody;

		loadBufferPackageSent(recipientPubKey, recipientHub);
		message.printTo(bufferForPackageSent.message);
#ifdef DEBUG_PRINT
		Serial.println(bufferForPackageSent.message);
#endif
		return true;
	} else {
#ifdef DEBUG_PRINT
			Serial.println(F("Buffer not free to send message"));
#endif
		return false;
	}
}

bool sendXpubkeyTodevice(const char recipientPubKey[45], const char * recipientHub){

	if (bufferForPackageSent.isFree){
		const size_t bufferSize = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4);
		StaticJsonBuffer<bufferSize> jsonBuffer;
		JsonObject & message = jsonBuffer.createObject();
		message["from"] = (const char*) byteduino_device.deviceAddress;
		message["device_hub"] = (const char*) byteduino_device.hub;
		message["subject"] = "my_xpubkey";
		
		JsonObject & objBody = jsonBuffer.createObject();
		objBody["wallet"]= (const char*) newWallet.id;
		objBody["my_xpubkey"]= (const char*) byteduino_device.keys.extPubKey;
		message["body"]= objBody;

		loadBufferPackageSent(recipientPubKey, recipientHub);
		message.printTo(bufferForPackageSent.message);
#ifdef DEBUG_PRINT
		Serial.println(bufferForPackageSent.message);
#endif
		return true;
	} else {
#ifdef DEBUG_PRINT
		Serial.println(F("Buffer not free to send pubkey"));
#endif
		return false;
	}
}

