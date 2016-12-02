/*******************************************************************************************************************
** Class definition header for the DSFamily class. Defines the methods and variables in the class                 **
**                                                                                                                **
** This program is free software: you can redistribute it and/or modify it under the terms of the GNU General     **
** Public License as published by the Free Software Foundation, either version 3 of the License, or (at your      **
** option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY     **
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   **
** GNU General Public License for more details. You should have received a copy of the GNU General Public License **
** along with this program.  If not, see <http://www.gnu.org/licenses/>.                                          **
**                                                                                                                **
*******************************************************************************************************************/
#include "DSFamily.h"                                                         // Include the header definition    //
/*******************************************************************************************************************
** Declare constants used in the class, but ones that are not visible as public or private class components       **
*******************************************************************************************************************/
const uint8_t  DS18B20_FAMILY          =   0x28;                              // The Family byte for DS18B20      //
const uint8_t  DS18S20_FAMILY          =   0x10;                              // The family byte for DS18S20      //
const uint8_t  DS28EA00_FAMILY         =   0x42;                              // The family byte for DS28EA00     //
const uint8_t  DS1825_FAMILY           =   0x3B;                              // The family byte for DS1825       //
const uint8_t  DS1822_FAMILY           =   0x22;                              // The family byte for DS1822       //
const uint8_t  DS_START_CONVERT        =   0x44;                              // Command to start conversion      //
const uint8_t  DS_READ_SCRATCHPAD      =   0xBE;                              // Command to read the temperature  //
const uint8_t  DS_READ_POWER_SUPPLY    =   0xB4;                              // Command to read the temperature  //
const uint8_t  DS_WRITE_SCRATCHPAD     =   0x4E;                              // Write to the DS scratchpad       //
const uint8_t  DS_COPY_SCRATCHPAD      =   0x48;                              // Copy the contents of scratchpad  //
const uint8_t  DS_SKIP_ROM             =   0xCC;                              // Skip the ROM address on 1-Wire   //
const uint8_t  DS_SELECT_ROM           =   0x55;                              // Select the ROM address on 1-Wire //
const uint8_t  DS_SEARCH               =   0xF0;                              // Search the 1-Wire for devices    //
const int16_t  DS_BAD_TEMPERATURE      = 0xFC90;                              // Bad measurement value, -55°C     //
const uint8_t  DS_MAX_NV_CYCLE_TIME    =    100;                              // Max ms taken to write NV memory  //
const uint8_t  DS_USER_BYTE_1          =      2;                              // The 2nd scratchpad byte          //
const uint8_t  DS_USER_BYTE_2          =      3;                              // The 3rd scratchpad byte          //
const uint8_t  DS_CONFIG_BYTE          =      4;                              // The 4th scratchpad byte          //
const uint16_t DS_12b_CONVERSION_TIME  =    750;                              // Max ms taken to convert @ 12bits //
const uint16_t DS_11b_CONVERSION_TIME  =    375;                              // Max ms taken to convert @ 11bits //
const uint16_t DS_10b_CONVERSION_TIME  =    188;                              // Max ms taken to convert @ 10bits //
const uint16_t DS_9b_CONVERSION_TIME   =     94;                              // Max ms taken to convert @ 9bits  //

/*******************************************************************************************************************
** Class Constructor instantiates the class and uses the initializer list to also instantiate the 1-Wire microLAN **
** on the defined pin and to set the maximum number of thermometers that the system can store in EEPROM. The      **
** latter is dynamic because it depends upon which Atmel processor is being used, as each one has different amount**
** off EEPROM space available                                                                                     **
*******************************************************************************************************************/
DSFamily_Class::DSFamily_Class(const uint8_t OneWirePin,                      // CONSTRUCTOR - Instantiate 1-Wire //
                               const uint8_t ReserveRom)                      // protocol                         //
                 : ConversionMillis(DS_12b_CONVERSION_TIME),                  // Default conversion ms to maximum //
                   _MaxThermometers((E2END-ReserveRom)/8) {                   //                                  //
   pinMode(OneWirePin, INPUT);                                                // Make the 1-Wire pin an input     //
   bitmask = PIN_TO_BITMASK(OneWirePin);                                      // Set the bitmask                  //
   baseReg = PIN_TO_BASEREG(OneWirePin);                                      // Set the base register            //
   reset_search();                                                            // Reset the search status          //
} // of class constructor                                                     //----------------------------------//

/*******************************************************************************************************************
** Class Destructor currently does nothing and is included for compatibility purposes                             **
*******************************************************************************************************************/
DSFamily_Class::~DSFamily_Class() { }                                         // unused DESTRUCTOR                //

