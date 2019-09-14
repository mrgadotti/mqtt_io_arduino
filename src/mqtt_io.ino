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

// Broker Hostname/IP and Port
#define BROKER_HOST "mqtt.eclipse.org" // public broker 
#define BROKER_PORT 1883

uint16_t bit_input = 0x00;
uint16_t last_bit_input = 0x00;

// Configure GPIO 
void set_gpio() {
  pinMode(13, OUTPUT);
  pinMode(22,INPUT_PULLUP);
  pinMode(24,INPUT_PULLUP);
}

// Connect to MQTT and Subscrib topics
void connect() {
  Serial.print("Connecting...");
  while (!client.connect("arduino", "try", "try")) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nConnected to " + String(BROKER_HOST));
  // Subscrib to specific topics
  client.subscribe("io/in/set");
  client.subscribe("io/out/pin");
}

void messageReceived(String &topic, String &payload) {
  // Print payload from subscribed topics
  Serial.println(topic + " -> " + payload);
  // Read value from pin and publish
  if (topic.equals("io/in/set")) {
    client.publish("io/in/val", payload + ":" + String(digitalRead(payload.toInt())));
  }
  // Write value to pin and publish
  uint8_t pin, val;
  if (topic.equals("io/out/pin")) {
    // Get pin and value separated by :
    for (uint8_t i = 0; i < payload.length(); i++) {
      if (payload.substring(i, i + 1) == ":") {
        pin = payload.substring(0, i).toInt();
        val = payload.substring(i + 1).toInt();
        break;
      }
    }
    // Write value to especific gpio
    digitalWrite(pin, val);
  }
}
// Publish especific pin using topic
void publishPin(uint8_t pin, uint8_t val) {
  client.publish("io/in/" + String(pin), String(val));
}

void setup() {
  // Set all gpio 
  set_gpio();

  Serial.begin(115200);
  Serial.println("Ethernet with DHCP");
  // Get IP Address using DHCP
  set_dhcp:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // No point in carrying on, so do nothing forevermore:
    Serial.println("Try again to initialize Ethernet with DHCP");
    // Restart DHCP process 
    goto set_dhcp;
  }
  // Print local IP address
  Serial.print("IP address: ");
  Serial.println(Ethernet.localIP());

  // Broker settings
  client.begin(BROKER_HOST,BROKER_PORT, net);
  // Subscribe Callback
  client.onMessage(messageReceived);
  connect();
}

void loop() {
  // Print Ethernet changes
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
  // MQTT
  client.loop();
  // Check Broker connection status
  if (!client.connected()) {
    connect();
  }
  // set intput pin on bit var
  bitWrite(bit_input,0,digitalRead(13));
  bitWrite(bit_input,1,digitalRead(22));
  bitWrite(bit_input,2,digitalRead(24));
  // Check input changes
  if (bit_input != last_bit_input)
  {
    Serial.print("Input (15-0): ");
    Serial.println(bit_input, BIN);
    last_bit_input = bit_input;
    client.publish("io/in/event", String(bit_input));
  }
}