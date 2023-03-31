TrackerGPS
 Daniel Gómez López

About
----------------------
This project is a prototype aimed at developing a GPS tracker. GPS coordinates, speed, and battery data 
are obtained from an ESP32 + SIM7000G and encapsulated in a JSON format. The data is sent via MQTT to a 
QT interface that processes it and allows users to read the information, represent the location on a map 
(QML), apply speed or perimeter restrictions through alarms, and check the battery voltage and percentage 
and speed.

The device features an SSD1306 OLED screen that displays information such as battery percentage, latitude, 
longitude, and speed.

Tools
----------------------

[QT]
Qt Creator 8.0.1 -> QT 5.15.1 MSVC2019 64bit

[ESP32]
Visual Studio Code
 Extensions:
	Arduino
	Serial Monitor 


Resources
----------------------

[LilyGO SIM7000G]
	https://github.com/Xinyuan-LilyGO/LilyGO-T-SIM7000G

[GSM/GPS Functionality]
	https://github.com/vshymanskyy/TinyGSM

[MQTT Functionality]
	https://github.com/knolleary/pubsubclient
	https://pubsubclient.knolleary.net/api
