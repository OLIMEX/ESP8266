var ESP8266 = require('esp8266-iotjs');
var board = new ESP8266(true); //pass True for debug mode ON

board.on('push',function() {
	this.relayToggle();
});