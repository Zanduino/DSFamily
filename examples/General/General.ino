/*! @file General.ino

@section General_intro_section Description

This program demonstrates some of the general functionality in the DSFamily library.\n\n

The following constant can/should be adjusted according to the 1-Wire configuration and user preferences:\n
"ONE_WIRE_PIN"           is the Arduino pin on which the data line of the 1-Wire system is attached\n\n

The precision of the DS-Family devices is programmable from 9 to 12 bits and the granularity of readings is 
0.0625°C. All temperature readings coming back from the library are signed 16 bit numbers in increments of 
0.0625°C. This means if a temperature reading is "355" it equates to 21.875°C.

@section Generallicense License

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation, either version 3 of the License, or (at your
option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details. You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

@section Generalauthor Author

Written by Arnd\@SV-Zanshin

@section Generalversions Changelog

Version | Date       | Developer                     | Comments
------- | ---------- | ----------------------------- | ---------------------------------------------------
1.0.2   | 2019-01-27 | https://github.com/SV-Zanshin | Changed documentation to doxygen style
1.0.1   | 2017-07-14 | https://github.com/SV-Zanshin | Removed calibration references in program as they are unused
1.0.0   | 2016-11-30 | https://github.com/SV-Zanshin | Initial coding
*/
#include <DSFamily.h> // DS Thermometers calls and methods
/*******************************************************************************************************************
** Declare all program constants                                                                                  **
*******************************************************************************************************************/
const uint32_t SERIAL_BAUD_RATE    = 115200; ///< Serial communication baud rate
const uint8_t  ONE_WIRE_PIN        =      5; ///< 1-Wire attached to PIN 5
const float    DS_DEGREES          = 0.0625; ///< Degrees per DS unit
const uint8_t  SPRINTF_BUFFER_SIZE =     64; ///< Maximum single sprintf() buffer
/*******************************************************************************************************************
** Declare global variables and instantiate classes                                                               **
*******************************************************************************************************************/
uint8_t        thermometers = 0;            ///< The number of devices found
char           buffer[SPRINTF_BUFFER_SIZE]; ///< buffer for sprintf() calls
uint8_t        ROMBuffer[8];                ///< Holds unique address of device
DSFamily_Class DSFamily(ONE_WIRE_PIN,128);  ///< Start DSFamily, reserve 128Bytes

/*!
    @brief    Arduino method called once at startup to initialize the system
    @details  This is an Arduino IDE method which is called first upon boot or restart. It is only called one time
              and then control goes to the main "loop()" method, from which control never returns
    @return   void
*/
void setup() 
{
  Serial.begin(SERIAL_BAUD_RATE); // initiate serial I/O communications
#ifdef  __AVR_ATmega32U4__  // If this is a 32U4 processor, then wait 3 seconds to initialize USB
  delay(3000);
#endif
  Serial.print("\n\nGeneral DSFamily test program Version [1.0.1]\n- Discovered ");
  thermometers = DSFamily.ScanForDevices();
  Serial.print(thermometers);
  Serial.print(" DS-Family thermometers.\n");
  if (DSFamily.Parasitic)
  {
    Serial.print("- One or more devices in Parasitic mode.\n");
  }
  else
  {
    Serial.print("- No parasitic devices in use.\n");
  } // if-then-else parasitic
} // of method setup()

/*!
    @brief    Arduino method for the main program loop
    @details  This is the main program for the Arduino IDE, it is an infinite loop and keeps on repeating.
    @return   void
*/
void loop() 
{
  int16_t temperature;
  static uint8_t precision = 12; // start with 12 bits precision
  Serial.print("- Setting all thermometers to ");
  Serial.print(precision);
  Serial.print(" bits precision\n");
  for (uint8_t i=0;i<DSFamily.ThermometersFound;i++)
    DSFamily.SetDeviceResolution(i,precision);   // Set resolution to current value
  DSFamily.DeviceStartConvert();                 // Start conversion on all devices
  Serial.print("- Starting measurement (waiting ");
  Serial.print(DSFamily.ConversionMillis);
  Serial.print("ms).\n");
  delay(DSFamily.ConversionMillis);
  Serial.print("- Minimum temperature is ");
  temperature = DSFamily.MinTemperature();     // retrieve the Minimum temperature
  Serial.print(temperature);
  Serial.print(" units (");
  Serial.print(temperature*DS_DEGREES,3);
  Serial.print("C)\n");
  Serial.print("- Maximum temperature is ");
  temperature = DSFamily.MaxTemperature();     // retrieve the Maximum temperature
  Serial.print(temperature);
  Serial.print(" units (");
  Serial.print(temperature*DS_DEGREES,3);
  Serial.print("C)\n");
  Serial.print("- Average temperature is ");
  temperature = DSFamily.AvgTemperature();     // retrieve the average temperature
  Serial.print(temperature);
  Serial.print(" units (");
  Serial.print(temperature*DS_DEGREES,3);
  Serial.print("C)\n");
  Serial.print("- Standard deviation is ");
  float stdDev = DSFamily.StdDevTemperature(); // retrieve the standard deviation
  Serial.print(stdDev,3);
  Serial.print("\n- Waiting 10 seconds...");
  delay(10000);
  Serial.print("\n\n");
  if (--precision==8) precision=12; // decrement precision between 9-12
} // of method loop()
