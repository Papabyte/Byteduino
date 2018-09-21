// Byteduino lib - papabyte.com
// MIT License

#include "byteduino.h"
#include "random_gen.h"
#include "signature.h"
#include "libs/SHA256.h"


void setAndSendNewMessengerKey();
void getHashToSignForUpdatingMessengerKey(uint8_t* hash);
void getB64SignatureForUpdatingMessengerKey(char* sigb64, const uint8_t* hash);
void rotateMessengerKey();
void loadPreviousMessengerKeys();
void manageMessengerKey();