require('./main');
require('./lib/clock');
require('./lib/open-fest-program');

var Connections  = require('./lib/connections');
var Properties   = require('./lib/properties');
var Actions      = require('./lib/actions');
var Triggers     = require('./lib/triggers');
var ParamBuilder = require('./lib/param-builder');
var Messages     = require('./lib/messages');
var Badges       = require('./lib/badges');
var Badges       = require('./lib/open-fest-program');

try {
	var Twitter = require('./lib/twitter');
} catch (err) {
	console.log(err.message);
}

/****************************************************************************
 * Miscellaneous
 ****************************************************************************/

function rgbFlash(ledNode, r, g, b) {
	var ledConnection = Connections.get(ledNode);
	if (ledConnection !== null) {
		ledConnection.sendUTF(
			JSON.stringify(
				{
					Method: 'POST',
					URL: '/mod-rgb',
					Data: {R: r, G: g, B: b}
				}
			)
		);
		
		setTimeout(
			function () {
				ledConnection.sendUTF(
					JSON.stringify(
						{
							Method: 'POST',
							URL: '/mod-rgb',
							Data: {R: 0, G: 0, B: 0}
						}
					)
				);
			},
			2000
		);
	}
}

/****************************************************************************
 * Switch
 ****************************************************************************/
var RelayState = {};
function Relay(params) {
	RelayState[params.node] = params.relay;
}

function RelayToggle(params) {
	var connection = Connections.get(params.node);
	if (connection === null) {
		return;
	}
	
	connection.sendUTF(
		JSON.stringify(
			{
				Method: 'POST',
				URL: '/relay',
				Data: {
					Relay: RelayState[connection.node.Token] ? 0 : 1
				}
			}
		)
	);
}

Actions.
	register('Relay', Relay).
	parameter('node',  'string', true).
	parameter('relay', 'number', true)
;

Actions.
	register('RelayToggle', RelayToggle).
	parameter('node', 'string', true)
;	

// Relay
Triggers.onPropertyChange(
	null, 'ESP8266', 'Relay', null, null,
	'Relay',
	ParamBuilder().
		parameter('node', '[NodeToken]').
		parameter('relay', '[Relay]')
);
	
// Button
Triggers.onPropertyChange(
	null, 'ESP8266', 'Button', null, 'Short Release',
	'RelayToggle',
	ParamBuilder().
		parameter('node', 'ESP_SWITCH')
);

/****************************************************************************
 * EVB Display
 ****************************************************************************/

function EVBDisplay(params) {
	console.log('EVBDisplay('+JSON.stringify(params)+')');
	var messageQueue = Messages.init(params.node, 'MOD-LED8x8RGB', '/mod-led-8x8-rgb', 1, 6, 70);
	if (typeof Twitter == 'object') {
		messageQueue.defaultMsg(
			{
				text: 'Tweet with '+Twitter.tracking().join(' or ')+' to see your message here ;-)',
				r: 1, g: 1, b: 0
			}
		);
	}
}

Actions.
	register('EVBDisplay', EVBDisplay).
	parameter('node', 'string', true)
;
 
// EVBDisplay
Triggers.onRegisterDevice(
	null, '/mod-led-8x8-rgb',
	'EVBDisplay',
	ParamBuilder().
		parameter('node', '[NodeToken]')
);

/****************************************************************************
 * Door Actions
 ****************************************************************************/
 
function DoorStayClosed(params) {
	rgbFlash(params.ledNode, 50, 0, 0);
}

function DoorOpen(params) {
	var connection = Connections.get(params.node);
	if (connection === null) {
		return;
	}
	
	rgbFlash(params.ledNode, 0, 50, 0);
	
	connection.sendUTF(
		JSON.stringify(
			{
				Method: 'POST',
				URL: '/mod-io2',
				Data: {Relay1: 1}
			}
		)
	);
	
	setTimeout(
		function () {
			connection.sendUTF(
				JSON.stringify(
					{
						Method: 'POST',
						URL: '/mod-io2',
						Data: {Relay1: 0}
					}
				)
			);
		},
		2000
	);
}

