# step-tracker

A wearable pedometer built on the **Arduino Nano RP2040 Connect**. An on-device ML model trained with **Edge Impulse** classifies motion from the board's accelerometer, and the results are sent over **Bluetooth Low Energy** to a companion iOS app written in SwiftUI.

## How it works

1. The board samples 3-axis accelerometer data at 88 Hz.
2. Every 2-second window (176 samples) goes through spectral-analysis feature extraction and a small TensorFlow Lite neural network.
3. The model classifies the window as `idle` or `walk`.
4. The board advertises a BLE service, and the iOS app connects to read or subscribe to its characteristics.

## Repository structure

```
step-tracker/
├── Arduino/
│   └── step_tracker_model.zip   # Edge Impulse Arduino library (model + inference SDK)
└── swiftCode/
    ├── MyApp.swift              # App entry point
    ├── ContentView.swift        # Root view
    ├── ScanAndConnectView.swift # Scan, connect, and inspect UI
    ├── BLEManager.swift         # CoreBluetooth central manager
    └── BLEConstants.swift       # Service UUID, timeouts, and limits
```

## Hardware

- Arduino Nano RP2040 Connect (onboard IMU and BLE)
- iPhone with Bluetooth enabled

## Setup

### Arduino

1. In the Arduino IDE, go to **Sketch → Include Library → Add .ZIP Library…** and select `Arduino/step_tracker_model.zip`.
2. Install the **Arduino Mbed OS Nano Boards** core and the **ArduinoBLE** library.
3. Use the library's examples (**File → Examples → nano-ble-wearable_inferencing**) as a starting point for running inference on the board.
4. Advertise a BLE service with UUID `39B10010-E8F2-537E-4F6C-D104768A1214` so the iOS app can find the board.

### iOS app

1. Create a new SwiftUI app in Xcode (iOS 17+, required for `@Observable`).
2. Add the files from `swiftCode/` to the project, replacing the generated app and content view files.
3. Add `NSBluetoothAlwaysUsageDescription` to `Info.plist`.
4. Build and run on a physical device. Bluetooth does not work in the simulator.

## Using the app

1. Power on the board.
2. Tap **Scan**. The app searches for 15 seconds for devices advertising the step-tracker service.
3. Tap **Connect** on the board when it appears.
4. Once connected, the app lists the board's services and characteristics. Tap **Read** for a one-time value or **Subscribe** for live updates. Values appear in hex, decimal, and UTF-8.
5. Open **Activity log** to see connection events and errors.

## Model details

| Property        | Value                                     |
|-----------------|-------------------------------------------|
| Platform        | Edge Impulse (`nano-ble-wearable` project)|
| Input           | 3-axis accelerometer, 88 Hz               |
| Window          | 176 samples (2 s)                         |
| Features        | Spectral analysis                         |
| Classifier      | TensorFlow Lite (compiled)                |
| Labels          | `idle`, `walk`                            |

## Built with

- [Edge Impulse](https://edgeimpulse.com/)
- [ArduinoBLE](https://www.arduino.cc/reference/en/libraries/arduinoble/)
- SwiftUI and CoreBluetooth