/*******************************************************************************************************************
** Use the standardized 1-Wire microLAN search mechanism to discover all DS devices. Each device has a unique     **
** 8-byte ROM address, which is stored at the end of program's EEPROM. Since each Atmel chip has a different      **
** amount of memory, and the class constructor allows the user to specify a number of bytes to reserve at the the **
** beginning of the EEPROM memory the maximum number of devices that can be processed by the class is variable.   **
** After each device is discovered the resolution is set to the maximum value and a conversion is initiated. This **
** is done as soon as possible since a conversion at maximum resolution takes up to 750ms                         **
*******************************************************************************************************************/
uint8_t DSFamily_Class::ScanForDevices() {                                    //                                  //
  uint8_t tempTherm[8];                                                       // Temporary DS address array       //
  reset_search();                                                             // Reset the search status          //
  ThermometersFound = 0;                                                      // Reset the number of thermometers //
  while (search(tempTherm)) {                                                 // Use the 1-Wire "search" method   //
    if (tempTherm[0]==DS18B20_FAMILY || tempTherm[0]==DS18S20_FAMILY ||       // Only add the recognized DS-      //
        tempTherm[0]==DS28EA00_FAMILY || tempTherm[0]==DS1822_FAMILY ||       // Family devices                   //
        tempTherm[0]==DS1825_FAMILY ) {                                       //                                  //
      EEPROM.put(E2END-((ThermometersFound+1)*8),tempTherm);                  // Write thermometer data to EEPROM //
      SetDeviceResolution(ThermometersFound,12);                              // Set to maximum resolution        //
      if (ThermometersFound<=_MaxThermometers) ThermometersFound++;           // Increment if we have room        //
    } // of if-then we have a DS18x20 family device                           // otherwise we just skip the device//
  } // of while there are still new devices on the 1-Wire bus                 //                                  //
  reset();                                                                    // Reset the 1-Wire bus, then       //
  write_byte(DS_SKIP_ROM);                                                    // Send Skip ROM code               //
  write_byte(DS_READ_POWER_SUPPLY);                                           // Send command to read power supply//
  Parasitic = !read_bit();                                                    // Read the power status from bus   //
  DeviceStartConvert();                                                       // Start conversion for all devices //
  return(ThermometersFound);                                                  // return number of devices detected//
} // of method ScanForDevices                                                 //----------------------------------//

/*******************************************************************************************************************
** Method Read1WireScratchpad() method to read the contents of a given DS devices scratchpad from the 1-Wire      **
** microLAN into local memory.                                                                                    **
*******************************************************************************************************************/
boolean DSFamily_Class::Read1WireScratchpad(const uint8_t deviceNumber,       //                                  //
                                           uint8_t buffer[9]) {               //                                  //
  SelectDevice(deviceNumber);                                                 // Reset the 1-wire, address device //
  write_byte(DS_READ_SCRATCHPAD);                                             // Request device send Scratchpad   //
  for (uint8_t i=0;i<9;i++) buffer[i] = read_byte();                          // read all 9 bytes sent by DS      //
  return(crc8(buffer,8) == buffer[8] );                                       // Return false if bad CRC checksum //
} // of method Read1WireScratchpad()                                          //----------------------------------//

/*******************************************************************************************************************
** method ReadDeviceTemp() to return the current temperature value for a given device number. All devices except  **
** the DS18S20 return raw values in 0.0625°C increments, so the 0.5°C increments of the DS18S20 are converted to  **
** the same scale as the other devices. A check is done to see if there are still conversion(s) being done and a  **
** delay is made until any conversions have time to complete. We only store ony value for conversion start time,  **
** so the delay might be for another devices and might not be necessary, but the alternative is to store the      **
** conversion times for each device which would potentially consume a lot of available memory                     **
*******************************************************************************************************************/
int16_t DSFamily_Class::ReadDeviceTemp(const uint8_t deviceNumber,            //                                  //
                                       const bool raw=false) {                //                                  //
  uint8_t dsBuffer[9];                                                        // Buffer to hold scratchpad return //
  int16_t temperature = DS_BAD_TEMPERATURE;                                   // Holds return value               //
  if ((_ConvStartTime+ConversionMillis)>millis())                             // If a conversion is still running //
    delay(millis()-(_ConvStartTime+ConversionMillis));                        // then wait until it is finished   //
  if ( deviceNumber < ThermometersFound &&                                    // on a successful read from the    //
       Read1WireScratchpad(deviceNumber,dsBuffer)) {                          // device scratchpad                //
    if (dsBuffer[0]==DS1822_FAMILY) {                                         // The results returned by DS18S20  //
      temperature = ((dsBuffer[1] << 8) | dsBuffer[0])<<3;                    // get the raw reading and apply    //
      temperature = (temperature & 0xFFF0) + 12 - dsBuffer[6];                // value from "count remain" byte   //
    } else temperature = (dsBuffer[1]<<8)|dsBuffer[0];                        // Results come in 2s complement    //
    if (dsBuffer[2]^dsBuffer[3] == 0xFF &&                                    // Apply any calibration offset     //
        !raw) temperature += (int8_t)dsBuffer[2];                             // if raw is not true               //
  } // of if-then the read was successful                                     // of if-then-else the DS18x20 read //
  return(temperature);                                                        // Return our computed reading      //
} // of method ReadDeviceTemp()                                               //----------------------------------//

