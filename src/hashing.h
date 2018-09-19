// Byteduino lib - papabyte.com
// MIT License

#include "byteduino.h"
#include <ArduinoJson.h>
#include "libs/SHA256.h"
#include "libs/ripemd-160.h"

bool getBase64HashForJsonObject (char* hashB64, JsonObject& object);
bool getSHA256ForJsonObject(uint8_t hash[32] ,JsonObject& object);

void updateHash (SHA256* hasher,const char * string, size_t length);
void updateHash (ripemd160_ctx* hasher,const char * string, size_t length);

void getSHA256(uint8_t * hash ,const char * string, size_t inputLength, size_t outputLength);
void getRipeMD160ForObject(uint8_t hash[20] ,JsonObject& object);
void getRipeMD160ForString(uint8_t hash[20] , const char * string, size_t length);

void getDeviceAddress(const char * pubkey, char deviceAddress[34]);
template <class T> bool updateHashForObject (T hasher, JsonObject& object);
template <class T> void getChash160 (T input, char chash[32]);
template <class T> bool updateHashForArray (T hasher, JsonArray& array);

bool isChar1BeforeChar2(const char* char1, const char* char2);

