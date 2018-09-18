// Byteduino lib - papabyte.com
// MIT License

#include <ArduinoJson.h>
#include "byteduino.h"
#include "hub_challenge.h"
#include "wallet.h"
#include "pairing.h"

void respondToHub(uint8_t *payload);
void respondToRequestFromHub(JsonArray& arr);
void respondToJustSayingFromHub(JsonObject& justSayingObject);

void sendErrorResponse(const char* tag, const char* error);
void sendHeartbeat();
void treatResponseFromHub(JsonArray& arr);
void treatReceivedPackage();
void requestCorrespondentPubkey(char senderPubkey [45]);
void getTag(char * tag, const char * extension);
void secondWebSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void connectSecondaryWebsocket();
bool isValidArrayFromHub(JsonArray& arr);
int sendTxtMessage(const char recipientPubkey [45],const char * deviceHub, const char * text);
void setCbTxtMessageReceived(cbMessageReceived cbToSet);
String getDomain(const char * hub);
String getPath(const char * hub);
