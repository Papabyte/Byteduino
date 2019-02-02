// Byteduino Post Datafeed - papabyte.com
// MIT License

#include <Arduino.h>
#include <byteduino.h>

#if defined (ESP32)
#include <WiFiMulti.h>
WiFiMulti WiFiMulti;
#endif

#if defined (ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti WiFiMulti;
#endif

bool payment_placed_in_buffer = false;

void onPaymentResult(const int id, const int result_code, const char * unit_hash) {
  Serial.println(id); // id that was attributed to payment
  Serial.println(unit_hash); // hash of composed unit
  switch (result_code) {
    case PAYMENT_ACKNOWLEDGED:
      Serial.println(F("Payment accepted by hub"));
      break;
    case NOT_ENOUGH_FUNDS:
      Serial.println(F("Not enough funds"));
      break;
    case TIMEOUT_PAYMENT_NOT_SENT: // something went wrong, payment is not sent.
      Serial.println(F("Timeout expired - Payment not sent"));
      break;
    case TIMEOUT_PAYMENT_NOT_ACKNOWLEDGED: // unit has been sent to hub but no response was made, we don't know if the payment was received by Obyte network. This is unlikely to happen but it's a case that has to be handled.
                                           // In case the hub is very long to respond, it's always possible that a second event comes later indicating a payment accepted or refused
      Serial.println(F("Timeout expired - Payment not acknowledged by hub"));
      break;
    case PAYMENT_REFUSED: // payment has been refused by hub
      Serial.println(F("Payment refused by hub"));
      break;
  }

}

void setup() {

  setDeviceName("Byteduino");
  setHub("obyte.org/bb"); //main net hub
  //setTestNet(); //uncomment this to switch to testnet

  //don't forget to change the keys below, you will get troubles if more than 1 device is connected using the same keys
  setPrivateKeyM1("lgVGw/OfKKK4NqtK9fmJjbLCkLv7BGLetrdvsKAngWY=");
  setExtPubKey("xpub6Chxqf8hRQoLRJFS8mhFCDv5yzUNC7rhhh36qqqt1WtAZcmCNhS5pPndqhx8RmgwFhGPa9FYq3iTXNBkYdkrAKJxa7qnahnAvCzKW5dnfJn");
  setPrivateKeyM4400("qA1CxKyzvpKX9+IRhLH8OjLYY06H3RLl+zqc3lT86aI=");

  setCbPaymentResult(&onPaymentResult);


  Serial.begin(115200);
  while (!Serial)
    continue;

  //we don't need an access point
  WiFi.softAPdisconnect(true);

  //set your wifi credentials here
  WiFiMulti.addAP("My_SSID", "My_PASSWORD");

  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
    Serial.println(F("Not connected to wifi yet"));
  }

}

void loop() {
  // put your main code here, to run repeatedly:
  if (isDeviceConnectedToHub() && !payment_placed_in_buffer) {
    
    int error =  postDataFeed("I_like","Obyte", 563912); // we post a datafeed with I_like as key and Obyte as value, we attribute 563912 as id for this operation. Only the amount needed to pay for transaction fees will be spend.
    switch (error) {
      case SUCCESS: // it doesn't mean that the datafeed is actually posted yet, the final status will be given by the onPaymentResult callback
        Serial.println(F("Payment successfully placed in buffer"));
        payment_placed_in_buffer = true;
        break;
      case BUFFER_NOT_FREE: // another payment is ongoing so this attempt was ignored
        Serial.println(F("Buffer is not free"));
        break;
    }
  }

  byteduino_loop();
}
