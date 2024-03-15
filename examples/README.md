Examples:

The URL under `lib_deps` may be replaced by a path to your local ClusterDuck-Protocol folder like so:
```
file://<DIRECTORY>/ClusterDuck-Protocol
```

## 1. Ducks

#### DuckLink
The DuckLink is the base unit of the mesh network. It can only send messages into the ClusterDuck network.

#### MamaDuck
The MamaDuck is a critical device in the ClusterDuck Protocol. In addition to sending its own payloads, it can receive messages from other Ducks and relay them along the mesh network.

#### PapaDuck
The PapaDuck is the final destination of the mesh network. It acts as a gateway to to collect all messages in the network and push it up to the cloud.

#### DetectorDuck
The DetectorDuck is a tool to help deploy the ClusterDuck Network. Using RSSI, it provides alerts if the user leaves the range of the network.

## 2. Custom Mama Examples
The Custom Mama Examples are examples where we show you how to use APIs to customize your own ClusterDuck Network. For example, you can adjust, Lora settings, WiFi settings, etc.