/*******************************************************************************************************************
** method DeviceStartConvert() to start the sampling and conversion on a device. At maximum resolution this       **
** conversion can take 750ms. If the optional parameter is not specified then all device conversions are started  **
** at the same time                                                                                               **
*******************************************************************************************************************/
void DSFamily_Class::DeviceStartConvert(const uint8_t deviceNumber=UINT8_MAX){//                                  //
  ParasiticWait();                                                            // Wait for conversion if necessary //
  if (deviceNumber==UINT8_MAX) {                                              // If no parameter specified, use   //
    reset();                                                                  // Reset 1-wire network             //
    write_byte(DS_SKIP_ROM);                                                  // Tell all devices to listen       //
  } else SelectDevice(deviceNumber); // of if-then all devices or just one    //                                  //
  write_byte(DS_START_CONVERT);                                               // Initiate temperature conversion  //
  _ConvStartTime = millis();                                                  // Store start time of conversion   //
} // of method DeviceStartConvert                                             //----------------------------------//

/*******************************************************************************************************************
** CalibrateDSFamily() method to calibrate all thermometers. Each DS has a persistent 2 user bytes which can be   **
** both read and updated.  These can be used for triggering alarms, but in this application we are going to set   **
** these to provide a calibration offset value so that all DS devices on the 1-wire can be set to accurately      **
** show the same temperature. This only works when all of the thermometers are at the same temperature, which can **
** be done by various methods. What temperature is used for the calibration is unimportant, although a calibration**
** at typical operating temperatures makes the most sense.                                                        **
**                                                                                                                **
** The calibration method used here is quite simple and straightforward. First, all devices are measured for a    **
** period of time defined in MEASUREMENT_ITERATIONS. The average of all readings is computed and that value is    **
** assumed to be the correct and accurate temperature reading. Each thermometer's offset to this standard value   **
** is computed and is written to the two user bytes.                                                              **
**                                                                                                                **
** In order to ensure that the correct values are used at runtime, the values are written to the two user bytes   **
** so that XOR'ing them together always results in a value of 0xFF.                                               **
*******************************************************************************************************************/
void DSFamily_Class::Calibrate(const uint8_t iterations=30) {                 //                                  //
  const uint8_t  DS_MAX_THERMOMETERS  =  32;                                  // Specify a maximum number here    //
  int64_t stats1[DS_MAX_THERMOMETERS] = {0};                                  // store statistics per device      //
  int64_t tempSum                     =   0;                                  // Stores interim values            //
  int8_t  offset                      =   0;                                  // Stores the computed offset value //
  uint8_t ThermometersUsed = min(DS_MAX_THERMOMETERS,ThermometersFound);      // Use the lower of the 2 values    //
  for (uint8_t i=0;i<iterations;i++) {                                        // Loop to get a good sampling      //
    for(uint8_t x=0;x<ThermometersUsed;x++) {                                 // process each thermometer         //
      stats1[x] += ReadDeviceTemp(x,true);                                    // read raw temperature, no offset  //
    }  // of for each thermometer loop                                        //                                  //
    DeviceStartConvert();                                                     // Start conversion on all devices  //
    delay(ConversionMillis);                                                  // Wait to complete measurements    //
  } // of for loop                                                            //                                  //
  for (uint8_t i=0;i<ThermometersUsed;i++) tempSum += stats1[i];              // Add value to standard dev comps  //
  tempSum = tempSum/iterations/ThermometersUsed;                              // tempSum now holds average value  //
  for (uint8_t i=0;i<ThermometersUsed;i++) {                                  // Loop for every thermometer found //
    offset = tempSum-(stats1[i]/iterations);                                  // Compute offset value from mean   //
    SetDeviceCalibration(i,offset);                                           // Set the new user bytes 1&2       //
  } // of for-next all thermometers                                           //                                  //
} // of method CalibrateDS18B20                                               //----------------------------------//

/*******************************************************************************************************************
** Method SetDeviceCalibration to set the user bytes 1 and 2 to the calibration computed                          **
*******************************************************************************************************************/
void DSFamily_Class::SetDeviceCalibration(const uint8_t deviceNumber,         //                                  //
                                          const int8_t offset) {              //                                  //
  uint8_t dsBuffer[9];                                                        // Temporary scratchpad buffer      //
  Read1WireScratchpad(deviceNumber,dsBuffer);                                 // Read from the device scratchpad  //
  SelectDevice(deviceNumber);                                                 // Reset 1-wire, address device     //
  write_byte(DS_WRITE_SCRATCHPAD);                                            // Write scratchpad, send 3 bytes   //
  write_byte(offset);                                                         // Write user Byte 1                //
  write_byte(offset^0xFF);                                                    // Write the XOR'd value user Byte 1//
  write_byte(dsBuffer[DS_CONFIG_BYTE]);                                       // Set configuration register back  //
  write_byte(DS_COPY_SCRATCHPAD);                                             // Copy scratchpad values to NV mem //
  delay(DS_MAX_NV_CYCLE_TIME);                                                // Give the DS18x20 time to process //
} // of method SetDeviceCalibration()                                         //----------------------------------//

