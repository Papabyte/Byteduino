// Byteduino Get Balance - papabyte.com
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

bool balances_requested = false;

void onBalancesReceived(JsonObject& balances) {
  int stable_balance = balances["base"]["stable"];
  int pending_balance = balances["base"]["pending"];
  Serial.println("Bytes confirmed:");
  Serial.println(stable_balance);
  Serial.println("Bytes waiting for confirmation:");
  Serial.println(pending_balance);
}

void setup() {

  setDeviceName("Byteduino");
  setHub("obyte.org/bb"); //main hub
  //setTestNet(); //comment this to switch to mainnet

  //don't forget to change the keys below, you will get troubles if more than 1 device is connected using the same keys
  setPrivateKeyM1("lgVGw/OfKKK4NqtK9fmJjbLCkLv7BGLetrdvsKAngWY=");
  setExtPubKey("xpub6Chxqf8hRQoLRJFS8mhFCDv5yzUNC7rhhh36qqqt1WtAZcmCNhS5pPndqhx8RmgwFhGPa9FYq3iTXNBkYdkrAKJxa7qnahnAvCzKW5dnfJn");
  setPrivateKeyM4400("qA1CxKyzvpKX9+IRhLH8OjLYY06H3RLl+zqc3lT86aI=");

  setCbBalancesReceived(&onBalancesReceived);


  Serial.begin(115200);
  while (!Serial)
    continue;

  //we don't need an access point
  WiFi.softAPdisconnect(true);

  //set your wifi credentials here
  WiFiMulti.addAP("My_SSID", "My_Password");

  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
    Serial.println(F("Not connected to wifi yet"));
  }

}

void loop() {
  // put your main code here, to run repeatedly:
  if (isDeviceConnectedToHub() && !balances_requested) {
    getAvailableBalances();
    balances_requested = true;
  }

  byteduino_loop();
}
