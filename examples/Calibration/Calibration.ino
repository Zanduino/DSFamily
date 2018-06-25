/*******************************************************************************************************************
** This program demonstrates the calibration function in the DSFamily library.                                    **
**                                                                                                                **
**                                                                                                                **
** The following 2 constants can/should be adjusted according to the 1-Wire configuration and user preferences:   **
** "ONE_WIRE_PIN"           is the Arduino pin on which the data line of the 1-Wire system is attached            **
** "CALIBRATION_ITERATIONS" is the number of iterations to average temperature measurements per thermometer       **
**                                                                                                                **
**                                                                                                                **
** The precision of the DS-Family devices is programmable from 9 to 12 bits and the granularity of readings is    **
** 0.0625°C. Unfortunately there is a certain amount of deviation between devices so that they read slightly      **
** different temperatures. The calibration process measures the average temperatures of the different devices     **
** and then assumes that the average value of all readings for all thermometers is the correct value and then an  **
** offset to that ideal temperature is computed.                                                                  **
**                                                                                                                **
** Each DS-Family thermometer has 2 user bytes available for reading/writing. These two bytes can be used for     **
** high and low temperature alarms; but these are used for calibration purposes by this library. In order to      **
** prevent errors when one or both bytes are modified, the exclusive OR of the two values must be 0xFF in order   **
** for a calibration offset to be applied.                                                                        **
**                                                                                                                **
** This program is free software: you can redistribute it and/or modify it under the terms of the GNU General     **
** Public License as published by the Free Software Foundation, either version 3 of the License, or (at your      **
** option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY     **
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   **
** GNU General Public License for more details. You should have received a copy of the GNU General Public License **
** along with this program.  If not, see <http://www.gnu.org/licenses/>.                                          **
**                                                                                                                **
** Vers.  Date       Developer                     Comments                                                       **
** ====== ========== ============================= ============================================================== **
** 1.0.0  2016-11-30 https://github.com/SV-Zanshin Initial coding                                                 **
**                                                                                                                **
*******************************************************************************************************************/
#include <DSFamily.h>                                                         // DS Thermometers calls and methods//

/*******************************************************************************************************************
** Declare all program constants                                                                                  **
*******************************************************************************************************************/
#define PROGNAME                      F("Calibration")                        // Program Name                     //
#define PROGVERSION                      F(" [1.0.0]")                        // Version; reflected in comments   //
const uint16_t SERIAL_BAUD_RATE       =        115200;                        // Serial communication baud rate   //
const uint8_t  ONE_WIRE_PIN           =             5;                        // 1-Wire attached to PIN 5         //
const uint8_t  CALIBRATION_ITERATIONS =            30;                        // Number of iterations to run      //
const float    DS_DEGREES             =        0.0625;                        // Degrees per DS unit              //
const uint8_t  SPRINTF_BUFFER_SIZE    =            64;                        // Maximum single sprintf() buffer  //

/*******************************************************************************************************************
** Declare global variables and instantiate classes                                                               **
*******************************************************************************************************************/
uint8_t        thermometers = 0;                                              // The number of devices found      //
char           buffer[SPRINTF_BUFFER_SIZE];                                   // buffer for sprintf() calls       //
uint8_t        ROMBuffer[8];                                                  // Holds unique address of device   //
DSFamily_Class DSFamily(ONE_WIRE_PIN,128);                                    // Start DSFamily, reserve 128Bytes //

/*******************************************************************************************************************
** Method Setup(). This is an Arduino IDE method which is called upon boot or restart. It is only called one time **
** and then control goes to the main loop, which loop indefinately.                                               **
*******************************************************************************************************************/
void setup() {                                                                //                                  //
  Serial.begin(SERIAL_BAUD_RATE);                                             // initiate serial I/O communication//
  delay(4000);                                                                // Wait for serial communications   //
  Serial.print("\n\n");                                                       //                                  //
  Serial.print(PROGNAME);                                                     // Display program name             //
  Serial.print(PROGVERSION);                                                  // Display program version          //
  thermometers = DSFamily.ScanForDevices();                                   // Search and store Thermometers    //
  Serial.print("\n- Discovered ");                                            //                                  //
  Serial.print(thermometers);                                                 //                                  //
  Serial.print(" DS-Family thermometers.\n");                                 //                                  //
} // of method setup()                                                        //----------------------------------//

