# TempEmailer
Arduino uno temperature (and humidity) logger

This project is for a very small temperature and humidity log device that creates daily emails with a graph.
The readings are done every 10 minutes and logged to a daily file

Each night, the log file and a small simple bmp graph file are emailed.
If the temperature exceeds a limit, an email alert is generated. 
This alert is repeated every 1/2 hour if the temperature remains over limit.

The graph is a simple BMP file.

Due to memory constraints, the code is low on variables.

The email does not do TLS or SSL so a suitable email server must be used.
It has been tested on office 365 with a connector for the IP address of the device.

The project uses an Uno with an sd/ethernet shield and a standard temp humidity sensor

Wireless could be used.

At present,
Sketch uses (88%) of program storage space. Maximum is 32256 bytes.
Global variables use  (83%) of dynamic memory.
Maximum is 2048 bytes.

Looking at ways to make the code and variables smaller - so maybe add features.

This project is public and may be used as you feel fit. 
It would be nice to have my name included in your version, but I can live without it

Any contributions will be acknowledged as fantastic and placed in the code contribution sections.


