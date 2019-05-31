// Byteduino lib - papabyte.com
// MIT License
#include "hashing.h"

//#define DEBUG_HASHING

const byte offsets[] = {1,5,6,11,20,22,28,33,36,41,49,58,65,74,77,79,82,90,94,100,102,108,112,115,118,126,129,131,138,147,152,154};
const char base32Chars [] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";


bool getBase64HashForJsonObject (char* hashB64, JsonObject& object, bool bVersion2){
	uint8_t hash[32];
	if (!getSHA256ForJsonObject(hash,object,bVersion2))
		return false;
	Base64.encode(hashB64,  (char *) hash, 32);
	return true;
}

bool getSHA256ForJsonObject(uint8_t hash[32] ,JsonObject& object, bool bVersion2){
	SHA256 hasher;
	if (!updateHashForObject<SHA256&>(hasher, object, true, bVersion2)){
#ifdef DEBUG_PRINT
		Serial.println(F("Error while hashing object"));
#endif
		return false;
	}
	hasher.finalize(hash,32);
		return true;
}

void getSHA256(uint8_t *hash ,const char * string, size_t inputLength, size_t outputLength){
	SHA256 hasher;
	updateHash (hasher,string, inputLength);
	hasher.finalize(hash,outputLength);
}

void getRipeMD160ForString(uint8_t hash[20] , const char * string, size_t length){
	ripemd160_ctx* hasher = new ripemd160_ctx;
	rhash_ripemd160_init(hasher);
	updateHash(hasher,"s\0",2);
	updateHash(hasher,string,length);
	rhash_ripemd160_final(hasher,hash);
	delete hasher;
}

void getRipeMD160ForArray(uint8_t hash[20] ,JsonArray& object){
	ripemd160_ctx* hasher = new ripemd160_ctx;
	rhash_ripemd160_init(hasher);
	updateHashForArray<ripemd160_ctx*>(hasher, object, true, false);
	rhash_ripemd160_final(hasher,hash);
	delete hasher;
}

void updateHash (SHA256& hasher,const char * string, size_t length) {
	hasher.update(string,length);
}

void updateHash (ripemd160_ctx* hasher,const char * string, size_t length) {
	rhash_ripemd160_update(hasher,(const unsigned char*)string,length);
}

template <class T> bool updateHashForArray (T hasher, JsonArray& array, bool isFirst, bool bVersion2) {
	size_t arraySize = array.size();
	bool isFirstArrayItem = true;
	if (arraySize == 0) //empty array not accepted
		return false;
	
 if (isFirst || bVersion2){
#ifdef DEBUG_HASHING
		Serial.println("[");
#endif
		updateHash(hasher,"[",1);
	}else{
#ifdef DEBUG_HASHING
		Serial.println("$[");
#endif
		updateHash(hasher,"\0[",2);
	}
	//we hash keys and values by chunks 
	for (int i = 0; i < arraySize;i++){
		if (array[i].is<JsonObject>()){
			JsonObject& subObject = array[i];
			if (!updateHashForObject<T>(hasher, subObject, false, bVersion2))
				return false;
		} else if (array[i].is<JsonArray>()){
			JsonArray& subArray = array[i];
			if (!updateHashForArray<T>(hasher, subArray, false, bVersion2))
				return false;
		} else if (array[i].is<char*>()){
			const char* charToHash = array[i];
			if (charToHash != nullptr)
				updateHashForChar<T>(hasher, charToHash, bVersion2);
			else
				return false;
		} else if (array[i].is<int>()){
			int integer = array[i];
			updateHashForInteger<T>(hasher, integer, bVersion2);
		} else if (array[i].is<bool>()){
			bool flag = array[i];
			updateHashForBoolean<T>(hasher, flag, bVersion2);
		} else {
			return false;
		}
		isFirst = false;
		if(i != (arraySize-1) && bVersion2){ //if not last array item
#ifdef DEBUG_HASHING
			Serial.println(",");
#endif
			updateHash(hasher,",",1);
		}
	
	}
	if (bVersion2){
#ifdef DEBUG_HASHING
		Serial.println("]");
#endif
		updateHash(hasher,"]",1);
	}else{
#ifdef DEBUG_HASHING
		Serial.println("$]");
#endif
		updateHash(hasher,"\0]",2);
	}
	return true;
}


template <class T> bool updateHashForBoolean (T hasher, bool flag, bool bVersion2){

	if (bVersion2){
		if (flag){
#ifdef DEBUG_HASHING
			Serial.println("true");
#endif
			updateHash(hasher,"true",4);
		} else {
#ifdef DEBUG_HASHING
			Serial.println("false");
#endif
			updateHash(hasher,"false",5);
		}

	} else {

		updateHash(hasher,"\0b\0",3);
#ifdef DEBUG_HASHING
		Serial.println("$b$");
#endif
		if (flag){
#ifdef DEBUG_HASHING
			Serial.println("true");
#endif
			updateHash(hasher,"true",4);
	} else {
#ifdef DEBUG_HASHING
			Serial.println("false");
#endif
			updateHash(hasher,"false",5);
		}
	}
}