/*******************************************************************************************************************
** Method GetDeviceCalibration to get the calibration offset for a device. Return INT16_MIN if the device isn't   **
** calibrated                                                                                                     **
*******************************************************************************************************************/
int8_t DSFamily_Class::GetDeviceCalibration(const uint8_t deviceNumber) {     //                                  //
  int8_t offset = INT8_MIN;                                                   // Default to an invalid value      //
  uint8_t dsBuffer[9];                                                        // Temporary scratchpad buffer      //
  Read1WireScratchpad(deviceNumber,dsBuffer);                                 // Read from the device scratchpad  //
  if (dsBuffer[2]^dsBuffer[3]==0xFF) offset = (int8_t)dsBuffer[2];            // Use the calibration value        //
  return (offset);                                                            // Return the computed offset       //
} // of method GetDeviceCalibration()                                         //----------------------------------//

/*******************************************************************************************************************
** Method SelectDevice will reset the 1-Wire microLAN and select the device number specified                      **
*******************************************************************************************************************/
void DSFamily_Class::SelectDevice(const uint8_t deviceNumber) {               //                                  //
  uint8_t ROMBuffer[8];                                                       // Array to hold unique DS ID       //
  ParasiticWait();                                                            // Wait for conversion if necessary //
  for (uint8_t i=0;i<8;i++)                                                   //                                  //
    ROMBuffer[i]=EEPROM.read(i+E2END-((deviceNumber+1)*8));                   // Read the EEPROM byte             //
  reset();                                                                    // Reset 1-wire communications      //
  select(ROMBuffer);                                                          // Select only current device       //
} // of method SelectDevice()                                                 //----------------------------------//

/*******************************************************************************************************************
** Method GetDeviceROM will return the 8-byte ROM address buffer                                                  **
*******************************************************************************************************************/
void DSFamily_Class::GetDeviceROM(const uint8_t deviceNumber,                 //                                  //
                                  uint8_t ROMBuffer[8]) {                     // Array to hold unique DS ID       //
  for (uint8_t i=0;i<8;i++)                                                   // loop for each byte in the buffer //
  ROMBuffer[i]=EEPROM.read(i+E2END-((deviceNumber+1)*8));                     // Read the EEPROM byte             //
} // of method GetDeviceROM()                                                 //----------------------------------//

/*******************************************************************************************************************
** Method MinTemperature reads all current device temperatures and returns the lowest value. If the optional      **
** skipDeviceNumber is specified then that device number is skipped; this is used when one of the thermometers is **
** out-of-band - i.e. if it is attached to an evaporator plate and reads much lower than the others.              **
*******************************************************************************************************************/
int16_t DSFamily_Class::MinTemperature(uint8_t skipDeviceNumber=UINT8_MAX) {  // Minimum of devices, optional skip//
  int16_t minimumTemp = INT16_MAX;                                            // Starts at highest possible value //
  int16_t deviceTemp;                                                         // Store temperature for comparison //
  for (uint8_t i=0;i<ThermometersFound;i++) {                                 // loop each thermometer found      //
    deviceTemp = ReadDeviceTemp(i);                                           // retrieve the temperature reading //
    if (i!=skipDeviceNumber && deviceTemp<minimumTemp)                        // if value is less than minimum    //
      minimumTemp=deviceTemp;                                                 //                                  //
  } // of for-next each thermometer                                           //                                  //
  return(minimumTemp);                                                        //                                  //
} // of method MinTemperature                                                 //----------------------------------//

/*******************************************************************************************************************
** Method MaxTemperature reads all current device temperatures and returns the lowest value. If the optional      **
** skipDeviceNumber is specified then that device number is skipped; this is used when one of the thermometers is **
** out-of-band - i.e. if it is attached to a heat source and reads much higher than the others.                   **
*******************************************************************************************************************/
int16_t DSFamily_Class::MaxTemperature(uint8_t skipDeviceNumber=UINT8_MAX) {  // Maximum temperature of devices   //
  int16_t maximumTemp = INT16_MIN;                                            // Starts at lowest possible value  //
  int16_t deviceTemp;                                                         // temporarily store temperature    //
  for (uint8_t i=0;i<ThermometersFound;i++) {                                 // loop through each thermometer    //
    deviceTemp = ReadDeviceTemp(i);                                           // retrieve the temperature reading //
    if (i!=skipDeviceNumber&&deviceTemp>maximumTemp) maximumTemp=deviceTemp;  // if greater than minimum reset    //
  } // of for-next each thermometer                                           //                                  //
  return(maximumTemp);                                                        //                                  //
} // of method MaxTemperature                                                 //----------------------------------//

/*******************************************************************************************************************
** Method AvgTemperature reads all current device temperatures and returns the average value. If the optional     **
** skipDeviceNumber is specified then that device number is skipped; this is used when one of the thermometers is **
** out-of-band                                                                                                    **
*******************************************************************************************************************/
int16_t DSFamily_Class::AvgTemperature(const uint8_t skipDeviceNumber=UINT8_MAX){// Average temperature of devices//
  int16_t AverageTemp = 0;                                                    // return value starts at 0         //
  int16_t deviceTemp;                                                         // store temperature for comparison //
  for (uint8_t i=0;i<ThermometersFound;i++) {                                 // loop through each thermometer    //
    if (i!=skipDeviceNumber) AverageTemp += ReadDeviceTemp(i);                // add temperature to the sum       //
  } // of for-next each thermometer                                           //                                  //
  if (skipDeviceNumber==UINT8_MAX)                                            // Divide by number of thermometers //
     AverageTemp = AverageTemp/ThermometersFound;                             //                                  //
     else  AverageTemp = AverageTemp/(ThermometersFound-1);                   //                                  //
  return(AverageTemp);                                                        //                                  //
} // of method AvgTemperature                                                 //----------------------------------//

