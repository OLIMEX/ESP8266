
const ESP8266_button           = 'ESP8266';
const ESP8266_button_URL       = '/button';


const ESP8266_relay_URL       = '/relay';

var lastRelayState = 0;
var count = 0;


// Initialization
var express = require('express');
var app = express();

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
        
        connection.on(
            'message', 
            function(message) {
                if (message.type === 'utf8') {
                    try {
                        var data = JSON.parse(message.utf8Data);
                        
                        // Is this ESP8266 Event Message
                        if (typeof data.EventData != 'undefined') {
                            if (typeof data.EventData.Device != 'undefined') {

                                switch(data.EventURL){
                                    case ESP8266_button_URL :
                                                if (data.EventData.Data.Button == 'Press' || data.EventData.Data.Button == 'Short Press') {
                                                    console.log("Event is " + data.EventData.Data.Button);
                                                    toggle(connection,ESP8266_relay_URL,lastRelayState); 
                                                };       
                                        break;
                                    case ESP8266_relay_URL :
                                                console.log("Event is relay : " + data.EventData.Data.Relay);
                                                setRelayState(data.EventData.Data.Relay);
                                        break;
                                }
                            }
                        }
                        
                    } catch (err) {
                        console.log(err);
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

/*
 * Toggle Relay State
 */
function toggle(connection, url,data) {
    
    data == 0 ? data = 1 : data = 0;

    // Send message to ESP8266 to change the state
    connection.sendUTF(
        JSON.stringify(
            {
                URL: url,
                Method: 'POST',
               Data: {
                Relay: data
                }
            }
        )
    );
    setRelayState(data);
    count ++;
    console.log('Relay is switched  ' + count + '  times');
}

function setRelayState(state){
    lastRelayState = state;
}