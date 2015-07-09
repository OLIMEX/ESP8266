if (typeof process.env.TWITTER_CONSUMER_KEY == 'undefined') {
	console.log('FATAL ERROR: Missing TWITTER_CONSUMER_KEY environment variable');
	process.exit(1001);
}

if (typeof process.env.TWITTER_CONSUMER_SECRET == 'undefined') {
	console.log('FATAL ERROR: Missing TWITTER_CONSUMER_SECRET environment variable');
	process.exit(1001);
}

if (typeof process.env.TWITTER_TOKEN == 'undefined') {
	console.log('FATAL ERROR: Missing TWITTER_TOKEN environment variable');
	process.exit(1001);
}

if (typeof process.env.TWITTER_TOKEN_SECRET == 'undefined') {
	console.log('FATAL ERROR: Missing TWITTER_TOKEN_SECRET environment variable');
	process.exit(1001);
}

var TRACK = [
	'#olimex',
	'@olimex'
];

// Init Messages queue
var messages = [];

const DEFAULT_MESSAGE         = 'Tweet with '+TRACK.join(' or ')+' to see your message here ;-)';
const MESSAGE_PAUSE           = 2000;

const MOD_TC_MK2              = 'MOD-TC-MK2';

const MOD_LED8x8RGB           = 'MOD-LED8x8RGB';
const MOD_LED8x8RGB_URL       = '/mod-led-8x8-rgb';

const MOD_LED8x8RGB_COLS      = 4;
const MOD_LED8x8RGB_ROWS      = 1;
const MOD_LED8x8RGB_SPEED     = 70;

// Initialization
var express = require('express');
var app = express();

var bodyParser = require('body-parser');
var jsonParser = bodyParser.json();

var http = require('http').Server(app);
var ws = require('websocket').server;

/*
 * Initialize WebSocket Server
 */
var websocket = new ws({
	httpServer: http,
	autoAcceptConnections: false
});

/*
 * Set common response HTTP headers
 */
function responseHeaders(response) {
	response.setHeader('Access-Control-Allow-Origin', '*');
	response.setHeader('Access-Control-Allow-Headers', 'Authorization, Content-Type');
}

/*
 * Handle HTTP OPTIONS requests
 */
app.options('*', 
	function(request, response) {
		console.log('OPTIONS ['+request.url+']');
		responseHeaders(response);
		response.send();
	}
);

/*
 * Handle HTTP GET requests
 */
app.get('*', 
	function(request, response) {
		console.log('GET ['+request.url+']');
		responseHeaders(response);
		response.send('{"Status" : "OK"}');
	}
);

/*
 * Listen on default HTTP port
 */
http.listen(
	80, 
	function() {
		console.log('Listening on *:80');	
	}
);

/*
 * Check if origin is allowed
 */
function originIsAllowed(origin) {
	// FIX THIS TO USE IN PRODUCTION ENVIRONMENT
	return true;
}

/*
 * Handle WebSocket requests
 */
websocket.on(
	'request', 
	function(request) {
		console.log('WEBSOCKET: Connection request ['+request.resource+']');
		
		if (!originIsAllowed(request.origin)) {
			// Make sure we only accept requests from an allowed origin 
			request.reject();
			console.log('WEBSOCKET: Connection from origin [' + request.origin + '] rejected.');
			return;
		}

		var connection = request.accept();
		console.log('WEBSOCKET: Connection accepted');
		
		// Start display messages
		setTimeout(
			function () {
				nextMessage(connection, MOD_LED8x8RGB_URL);
			},
			MESSAGE_PAUSE
		);
		
		connection.on(
			'message', 
			function(message) {
				if (message.type === 'utf8') {
					try {
						var data = JSON.parse(message.utf8Data);
						
						// Is this ESP8266 Event Message
						if (typeof data.EventData != 'undefined') {
							if (typeof data.EventData.Device != 'undefined') {
								// Is this is MOD-LED8x8RGB event
								if (
									data.EventData.Device == MOD_LED8x8RGB &&
									data.EventData.Status == 'Done'
								) {
									// Display next message after 2 seconds
									setTimeout(
										function () {
											nextMessage(connection, MOD_LED8x8RGB_URL)
										},
										MESSAGE_PAUSE
									);
								}
								
								// Is this is MOD-TC-MK2 event
								if (
									data.EventData.Device == MOD_TC_MK2 && 
									data.EventData.Status == 'OK'
								) {
									messages.push(
										{
											// Temperature message
											text: 'Temperature: '+data.EventData.Data.Temperature.toFixed(2),
											// Color WHITE
											color: {r: 1, g: 1, b: 1}
										}									
									);
								}
							}
						}
						
						console.log('WEBSOCKET: ' + message.utf8Data + '\n');
					} catch (err) {
						// console.log(err);
						console.log(message.utf8Data);
					}
				} else if (message.type === 'binary') {
					console.log('WEBSOCKET: Received Binary Message of ' + message.binaryData.length + ' bytes');
				}
			}
		);
		
		connection.on(
			'close', 
			function(reasonCode, description) {
				console.log('WEBSOCKET: CLOSE ' + connection.remoteAddress + ' ['+reasonCode+'] ['+description+']');
			}
		);
	}
);

// Init Twitter Stream 
var TwitterStream = require('node-tweet-stream');
var twitter = new TwitterStream(
	{
		consumer_key    : process.env.TWITTER_CONSUMER_KEY,
		consumer_secret : process.env.TWITTER_CONSUMER_SECRET,
		token           : process.env.TWITTER_TOKEN,
		token_secret    : process.env.TWITTER_TOKEN_SECRET
	}
);

/*
 * Handle tweets
 */
twitter.on(
	'tweet', 
	function (tweet) {
		console.log('TWEET: ['+tweet.user.name+'] '+tweet.text+'\n');
		messages.push(
			{
				// User name and tweet text
				text: tweet.user.name+': '+tweet.text,
				// Color CYAN
				color: {r: 0, g: 1, b: 1}
			}
			
		);
	}
);


/*
 * Handle Twitter errors
 */
twitter.on(
	'error', 
	function (err) {
		console.log('TWEET ERROR: '+err+'\n');
	}
);

for (i in TRACK) {
	twitter.track(TRACK[i]);
}

/*
 * Display next message
 */
function nextMessage(connection, url) {
	var msg;
	
	// Is there are tweets in the queue
	if (messages.length > 0) {
		// Get first message in the queue
		msg = messages.shift();
	} else {
		msg = {
			// Default message
			text: DEFAULT_MESSAGE,
			// Default message color YELLOW
			color: { r: 1, g: 1, b: 0}
		};
	}
	
	// Send message to ESP8266 to display it
	connection.sendUTF(
		JSON.stringify(
			{
				URL: url,
				Method: 'POST',
				Data: {
					cols: MOD_LED8x8RGB_COLS,
					rows: MOD_LED8x8RGB_ROWS,
					R: msg.color.r,
					G: msg.color.g,
					B: msg.color.b,
					Speed: MOD_LED8x8RGB_SPEED,
					Text: msg.text
				}
			}
		)
	);
}
