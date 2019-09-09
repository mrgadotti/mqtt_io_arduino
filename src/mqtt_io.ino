#include <SPI.h>
#include <Ethernet.h>
#include <MQTT.h>

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
};

EthernetClient net;
MQTTClient client;

unsigned long lastMillis = 0;

void connect() {
  Serial.print("Connecting...");
  while (!client.connect("arduino", "try", "try")) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nConnected!");
  client.subscribe("io/read/in");
  client.subscribe("io/out/write");
}

void messageReceived(String &topic, String &payload) {
  Serial.println(topic + " -> " + payload);
  if(topic.equals("io/read/in")){
    Serial.println("INPUT");
    Serial.println(digitalRead(payload.toInt()));
  }
  if(topic.equals("io/out/write")){
    uint8_t pin, val;
    // Get pin and value separated by :
    for (uint8_t i = 0; i < payload.length(); i++) {
      if (payload.substring(i, i+1) == ":") {
        pin = payload.substring(0, i).toInt();
        val = payload.substring(i+1).toInt();
        break;
      }
    }
    Serial.print(pin);
    Serial.print(":");
    Serial.println(val);
    // Write value to pin
    digitalWrite(pin,val);
  }
}

void setup() {

  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("Initialize Ethernet with DHCP:");
  set_dhcp:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // no point in carrying on, so do nothing forevermore:
    Serial.println("Try again to initialize Ethernet with DHCP");
    goto set_dhcp;
  }
  // print your local IP address:
  Serial.print("IP address: ");
  Serial.println(Ethernet.localIP());

  client.begin("mqtt.eclipse.org", net);
  client.onMessage(messageReceived);
  connect();
}

void loop() {
  switch (Ethernet.maintain()) {
    case 1:
      //renewed fail
      Serial.println("Error: renewed fail");
      break;

    case 2:
      //renewed success
      Serial.println("Renewed success");
      //print your local IP address:
      Serial.print("My IP address: ");
      Serial.println(Ethernet.localIP());
      break;

    case 3:
      //rebind fail
      Serial.println("Error: rebind fail");
      break;

    case 4:
      //rebind success
      Serial.println("Rebind success");
      //print your local IP address:
      Serial.print("IP address: ");
      Serial.println(Ethernet.localIP());
      break;

    default:
      //nothing happened
      break;
  }
  client.loop();

  if (!client.connected()) {
    connect();
  }

  // publish a message roughly every second.
  /*if (millis() - lastMillis > 1000) {
    lastMillis = millis();
    client.publish("/hello", "world");
  }*/
}