# MEng_Project_Code

This repository contains the final code used for my collaborative master's project at the university of Manchester.

The project titled 'Distributed Wireless Sensing and Control for Harsh Industrial Environments' successfully evaluated the robustness of distributed control across a wireless network. The code included in this repository was used to establish the OpenThread network utilised to wireless send sensing and control information between 6 nRF52833DK boards to control a created 3D transport system.

For further information on my master's project please watch the team's 5 minute video accessed through the following link:
https://www.linkedin.com/feed/update/urn:li:activity:7088111463969509377/

The repository contains two folders. One of which contains the code utilised by Motor nodes responsible for sending sensing data to the central processing node and receiving instructions for driving the motors. The other contains the code for the central processing node responsible for executing the system's control algorithm and producing instructions for system actuation following the receipt of sensing data from motor nodes.
