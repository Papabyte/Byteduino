// Byteduino lib - papabyte.com
// MIT License
#include "byteduino.h"
#include "libs/SHA256.h"
#include "signature.h"

void getHashToSignForChallenge(uint8_t* hash, const char* challenge, const char* publicKeyM1b64);
void respondToHubChallenge(const char* challenge);

