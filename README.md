##  A very light C/C++ implementation of Byteball for Arduino

**Library still at an experimental stage and under heavy development!**

#### Introduction

Byteball is a last generation crypto currency using a Directed Acyclic Graph instead of a blockchain like Bitcoin and its clones do. Brillantly designed, it offers a lot of features without suffering from complexity which makes it a great platform for IOT devices. This library is made to help hobbyists and professionals to implement Byteball into microcontrollers and build cool and original projects.

#### Features

- Handle pairing request
- Send and receive encrypted messages
- Handle multidevice wallet creation
- Cosign an unit containing transactions and/or data (private assets not supported)


#### Supported hardware

- [ ] ESP8266 [Arduino for ESP8266](https://github.com/esp8266/Arduino/)
- Limitations: due to the low amount of RAM available for program (around 40KB), this device cannot treat large messages. It cannot handle units containing more than 30 inputs/outputs and it must use the same hub as all paired devices and cosigners.
- Recommended board: Wifi WeMos D1 Mini Pro (~$5 retail price), including 16MB flash and USB-to-serial converter.

- [ ] ESP32 [Arduino for ESP32](https://github.com/espressif/arduino-esp32)
- coming soon!

#### Get started
##### Installation
git clone or unpack this github into the Arduino libraries folder.

##### External libraries
Some external libraries are required. You can add them to your Arduino IDE through the libraries manager.

 - **ArduinoJson** by Benoit Blanchon version 5.13.2 (not compatible with version 6!)
 - **micro-ecc** by Kenneth MacKay version 1.0.0
 - **WebSockets** by Markus Sattler version 2.1.0

Other third party code that needed to be tweaked is included in the project under the /libs folder.

##### Keys generation
coming soon

##### Minimal sketch

    #include <Arduino.h>
    #include <ESP8266WiFiMulti.h>
    #include <byteduino.h>
    
    ESP8266WiFiMulti WiFiMulti;
    
    void setup() {
    
      setDeviceName("Byteduino");
      setHub("byteball.org/bb-test"); //hub for testnet
    
      //don't forget to change the keys below, you will get troubles if more than 1 device is connected using the same keys
      setPrivateKeyM1("lgVGw/OfKKK4NqtK9fmJjbLCkLv7BGLetrdvsKAngWY=");
      setExtPubKey("xpub6Chxqf8hRQoLRJFS8mhFCDv5yzUNC7rhhh36qqqt1WtAZcmCNhS5pPndqhx8RmgwFhGPa9FYq3iTXNBkYdkrAKJxa7qnahnAvCzKW5dnfJn");
      setPrivateKeyM4400("qA1CxKyzvpKX9+IRhLH8OjLYY06H3RLl+zqc3lT86aI=");
    
      Serial.begin(115200);
      while (!Serial)
        continue;
    
      //we don't need an access point
      WiFi.softAPdisconnect(true);
    
      //set your wifi credentials here
      WiFiMulti.addAP("my_wifi_ssid", "my_wifi_password");
    
      while (WiFiMulti.run() != WL_CONNECTED) {
        delay(100);
        Serial.println(F("Not connected to wifi yet"));
      }
    
    }
    
    void loop() {
      // put your main code here, to run repeatedly:
      byteduino_loop();
    }


This code connects your device to wifi network, starts a websocket and authenticates to the hub.
By default the device will acknowledge any pairing or wallet creation request.

The serial monitor should return this:
```
Not connected to wifi yet
Not connected to wifi yet
Device address: 06XOQ6ZVWTV2ZWAEAIPFPJRGGQBMTHC6J
Device name: Byteduino
Pairing code: ArxPWIrgUvi5YuvugpJEE1aLwu9bRBOxoRFHSX9o6IyJ@byteball.org/bb-test#0000
Extended Pub Key:
xpub6Chxqf8hRQoLRJFS8mhFCDv5yzUNC7rhhh36qqqt1WtAZcmCNhS5pPndqhx8RmgwFhGPa9FYq3iTXNBkYdkrAKJxa7qnahnAvCzKW5dnfJn
Wss connected to: /bb-test
Authenticated by hub
```
To get verbose serial output, add `#define DEBUG_PRINT` to your sketch.

##### Functions references
Coming soon

##### Example sketches
Check example sketches in [examples folder](https://github.com/Papabyte/byteduino/tree/master/examples).

- [echo.ino](https://github.com/Papabyte/byteduino/blob/master/examples/echo/echo.ino): send back to sender any text message received
- [sign_anythin.ino](https://github.com/Papabyte/byteduino/blob/master/examples/sign_anything/sign_anything.ino): cosign any unit received