template <class T> bool updateHashForChar (T hasher, const char * charToHash, bool bVersion2){
	if (bVersion2){
#ifdef DEBUG_HASHING
			Serial.println("\"");
#endif
			updateHash(hasher,"\"",1);
#ifdef DEBUG_HASHING
			Serial.println(charToHash);
#endif
			updateHash(hasher,charToHash,strlen(charToHash));
			#ifdef DEBUG_HASHING
			Serial.println("\"");
#endif
			updateHash(hasher,"\"",1);
	} else {
#ifdef DEBUG_HASHING
		Serial.println("$s$");
#endif
		updateHash(hasher,"\0s\0",3);
#ifdef DEBUG_HASHING
		Serial.println(charToHash);
#endif
		updateHash(hasher,charToHash,strlen(charToHash));
	}
}

template <class T> bool updateHashForInteger (T hasher, const int integer, bool bVersion2){
	char str[16];
	sprintf(str, "%d", integer);
	if (bVersion2){
#ifdef DEBUG_HASHING
			Serial.println(str);
#endif
			updateHash(hasher,str,strlen(str));

	}else{
#ifdef DEBUG_HASHING
			Serial.println("$n$");
#endif
			updateHash(hasher,"\0n\0",3);
#ifdef DEBUG_HASHING
			Serial.println(str);
#endif
			updateHash(hasher,str,strlen(str));
	}
} 

template <class T> bool updateHashForObject (T hasher, JsonObject& object, bool isFirst, bool bVersion2) {

	char sortedKeys[MAX_KEYS_COUNT][MAX_KEY_SIZE];
	int keysCount = 0;
	if (object.size() > MAX_KEYS_COUNT)
		return false;
	//we create an array of object keys sorted by alphabetic order
	for (JsonObject::iterator it=object.begin(); it!=object.end(); ++it) {
		if (strlen(it->key) > MAX_KEY_SIZE)
			return false;
		if (keysCount > 0){
			int i = keysCount;
			while (isChar1BeforeChar2(it->key,sortedKeys[i-1]) && i>0){
				i--;
			}
			for (int j = keysCount; j>i;j--){
				strcpy(sortedKeys[j], sortedKeys[j-1]);
			}
				strcpy(sortedKeys[i], it->key);
		} else {
				strcpy(sortedKeys[0], it->key);
		}
		keysCount++;
	}
	if (keysCount == 0) //empty object not accepted
		return false;

	//we hash keys and values by chunks 

	for (int i = 0; i < keysCount;i++){
		if (!isFirst && !bVersion2){
#ifdef DEBUG_HASHING
			Serial.println("$");
#endif
			updateHash(hasher,"\0",1);
		}

		if (bVersion2){
			if (i==0){
#ifdef DEBUG_HASHING
			Serial.println("{");
#endif
			updateHash(hasher,"{",1);
			} 
#ifdef DEBUG_HASHING
			Serial.println("\"");
#endif
			updateHash(hasher,"\"",1);
		}

			const char* key = sortedKeys[i];
#ifdef DEBUG_HASHING
			Serial.println(key);
#endif
			updateHash(hasher,key,strlen(key));
		if (bVersion2){
#ifdef DEBUG_HASHING
			Serial.println("\":");
#endif
			updateHash(hasher,"\":",2);
		}
		if (object[key].is<JsonObject>()){


			JsonObject& subObject = object[key];
			if (!updateHashForObject<T>(hasher, subObject, false, bVersion2))
				return false;
		} else if (object[key].is<JsonArray>()){

			JsonArray& subArray = object[key];
			if (!updateHashForArray<T>(hasher, subArray, false, bVersion2))
				return false;
		} else if (object[key].is<int>()){
			int integer = object[key];
			updateHashForInteger<T>(hasher, integer, bVersion2);

		} else if (object[key].is<char*>()){
			const char* charToHash = object[sortedKeys[i]];
			if (charToHash == nullptr)
				return false;

			updateHashForChar<T>(hasher, charToHash, bVersion2);
		} else if (object[key].is<bool>()){
			bool flag = object[key];

			updateHashForBoolean<T>(hasher, flag, bVersion2);
		} else {
			return false;
		}
		isFirst = false;
		if (bVersion2){
			if(i != (keysCount-1)){ //if not last array item
#ifdef DEBUG_HASHING
				Serial.println(",");
#endif
				updateHash(hasher,",",1);
			} else{
#ifdef DEBUG_HASHING
				Serial.println("}");
#endif
				updateHash(hasher,"}",1);
			}
		}
	}
	return true;
}



