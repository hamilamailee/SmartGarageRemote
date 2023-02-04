# SmartGarageRemote

- Fall 2022
- Contributors: Saee Saadat - Neda Taghizadeh - Negar Nobakhti - Hamila Mailee

In this project, we design a framework that enables us to control our garage door with a remote webpage using the internet. To this end, we set up a server, and from the user’s side, a message containing desired instructions will be sent to this server. The protocol for transferring these messages is MQTT, and NodeMCU is the intermediary that receives the client’s messages. A pair of transmitter/receiver modules are needed to interpret the received signals and open/close the door.
