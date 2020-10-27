# TempEmailer
Temp logger with email and graph

arduino uno temperature (and humidity) logger

This project is for a very small temperature and humidity log device that creates daily emails with a graph.
The readings are done every 10 minutes and logged to a daily file
Each night, the log file and a small simple bmp graph file are emailed.
If the temperature exceeds a limit, and email alert is gernerated, This alert is repeated every hour if the temperature remains over limit.

The graph is a simple BMP file.

Due to memory constraints, the code is low on variables.

The email does not do TLS or SSL so a suitable email server must be used.
It has been tested on office 365 with a connector for the ip address of the device.

