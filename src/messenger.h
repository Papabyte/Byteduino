// Byteduino lib - papabyte.com
// MIT License


#include "byteduino.h"
#include "ArduinoJson.h"
#include "hashing.h"
#include "signature.h"

#include "libs/GCM.h"
#include "libs/AES.h"


void treatReceivedMessage(JsonObject& messageBody);
bool checkMessageBodyStructure(JsonObject& messageBody);
bool checkEncryptedPackageStructure(JsonObject& encryptedPackage);
bool checkMessageStructure(JsonObject& message);
void deleteMessageFromHub(const char* messageHash);
void decryptPackageAndPlaceItInBuffer(JsonObject& encryptedPackage, const char* senderPubkey);
void getMessageHashB64(char * hashB64, JsonObject& message);
void requestRecipientMessengerTempKey();
void checkAndUpdateRecipientKey(JsonObject& objResponse);
void encryptPackage(const char * recipientTempMessengerkey, char * messageB64,char * ivb64, char * authTagB64);
void encryptAndSendPackage();
void loadBufferPackageSent(const char * recipientPubKey, const char *  recipientHub);
void treatInnerPackage(JsonObject&  encryptedPackage);
void managePackageSentTimeOut();
void refreshMessagesFromHub();
void treatSentPackage();