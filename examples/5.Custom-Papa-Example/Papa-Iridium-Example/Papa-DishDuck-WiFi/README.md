# DISHDUCK 2.0 Hybrid:

## The need for revision 
Before the DishDuck, the CDN operated where the DuckLinks, MamaDucks, and PapaDucks communicate data with each other using LoRa. To get that data, the PapaDuck has to be connected to the internet so it can push up the data for access. What happens when the PapaDuck loses its connection to the internet? The DishDuck.
The DishDuck is a PapaDuck with an additional device attached to itself called the RockBlock 9603. The RockBlock acts like a modem for the Iridium Network which is a network of 66 satellites in the earth’s orbit. So when the DishDuck loses internet connection, it can use the RockBlock to transmit data to the satellite using Short-Burst Data. With the DishDuck, combining the PapaDuck and RockBlock, we can now retrieve our sensor data from anywhere in the world without relying on internet or GSM connectivity. 

## Use Case Idea: 
The Dishduck was thought about after the recent earthquakes that have hit Puerto Rico. During these earthquakes, we realized that our sensors and ducklinks lost internet connectivity (backhaul) and could not send data back to the cloud. So we thought about what is a more reliable way to send data from our devices if we lose internet connection? So we thought about using the Iridium Network which has 75 satellites above in earth's orbit. Allowing for almost a guaranteed connection anywhere anytime, as long as you have a view of the sky. 

## Version 2.0 Hybrid DishDuck 
We had to come up with a way in which we could make the DishDuck smarter so it knows when it has connection to WiFi and can switch to the RockBlock Modem when WiFi goes down. That is what makes this version Hybrid vs Regular DishDuck. It is a smarter way to go about things because you don't want to always use up all your RockBlock airtime credits if you can send data over avaible WiFi connection. 
