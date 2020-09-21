# arduino-LPG-detector-indicator
LP Gas Level indicator and leakage detector

The above source code is the arduino program for a LP gas level indicator and leakage detector that serves as a dual purpose device designed to indicate the amount of gas remaining in the gas cylinder on an LCD screen and to sense the presence of a leakage. In the case of such an event a buzzer turns on to inform the people around and automatically send an SMS to the respective caretaker of the premises. 

Main Components of the device
•	Arduino UNO (Microcontroller)
•	MQ2 gas sensor module
•	Load cell module and amplifier module
•	LCD (Liquid Crystal Display) unit
•	GSM(Global System for Mobile communication) module
•	Piezo electric buzzer

The device monitors the gas level using three Load cells which is a weight sensor that is based on the strain gauge principles. By monitoring the weight change in the gas cylinder a measurement of the gas remaining is calculated. The changes are incessantly displayed as a numeric value in the LCD display.
If the level reaches the specified minimum, the LCD will display a message and an LED will light up and eventually the piezo buzzer will turn on.
In the case of a LPG leakage, when the MQ2 detector senses LPG gas in the air, the piezo buzzer will again buzz off to alert those around. MQ2 gas sensors detect other gases but it is most sensitive to LPG compared to the other sensors During this process, a remote indication of a leakage is provided by sending an SMS to those of concern using the GSM module.

References
The Arduino libraries required for this project,
> HX711 Source:  https://github.com/bogde/HX711
> LiquidCrystal Arduino Library
> Wire Library

