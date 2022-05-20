Examples:

The URL under `lib_deps` may be replaced by a path to your local ClusterDuck-Protocol folder like so:
```
file://<DIRECTORY>/ClusterDuck-Protocol
```

## 1. Ducks

#### MamaDuck
Is the Central Duck of the mesh and can receive and relay messages to other ducks as well send its own payloads.

#### PapaDuck
The PapaDuck is the final destination of the mesh and is used to collect all messages and push them up to the cloud.

#### DuckLink
The DuckLink is the base unit of the Mesh and can only send messages into the ClusterDuck network.

#### DetectorDuck
The DetectorDuck is used to deploy all the other Ducks, it provides the RSSI value of the closest Duck and can provide feedback on range.

## 2. Custom Mama Examples
The Custom Mama Example is there to show how a mama is build and how to modify it, This example can receive custom Lora settings and has code for a Oles display.

## 3. Sensor Examples

#### BMP180Example
The BMP180 example is a custom Mama that will send sensor data from the BMP 180 (Temperature, Pressure, Humidity) inside its own payload into the mesh.

#### BMP280Example
The BMP280 example is a custom Mama that will send sensor data from the BMP 280 (Temperature, Pressure, Humidity) inside its own payload into the mesh.

#### DHT11Example
The DHT example is a custom Mama that will send sensor data from the DHT11 (Temperature and Humidity) inside its own payload into the mesh.

#### DustSensorExample
The DustSensor example is a custom mama that will send sensor data from a Dust Sensor inside its own payload into the mesh.

#### MQ7Example
The MQ7 example is a custom mama that will send sensor data from the MQ7 Gas sensor (Carbon Monoxide) inside its own payload into the mesh.

#### WS2812Example
The WS2812 example is a Mama Duck, which also brings some light to the world by incorporating some WS2812 LEDs via the FastLED library.

## 4. Ble-Duck-App
The BLE example is a custom mama that will accept data from the Duck App over bluetooth.
https://github.com/Project-Owl/DuckApp

## 5. Custom Papa Examples

#### PaPa-DishDuck and PaPa-DishDuck-WiFi
The papa Iridium Example is built to have an extra Iridium Satelite Backhaul when WiFi brakes down.

#### Papa-Downtime-Counter
...

### PaPi-DMS-Lite-Examples
https://github.com/Project-Owl/dms-lite-docker

#### PapiDuck-Serial-Example
The Serial Papi Example is a custom papa that will print the incoming data into the Serial Monitor for the PAPI to read.

#### PapiDuck-WiFi-Example
The WiFi Papi Example is a custom papa that will send the incoming data into the Papi.

## 6. TTGO T-Beam Examples
If you have a TTGO T-Beam you can use these custom examples for it.

#### TBeam-AXP-Example
Demonstrate the power chip

#### TBeam-GPS-Example
GPS Example using TinyGPS++

#### TBeam-Telemetry
Using the TTGO's hardware we can collect all different kinds of live board information such as: onboard temperature, battery voltage (if 18650 installed), and charging.

#### TTGOGPSExample
This example is a Mama Duck, that has GPS capabilities and will send the GPS data with the GPS topic based on the set timer.

## 7. Encryption
You can enable encryption. CDP comes with default settings, but set your own IV and AES256 key when in use.

#### DecryptionPapa
Decryption is a very fast operation and using this example your messages will be decrypted before sending to the cloud. Remember to set IV and AES256 key to be the same as what you use on other devices.