//compare alphabetic order
bool isChar1BeforeChar2(const char* char1, const char* char2){

	int char1Length = strlen(char1);
	for (int i = 0; i < char1Length; i++){
		
		if (char1[i]<char2[i]){
			return true;
		} else if (char1[i]>char2[i]){
			return false;
		}
	}
	return true;
}

void getChecksum(char hash160[20], uint8_t checksum[]){
	uint8_t hashSHA256[32];
	getSHA256(hashSHA256, &hash160[4],16,32);
	checksum[0] = hashSHA256[5];
	checksum[1] = hashSHA256[13];
	checksum[2] = hashSHA256[21];
	checksum[3] = hashSHA256[29];
}

void getDeviceAddress(const char * pubkey, char deviceAddress[34]){
	char chash[33];
	getChash160ForString(pubkey, chash);
	deviceAddress[0] = 0x30;
	memcpy(deviceAddress+1, chash,33);
}


void getChash160ForString (const char * input, char chash[33]) {
	uint8_t hash160[20];
	getRipeMD160ForString(hash160, input, strlen(input));
	getChash160 (hash160, chash);
}

void getChash160ForArray (JsonArray& input, char chash[33]) {
	uint8_t hash160[20];
	getRipeMD160ForArray(hash160, input);
	getChash160 (hash160, chash);
}

void getChash160 (uint8_t * hash160, char chash[33]){

	bool checksumBin [32];
	bool hash160Bin [128];
	bool checksumedHashBin [160];

	uint8_t checksum[4];
	getChecksum((char *) hash160,checksum);

	//convert checksum bytes to bin
	for (int i = 0; i < 4;i++) {
		for (int j = 0; j < 8;j++) {
			checksumBin[i*8+j] = (checksum[i] << j) & 0x80;
		}
	}
	
	//convert hash160 to bin
	for (int i = 0; i < 16;i++) {
		for (int j = 0; j < 8;j++) {
			hash160Bin[i*8+j] = (hash160[i+4] << j) & 0x80;
		}
	}

	//mixChecksumIntoCleanData
	byte indexOffset = 0;
	for (byte i = 0 ;i<160;i++){
		if (i == offsets[indexOffset]){
			checksumedHashBin[i]= checksumBin[indexOffset];
			indexOffset++;

		} else {
			checksumedHashBin[i]= hash160Bin[i-indexOffset];
		}
	}

	//convert bin checksumed hash to base32
	int index =0;
	for (int i = 0; i<32;i++){
		index = checksumedHashBin[i*5] * 16 + checksumedHashBin[i*5+1] * 8 + checksumedHashBin[i*5+2] * 4 + checksumedHashBin[i*5+3] * 2  + checksumedHashBin[i*5+4];
		chash[i] = base32Chars[index];
	}
	chash[32]=0;
}

bool isValidChash160(const char * chash){

	if (strlen(chash) != 32){
#ifdef DEBUG_PRINT
		Serial.println(F("chash is not 32 length"));
#endif
		return false;
	}

	bool checksumBin [32];
	bool checksumedHashBin [160];
	bool hash160Bin [128];

	//convert base32 to bin
	char * pch;
	byte value = 0;
	for (byte i = 0; i<32; i++){
		pch = strchr(base32Chars, chash[i]);
		value = pch - base32Chars;
		for (byte j=5; j>0;j--){
			checksumedHashBin[i*5+j-1] = value & 1;
			value /=2;
		}
	}

	//unmix data
	byte indexOffset = 0;
	for (byte i = 0; i<160; i++){
		if (i == offsets[indexOffset]){
			checksumBin[indexOffset] = checksumedHashBin[i];
			indexOffset++;

		} else {
			hash160Bin[i-indexOffset] = checksumedHashBin[i];
		}
	}

	uint8_t hash160[20];
	//convert bin to byte
	for (int i = 0; i < 16;i++) {
		hash160[i+4] = binArrayToByte(&hash160Bin[i*8]);
	}

	uint8_t calculatedChecksum[4];
	getChecksum((char *) hash160,calculatedChecksum);

	//compare checksums
	for (int i = 0; i < 4;i++) {
		if (calculatedChecksum[i] != binArrayToByte(&checksumBin[i*8]))
			return false;
	}
	return true;
}

byte binArrayToByte (const bool binArray[8]){
	return binArray[0] * 128 + binArray[1] * 64 + binArray[2] * 32 + binArray[3] * 16 + binArray[4] * 8 
		+ binArray[5] * 4 + binArray[6] * 2  + binArray[7];
}