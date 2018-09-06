// Byteduino lib - papabyte.com
// MIT License

#include "byteduino.h"


void decodeAndDecompressPubKey(const char * pubkey, uint8_t decompressedPubkey[64]);
void getB64SignatureForHash(char * sigb64 ,const uint8_t * privateKey,const uint8_t * hash,size_t length);
void getCompressAndEncodePubKey (const uint8_t * privateKey, char * pubkeyB64);
bool decodeAndCopyPrivateKey(uint8_t * decodedPrivKey ,const char * privKeyB64);