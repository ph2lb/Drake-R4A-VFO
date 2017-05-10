# Drake-R4A-VFO
A Drake R4A VFO based on my FT301 VFO. Special version for Bill NZ0T 
 
Arduino based external VFO for Drake R4A with AD9850 with LCD  

#  Drake R4A VFO background info
The  Drake R4A uses a VFO in the range from 4995Khz to 5495Khz allowing to tune a 500Khz piece of a selected band.   
 
# This project
Contains the code for a Arduino based external VFO for the  Drake R4A with AD9850 DDS to generate the frequency and a  LCD  display as user interface. Allowing the user to switch band and step size and show the current (band) frequency. The bandplan is defined in a struct in the main source file (default US country full licensed bandplan).

# Hardware used : 
- Arduino Uno R3 
- DDS one of : 
  - QRP-Labs Arduino Shields (http://qrp-labs.com/uarduino.html) when using AD9850  module from QRP-Labs 
- Display option one (or both) : 
  - LCD keypad shield (http://www.hobbytronics.co.uk/arduino-lcd-keypad-shield) (warning : don't use D10 with this shield) 
    with analog button.  
- Simpel rotary encoder (gray code encoder) connected to 2 and A2 (2 for interrupt)
- A balance potentiometer connected to A3 for a +- 1Khz RIT.
 
