## node.js demo for using esp8266-iotjs npm package to control ESP266-EVB board

#### The esp8266-iotjs npm package is in Beta version and is in process of development

[Npm package link](https://www.npmjs.com/package/esp8266-iotjs)

#### Use at your own risk, it is Beta 0.0.1


#### Work properly with only one board connected to host at the time
only work with 
* Relay
* Button
* more comming soon ++


### Requirements

To use this demo you have to

1. Use Olimex IoT Firmware on your ESP8266-EVB
2. Configure your computer as IoT Server using ESP-Sample-Application.html
	* Select IoT tab
	* Check WebSocket
	* Uncheck SSL
	* Set Server to your computer IP address
	* Leave everything else blank

### Installation

First install node.js  and NPM on your computer.

Then type the following command in command prompt:
	
	cd ESP8266-EVB-Relay
	npm install 

### Usage
To start the demo use the following command
	
	[sudo] node index.js
	


## Events
 - ready (when is web socket is open for requests)
 - relayStateIsChange (Is triggered when relay state is change)
 - push (Is triggered when button is pressed)


 ## Methods
 - setRelayState(param) - Change relay state with giving param, accept (1 or 0)
 - relayOn() - TurnOn the relay
 - relayOff() - TurnOff the relay
 - relayToggle() - Toggle relay state