/*******************************************************************************************************************
** Method SetDeviceResolution will set the resolution of the DS devices to 9, 10, 11 or 12 bits. Lower resolution **
** results in a faster conversion time. The global ConversionMillis is set on the assumption that all devices are **
** set to the same resolution                                                                                     **
** Value Resolution Conversion                                                                                    **
** ===== ========== ==========                                                                                    **
**     9  0.5°C      93.75ms                                                                                      **
**    10  0.25°C    187.5 ms                                                                                      **
**    11  0.125°C   375   ms                                                                                      **
**    12  0.0625°C  750   ms                                                                                      **
*******************************************************************************************************************/
void DSFamily_Class::SetDeviceResolution(const uint8_t deviceNumber,          // Set resolution to 9,10,11 or 12 b//
                                  uint8_t resolution) {                       //                                  //
  uint8_t dsBuffer[9];                                                        // Buffer to hold scratchpad values //
  if(resolution<9 || resolution>12) resolution = 12;                          // Default to full resolution       //
  switch (resolution) {                                                       // Now select which action to do    //
    case 12:                                                                  // When 12 bits of precision used   //
      ConversionMillis = DS_12b_CONVERSION_TIME;                              // Set the conversion time          //
      break;                                                                  //----------------------------------//
    case 11:                                                                  // When 12 bits of precision used   //
      ConversionMillis = DS_11b_CONVERSION_TIME;                              // Set the conversion time          //
      break;                                                                  //----------------------------------//
    case 10:                                                                  // When 12 bits of precision used   //
      ConversionMillis = DS_10b_CONVERSION_TIME;                              // Set the conversion time          //
      break;                                                                  //----------------------------------//
    case 9:                                                                   // When 12 bits of precision used   //
      ConversionMillis = DS_9b_CONVERSION_TIME;                               // Set the conversion time          //
      break;                                                                  //----------------------------------//
    default:                                                                  // Otherwise                        //
      ConversionMillis = DS_12b_CONVERSION_TIME;                              // Set the conversion time          //
  } // of switch statement for precision                                      //----------------------------------//
  resolution = (resolution-9)<<5;                                             // Shift resolution bits over       //
  Read1WireScratchpad(deviceNumber,dsBuffer);                                 // Read device scratchpad           //
  SelectDevice(deviceNumber);                                                 // Reset 1-wire, address device     //
  write_byte(DS_WRITE_SCRATCHPAD);                                            // Write scratchpad, send 3 bytes   //
  write_byte(dsBuffer[DS_USER_BYTE_1]);                                       // Restore the old user byte 1      //
  write_byte(dsBuffer[DS_USER_BYTE_2]);                                       // Restore the old user byte 2      //
  write_byte(resolution);                                                     // Set configuration register       //
  write_byte(DS_COPY_SCRATCHPAD);                                             // Copy scratchpad to NV memory     //
  delay(DS_MAX_NV_CYCLE_TIME);                                                // Give the DS18x20 time to process //
} // of method SetDeviceResolution                                            //----------------------------------//

/*******************************************************************************************************************
** Method SetUserBytes to set the user bytes 1 and 2 to the calibration computed                                  **
*******************************************************************************************************************/
uint8_t DSFamily_Class::GetDeviceResolution(const uint8_t deviceNumber) {     //                                  //
   uint8_t resolution;                                                        // Store the return value           //
   uint8_t dsBuffer[9];                                                       // Hold scratchpad return values    //
   Read1WireScratchpad(deviceNumber,dsBuffer);                                // Read from the device scratchpad  //
   resolution = (dsBuffer[DS_CONFIG_BYTE]>>5)+9;                              // get bits 6&7 from the config byte//
   return(resolution);                                                        //                                  //
} // of method GetDeviceResolution()                                          //----------------------------------//

/*******************************************************************************************************************
** Method StdDevTemperature reads all current device temperatures and returns the standard deviation. If the      **
** optional skipDeviceNumber is specified then that device number is skipped; this is used when one of the        **
** thermometers is out-of-band                                                                                    **
*******************************************************************************************************************/
float DSFamily_Class::StdDevTemperature(const uint8_t skipDeviceNumber=UINT8_MAX){//Average temperature of devices//
  float StdDev = 0;                                                           // computed standard deviation      //
  int16_t AverageTemp = AvgTemperature(skipDeviceNumber);                     // Compute the average              //
  for (uint8_t i=0;i<ThermometersFound;i++) {                                 // loop through each thermometer    //
    if (i!=skipDeviceNumber) StdDev += sq(AverageTemp-ReadDeviceTemp(i));     // add squared variance delta to sum//
  } // of for-next each thermometer                                           //                                  //
  if (skipDeviceNumber==UINT8_MAX)                                            // Divide by number of thermometers //
     StdDev = StdDev/ThermometersFound;                                       //                                  //
  else  StdDev = StdDev/(ThermometersFound-1);                                //                                  //
  StdDev = sqrt(StdDev);                                                      // compute the square root          //
  return(StdDev);                                                             //                                  //
} // of method StdDevTemperature                                              //----------------------------------//

