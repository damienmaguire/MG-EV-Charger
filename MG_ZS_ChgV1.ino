/*
MG ZS Gen 1 Charger controller V1

Copyright 2023 
Damien Maguire
*/

#include <due_can.h>  
#include <due_wire.h> 
#include <DueTimer.h>  
#include <Metro.h>

#define SerialDEBUG SerialUSB
 template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; } //Allow streaming


CAN_FRAME outframe;  //A structured variable according to due_can library for transmitting CAN data.
CAN_FRAME inFrame;    //structure to keep inbound inFrames


float Version=1.00;

uint8_t delay_stage1=30;
uint8_t delay_stage2=5;
uint8_t delay_stage3=30;

struct ChargerStatus {
  uint16_t ACvoltage = 0;
  uint16_t HVVoltage = 0;
  uint16_t MaxHV = 0;
  int8_t temperature = 0;
  uint8_t mode = 0;
  uint8_t current=0;
  uint8_t targetcurrent=0;
  uint8_t MaxACAmps=0;
  uint8_t ISetPnt=0;
  uint8_t CPLim=0;
  uint8_t ACAmps=0;
} charger_status;

struct ChargerControl {
  uint16_t HVDCSetpnt=0;
  uint8_t IDCSetpnt = 0;
  uint8_t modeSet = 0;
  uint8_t byt0=0;
  uint8_t byt1=0;
  uint8_t byt2=0;
  uint8_t byt5=0;
  uint8_t byt6=0;
  uint8_t byt7=0;
  bool active=0;
} charger_params;

int led = 13;         //onboard led for diagnosis
 
Metro timer_100ms=Metro(100); 

void setup() 
  {
  Can0.begin(CAN_BPS_500K);   // Charger CAN
  Can1.begin(CAN_BPS_500K);   // External CAN
  Can0.watchFor();
  Can1.watchFor();
  Serial.begin(115200);  
  pinMode(led, OUTPUT);
  }

  
  
void loop()
{ 
checkCAN();
checkforinput();
if(timer_100ms.check()) Msgs100ms();
}
 



void Msgs100ms()                      ////100ms messages here
{
        digitalWrite(13,!digitalRead(13));//blink led every time we fire this interrrupt.
   


        outframe.id = 0x29C;            //Charger control msg on local CAN
        outframe.length = 8;            //
        outframe.extended = 0;          //
        outframe.rtr=1;                 //
        outframe.data.bytes[0]=charger_params.byt0;    //goes to 0x40 at 400ms after byte 2.Max current. 0x08 = 2A dc
        outframe.data.bytes[1]=charger_params.byt1;    //goes to 0x08 for startup.This calls in the evse.
        outframe.data.bytes[2]=charger_params.byt2;    //goes to 0x24 approx 3 secs later.goes from 0x24 to 0x25 on byte 3 wrap.9 bit current command?
        outframe.data.bytes[3]=charger_params.IDCSetpnt;    //current request, ramps from 0x00, 0x08 , wraps back through 0xFE to 0x94
        //20 decimal =1A
        outframe.data.bytes[4]=0x00;   //always 0x00
        outframe.data.bytes[5]=charger_params.byt5;   //goes to 0x8C at same time as byte 2
        outframe.data.bytes[6]=charger_params.byt6;   //goes to 0x5A at same time as byte 2
        outframe.data.bytes[7]=charger_params.byt7;   //goes to 0x3C at same time as byte 2
        Can0.sendFrame(outframe);

        //can also go byte 1 from 0x00 to 0x08, wait 30 cycles then byte 2=0x24 , then 8C,5A,3C in 5,6,7, then byte 0 to 0x20 or 0x40.

        if(charger_params.active)
        {
           delay_stage3=30;
           charger_params.byt1=0x08;
           
           if(delay_stage1!=0) delay_stage1--;
           if(delay_stage1==0)
           {
              charger_params.byt2=0x24;
              charger_params.byt5=0x8C;
              charger_params.byt6=0x5A;
              charger_params.byt7=0x3C;
              if(delay_stage2 !=0) delay_stage2--;
              if(delay_stage2 ==0) charger_params.byt0=0x08;
           }
            


        }


        if(!charger_params.active)
        {
             delay_stage1=30;
             delay_stage2=5;
              charger_params.IDCSetpnt=0;
              charger_params.byt1=0x01;
              charger_params.byt0=0x20;
              charger_params.byt2=0x60;
              charger_params.byt5=0x00;
              charger_params.byt6=0x00;
              charger_params.byt7=0x00;
              if(delay_stage3!=0) delay_stage3--;
              if(delay_stage3==0)
              {
              charger_params.IDCSetpnt=0;
              charger_params.byt1=0x00;
              charger_params.byt0=0x00;
              charger_params.byt2=0x00;
              charger_params.byt5=0x00;
              charger_params.byt6=0x00;
              charger_params.byt7=0x00;

              }

        }


}






