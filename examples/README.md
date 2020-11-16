Examples:

## 1. Ducks
#### MamaDuck
Is the Central Duck of the mesh and can recieve and relay messages to other ducks as well send its own payloads.

#### PapaDuck
The PapaDuck is the final destination of the mesh and is used to collect all messages and push them up to the cloud.

#### DuckLink
The Ducklink is the base unit of the Mesh and can only send messages into the ClusterDuck network.

#### DetectorDuck
The DetectorDuck is used to deploy all the other Ducks, it provides the RSSI value of the closest Duck and can provide feedback on range.

## 2. Customise Mama Example
The Custom Mama Example is there to show how a mama is build and how to modify it, This example can recieve custom Lora settings and has code for a Oles display.

## 3. Sensor Examples

#### BMP180Example
The BMP180 example is a custom Mama that will send sensor data from the BMP 180 (Temperature, Pressure, Humidity) inside its own payload into the mesh.

#### BMP280Example
The BMP280 example is a custom Mama that will send sensor data from the BMP 280 (Temperature, Pressure, Humidity) inside its own payload into the mesh.

#### DHT11Example
The DHT example is a custom Mama that will send sensor data from the DHT11 (Temperature and Humidity) inside its own payload into the mesh.

#### DustSensorExample
The DustSensor example is a custom mama that will send sensor data from a Dust Sensorinside its own payload into the mesh.

#### MQ7Example
The MQ7 example is a custom mama that will send sensor data from the MQ7 Gas sensor (Carbon Monoxide) inside its own payload into the mesh.

## 4.Ble-Duck-App
The BLE example is a custom mama that will accept data from the Duck App over bluetooth.
https://github.com/Project-Owl/DuckApp

## 5.Papa-Iridium-Example
The papa Iridium Example is built to have an extra Iridium Satelite Backhaul when WiFi brakes down.

## 6.PaPi-DMS-Lite-Examples

#### Serial-PaPiDuckExample
The Serial Papi Example is a custom papa that will print the incoming data into the Serial Monitor for the PAPI to read

#### WIFI-PaPiDuckExample
The WiFi Papi Example is a custom papa that will send the incoming data into the Papi.
https://github.com/Project-Owl/dms-lite

## 7.TTGO-TBeam-Examples
If you have a TTGO T Beam you can use these custom examples for it 

#### TTGOGPSExample
This example is a Mama Duck, that has GPS capabilities and will send the GPS data with the GPS topic based on the set timer.


