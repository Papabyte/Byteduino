// Byteduino lib - papabyte.com
// MIT License

#include <ArduinoJson.h>
#include "byteduino.h"

void readWalletsJson(char * json);
String getWalletsJsonString();
void saveWalletDefinitionInFlash(const char* wallet,const char* wallet_name, JsonArray& wallet_definition_template);
void handleNewWalletRequest(char initiatiorPubKey [45], JsonObject& package);
void treatNewWalletCreation();
bool sendXpubkeyTodevice(const char recipientPubKey[45], const char * recipientHub);
bool sendWalletFullyApproved(const char recipientPubKey[45], const char * recipientHub);