# SHLighter
Two small programs for remotely controlling the LEDs of a Sense Hat using a gamepad.  
It should run on Linux, OS X and Windows.  
It's incredibly pointless and half-assed.  

## Installation
You need:  
- C compiler  
- development libraries of SDL2 and SDL2_net  
- Raspberry Pi with Sense Hat and Python  
- XInput controller  

You can either use the Makefile or compile the "src/shlighter_sender.c" file yourself. Just remember to link the above mentioned libraries.  
You'll need to set the IP address of the Raspberry Pi. That can be done by either changing the "ADDRESS" macro or running the program once and editing the created "host.txt" file.  
You can set the area of what LEDs will be lit up by changing the "XMIN", "XMAX", "YMIN" and "XMAX" variables in the "shlighter_receiver.py" file.  
Lastly copy the "shlighter_receiver.py" script to the Raspberry Pi.  

## Usage
Run "shlighter_receiver.py" on your Raspberry Pi.  
Connect a gamepad to your PC and run "shilghter_sender".  
Hold "A", "B", "X" and/or "Y" to set a color and push another button/axis to make the LEDs do stuff.  
To close both programs press the "Guide" button.  