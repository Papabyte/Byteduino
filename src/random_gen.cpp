// Byteduino lib - papabyte.com
// MIT License

#include "random_gen.h"

byte fillingIndex = 0;
int randomPoolTicker = 0;
uint8_t randomPool[87];
bool isRandomPoolReady = false;


bool isRandomGeneratorReady(){
	return isRandomPoolReady;
}

bool getRandomNumber(uint8_t *dest, unsigned start, unsigned size){
	if (isRandomPoolReady){
	int poolIndex;
	randomPool[size + start]++;
while (size) {
	poolIndex = size + start;
	while (poolIndex > 86){
		 poolIndex -= 86;
	}

	*dest = randomPool[poolIndex];
	dest++;
	size--;
}
		return 1;
	} else {
		return 0;
	}
}

//we use different ranges of random pool for every kind of function/data
int getRandomNumbersForUecc(uint8_t *dest, unsigned size) {
	return (int) getRandomNumber(dest, 0, size);
}

bool getRandomNumbersForVector(uint8_t *dest, unsigned size) {
	return getRandomNumber(dest, 32, size);
}

bool getRandomNumbersForPrivateKey(uint8_t *dest, unsigned size) {
	return getRandomNumber(dest, 48, size);
}
bool getRandomNumbersForTag(uint8_t *dest, unsigned size) {
	return getRandomNumber(dest, 80, size);
}

//Combine randomPool numbers obtained from ESP8266 hardware number generator and less significant bits of cycles counter.
void updateRandomPool(){

	randomPoolTicker++;
	if (randomPoolTicker == RANDOM_POOL_TICKER_RESET){ //need to wait around 5 mega cycles to be sure random number generator is reliable
		randomPoolTicker = 0;
		uint32_t fromHardRandomGen = READ_PERI_REG(RANDOM_REGISTER);
		randomPool[fillingIndex] = (0xff) ^ fromHardRandomGen;
		randomPool[fillingIndex+1] = (0xff) ^ (fromHardRandomGen >> 8);
		randomPool[fillingIndex+2] = (0xff) ^ (fromHardRandomGen >> 16) ^ GET_CYCLE_COUNT;
		randomPool[fillingIndex+3] = (0xff) ^ (fromHardRandomGen >> 24) ^ (GET_CYCLE_COUNT >> 8);

		fillingIndex+=4;
		if (fillingIndex > 86) {
			fillingIndex = 0;
			isRandomPoolReady = true;
		}
	}
}