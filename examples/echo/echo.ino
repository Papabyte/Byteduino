#include <Arduino.h>
#include <ESP8266WiFiMulti.h>
#include <byteduino.h>

ESP8266WiFiMulti WiFiMulti;


void onTxtMessageReceived(const char* senderPubKey, const char* senderHub, const char* messageReceived) {

  if (!sendTxtMessage(senderPubKey, senderHub, messageReceived)) {
    Serial.println(F("I couldn't send echo")); //reason can be buffer not free, wrong size public key, too long message or hub url
  };
  //mind that parameters will be deleted after function ends, if you need to copy them then do it by value
}

void setup() {

  setDeviceName("Byteduino");
  setHub("byteball.org/bb-test"); //hub for testnet

  //don't forget to change the keys below, you will get troubles if more than 1 device is connected using the same keys
  setPrivateKeyM1("lgVGw/OfKKK4NqtK9fmJjbLCkLv7BGLetrdvsKAngWY=");
  setExtPubKey("xpub6Chxqf8hRQoLRJFS8mhFCDv5yzUNC7rhhh36qqqt1WtAZcmCNhS5pPndqhx8RmgwFhGPa9FYq3iTXNBkYdkrAKJxa7qnahnAvCzKW5dnfJn");
  setPrivateKeyM4400("qA1CxKyzvpKX9+IRhLH8OjLYY06H3RLl+zqc3lT86aI=");

  setCbTxtMessageReceived(&onTxtMessageReceived);

  Serial.begin(115200);
  while (!Serial)
    continue;

  //we don't need an access point
  WiFi.softAPdisconnect(true);

  //set your wifi credentials here
  WiFiMulti.addAP("you_wifi_SSID", "your_wifi_password");

  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
    Serial.println(F("Not connected to wifi yet"));
  }

}

void loop() {
  // put your main code here, to run repeatedly:
  byteduino_loop();
}
