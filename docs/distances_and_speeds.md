# Distances and Speeds (Actual vs. Theoretical)

This document provides information regarding the transmission range, data speeds, and network performance of the ClusterDuck Protocol (CDP) based on LoRa technology, highlighting both theoretical limits and findings from real-world deployments.

---

## Performance Summary

| Metric | Theoretical Limit | Real-World/Deployment Observations |
| :--- | :--- | :--- |
| **Transmission Range** | Up to **15+ km** (clear line-of-sight, high altitude) | **100m – 300m** (dense urban/heavy foliage)<br>**300m – 800m** (suburban/moderate foliage)<br>**1km – 4km+** (clear line-of-sight/elevated) |
| **Data Throughput** | Up to **50 kbps** (LoRa physical layer) | **Low Bandwidth (~9.6 kbps)**<br>Optimized for small packets (~256 bytes payload) |
| **Latency** | Milliseconds (point-to-point) | **1 – 5 seconds** (dependent on hop count, Spreading Factor, and network congestion) |
| **Packet Error Rate** | 0% (ideal lab conditions) | **10% – 50%** (highly dependent on node spacing, interference, and environmental obstacles) |

---

## Distance & Range

LoRa (Long Range) is designed for low-power, long-distance communication. However, actual performance is heavily influenced by physical obstacles, environmental conditions, and node placement.

### 1. Environmental Impact
* **Dense Urban/Foliage:** Structures, concrete walls, and wet leaves absorb RF energy (especially at 915 MHz). Expect individual link ranges of **100m to 300m**.
* **Open Fields/Suburbs:** Fewer obstructions allow links of **300m to 800m**.
* **Elevated Line-of-Sight (LoS):** If nodes have clear visibility to one another (e.g., hilltop to hilltop, rooftop to rooftop), ranges of **1km to 4km+** are regularly achieved.

### 2. Node Spacing in Deployments
During early pilot tests (such as the Project OWL deployment in Puerto Rico), MamaDucks were spaced between **400m to 1km** apart. Spacing must be adjusted dynamically based on the local topology:
* A spacing of **400m** with clear line-of-sight provides stable, high-reliability links.
* If a link has to pass through buildings or dense vegetation, spacing should be reduced to **150m - 200m** to maintain connectivity.

---

## Transmission Speed & Bandwidth

The ClusterDuck Protocol is designed strictly as a **low-bandwidth** mesh protocol. It is optimized for transmitting critical, lightweight data packets during emergencies rather than high-speed internet traffic.

### 1. Throughput Limits
* **LoRa Constraints:** While LoRa can technically reach up to 50 kbps under specific Spreading Factor (SF) and bandwidth configurations, the ClusterDuck Protocol typically operates at **9.6 kbps** or lower.
* **Small Packets:** The protocol utilizes small packets (maximum payload is typically restricted to 256 bytes) to maximize transmission success and minimize time-on-air.
* **Non-Supported Traffic:** CDP does **not** support media streaming, web browsing, or large file transfers. It is meant for emergency text messages, GPS coordinates, and sensor telemetry.

### 2. Latency & Multi-hop Overhead
* Every hop in the mesh introduces processing delay (re-transmitting the packet) and channel contention (waiting for the channel to be clear).
* Higher Spreading Factors (SF) increase range and stability but exponentially increase time-on-air, thereby increasing latency and reducing throughput.
* A packet traveling through 3-4 hops can take **1 to 5 seconds** to reach the destination PapaDuck.

---

## Deployment Insights & Case Studies

Real-world field trials have provided key data on how these networks perform in practice:

### Puerto Rico Deployment (March 2019)
* **Scale:** ~60 DuckLink and MamaDuck nodes deployed across 5 regions, covering a total area of about 1 square mile.
* **Initial Challenges:** Early testing saw packet success rates of around **50%** due to dense vegetation, concrete buildings, and sub-optimal node placement.
* **Optimization:** By adjusting the spacing, elevating nodes, and utilizing better paths, the packet error rate was reduced to **10%** (a 90% success rate).

### Houston and Evacuation Tests
* **Deployment Speed:** The time required to power on, place, and boot a node was optimized down from 15 minutes to **90 seconds** per node.
* **Network Warm-up:** Once turned on, nodes typically discover neighbors and self-heal the mesh path within seconds.

---

## How to Optimize Performance & Range

If you are planning a deployment, you can significantly improve range and reduce error rates by following these guidelines:

### 1. Elevate the Nodes
* **Fresnel Zone Blockage:** Ground-level placement causes the ground itself to block and scatter the signal. 
* **Height Recommendation:** Ensure all nodes are placed at least **1.5m to 3m** above the ground.
* **Rooftops & Masts:** Mount MamaDucks and PapaDucks on roofs, poles, or trees wherever possible.
* **Balloons/Lofting:** In extreme terrain (e.g., valleys or dense forests), lifting nodes using helium balloons (SpaceDucks) can establish an overhead clear line-of-sight to many ground nodes simultaneously.

### 2. Upgrade the Antennas
* **Omnidirectional Antennas:** Use high-quality, tuned omnidirectional antennas (e.g., 3 dBi or 5 dBi) for general mesh nodes that need to communicate in all directions.
* **Directional Antennas:** For long-distance point-to-point links (such as a remote MamaDuck sending data to a distant PapaDuck), consider directional antennas like **Yagi antennas** or DIY **cantennas** (e.g., Pringles cans). These focus the radio energy in a single direction to bridge gaps of several kilometers, but require precise physical alignment.