/*******************************************************************************************************************
** 1-Wire: Method reset_search(). You need to use this function to start a search again from the beginning. You   **
** do not need to do it for the first search, though you could.                                                   **
*******************************************************************************************************************/
void DSFamily_Class::reset_search() {                                         //                                  //
  LastDiscrepancy       = 0;                                                  //                                  //
  LastDeviceFlag        = FALSE;                                              //                                  //
  LastFamilyDiscrepancy = 0;                                                  //                                  //
  for(int i = 7; ; i--) {                                                     //                                  //
    ROM_NO[i] = 0;                                                            //                                  //
    if ( i == 0) break;                                                       //                                  //
  } // of for-next each ROM byte                                              //                                  //
} // of method reset_search                                                   //----------------------------------//

/*******************************************************************************************************************
** 1-Wire: Method reset(). Perform the onewire reset function.  Wait up to 250uS for the bus to come high, if it  **
** doesn't then it is broken or shorted and we return a 0; Returns 1 if a device asserted a presence pulse, 0     **
** otherwise.                                                                                                     **
*******************************************************************************************************************/
uint8_t DSFamily_Class::reset(void) {                                         //                                  //
  IO_REG_TYPE mask = bitmask;                                                 // Set the bitmask                  //
  volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;                             // point to the base register       //
  uint8_t r;                                                                  // Return value                     //
  uint8_t retries = 125;                                                      // Maximum number of retries        //
  noInterrupts();                                                             // Disable interrupts for now       //
  DIRECT_MODE_INPUT(reg, mask);                                               // Send to register                 //
  interrupts();                                                               // Enable interrupts again          //
  do {                                                                        // wait until the wire is high...   //
    if (--retries == 0) return 0;                                             // wire is high, so return          //
    delayMicroseconds(2);                                                     // Wait a bit                       //
  } while ( !DIRECT_READ(reg, mask));                                         // wait until the wire is high...   //
  noInterrupts();                                                             // Disable interrupts for now       //
  DIRECT_WRITE_LOW(reg, mask);                                                //                                  //
  DIRECT_MODE_OUTPUT(reg, mask);                                              // drive output low                 //
  interrupts();                                                               // Enable interrupts again          //
  delayMicroseconds(480);                                                     // Wait 480 microseconds            //
  noInterrupts();                                                             // Disable interrupts for now       //
  DIRECT_MODE_INPUT(reg, mask);                                               // allow it to float                //
  delayMicroseconds(70);                                                      // Wait 70 microseconds             //
  r = !DIRECT_READ(reg, mask);                                                // Read the status                  //
  interrupts();                                                               // Enable interrupts again          //
  delayMicroseconds(410);                                                     // Wait again                       //
  return r;                                                                   // return the result                //
} // of method reset()                                                        //----------------------------------//

/*******************************************************************************************************************
** 1-Wire: Method write_bit(). Write a bit. Port & bit is used to cut lookup time and provide more certain timing.**
*******************************************************************************************************************/
void DSFamily_Class::write_bit(uint8_t v) {                                   //                                  //
  IO_REG_TYPE mask=bitmask;                                                   // Register mask                    //
  volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;                             // Register                         //
  if (v & 1) {                                                                // If we are writing a 1            //
    noInterrupts();                                                           // Disable interrupts for now       //
    DIRECT_WRITE_LOW(reg, mask);                                              //                                  //
    DIRECT_MODE_OUTPUT(reg, mask);                                            // drive output low                 //
    delayMicroseconds(10);                                                    // Wait                             //
    DIRECT_WRITE_HIGH(reg, mask);                                             // drive output high                //
    interrupts();                                                             // Enable interrupts again          //
    delayMicroseconds(55);                                                    // Wait                             //
    } else {                                                                  //                                  //
    noInterrupts();                                                           // Disable interrupts for now       //
    DIRECT_WRITE_LOW(reg, mask);                                              //                                  //
    DIRECT_MODE_OUTPUT(reg, mask);                                            // drive output low                 //
    delayMicroseconds(65);                                                    // Wait                             //
    DIRECT_WRITE_HIGH(reg, mask);                                             // drive output high                //
    interrupts();                                                             // Enable interrupts again          //
    delayMicroseconds(5);                                                     // Wait                             //
  } // of if-then we have a "true" to write                                   //                                  //
} // of method write_bit()                                                    //----------------------------------//

