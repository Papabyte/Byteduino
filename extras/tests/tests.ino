#include "hashing.h"

bool testMaxKeySize() {

  DynamicJsonBuffer jb(1000);
  JsonObject& obj = jb.createObject();
  char key  [MAX_KEY_SIZE + 2] ;
  for (int i = 0; i <= MAX_KEY_SIZE; i++) {
    key[i] = 0x32;
  }

  SHA256 hasher;

  key[MAX_KEY_SIZE + 1] = 0x00;
  key[MAX_KEY_SIZE] = 0x00;

  obj[key] = "value";

  if (!updateHashForObject<SHA256&>(hasher, obj, false, true)) { //normal key size
    return false;
  }
  key[MAX_KEY_SIZE] = 0x32;
  obj[key] = "value";

  if (updateHashForObject<SHA256&>(hasher, obj, false, true)) { //oversized key
    return false;
  }

  return true;
}


bool testMaxKeysCount() {

  DynamicJsonBuffer jb(1000);
  JsonObject& obj = jb.createObject();
  char key[30];
  for (int i = 0; i < MAX_KEYS_COUNT; i++) {
    sprintf(key, "%d", i);
    obj[key] = "value";
  }

  SHA256 hasher;
  if (!updateHashForObject<SHA256&>(hasher, obj, true, false) || !updateHashForObject<SHA256&>(hasher, obj, false, false) || !updateHashForObject<SHA256&>(hasher, obj, true, true) || !updateHashForObject<SHA256&>(hasher, obj, false, true)) {
    return false;
  }
  sprintf(key, "%d", MAX_KEYS_COUNT);
  obj[key] = "value";

  if (updateHashForObject<SHA256&>(hasher, obj, true, false) || updateHashForObject<SHA256&>(hasher, obj, false, false) || updateHashForObject<SHA256&>(hasher, obj, true, true) || updateHashForObject<SHA256&>(hasher, obj, false, true)) { //overcount
    return false;
  }
  return true;
}

bool testNullValueInObject() {

  DynamicJsonBuffer jb(1000);
  JsonObject& obj = jb.createObject();
  obj["key"] = (char*) 0;
  SHA256 hasher;
  if (updateHashForObject<SHA256&>(hasher, obj, true, false) || updateHashForObject<SHA256&>(hasher, obj, false, false) || updateHashForObject<SHA256&>(hasher, obj, true, true) || updateHashForObject<SHA256&>(hasher, obj, false, true)) {
    return false;
  }
  return true;
}


bool testObjectHash() {
  if (testMaxKeySize()) {
    Serial.println("Max key size test passed");
  }
  else {
    Serial.println("Max key size test failed");
    return false;
  }

  if (testMaxKeysCount()) {
    Serial.println("Max keys count test passed");
  }
  else {
    Serial.println("Max keys count test failed");
    return false;

  }
  if (testNullValueInObject()) {
    Serial.println("Null value in object test passed");
  }
  else {
    Serial.println("Null value in object test failed");
    return false;

  }

  DynamicJsonBuffer jb(1000);
  JsonObject& obj = jb.parse("{\"key\":\"value\", \"key2\":\"2\",\"key4\":{\"key\":\"value\",\"key7\":[45,\"string\",\"0\", [\"string\",true,false]]}, \"key6\":true, \"key5\":18}");
  char base64Hash[45];
  getBase64HashForJsonObject(base64Hash, obj, false);
  if (strlen(base64Hash) != 44) {
    Serial.println("Wrong base 64 hash length");
    return false;
  }
  if (strcmp(base64Hash, "bz8SOiezDEAOlP7EfZbiK84Q8wecOERpyq2F2eJdw2g=") != 0) {
    Serial.println("Wrong calculated hash");
    return false;
  }
  getBase64HashForJsonObject(base64Hash, obj, true);
  if (strlen(base64Hash) != 44) {
    Serial.println("Wrong base 64 hash length");
    return false;
  }
  if (strcmp(base64Hash, "MH6OypP+HuecTHIy5n/xABQx2gkVfjxaKseb5/52vV8=") != 0) {
    Serial.println("Wrong calculated hash");
    return false;
  }

  getBase64HashForJsonObject(base64Hash, obj, true);


  return true;
}



bool testNullValueInArray() {

  DynamicJsonBuffer jb(1000);
  JsonArray& arr = jb.createArray();
  arr.add((char*)0);
  SHA256 hasher;
  if (updateHashForArray<SHA256&>(hasher, arr, true, false) || updateHashForArray<SHA256&>(hasher, arr, false, false) || updateHashForArray<SHA256&>(hasher, arr, true, true) || updateHashForArray<SHA256&>(hasher, arr, false, true)) {
    return false;
  }
  return true;
}

bool testEmptyArray() {
  DynamicJsonBuffer jb(1000);
  JsonArray& arr = jb.createArray();

  SHA256 hasher;
  if (updateHashForArray<SHA256&>(hasher, arr, true, false) || updateHashForArray<SHA256&>(hasher, arr, false, false) || updateHashForArray<SHA256&>(hasher, arr, true, false) || updateHashForArray<SHA256&>(hasher, arr, false, false)) {
    return false;
  }
  return true;

}


bool testArrayHash() {
  if (testEmptyArray()) {
    Serial.println("Empty array test passed");
  }
  else {
    Serial.println("Empty array count test failed");
    return false;
  }
  if (testNullValueInArray()) {
    Serial.println("Null value in array test passed");
  }
  else {
    Serial.println("Null value in array test failed");
    return false;
  }

  DynamicJsonBuffer jb(1000);
  JsonArray& arr = jb.parse("[\"sig\",{\"pubkey\": \"Avuy9tgZA7UoLtDla8ktdYvVNO0hiIVBS8pC29YeZg4y\"}]");
  char chash[33];
  getChash160ForArray(arr, chash);
  if (strlen(chash) != 32) {
    Serial.println("wrong chash length");
    return false;
  }
  if (strcmp("HJVVOVFUGDA3NGO33Y3LTP3IDOZHLOMX", chash) != 0) {
    Serial.println("wrong chash");
    return false;

  }
  return true;

}

void setup()
{
  Serial.begin(115200);
  if (!testObjectHash() || !testArrayHash()) {
    Serial.println("Test failed");
  } else {
    Serial.println("All tests passed");
  }

}


void loop()
{

}


