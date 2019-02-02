// Byteduino lib - papabyte.com
// MIT License

typedef struct keyring{
	 uint8_t privateM1[32];
	 char publicKeyM1b64[45];
	 char extPubKey[112];
	 uint8_t privateM4400[32];
	 char publicKeyM4400b64[45];
} keyring;


typedef struct Byteduino{
	keyring keys;
	char hub[MAX_HUB_STRING_SIZE];
	int port = 443;
	bool isInitialized = false;
	bool isConnected = false;
	bool isAuthenticated = false;
	bool isConnectingToSecondWebSocket = false;
	int  messengerKeyRotationTimer = 10;
	char deviceName[MAX_DEVICE_NAME_STRING_SIZE];
	char deviceAddress[34];
	char fundingAddress[33];
	bool isTestNet = false;

} Byteduino;

typedef struct messengerKeys{
uint8_t privateKey[32];
char pubKeyB64[45];
uint8_t previousPrivateKey[32];
char previousPubKeyB64[45];
} messengerKeys;

typedef struct bufferPackageReceived{
	bool isFree = true;
	uint8_t message[RECEIVED_PACKAGE_BUFFER_SIZE];
	char senderPubkey[45];
	bool hasUnredMessage = false;
	bool isRequestingNewMessage = false;
} bufferPackageReceived;


typedef struct bufferPackageSent{
	char message[SENT_PACKAGE_BUFFER_SIZE];
	char recipientPubkey[45];
	char recipientHub[MAX_HUB_STRING_SIZE];
	bool isOnSameHub = false;
	char recipientTempMessengerkey[45];
	bool isRecipientTempMessengerKeyKnown = false;
	bool isFree = true;
	bool isRecipientKeyRequested = false;
	int timeOut = 0;
} bufferPackageSent;

typedef struct queueXpubkeyTobeSent{
	bool isFree = true;
	char recipientPubKey[45];
	char recipientHub[MAX_HUB_STRING_SIZE];
} queueXpubkeyTobeSent;

typedef struct walletCreation{
	bool isCreating = false;
	queueXpubkeyTobeSent xPubKeyQueue[MAX_COSIGNERS];
	char id[45];
	char initiatorPubKey[45];
	char initiatorHub[MAX_HUB_STRING_SIZE];
} walletCreation;


typedef struct waitingConfirmationRoom{
	bool isFree = true;
	bool isConfirmed = false;
	bool isRefused = false;
	char recipientPubKey[45];
	char recipientHub[MAX_HUB_STRING_SIZE];
	char signing_path[MAX_SIGNING_PATH_SIZE];
	char address[33];
	uint8_t hash[32];
	char signedText[45];
	char sigb64[89];
	char JsonDigest[500];
} waitingConfirmationRoom;

typedef struct bufferPaymentStructure{
	bool isFree = true;
	int id = 0;
	int timeOut = 0;
	int amount = 0;
	char recipientAddress[33];
	char data[MAX_DATA_SIZE];
	bool arePropsReceived = false;
	bool isDefinitionReceived = false;
	bool requireDefinition = false;
	bool areInputsRequested = false;
	bool isPosted = false;
	bool hasDataFeed = false;
	char dataFeedJson [MAX_DATA_FEED_JSON_SIZE];
	char parent_units [2][45];
	char last_ball[45];
	char last_ball_unit[45];
	char witness_list_unit[45];
	char unit[45];
} bufferPaymentStructure;


typedef void (*cbMessageReceived)(const char* senderPubKey, const char* senderHub, const char* messageReceived);
typedef void (*cbSignatureToConfirm)(const char * signedTxt, const char* JsonDigest);
typedef void (*cbPaymentResult)(const int id, const int result_code, const char * unit_hash);
typedef void (*cbBalancesReceived)(JsonObject& balances);

