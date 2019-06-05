# tokenRingBus

## Idea
Create a token-ring bussystem for two or more Arduino Unos which utilizes the serial monitor of the Arduino IDE to send messages between them.

## Token structure
Each byte has different purpose:
1. Source address
2. Destination address
3. Destination address of payload from client 1
4. Payload of client 1
5. Destination address of payload from client 2
6. Payload of client 2
7. ...repeates for more clients

## File naming structure
* tokenMaster: Initializes token
* tokenSlaves: Will not initialize token

## Adding more clients
If you want to add more clients copy tokenSlave1 and rename it. Then you have to manually change the clientID. DestinationID can be set to any client in ring (except own).
