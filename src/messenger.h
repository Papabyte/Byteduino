// Byteduino lib - papabyte.com
// MIT License


#include "byteduino.h"
#include "ArduinoJson.h"
#include "hashing.h"
#include "signature.h"

#include "libs/GCM.h"
#include "libs/AES.h"



//#include "Crypto.h"



void respondToMessage(JsonObject& messageBody);
bool checkMessageBodyStructure(JsonObject& messageBody);
bool checkMessageStructure(JsonObject& message);
void deleteMessageFromHub(const char* messageHash);
void decryptPackageAndPlaceItInBuffer(JsonObject& message);
void getMessageHashB64(char * hashB64, JsonObject& message);
void requestRecipientMessengerTempKey();
void checkAndUpdateRecipientKey(JsonObject& objResponse);
void encryptAndSendPackage();
void loadBufferPackageSent(const char * recipientPubKey, const char *  recipientHub);