Actions.
	register('DoorOpen', DoorOpen).
	parameter('node',    'string', true).
	parameter('ledNode', 'string', true)
;

Actions.
	register('DoorStayClosed', DoorStayClosed).
	parameter('ledNode', 'string', true)
;

Triggers.onPropertyChange(
	'EVB', 'MOD-FINGER', 'Status', null, 'Match found', 
	'DoorOpen',
	ParamBuilder().
		parameter('node',    'EVB').
		parameter('ledNode', 'EVB')
);

Triggers.onPropertyChange(
	'EVB', 'MOD-FINGER', 'Error', null, 'Not found',
	'DoorStayClosed',
	ParamBuilder().
		parameter('msgNode', 'EVB').
		parameter('ledNode', 'EVB')
);

/****************************************************************************
 * IFTTT Actions
 ****************************************************************************/

 function IFTTTButton(params) {
	rgbFlash(params.ledNode, 0, 50, 50);
 }
 
function IFTTTDo(params) {
	rgbFlash(params.ledNode, 50, 0, 50);
}

function IFTTTSwitch(params) {
	var connection = Connections.get(params.node);
	if (connection === null) {
		return;
	}
	
	connection.sendUTF(
		JSON.stringify(
			{
				Method: 'POST',
				URL: '/switch2',
				Data: JSON.parse('{"Relay'+params.relay+'" : 2}')
			}
		)
	);
}

Actions.
	register('IFTTTButton', IFTTTButton).
	parameter('ledNode', 'string', true)
;

Actions.
	register('IFTTTDo', IFTTTDo).
	parameter('ledNode', 'string', true)
;

Actions.
	register('IFTTTSwitch', IFTTTSwitch).
	parameter('node',  'string', true).
	parameter('relay', 'number', true)
;

// IFTTT triggers
Triggers.register(
	'$[?(@.IFTTTEvent=="button")]',
	'IFTTTButton',
	ParamBuilder().
		parameter('ledNode', 'EVB')
);

Triggers.register(
	'$[?(@.IFTTTEvent=="do")]',
	'IFTTTDo',
	ParamBuilder().
		parameter('ledNode', 'EVB')
);

Triggers.register(
	'$[?(@.IFTTTEvent=="switch1")]',
	'IFTTTSwitch',
	ParamBuilder().
		parameter('node',   'SWITCH2').
		parameter('relay', 1)
);
 
Triggers.register(
	'$[?(@.IFTTTEvent=="switch2")]',
	'IFTTTSwitch',
	ParamBuilder().
		parameter('node', 'SWITCH2').
		parameter('relay', 2)
);
 
 /****************************************************************************
 * Messages
 ****************************************************************************/

// Custom triggers
Triggers.onPropertyChange(
	'EVB', 'MOD-TC-MK2', 'Temperature', null, null,
	'Messages.display',
	ParamBuilder().
		parameter('node', '*').
		parameter('text', '[Temperature]°C')
);

Triggers.onPropertyChange(
	'EVB_BAT', 'ESP8266', 'BatteryPercent', null, null,
	'Messages.display',
	ParamBuilder().
		parameter('node', 'EVB').
		parameter('text', '[BatteryState]: [BatteryPercent]%').
		parameter('r', 0).
		parameter('g', 1).
		parameter('b', 1)
);

// Twitter triggers
Triggers.register(
	'$[?(@.TwitterEvent=="Tweet")]',
	'Messages.display',
	ParamBuilder().
		parameter('node', '*').
		parameter('text', '[TweetUser]: [TweetText]').
		parameter('r', 1).
		parameter('g', 0).
		parameter('b', 1)
);

/*
setInterval(
	function () {
		Messages.display(
			{
				node: '*',
				text: 'Test broadcast message from Olimex Development Team. Sorry for flooding!',
				r: 1, g: 0, b: 0
			}
		);
	},
	
	5000
);
*/
