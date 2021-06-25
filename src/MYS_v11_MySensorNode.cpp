/*
* MYS_v11_MySensorNode.ino - Firmware for Mys v1.1 board temperature and humidity sensor Node with nRF24L01+ module
*
* Copyright 2014 Tomas Hozza <thozza@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, see <http://www.gnu.org/licenses/>.
*
* Authors:
* Tomas Hozza <thozza@gmail.com>
* Richard Clarke <richard.ns@clarke.biz>
*
* MySensors library - http://www.mysensors.org/
* Mys v1.1 -http://www.sa2avr.se/mys-1-1/
* nRF24L01+ spec - https://www.sparkfun.com/datasheets/Wireless/Nordic/nRF24L01P_Product_Specification_1_0.pdf
*
Hardware Connections (Breakoutboard to Arduino):
 -VCC = 3.3V
 -GND = GND
 -SDA = A4 (use inline 10k resistor if your board is 5V)
 -SCL = A5 (use inline 10k resistor if your board is 5V)

Mys_v1.1 board compatible with Arduino PRO Mini 3.3V@8MHz

System Clock  = 8MHz

Information on porting a 1.5.x compatible sketch to 2.0.x

https://forum.mysensors.org/topic/4276/converting-a-sketch-from-1-5-x-to-2-0-x/2

 */

// Enable debug prints to serial monitor
//#define MY_DEBUG
#define DEBUG_RCC 1

// Enable and select radio type attached
#define MY_RADIO_RF24
//#define MY_RADIO_RFM69

#define MY_NODE_ID 22
/*Makes this static so won't try and find another parent if communication with
gateway fails*/
#define MY_PARENT_NODE_ID 0
#define MY_PARENT_NODE_IS_STATIC

/**
 * @def MY_TRANSPORT_WAIT_READY_MS
 * @brief Timeout in ms until transport is ready during startup, set to 0 for no timeout
 */
#define MY_TRANSPORT_WAIT_READY_MS (1000)

/*These are actually the default pins expected by the MySensors framework.
* This means we can use the default constructor without arguments when
* creating an instance of the Mysensors class. Other defaults will include
* transmitting on channel 76 with a data rate of 250kbps.
* MyGW1 = channel 76
* MyGW2 = channel 100
*/
#define MY_RF24_CE_PIN 9
#define MY_RF24_CS_PIN 10
#define MY_RF24_CHANNEL 100

#define DOOR_PIN 6

#include <MySensors.h> 
#include <stdint.h>
#include <math.h> 
#include <Wire.h>
#include <BatterySense.hpp>

// Include the Bounce2 library found here :
// https://github.com/thomasfredericks/Bounce2
#include <Bounce2.h>

// Sleep time between sensor updates (in milliseconds)
//static const uint32_t DAY_UPDATE_INTERVAL_MS = 30000;
//static const uint32_t DAY_UPDATE_INTERVAL_MS = 2500;

static const uint32_t DAY_UPDATE_INTERVAL_MS = 10000;

enum child_id_t
{
  CHILD_ID_HUMIDITY,
  CHILD_ID_TEMP,
  CHILD_ID_UV,
  CHILD_ID_VOLTAGE,
  CHILD_ID_EXT_VOLTAGE,
  CHILD_ID_DOOR
};

uint32_t clockSwitchCount = 0;

Bounce debouncer = Bounce(); 
int oldValue=-1;

/*****************************/
/********* FUNCTIONS *********/
/*****************************/

//Create an instance of the sensor objects
MyMessage msgVolt(CHILD_ID_VOLTAGE, V_VOLTAGE);
void switchClock(unsigned char clk);

/*Set true to have clock throttle back, or false to not throttle*/
bool throttlefreq = false;
bool cpu_is_throttled = false;

BatteryLevel batt;
/**********************************/
/********* IMPLEMENTATION *********/
/**********************************/
/*If you need to do initialization before the MySensors library starts up,
define a before() function */
void before()
{

}

/*To handle received messages, define the following function in your sketch*/
void receive(const MyMessage &message)
{
  /*Handle incoming message*/
}

/* If your node requests time using requestTime(). The following function is
used to pick up the response*/
void receiveTime(unsigned long ts)
{
}

/*You can still use setup() which is executed AFTER mysensors has been
initialised.*/
void setup()
{
  
  Wire.begin();
  pinMode(DOOR_PIN,OUTPUT);
  // Activate internal pull-up
  digitalWrite(DOOR_PIN,HIGH);
  Serial.begin(9600);

  // After setting up the button, setup debouncer
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(5);
  
}

void presentation()
{
   // Send the sketch version information to the gateway and Controller
  sendSketchInfo("DoorSensor", "0.6");
  // Register all sensors to gateway (they will be created as child devices)
  present(CHILD_ID_VOLTAGE, S_MULTIMETER);
  present(CHILD_ID_DOOR, S_DOOR);  
   
}


void loop()
{

  uint32_t update_interval_ms = DAY_UPDATE_INTERVAL_MS;

  clockSwitchCount++;
  #if DEBUG_RCC
  Serial.print("clockSwitchCount = ");
  Serial.print(clockSwitchCount,DEC);
  Serial.println();
  #endif
  
  // When we wake up the 5th time after power on, switch to 1Mhz clock
  // This allows us to print debug messages on startup (as serial port is dependent on oscillator settings).
  if ( (clockSwitchCount == 5) && throttlefreq)
  {
    /* Switch to 4Mhz by setting clock prescaler to divide by 2 for the reminder of the sketch, 
     * to save power but more importantly to allow operation down to 1.8V
     * 
      */
    
    #if DEBUG_RCC
    Serial.print("Setting CPU Freq to 4MHz");
    Serial.println();
    #endif
    switchClock(0x01); // divide by 2, to give 4MHz on 8MHz, 3V3 Pro Mini
    cpu_is_throttled = true;
    throttlefreq = false;
  } //end if

  uint16_t battLevel = batt.getVoltage();
  send(msgVolt.set(battLevel,1));

  debouncer.update();
  // Get the update value
  int value = debouncer.read();
 
  if (value != oldValue) {
     // Send in the new value
     send(msg.set(value==HIGH ? 1 : 0));
     oldValue = value;
  }



  sleep(update_interval_ms); 
  
  
} //end loop





void switchClock(unsigned char clk)
{
  cli();

  CLKPR = 1<<CLKPCE; // Set CLKPCE to enable clk switching
  CLKPR = clk;
  sei();
  
}


