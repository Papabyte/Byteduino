// Byteduino lib - papabyte.com
// MIT License

#include "signature.h"

#if defined(ESP32)
extern hw_timer_t * watchdogTimer;
#endif

bool decodeAndCopyPrivateKey(uint8_t * decodedPrivKey ,const char * privKeyB64){

	if(strlen(privKeyB64) == 44){
		Base64.decode(decodedPrivKey, privKeyB64, 44);
		return true;
	}
	return false;
}

void getCompressAndEncodePubKey (const uint8_t * privateKey, char * pubkeyB64){

	uint8_t pubKey[64];
	uint8_t compressedPubkey[33];
	uECC_compute_public_key(privateKey, pubKey,uECC_secp256k1());
	uECC_compress(pubKey, compressedPubkey,uECC_secp256k1());
	Base64.encode(pubkeyB64, (char *) compressedPubkey, 33);

}

void decodeAndDecompressPubKey(const char * pubkey, uint8_t decompressedPubkey[64]){

	uint8_t compressedRecipientPubkey[33];
	Base64.decode(compressedRecipientPubkey, pubkey, 45);
	uECC_decompress(compressedRecipientPubkey, decompressedPubkey, uECC_secp256k1());

}


void getB64SignatureForHash(char * sigb64 ,const uint8_t * privateKey, const uint8_t * hash,size_t length){

	uint8_t signature[64];
	do {
		uECC_sign(privateKey, hash, length, signature, uECC_secp256k1());
		FEED_WATCHDOG;
	} while (signature[32] > 0x79);//hub would refuse a high-s signature
	Base64.encode(sigb64, (char *) signature, 64);

}
