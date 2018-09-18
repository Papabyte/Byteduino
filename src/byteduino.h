// Byteduino lib - papabyte.com
// MIT License


#include "libs/WebSocketsClient.h"
#include "definitions.h"

#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_
#include "communication.h"
#endif


#ifndef _STRUCTURES_H_
#define _STRUCTURES_H_
#include "structures.h"
#endif

#ifndef _MESSENGER_H_
#define _MESSENGER_H_
#include "messenger.h"
#endif

#ifndef _KEY_ROTATION_H_
#define _KEY_ROTATION_H_
#include "key_rotation.h"
#endif

#ifndef _COSIGNING_H_
#define _COSIGNING_H_
#include "cosigning.h"
#endif

#ifndef _WALLET_H_
#define _WALLET_H_
#include "wallet.h"
#endif


#include "EEPROM.h"
#include "Arduino.h"
#include "uECC.h"
#include "libs/Base64.h"
#include "random_gen.h"

//#define DEBUG_PRINT
//#define REMOVE_COSIGNING

void byteduino_init ();
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);

void byteduino_loop();
void timerCallback(void * pArg);
String getDeviceInfosJsonString();
void setHub(const char * hub);
void setDeviceName(const char * deviceName);
void setExtPubKey(const char * extPubKey);
void setPrivateKeyM1(const char * privKeyB64);
void setPrivateKeyM4400(const char * privKeyB64);
void printDeviceInfos();
