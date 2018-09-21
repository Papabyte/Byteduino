##  A very light C/C++ implementation of Byteball for Arduino ESP8266 and ESP32

**Library still at an experimental stage and under heavy development!**

#### Introduction

[Byteball](byteball.org) is a last generation crypto currency using a Directed Acyclic Graph (DAG) instead of a blockchain like Bitcoin and its clones do. Brillantly designed, it offers a lot of features without suffering from complexity which makes it a great platform for IoT devices. This library is made to help hobbyists and professionals to implement Byteball into microcontrollers and build cool and original projects.

To get support or discuss about this library, request an invitation to [Byteball Slack](http://slack.byteball.org/) and join #byteduino channel.

#### What can I do with this library?

##### Encrypted messenging
Byteball protocol integrates a communication layer that is used for private chat but also in background to exchange data like smart-contract definitions or private assets history.
Independently from any posting in DAG, this layer is available on your Byteduino device for your own benefits:
- Deploy state of the art cryptography based on ECDSA signing and AES encryption. You are sure of the identity of your correspondent and protect your communication even from the middle-men.
- Don't run a server that relays messages, use those from Byteball network. Your devices can always communicate without specific network configuration as long as they are connected to internet.
- Chat with your device using any GUI Byteball wallet running on macOS, Android, Iphone, Linux or Windows.
- Write Javascript application on top of an [headless wallet](https://github.com/byteball/headless-byteball) and have it interacting with your device.

##### Transaction cosigning
On such small device, it will never be possible to implement a full-featured Byteball client. But when needed, it's possible to delegate work to a distant server by charging it to prepare transactions that the device cosign.
after having checked only critical points. This way we ensure that the transaction has been ordered by the Byteduino device although the distant server could have been compromized.
The [hardware-cosigner](https://github.com/Papabyte/Hardware-cosigner) used as second factor authentification for a GUI wallet is a practical application of that.

##### Transaction broadcasting
coming next!
Beside sending payments, this function will allow to post data into the immutable public ledger while proving they existed at time they were posted.


#### Supported hardware
- [ ] ESP8266 [Arduino for ESP8266](https://github.com/esp8266/Arduino/)
- Limitations: due to the low amount of RAM available for program (around 40KB), this device cannot treat very large messages. It cannot handle units containing more than 50 to 100 inputs/outputs and it must use the same hub as all paired devices and cosigners.
- Recommended board: Wifi WeMos D1 Mini Pro (~$5 retail price), including 16MB flash and USB-to-serial converter.

- [ ] ESP32 [Arduino for ESP32](https://github.com/espressif/arduino-esp32)
- Recommended board: any board based on ESP32 WROOM module like ESP-32S NodeMCU or WeMos Lolin D32 ($5 to $10 retail price)

#### Get started
##### Installation
Search for Byteduino in Arduino library manager and install the last version.

##### External libraries
Some external libraries are required, you have to add them to your Arduino IDE through the library manager:

 - **ArduinoJson** by Benoit Blanchon version 5.13.2 (not compatible with version 6!)
 - **micro-ecc** by Kenneth MacKay version 1.0.0

Other third party code that needed to be tweaked is included in the project under the /libs folder.

##### Keys generation
For now the Byteduino device doesn't generate the private key used to derive identification and payments addresses for 2 reasons:
 - the hardware random number generator embedded in the chip is poorly document and we are not sure it's cryptographically secure.
 - we need to derive an extended pubkey to be provided to other cosigners, the required code isn't implemented yet.

So we use a good old computer to generate keys and we configure the Byteduino device in its sketch.

- [Browser based keys generator](https://papabyte.github.io/byteduino-keys-generator/)
- [Nodejs keys generator](https://github.com/Papabyte/byteduino/blob/master/extras/keys-generator/)

Copy-paste into your setup() the 3 functions with keys as parameters.

##### Minimal sketch

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
By default the device will acknowledge any pairing or wallet creation request (**for single address wallet only**).

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
To get verbose serial output, add `#define DEBUG_PRINT` to your byteduino.h.

##### Functions references
Coming soon

##### Example sketches
Check example sketches in [examples folder](https://github.com/Papabyte/byteduino/tree/master/examples).

- [echo.ino](https://github.com/Papabyte/byteduino/blob/master/examples/echo/echo.ino): send back to sender any text message received
- [sign_anythin.ino](https://github.com/Papabyte/byteduino/blob/master/examples/sign_anything/sign_anything.ino): cosign any unit received

##### Example projects

- [Hardware cosigner](https://github.com/Papabyte/Hardware-cosigner): hardware cosigner with web interface, compatible with GUI wallet to secure your funds.

#### Go further
The Byteball platform has rich features like human readable smart-contracts and the potential to solve a lot of issues. To know more about it, it's advised to read the very detailled [whitepaper](https://byteball.org/Byteball.pdf).

