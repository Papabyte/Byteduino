## Byteduino Nodejs keys generator

### Instructions
Once you installed nodejs on your computer, go in this folder and execute `npm install` then `node generate.js`. To restore a previous configuration, input a seed when the console prompts it or simply press enter to generate a new seed.
```Enter your seed or just press enter to generate a new one

Seed: come stick across panda father nurse butter rural crew virtual copy eternal
////// It's strongly recommended to write down this seed. \\\

Configure your Byteduino device by adding these functions in your setup():

setPrivateKeyM1("XBptNVmXD6iIV/9v8fWcW3u6ppoLruHmkVOOhDq3qhY=");
setExtPubKey("xpub6DTHVh8wc4U8x8zxwo8uYHFGD6M5FMtqbb9p7EutDWsGd1YNKvxVoC9dHV2jgUs3J7P8q7rHiGY9UkYqkdNfH95YdDSRRMaxCGcU8ScGsRa");
setPrivateKeyM4400("QPrHIP3fhPdLFfnXEXakwMegMm0IQRCmbQBCl/y4Iiw=");

The default pairing code will be:
A/n+A6gRfqy7GI19pMCRoDCPY8KOyy8Khruz0dlvrqhb@byteball.org/bb#0000
A/n+A6gRfqy7GI19pMCRoDCPY8KOyy8Khruz0dlvrqhb@byteball.org/bb-test#0000 for testnet
```
Copy paste in your setup() the 3 functions with keys as parameters to configre your device.
