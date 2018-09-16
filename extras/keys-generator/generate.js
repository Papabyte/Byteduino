"use strict";

var Mnemonic = require('bitcore-mnemonic');
var Bitcore = require('bitcore-lib');
var ecdsa = require('secp256k1');
var readline = require('readline-sync');

var seed = readline.question("Enter your seed or just press enter to generate a new one\n");

if (seed.length>0){
	if (!Mnemonic.isValid(seed)){
		return console.log("seed is not valid");
	}
	
	var mnemonic = new Mnemonic(seed);
} else {
	var mnemonic = new Mnemonic(); // generates new mnemonic
	while (!Mnemonic.isValid(mnemonic.toString()))
		mnemonic = new Mnemonic();
}

console.log("Seed: " + mnemonic.toString());
console.log("////// It's strongly recommended to write down this seed. \\\\\\\n");

var xPrivKey = mnemonic.toHDPrivateKey("");
console.log("Configure your Byteduino device by adding these functions in your setup():\n");
console.log("setPrivateKeyM1(\"" + xPrivKey.derive("m/1'").privateKey.bn.toBuffer({size:32}).toString('base64')+ "\");");
console.log("setExtPubKey(\""+ Bitcore.HDPublicKey(xPrivKey.derive("m/44'/0'/0'")).toString('base64')+ "\");");
console.log("setPrivateKeyM4400(\"" + xPrivKey.derive("m/44'/0'/0'/0/0").privateKey.bn.toBuffer({size:32}).toString('base64')+ "\");");

console.log("\nThe default pairing code will be:");
console.log(ecdsa.publicKeyCreate(xPrivKey.derive("m/1'").privateKey.bn.toBuffer({size:32}), true).toString('base64')+"@byteball.org/bb#0000 for mainet");
console.log(ecdsa.publicKeyCreate(xPrivKey.derive("m/1'").privateKey.bn.toBuffer({size:32}), true).toString('base64')+"@byteball.org/bb-test#0000 for testnet");

