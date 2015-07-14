/*
 *  This code switches a relay connected to port 5 (GPIO5) on an ESP8266.
 *
 *  It will connect to MQTT and listens on topic house/2/attic/cv/thermostat/relay
 *  for 'on' and 'off' commands. Every 60 seconds, it will publishes te current
 *  state on house/2/attic/cv/thermostat/relay_state
 *
 *  Dimitar Manovski
 *  support@smart-republic.com
 *
 * 
 *
 */


#include <ESP8266WiFi.h>
#include <PubSubClient.h>

int RelayPin = 5;    // RELAY connected to digital pin 5

const char* ssid     = "YOUR_OWN_SSID";
const char* password = "YOUR_OWN_PASSWORD";

IPAddress server(10, 0, 0, 1); // Change this to the ip address of the MQTT server
PubSubClient client(server);

void callback(const MQTT::Publish& pub) {
  Serial.print(pub.topic());
  Serial.print(" => ");
  Serial.println(pub.payload_string());

  if ( pub.topic() == "house/2/attic/cv/thermostat/relay" ) {
    if (pub.payload_string() == "on" ) {
      digitalWrite(RelayPin, HIGH);   // turn the RELAY on
      client.publish("house/2/attic/cv/thermostat/relay_state","on");
    } else if ( pub.payload_string() == "off" ) {
      digitalWrite(RelayPin, LOW);    // turn the RELAY off
      client.publish("house/2/attic/cv/thermostat/relay_state","off");
    } else {
      Serial.print("I do not know what to do with ");
      Serial.print(pub.payload_string());
      Serial.print(" on topic ");
      Serial.println(pub.topic());
    }
  }
}

void connect_to_MQTT() {
  client.set_callback(callback);

  if (client.connect("thermostat_relay")) {
    Serial.println("(re)-connected to MQTT");
    client.subscribe("house/2/attic/cv/thermostat/relay");
  } else {
    Serial.println("Could not connect to MQTT");
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);

  // Connecting to our WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  connect_to_MQTT();

  // initialize pin 5, where the relay is connected to.
  pinMode(RelayPin, OUTPUT);
}

int tellstate = 0;

void loop() {
  client.loop();

  if (! client.connected()) {
    Serial.println("Not connected to MQTT....");
    connect_to_MQTT();
    delay(5000);
  }

  // Tell the current state every 60 seconds
  if ( (millis() - tellstate) > 60000 ) {
    if ( digitalRead(RelayPin) ) {
       client.publish("house/2/attic/cv/thermostat/relay_state","on");
    } else {
      client.publish("house/2/attic/cv/thermostat/relay_state","off");
    }
    tellstate = millis();
  }
}
