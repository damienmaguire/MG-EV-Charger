# MG-EV-Charger
Reverse engineering of the on board charger (OBC) used in MG Electric Vehicles

SAIC motors chargers are cheap and plentiful and some offer V2L (Vehicle To Load) function making them useful in electric vehicle conversion projects.

06/08/23 : Added CAN logs from a 2020 43kWh MG ZS EV during charge.


08/08/23 : Added CAN log from charger "Local CAN" just powered by 12v.

23/11/23 : Actual pinout from connector BY400 on charger : 

Pin 1     +12v
Pin 2     Gnd
Pin 3    CAN3H
Pin 4    CAN3L
Pin 5    CAN2H
Pin 6    CAN2L
Pin 7
Pin 8   Onbd Pstv SNR+
Pin 9   Charger wake up
Pin 10  Onbd Ngtv SNR+
Pin 11 Onbd Charge CC
Pin 12 Onbd SNR-

CAN3 is local between charger and charger controller. On there 0x29C would seem to be a command from controller to charger.
CAN2 is pt can.
100k resistors between Pins 12, 8 and 10 seem to satisfy the charge port temp sensors.
Pin 9 seems to do nothing and is not reported on CAN.
Pin 11 is weird. Firstly, it is looped through the HVIL on the AC connector but not the DC. With the HVIL shorted, applying 12v will cause byte 0 in 0x3B8 to read 0x64 (dec 100).
Up to 21k of resistance from 12v to this pin will maintain 0x64. At 22k byte 0 chages from 0x64 to 0x00. No inbetweens.
Seems to be 1 second between state chages on this byte.
In logs from a charging vehicle we see byte 0 go from 0x00 , 06x4 and then ramp down to 0x36 during charge. On my bench charger, with no connection to pin 11 I get 0x00 to 0x64 and then back to 0x00 only.

