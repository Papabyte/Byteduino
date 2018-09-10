// Byteduino lib - papabyte.com
// MIT License

#include "byteduino.h"

void handleSignatureRequest(const char senderPubkey[45],JsonObject& package);
void treatWaitingSignature();
void stripSignAndAddToConfirmationRoom(const char recipientPubKey[45], JsonObject& body);
bool confirmSignature(const char * signedTxt);
void setCbSignatureToConfirm(cbSignatureToConfirm cbToSet);
void getOnGoingSignatureJson(char* json);