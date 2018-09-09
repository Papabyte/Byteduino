// Byteduino lib - papabyte.com
// MIT License

#include <Arduino.h>
#include <definitions.h>

int getRandomNumbersForUecc(uint8_t *dest, unsigned size);
bool getRandomNumbersForVector(uint8_t *dest, unsigned size);
bool getRandomNumbersForPrivateKey(uint8_t *dest, unsigned size);
bool getRandomNumbersForTag(uint8_t *dest, unsigned size);
void updateRandomPool();
bool isRandomGeneratorReady();
