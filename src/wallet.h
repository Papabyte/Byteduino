// Byteduino lib - papabyte.com
// MIT License

#include <ArduinoJson.h>
#include "byteduino.h"

void readWalletsJson(char * json);
void saveWalletDefinitionInFlash(const char* wallet,const char* wallet_name, JsonArray& wallet_definition_template);
void handleNewWalletRequest(char initiatiorPubKey [45], JsonObject& package);
void treatNewWalletCreation();
bool sendXpubkeyTodevice(const char recipientPubKey[45]);
bool sendWalletFullyApproved(const char recipientPubKey[45]);
