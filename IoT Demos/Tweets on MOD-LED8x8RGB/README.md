# node.js demo for displaying tweets with #olimex or @olimex on MOD-LED8x8RGB

## Requirements

To use this demo you have to

1. Use Olimex IoT Firmware on your ESP8266-EVB
2. Configure your computer as IoT Server using ESP-Sample-Application.html
	* Select IoT tab
	* Check WebSocket
	* Uncheck SSL
	* Set Server to your computer IP address
	* Leave everything else blank

## Installation

First install node.js on your computer.

Then use the following command in command prompt:
	
	npm install olimex-iot-demo

## Usage

You should have the following environment variables set to be able to use Twitter API:
	
	TWITTER_CONSUMER_KEY
	TWITTER_CONSUMER_SECRET
	TWITTER_TOKEN
	TWITTER_TOKEN_SECRET

These are unique for every twitter account and they are obtained from https://apps.twitter.com
	
To start the demo use the following command
	
	[sudo] node node_modules\olimex-iot-demo\index.js
	
