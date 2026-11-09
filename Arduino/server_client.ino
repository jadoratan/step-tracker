#include <ArduinoBLE.h>

int i = 0;

BLEService server("19B10000-E8F2-537E-4F6C-D104768A1214"); // Bluetooth® Low Energy LED Service

// Bluetooth® Low Energy LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLEByteCharacteristic switchCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

const int ledPin = LED_BUILTIN; // pin to use for the LED

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");

    while (1);
  }

  // set advertised local name and service UUID:
  BLE.setLocalName("StepCounter");
  BLE.setAdvertisedService(server);

  // add the characteristic to the service
  server.addCharacteristic(switchCharacteristic);

  // add service
  BLE.addService(server);

  // set the initial value for the characteristic:
  switchCharacteristic.writeValue(i);

  // start advertising
  BLE.advertise();

  Serial.println("BLE LED Server");
}

void loop() {
  // listen for Bluetooth® Low Energy peripherals to connect:
  BLEDevice client = BLE.central();

  // if a client is connected to peripheral:
  if (client) {
    Serial.print("Connected to client: ");
    // print the client's MAC address:
    Serial.println(client.address());

    // while the client is still connected to peripheral:
    while (client.connected()) {
      // if the remote device wrote to the characteristic,
      i++;
      // if (switchCharacteristic.written()) {
        switchCharacteristic.writeValue(i);
        // Serial.println(i);
      // }
      delay(200);
    }

    // when the client disconnects, print it out:
    Serial.print(F("Disconnected from client: "));
    Serial.println(client.address());
  }
}
