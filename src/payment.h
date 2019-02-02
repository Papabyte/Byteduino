// Byteduino lib - papabyte.com
// MIT License

#include "ArduinoJson.h"
#include "byteduino.h"

void requestDefinition(const char* address);
void handleDefinition(JsonObject& receivedObject) ;
void requestInputsForAmount(int amount, const char * address);
void handleInputsForAmount(JsonObject& receivedObject, const char * tag);
void getParentsAndLastBallAndWitnesses();
void getPaymentAddressFromPubKey(const char * pubKey, char * paymentAddress);
void handleUnitProps(JsonObject& receivedObject);
void managePaymentCompositionTimeOut();
int sendPayment(int amount, const char * address, const int payment_id);
void treatPaymentComposition();
void composeAndSendUnit(JsonArray& arrInputs, int total_amount);
void handlePostJointResponse(JsonObject& receivedObject, const char * tag);
int loadBufferPayment(const int amount, const bool hasDatafeed, const char * dataFeed, const char * recipientAddress, const int id);
int postDataFeed(const char * key, const char * value, const int id);
void handleBalanceResponse(JsonObject& receivedObject);
void setCbPaymentResult(cbPaymentResult cbToSet);
void setCbBalancesReceived(cbBalancesReceived cbToSet);
void getAvailableBalances();