/*******************************************************************************************************************
** 1-Wire: Method read_bit(). Read a bit. Port & bit is used to cut lookup time and provide more certain timing.  **
*******************************************************************************************************************/
uint8_t DSFamily_Class::read_bit(void) {                                      //                                  //
  IO_REG_TYPE mask=bitmask;                                                   // Register mask                    //
  volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;                             // Register                         //
  uint8_t r;                                                                  // Return bit                       //
  noInterrupts();                                                             // Disable interrupts for now       //
  DIRECT_MODE_OUTPUT(reg, mask);                                              //                                  //
  DIRECT_WRITE_LOW(reg, mask);                                                //                                  //
  delayMicroseconds(3);                                                       // Wait                             //
  DIRECT_MODE_INPUT(reg, mask);                                               // let pin float, pull up will raise//
  delayMicroseconds(10);                                                      // Wait                             //
  r = DIRECT_READ(reg, mask);                                                 //                                  //
  interrupts();                                                               // Enable interrupts again          //
  delayMicroseconds(53);                                                      // Wait                             //
  return r;                                                                   // Return result                    //
} // of method read_bit()                                                     //----------------------------------//

/*******************************************************************************************************************
** 1-Wire: Method write_byte(). Write a byte. The writing code uses the active drivers to raise the pin high, if  **
** you need power after the write (e.g. DS18S20 in parasite power mode) then set 'power' to 1, otherwise the pin  **
** will go tri-state at the end of the write to avoid heating in a short or other mishap.                         **
*******************************************************************************************************************/
void DSFamily_Class::write_byte(uint8_t v, uint8_t power = 0 ) {              //                                  //
  uint8_t bitMask;                                                            // Bit mask                         //
  for (bitMask = 0x01; bitMask; bitMask <<= 1) write_bit( (bitMask & v)?1:0); // Write bits until empty           //
  if ( !power) {                                                              // Set pin in parasite mode         //
    noInterrupts();                                                           // Disable interrupts for now       //
    DIRECT_MODE_INPUT(baseReg, bitmask);                                      //                                  //
    DIRECT_WRITE_LOW(baseReg, bitmask);                                       //                                  //
    interrupts();                                                             // Enable interrupts again          //
  } // of if-then we have parasite mode                                       //                                  //
} // of method write_byte()                                                   //----------------------------------//

/*******************************************************************************************************************
** 1-Wire: Method read_byte(). Read a single byte.                                                                **
*******************************************************************************************************************/
uint8_t DSFamily_Class::read_byte() {                                         //                                  //
  uint8_t bitMask;                                                            // Bit Mask                         //
  uint8_t r = 0;                                                              // Return byte                      //
  for (bitMask = 0x01; bitMask; bitMask <<= 1) {                              // For each bit in the byte         //
    if ( read_bit()) r |= bitMask;                                            // Read bit into correct position   //
  } // of for-next each bit in the byte                                       //                                  //
  return r;                                                                   // Return byte that was read        //
} // of method read_byte()                                                    //----------------------------------//

/*******************************************************************************************************************
** 1-Wire: Method select(). Do a 1-Wire ROM Select                                                                **
*******************************************************************************************************************/
void DSFamily_Class::select(const uint8_t rom[8]) {                           //                                  //
  write_byte(DS_SELECT_ROM);                                                  // Send Select ROM code             //
  for (uint8_t i = 0; i < 8; i++) write_byte(rom[i]);                         // Send the ROM address bytes       //
} // of method select()                                                       //----------------------------------//