/*******************************************************************************************************************
** This is the main program for the Arduino IDE, it is an infinite loop and keeps on repeating.                   **
*******************************************************************************************************************/
void loop() {                                                                 //                                  //
  int64_t statsCAL[32], statsRAW[32];                                         // CALibrated and RAW Statistics    //
  int64_t avgCAL, avgRAW;                                                     //                                  //
  float   stdDevCAL, stdDevRAW;                                               //                                  //
  for (uint8_t i=0;i<32;i++) {                                                // Simple but slow method to zero   //
    statsCAL[i] = 0;                                                          // out all array elements           //
    statsRAW[i] = 0;                                                          //                                  //
  } // of for-next loop to clear array                                        //                                  //
  avgCAL = 0; avgRAW = 0; stdDevCAL = 0; stdDevRAW = 0;                       // Reset values to zero             //
  Serial.print("- Assuming thermometers are at the same temperature.\n");     //                                  //
  Serial.print("- Running Calibration for ");                                 //                                  //
  Serial.print(CALIBRATION_ITERATIONS);                                       //                                  //
  Serial.print(" iterations and ");                                           //                                  //
  Serial.print(CALIBRATION_ITERATIONS*DSFamily.ConversionMillis/1000);        //                                  //
  Serial.print(" seconds. Please Wait...");                                   //                                  //
  DSFamily.Calibrate(CALIBRATION_ITERATIONS);                                 // Perform calibration              //
  Serial.print("\n- Finished calibrating offsets as follows:\n\n");           //                                  //
  Serial.print("## Hex ROM Address  Raw  Off Corr Temp C \n");                //                                  //
  Serial.print("== ================ ==== === ==== =======\n");                //                                  //
  for(uint8_t i=0;i<DSFamily.ThermometersFound;i++) {                         // loop for each thermometer found  //
     DSFamily.GetDeviceROM(i,ROMBuffer);                                      // Read the unique ROM Address      //
     sprintf(buffer,"%02d %02X%02X%02X%02X%02X%02X%02X%02X %04d %3d %04d ",   // Write formatted string to buffer //
             i,ROMBuffer[0],ROMBuffer[1],ROMBuffer[2],ROMBuffer[3],           //                                  //
             ROMBuffer[4],ROMBuffer[5],ROMBuffer[6],ROMBuffer[7],             //                                  //
             DSFamily.ReadDeviceTemp(i,true),DSFamily.GetDeviceCalibration(i),//                                  //
             DSFamily.ReadDeviceTemp(i));                                     //                                  //
     Serial.print(buffer);                                                    // Output the formatted string      //
     Serial.println(DSFamily.ReadDeviceTemp(i)*DS_DEGREES,3);                 // sprintf() doesn't do floating    //
  } // of for-next each thermometer found loop                                //                                  //
  DSFamily.DeviceStartConvert();                                              // Start conversion on all devices  //
  delay(DSFamily.ConversionMillis);                                           // Wait for conversions to complete //
  Serial.print("\n- Computing statistics\n- Rerunning Calibration for ");     //                                  //
  Serial.print(CALIBRATION_ITERATIONS);                                       //                                  //
  Serial.print(" iterations and ");                                           //                                  //
  Serial.print(CALIBRATION_ITERATIONS*DSFamily.ConversionMillis/1000);        //                                  //
  Serial.print(" seconds. Please Wait...");                                   //                                  //
  for(uint8_t j=0;j<CALIBRATION_ITERATIONS;j++) {                             // loop the number of iterations    //
    for(uint8_t i=0;i<thermometers;i++) {                                     // loop for each thermometer found  //
      statsCAL[i] += DSFamily.ReadDeviceTemp(i);                              // compute calibrated statistics    //
      statsRAW[i] += DSFamily.ReadDeviceTemp(i,true);                         // compute raw statistics           //
    } // of for-next each thermometer                                         //                                  //
  } // of for-next each iteration                                             //                                  //
  for (uint8_t i=0;i<thermometers;i++) {                                      // Sum up the raw and calibrated    //
    avgCAL += statsCAL[i];                                                    // data in order to compute the     //
    avgRAW += statsRAW[i];                                                    // averages for both                //
  } // of for-next compute sums                                               //                                  //
  avgCAL = avgCAL/CALIBRATION_ITERATIONS/thermometers;                        // Store the average calibrated     //
  avgRAW = avgRAW/CALIBRATION_ITERATIONS/thermometers;                        // Store the average raw value      //
  for (uint8_t i=0;i<DSFamily.ThermometersFound;i++) {                        // Loop for every thermometer found //
    stdDevCAL += sq((statsCAL[i]/CALIBRATION_ITERATIONS)-avgCAL);             // Sum up the std deviation values  //
    stdDevRAW += sq((statsRAW[i]/CALIBRATION_ITERATIONS)-avgRAW);             // Sum up the std deviation values  //
  } // of for-next all thermometers                                           //                                  //
  stdDevCAL = stdDevCAL / thermometers;                                       // divide by number of thermometers //
  stdDevRAW = stdDevRAW / thermometers;                                       // divide by number of thermometers //
  Serial.print("\n- Variance/StdDev raw    : ");Serial.print(stdDevRAW,3);    //                                  //
  Serial.print(" / ");Serial.print(sqrt(stdDevRAW),3);                        //                                  //
  Serial.print("\n- Variance/StdDev after  : ");Serial.print(stdDevCAL,3);    //                                  //
  Serial.print(" / ");Serial.println(sqrt(stdDevCAL),3);                      //                                  //
  Serial.print("- Waiting 60 seconds before recomputing...");                 //                                  //
  delay(60000);                                                               // Wait 1 minute before looping     //
  Serial.print("\n\n");                                                       //                                  //
} // of method loop()                                                         //----------------------------------//
