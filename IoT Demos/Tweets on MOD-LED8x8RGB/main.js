var express = require('express');
var app = express();

var jsonParser = require('body-parser').json();

var http = require('http').Server(app);
var ws = require('websocket').server;
var websocket = new ws({
	httpServer: http,
	autoAcceptConnections: false
});

var JSONPath     = require('advanced-json-path');
var Connections  = require('./lib/connections');
var Triggers     = require('./lib/triggers');

const HTTP_PORT = 80;

function responseHeaders(response) {
	response.setHeader('Access-Control-Allow-Origin', '*');
	response.setHeader('Access-Control-Allow-Headers', 'Authorization, Content-Type');
}

app.options('*', 
	function(request, response) {
		console.log('OPTIONS ['+request.url+']');
		responseHeaders(response);
		response.send();
	}
);

app.get('/nodes', 
	function(request, response) {
		console.log('GET ['+request.url+'] ['+request.ip+']');
		responseHeaders(response);
		response.send(Connections.getNodes());
	}
);

app.get('*', 
	function(request, response) {
		console.log('GET ['+request.url+'] ['+request.ip+']');
		responseHeaders(response);
		response.send('{"Status" : "OK"}');
	}
);

app.post('*', jsonParser, 
	function(request, response) {
		console.log('POST ['+request.url+'] ['+request.ip+']');
		if (!request.body) {
			return response.sendStatus(400);
		}
		
		Triggers.fire(request.body);
		
		responseHeaders(response);
		response.send('{"Status" : "OK"}');
	}
);

http.listen(HTTP_PORT, 
	function() {
		console.log('NodeJS listening on *:'+HTTP_PORT);	
	}
);

function originIsAllowed(origin) {
	// FIX THIS TO USE IN PRODUCTION ENVIRONMENT
	return true;
}

websocket.on(
	'request', 
	function(request) {
		console.log('WEBSOCKET: Connection request ['+request.resource+'] ['+request.remoteAddress+']');
		
		if (!originIsAllowed(request.origin)) {
			// Make sure we only accept requests from an allowed origin 
			request.reject();
			console.log('WEBSOCKET: Connection from origin [' + request.origin + '] rejected.');
			return;
		}

		var connection = request.accept();
		Connections.add(connection);
		console.log('WEBSOCKET: Connection accepted');
		
		var time = new Date().getTime();
		
		connection.on(
			'message', 
			function(message) {
				if (message.type === 'utf8') {
					var data = null;
					try {
						data = JSON.parse(message.utf8Data);
						
						if (JSONPath(data, '$[?($.Name)][?($.Token)]')) {
							// Node identification
							Connections.setName(connection, data.Name);
							Connections.setToken(connection, data.Token);
							
							connection.sendUTF(
								JSON.stringify(
									{
										URL: '/devices',
										Method: 'GET'
									}
								)
							);
						}
						
						data.EventNode = {
							IP:    connection.remoteAddress,
							Name:  connection.node.Name,
							Token: connection.node.Token
						}
						
						var devices = JSONPath(data, '$[?($.EventURL == "/devices")].EventData.Data.Devices[?(@.Found == 1)]');
						if (devices !== false) {
							console.log('REGISTER DEVICES:');
							connection.node.Devices = connection.node.Devices.concat(devices);
						}
					} catch (err) {
						console.log('ERROR@'+connection.node.Token+' ['+err+']');
						console.log('Data: '+message.utf8Data);
						return;
					}
					
					// TODO Log data
					Triggers.fire(data);
				} else if (message.type === 'binary') {
					console.log('WEBSOCKET: ['+ connection.node.Token + '] Received Binary Message of ' + message.binaryData.length + ' bytes');
				}
			}
		);
		
		connection.on(
			'close', 
			function(reasonCode, description) {
				Triggers.fire(
					{
						EventURL: null,
						
						EventData: {
							Status: 'Connection close'
						},
						
						EventNode: {
							IP:    connection.remoteAddress,
							Name:  connection.node.Name,
							Token: connection.node.Token
						}			
					}
				);
				
				Connections.remove(connection);
				console.log('WEBSOCKET: ['+ connection.node.Token + '] CLOSE [' + connection.remoteAddress + '] ['+reasonCode+': '+description+']');
			}
		);
	}
);