void checkCAN()
{
//msgs from charger :





  if(Can0.available())
  {
    Can0.read(inFrame);

    if(inFrame.id ==0x3B8)
    {
      charger_status.ACvoltage=(inFrame.data.bytes[2]*2);//charger AC voltage is 8 bits from bit 16 to 23.
      charger_status.ACAmps=(inFrame.data.bytes[1]*.25);
      charger_status.HVVoltage=(inFrame.data.bytes[5]*2);
      charger_status.temperature=0;
      charger_status.current=(((((inFrame.data.bytes[5]<<8) | inFrame.data.bytes[4])>>2)&0x3ff)*0.2)-102;//10 bit bytes 4 and 5.
      charger_status.mode=0;
      charger_status.MaxACAmps=0;
      charger_status.MaxHV=0;
      charger_status.ISetPnt=0;
      charger_status.CPLim=(inFrame.data.bytes[0]);//
    }

  }

  
}

void checkforinput()
{ 
  //Checks for keyboard input from Native port 
   if (SerialDEBUG.available()) 
     {
      int inByte = SerialDEBUG.read();
      switch (inByte)
         {
     
          case 'd':     //Print data received from charger
                PrintRawData();
            break;
            
          case 'D':     //Print charger set points
                PrintSetting();
            break;
          
          case 'i':     //Set max HV Current
              SetHVI();
            break;
          case 'q':     //Set Max HV voltage
              SetHVV();
            break;
          case 'a':     //Activate charger     
              ActivateCharger();
            break;            
           
          case 's':     //Deactivate charger     
              DeactivateCharger();
            break;   

            break;          
           

          case '?':     //Print a menu describing these functions
              printMenu();
            break;
         
          }    
      }
}

void SetHVI()
{
  SerialDEBUG.println("");
   SerialDEBUG.print("Configured HV Current setpoint: ");
     if (SerialDEBUG.available()) {
    charger_params.IDCSetpnt = SerialDEBUG.parseInt();
  }
  if(charger_params.IDCSetpnt>255) charger_params.IDCSetpnt=255;
  SerialDEBUG.println(charger_params.IDCSetpnt);
}

void SetHVV()
{
  SerialDEBUG.println("");
  SerialDEBUG.print("Configured HV Voltage setpoint: ");
     if (SerialDEBUG.available()) {
    charger_params.HVDCSetpnt = SerialDEBUG.parseInt();
  }
  if(charger_params.HVDCSetpnt>500) charger_params.HVDCSetpnt=500;//limit to max 500volts
  SerialDEBUG.println(charger_params.HVDCSetpnt);
}

void ActivateCharger()
{   
    charger_params.active=1;
    SerialDEBUG.println("Charger activated");
}

void DeactivateCharger()
{
    charger_params.active=0;
    SerialDEBUG.println("Charger deactivated");
}


void printMenu()
{
   SerialDEBUG<<"\f\n=========== EVBMW VW Charger Controller "<<Version<<" ==============\n************ List of Available Commands ************\n\n";
   SerialDEBUG<<"  ?  - Print this menu\n ";
   SerialDEBUG<<"  d - Print recieved data from charger\n";
   SerialDEBUG<<"  D - Print configuration data\n";
   SerialDEBUG<<"  i  - Set max HV Current e.g. typing i5 followed by enter sets max current to 5Amps\n ";
   SerialDEBUG<<"  q  - Set max HV Voltage e.g. typing q400 followed by enter sets max HV Voltage to 400Volts\n ";
   SerialDEBUG<<"  a  - Activate charger.\n ";
   SerialDEBUG<<"  s  - Deactivate charger.\n ";   
   SerialDEBUG<<"**************************************************************\n==============================================================\n\n";
   
}

//////////////////////////////////////////////////////////////////////////////

void PrintRawData()
{
  SerialDEBUG.println("");
  SerialDEBUG.println("***************************************************************************************************");
  SerialDEBUG.print("ACVolts: ");
  SerialDEBUG.println(charger_status.ACvoltage);
  SerialDEBUG.print("HVDCVolts: ");
  SerialDEBUG.println(charger_status.HVVoltage);
  SerialDEBUG.print("Charger Temperature: ");
  SerialDEBUG.println(charger_status.temperature);
  SerialDEBUG.print("Max available AC current: ");
  SerialDEBUG.println(charger_status.MaxACAmps);
  SerialDEBUG.print("Max HV Voltage : ");
  SerialDEBUG.println(charger_status.MaxHV); 
  SerialDEBUG.print("HV I: ");
  SerialDEBUG.println(charger_status.ISetPnt); 
  SerialDEBUG.println("***************************************************************************************************");  
}


void PrintSetting()
{
  SerialDEBUG.println("");
  SerialDEBUG.println("***************************************************************************************************");
  SerialDEBUG.print("DC Volts Setpoint: ");
  SerialDEBUG.println(charger_params.HVDCSetpnt);
  SerialDEBUG.print("DC Current Setpoint: ");
  SerialDEBUG.println(charger_params.IDCSetpnt);
  SerialDEBUG.print("Charger Target mode: ");
  SerialDEBUG.println(charger_params.modeSet); 
  SerialDEBUG.println("***************************************************************************************************");  
}

