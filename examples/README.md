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