// Byteduino lib - papabyte.com
// MIT License

#include "ArduinoJson.h"
#include "byteduino.h"

void readPairedDevicesJson(char * json);
String getDevicesJsonString();
void savePeerInFlash(char peerPubkey[45],const char * peerHub, const char * peerName);
void acknowledgePairingRequest(char senderPubkey [45],const char * deviceHub, const char * reversePairingSecret);
void handlePairingRequest(JsonObject& package);