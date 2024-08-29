## 1. Ducks

#### DuckLink
The DuckLink is the base unit of the mesh network and can only send messages in the ClusterDuck Network.

#### MamaDuck
The MamaDuck is a critical device in the ClusterDuck Protocol. In addition to sending its own payloads, it can receive messages from other Ducks and relay them along the mesh network.

#### PapaDuck
The PapaDuck is the final destination of the mesh network. It acts as a gateway to to collect all messages in the network and push it up to the cloud.

#### DetectorDuck
The DetectorDuck is a tool to help deploy the ClusterDuck Network. Using RSSI, it provides alerts if the user leaves the range of the network.

## 2. Mini-ClusterDuck Network
This is a directory of a sample ClusterDuck Network consisting of a DuckLink, MamaDuck, and a PapaDuck that sends the data to MQTT.

## 3. Custom Mama Examples
The Custom Mama Examples are examples where we show you how to use APIs to customize your own ClusterDuck Network. For example, you can adjust, Lora settings, WiFi settings, etc.

## 4. Custom Papa Examples
The Custom Papa Examples are examples where we have a destination to send our ClusterDuck data. For example, OWL is currently using AWS so there is a custom PapaDuck example tailored to send data to AWS.