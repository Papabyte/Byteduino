#include <Arduino.h>
#include <ESP8266WiFiMulti.h>
#include <byteduino.h>

#if defined (ESP32)
#include <WiFiMulti.h>
WiFiMulti WiFiMulti;
#endif

#if defined (ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti WiFiMulti;
#endif

void onSignatureRequest(const char signedTxt[], const char* JsonDigest) {

  Serial.println("Going to sign: ");
  Serial.println(JsonDigest);

  if (!confirmSignature(signedTxt)) {
    Serial.println(F("Couldn't confirm this signature"));
  };
  //mind that parameters will be deleted after function ends, if you need to copy them then do it by value
}

void setup() {

  setDeviceName("Byteduino");
  setHub("obyte.org/bb");

  //don't forget to change the keys below, you will get troubles if more than 1 device is connected using the same keys
  setPrivateKeyM1("lgVGw/OfKKK4NqtK9fmJjbLCkLv7BGLetrdvsKAngWY=");
  setExtPubKey("xpub6Chxqf8hRQoLRJFS8mhFCDv5yzUNC7rhhh36qqqt1WtAZcmCNhS5pPndqhx8RmgwFhGPa9FYq3iTXNBkYdkrAKJxa7qnahnAvCzKW5dnfJn");
  setPrivateKeyM4400("qA1CxKyzvpKX9+IRhLH8OjLYY06H3RLl+zqc3lT86aI=");

  setCbSignatureToConfirm(&onSignatureRequest);

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