/*******************************************************************************************************************
** 1-Wire: Method search(). Search the 1-Wire microLAN using the Dallas Semiconductor search algorith and code.   **
** Perform a search. If this function returns a '1' then it has enumerated the next device and you may retrieve   **
** the ROM from the OneWire::address variable. If there are no devices, no further devices, or something horrible **
** happens in the middle of the enumeration then a 0 is returned.  If a new device is found then its address is   **
** copied to newAddr.  Use DSFamil_Class::reset_search() to start over.                                           **
** Return TRUE - device found, ROM number in ROM_NO buffer, FALSE - device not found, end of search               **
*******************************************************************************************************************/
uint8_t DSFamily_Class::search(uint8_t *newAddr) {                            //                                  //
  uint8_t id_bit_number, last_zero, rom_byte_number, search_result,           // local temporary variable         //
          id_bit, cmp_id_bit;                                                 // definitions                      //
  unsigned char rom_byte_mask, search_direction;                              //                                  //
  id_bit_number   = 1;                                                        // initialize values for searching  //
  last_zero       = 0;                                                        //                                  //
  rom_byte_number = 0;                                                        //                                  //
  rom_byte_mask   = 1;                                                        //                                  //
  search_result   = 0;                                                        //                                  //
  if (!LastDeviceFlag) {                                                      // if last call was not the last one//
    if (!reset()) {                                                           // 1-Wire reset                     //
      LastDiscrepancy       = 0;                                              // reset the search                 //
      LastDeviceFlag        = FALSE;                                          //                                  //
      LastFamilyDiscrepancy = 0;                                              //                                  //
      return FALSE;                                                           //                                  //
    } // of if-then we have a reset                                           //                                  //
    write_byte(DS_SEARCH);                                                    // issue the search command         //
    do {                                                                      // loop to do the search            //
      id_bit     = read_bit();                                                // read a bit                       //
      cmp_id_bit = read_bit();                                                // and then the complement          //
      if ((id_bit == 1) && (cmp_id_bit == 1)) break;                          // check for no devices on 1-wire   //
      else {                                                                  //                                  //
        if (id_bit != cmp_id_bit) search_direction = id_bit;                  // all devices coupled have 0 or 1  //
        else {                                                                //  bit write value for search      //
          // if this discrepancy is before the Last Discrepancy on a previous next then pick the same as last time//
          if (id_bit_number < LastDiscrepancy)                                //                                  //
            search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask)>0); //                                  //
          else                                                                // if equal to last pick then 1, if //
            search_direction = (id_bit_number == LastDiscrepancy);            // not then pick 0                  //
          if (search_direction == 0) {                                        // if 0 was picked then record its  //
            last_zero = id_bit_number;                                        // position in LastZero             //
            if (last_zero < 9) LastFamilyDiscrepancy = last_zero;             // check for Last discrepancy       //
          } // of if-then search direction is 0                               //                                  //
        } // of if-then-else  all devices have 0 or 1                         //                                  //
        if (search_direction == 1)                                            // set or clear the bit in the ROM  //
          ROM_NO[rom_byte_number] |= rom_byte_mask;                           // byte rom_byte_number with mask   //
        else                                                                  // rom_byte_mask                    //
          ROM_NO[rom_byte_number] &= ~rom_byte_mask;                          //                                  //
        write_bit(search_direction);                                          // serial number search direction   //
        id_bit_number++;                                                      // increment the byte counter       //
        rom_byte_mask <<= 1;                                                  // id_bit_number&shift rom_byte_mask//
        if (rom_byte_mask == 0) {                                             // if the mask is 0 then go to new  //
          rom_byte_number++;                                                  // SerialNum byte rom_byte_number   //
          rom_byte_mask = 1;                                                  // and reset mask                   //
        } // of if-then mask is 0                                             //                                  //
      } // of if-then-else device still found                                 //                                  //
    } // of loop until all devices found                                      //                                  //
    while(rom_byte_number < 8);                                               // loop until through ROM bytes 0-7 //
    if (!(id_bit_number < 65)) {                                              // if the search was successful then//
      LastDiscrepancy = last_zero;                                            // search successful so set         //
      if (LastDiscrepancy == 0) LastDeviceFlag = TRUE;                        // check for last device            //
      search_result = TRUE;                                                   //                                  //
    } // of if-then search was successful                                     //                                  //
  } // of if-then there are still devices to be found                         //                                  //
  if (!search_result || !ROM_NO[0]) {                                         // if no device found then reset    //
    LastDiscrepancy       = 0;                                                // counters so next 'search' will   //
    LastDeviceFlag        = FALSE;                                            // be like a first                  //
    LastFamilyDiscrepancy = 0;                                                //                                  //
    search_result         = FALSE;                                            //                                  //
  } // of if-then no device found                                             //                                  //
  for (int i = 0; i < 8; i++) newAddr[i] = ROM_NO[i];                         // Copy result buffer               //
  return search_result;                                                       // Return results                   //
} // of method search()                                                       //----------------------------------//

/*******************************************************************************************************************
** 1-Wire: Method crc8(). Compute the 8 bit crc of the returned buffer. This method uses the iterative method,    **
** which is slower than the default 1-Wire table lookup but that uses 255 bytes of program memory.                **
*******************************************************************************************************************/
uint8_t DSFamily_Class::crc8(const uint8_t *addr, uint8_t len) {              //                                  //
  uint8_t crc = 0;                                                            // Return crc value                 //
  while (len--) {                                                             //                                  //
    uint8_t inbyte = *addr++;                                                 //                                  //
    for (uint8_t i = 8; i; i--) {                                             //                                  //
      uint8_t mix = (crc ^ inbyte) & 0x01;                                    //                                  //
      crc >>= 1;                                                              //                                  //
      if (mix) crc ^= 0x8C;                                                   //                                  //
      inbyte >>= 1;                                                           //                                  //
    } // of for-next each byte in the ROM                                     //                                  //
  } // of while data still be to computed                                     //                                  //
  return crc;                                                                 // return computed CRC value        //
} // of method crc8()                                                         //----------------------------------//

/*******************************************************************************************************************
** Method ParasiticWait(). Any parasitically device needs to have a strong power pullup on the 1-Wire data line   **
** during conversion.  This means that the whole 1-Wire microLAN is effectively blocked during the rather lengthy **
** conversion time; since using the bus would cause the parasitically powered device to abort conversion. There-  **
** this function will wait until the last conversion request has had enough time to complete. The wait time might **
** be unnecessary, but since we only track the last conversion start rather than track each device independently  **
** this is the best we can do.                                                                                    **
*******************************************************************************************************************/
void DSFamily_Class::ParasiticWait() {                                        //                                  //
  if (Parasitic && ((_ConvStartTime+ConversionMillis)>millis())) {            // If parasitic & active conversion //
      delay(millis()-(_ConvStartTime+ConversionMillis));                      // then wait until it is finished   //
  } // of if-then we have a parasitic device on the 1-Wire                    //                                  //
} // of method ParasiticWait()                                                //----------------------------------//
