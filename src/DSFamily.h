/*******************************************************************************************************************
** Class definition header for the DSFamily class. Defines the methods and variables in the class                 **
**                                                                                                                **
** The goal of this class is to simplify using multiple DSFamily 1-Wire thermometers from an Arduino. Particularly**
** when the number of attached devices is unknown, quite a bit of valuable variable memory is consumed when       **
** allocating an array of {n} times the 8 Byte (64bit) Unique address; e.g. 128Bytes of storage would normally    **
** be reserved if there can be up to 16 1-Wire devices.                                                           **
**                                                                                                                **
** This class stores this address information at the end of the EEPROM memory, optionally reserving space at the  **
** beginning of EEPROM for other applications. Thus the maximum number of devices that can be processed depends   **
** upon the space reserved and the space available on the given Atmel processor. While the number of write-cycles **
** to EEPROM should be limited to 10,000; the order of the 1-Wire devices is deterministic and the EEPROM.h       **
** library calls will check to make sure that no writes are performed if the data hasn't changed, so using EEPROM **
** for this functionality is not an issue as few, if any, writes are done after the program has been run with a   **
** given configuration.                                                                                           **
**                                                                                                                **
** Access to the devices is done with an index number rather than the 64-Bit unique address, simplifying using    **
** the device. Several methods are built into the library to simplify basic operations on multiple thermometers,  **
** including allowing one of the thermometer readings to be ignored - important if one of the devices is placed   **
** elsewhere - i.e. one thermometer is on the board to measure ambient temperature, or one thermometer is placed  **
** directly on the evaporator plate in a refrigerator or freezer and is thus much colder than the others.         **
**                                                                                                                **
** While the DS Family of thermometers are quite accurate, there can still be significant variations between      **
** readings. The class contains a calibration routine which assumes that all of the devices are at the same       **
** temperature and will use the DS-Family's 2 User-definable bytes to store offset calibration information which  **
** ensures a significant improvement in accuracy.                                                                 **
**                                                                                                                **
** The Maxim DSFamily of thermometers use the 1-Wire microLAN protocol. There is an excellent library for 1-Wire, **
** written by Paul Stoffregen and located at http://www.pjrc.com/teensy/td_libs_OneWire.html. There is also an    **
** informative page at http://playground.arduino.cc/Learning/OneWire describing how to use the library. As there  **
** are parts of the code that are unnecessary for this DS implementation and in order to make this library self-  **
** sufficient, the code from version 2.0 (extracted 2016-11-23), has been included in this library.               **
**                                                                                                                **
** Although programming for the Arduino and in c/c++ is new to me, I'm a professional programmer and have learned,**
** over the years, that it is much easier to ignore superfluous comments than it is to decipher non-existent ones;**
** so both my comments and variable names tend to be verbose. The code is written to fit in the first 80 spaces   **
** and the comments start after that and go to column 117 - allowing the code to be printed in A4 landscape mode. **
**                                                                                                                **
** GNU General Public License                                                                                     **
** ==========================                                                                                     **
** This program is free software: you can redistribute it and/or modify it under the terms of the GNU General     **
** Public License as published by the Free Software Foundation, either version 3 of the License, or (at your      **
** option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY     **
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   **
** GNU General Public License for more details. You should have received a copy of the GNU General Public License **
** along with this program.  If not, see <http://www.gnu.org/licenses/>.                                          **
**                                                                                                                **
** Vers.  Date       Developer                     Comments                                                       **
** ====== ========== ============================= ============================================================== **
** 1.0.7  2018-06-26 https://github.com/SV-Zanshin Packaging  and documentation changes, optimiyed EEPROM         **
** 1.0.6  2018-06-25 https://github.com/SV-Zanshin Documentation changes                                          **
** 1.0.5  2017-07-31 https://github.com/SV-Zanshin Only function prototypes may have default values as this       **
**                                                 may cause compiler errors.                                     **
** 1.0.4  2016-12-29 https://github.com/SV-Zanshin Added error loop to Read1WireScratchpad(), corrected           **
**                                                 DS18S20 call in ReadDeviceTemp() to avoid false temps          **
** 1.0.3  2016-12-16 https://github.com/SV-Zanshin Added optional CalibrationTemp to Calibrate function           **
** 1.0.2  2016-12-03 https://github.com/SV-Zanshin Added optional ReadDeviceTemp "WaitSwitch", minimized waits    **
** 1.0.1  2016-12-02 https://github.com/SV-Zanshin Added delays for ReadDeviceTemp() and when a parasitic         **
**                                                 device is present                                              **
** 1.0.0  2016-12-01 https://github.com/SV-Zanshin Initial release                                                **
** 1.0.b5 2016-11-30 https://github.com/SV-Zanshin Moved 1-Wire calls to private, refactored some of the calls    **
** 1.0.b4 2016-11-29 https://github.com/SV-Zanshin Included sections of the 1-Wire library, see above             **
** 1.0.b3 2016-11-23 https://github.com/SV-Zanshin Refactored class naming DS18B20 to support multiple types      **
** 1.0.b2 2016-11-14 https://github.com/SV-Zanshin Made ScanForDevices return the number of devices found         **
** 1.0.b1 2016-11-10 https://github.com/SV-Zanshin Added SRAM template functions to read/write efficiently        **
*******************************************************************************************************************/
#include "Arduino.h"                                                          // Arduino data type definitions    //
#include <EEPROM.h>                                                           // Access the EEPROM memory         //
#if ARDUINO >= 100                                                            // Include depending on version     //
  #include "Arduino.h"                                                        // delayMicroseconds,               //
#else                                                                         // digitalPinToBitMask, etc.        //
  #include "WProgram.h"                                                       // for delayMicroseconds            //
  #include "pins_arduino.h"                                                   // for digitalPinToBitMask, etc     //
#endif                                                                        //                                  //
#define FALSE 0                                                               // FALSE and TRUE used by 1-Wire    //
#define TRUE  1                                                               // library code                     //
#if defined(__AVR__)                                                          // Platform specific I/O definitions//
  #define PIN_TO_BASEREG(OneWirePin)     (portInputRegister(digitalPinToPort(OneWirePin)))//                      //
  #define PIN_TO_BITMASK(OneWirePin)     (digitalPinToBitMask(OneWirePin))    //                                  //
  #define IO_REG_TYPE uint8_t                                                 //                                  //
  #define IO_REG_ASM asm("r30")                                               //                                  //
  #define DIRECT_READ(base, mask)        (((*(base)) & (mask)) ? 1 : 0)       //                                  //
  #define DIRECT_MODE_INPUT(base, mask)  ((*((base)+1)) &= ~(mask))           //                                  //
  #define DIRECT_MODE_OUTPUT(base, mask) ((*((base)+1)) |= (mask))            //                                  //
  #define DIRECT_WRITE_LOW(base, mask)   ((*((base)+2)) &= ~(mask))           //                                  //
  #define DIRECT_WRITE_HIGH(base, mask)  ((*((base)+2)) |= (mask))            //                                  //
#elif defined(__MK20DX128__)                                                  //                                  //
  #define PIN_TO_BASEREG(OneWirePin)     (portOutputRegister(OneWirePin))     //                                  //
  #define PIN_TO_BITMASK(OneWirePin)     (1)                                  //                                  //
  #define IO_REG_TYPE uint8_t                                                 //                                  //
  #define IO_REG_ASM                                                          //                                  //
  #define DIRECT_READ(base, mask)        (*((base)+512))                      //                                  //
  #define DIRECT_MODE_INPUT(base, mask)  (*((base)+640) = 0)                  //                                  //
  #define DIRECT_MODE_OUTPUT(base, mask) (*((base)+640) = 1)                  //                                  //
  #define DIRECT_WRITE_LOW(base, mask)   (*((base)+256) = 1)                  //                                  //
  #define DIRECT_WRITE_HIGH(base, mask)  (*((base)+128) = 1)                  //                                  //
#elif defined(__SAM3X8E__)                                                    //                                  //
  /*****************************************************************************************************************
  ** Arduino 1.5.1 may have a bug in delayMicroseconds() on Arduino Due. If you have trouble with OneWire on the  **
  ** Arduino Due, please check the status of delayMicroseconds() before reporting a bug in OneWire! See URL       **
  ** http://arduino.cc/forum/index.php/topic,141030.msg1076268.html#msg1076268 for details                        **
  *****************************************************************************************************************/
  #define PIN_TO_BASEREG(OneWirePin)      (&(digitalPinToPort(OneWirePin)->PIO_PER)) //                           //
  #define PIN_TO_BITMASK(OneWirePin)      (digitalPinToBitMask(OneWirePin))   //                                  //
  #define IO_REG_TYPE uint32_t                                                //                                  //
  #define IO_REG_ASM                                                          //                                  //
  #define DIRECT_READ(base, mask)         (((*((base)+15)) & (mask)) ? 1 : 0) //                                  //
  #define DIRECT_MODE_INPUT(base, mask)   ((*((base)+5)) = (mask))            //                                  //
  #define DIRECT_MODE_OUTPUT(base, mask)  ((*((base)+4)) = (mask))            //                                  //
  #define DIRECT_WRITE_LOW(base, mask)    ((*((base)+13)) = (mask))           //                                  //
  #define DIRECT_WRITE_HIGH(base, mask)   ((*((base)+12)) = (mask))           //                                  //
  #ifndef PROGMEM                                                             //                                  //
    #define PROGMEM                                                           //                                  //
  #endif                                                                      //                                  //
  #ifndef pgm_read_byte                                                       //                                  //
    #define pgm_read_byte(addr) (*(const uint8_t *)(addr))                    //                                  //
  #endif                                                                      //                                  //
#elif defined(__PIC32MX__)                                                    //                                  //
  #define PIN_TO_BASEREG(OneWirePin)      (portModeRegister(digitalPinToPort(OneWirePin)))//                      //
  #define PIN_TO_BITMASK(OneWirePin)      (digitalPinToBitMask(OneWirePin))   //                                  //
  #define IO_REG_TYPE uint32_t                                                //                                  //
  #define IO_REG_ASM                                                          //                                  //
  #define DIRECT_READ(base, mask)         (((*(base+4)) & (mask)) ? 1 : 0)    // PORTX + 0x10                     //
  #define DIRECT_MODE_INPUT(base, mask)   ((*(base+2)) = (mask))              // TRISXSET + 0x08                  //
  #define DIRECT_MODE_OUTPUT(base, mask)  ((*(base+1)) = (mask))              // TRISXCLR + 0x04                  //
  #define DIRECT_WRITE_LOW(base, mask)    ((*(base+8+1)) = (mask))            // LATXCLR  + 0x24                  //
  #define DIRECT_WRITE_HIGH(base, mask)   ((*(base+8+2)) = (mask))            // LATXSET + 0x28                   //
#else                                                                         //                                  //
  #error "Please define I/O register types here"                              //                                  //
#endif                                                                        //                                  //
#ifndef DSFamily_h                                                            // Guard code definition            //
  #define DSFamily_h                                                          // Define the name inside guard code//
  class DSFamily_Class {                                                      // Class definition                 //
    public:                                                                   // Publicly visible methods         //
      DSFamily_Class(const uint8_t OneWirePin, const uint8_t ReserveRom = 0 );// Class Constructor                //
      ~DSFamily_Class();                                                      // Class destructor                 //
      uint8_t  ThermometersFound = 0;                                         // Number of Devices  discovered    //
      uint16_t ConversionMillis;                                              // Current conversion milliseconds  //
      bool     Parasitic         = true;                                      // If one or more parasitic devices //
      uint8_t  ScanForDevices      ();                                        // Scan/rescan the 1-Wire microLAN  //
      int16_t  ReadDeviceTemp      (const uint8_t deviceNumber,               // Return the temperature           //
                                    const bool raw=false);                    // optionally using the raw value   //
      void     DeviceStartConvert  (const uint8_t deviceNumber=UINT8_MAX,     // Start conversion on device       //
                                    const bool    WaitSwitch=false);          // optionally wait for it to finish //
      void     Calibrate           (const uint8_t iterations=30,              // Calibrate thermometers to read   //
                                    const int16_t CalTemp=INT16_MAX);         // identically                      //
      int8_t   GetDeviceCalibration(const uint8_t deviceNumber);              // Get the device's calibration     //
      void     SetDeviceCalibration(const uint8_t deviceNumber,               // Set calibration bytes 1 & 2      //
                                    const int8_t  offset);                    //                                  //
      int16_t  MinTemperature      (const uint8_t skipDeviceNumber=UINT8_MAX);// Min of devices, optional skip    //
      int16_t  MaxTemperature      (const uint8_t skipDeviceNumber=UINT8_MAX);// Max of devices, optional skip    //
      int16_t  AvgTemperature      (const uint8_t skipDeviceNumber=UINT8_MAX);// Avg of devices, optional skip    //
      float    StdDevTemperature   (const uint8_t skipDeviceNumber=UINT8_MAX);// Standard deviation, optional skip//
      void     SetDeviceResolution (const uint8_t deviceNumber,uint8_t res);  // Set resolution 9,10,11 or 12 bits//
      uint8_t  GetDeviceResolution (const uint8_t deviceNumber);              // Retrieve the device resolution   //
      void     GetDeviceROM        (const uint8_t deviceNumber,               // Return the physical ROM values   //
                                    uint8_t ROMBuffer[8]);                    //                                  //
      uint8_t  crc8                (const uint8_t *addr, uint8_t len);        // Compute 1-Wire 8bit CRC          //
    private:                                                                  // Private variables and methods    //
      boolean  Read1WireScratchpad (const uint8_t deviceNumber,uint8_t bf[9]);// Read device's scratchpad contents//
      void     SelectDevice        (const uint8_t deviceNumber);              // Reset 1-Wire & select device     //
      void     ParasiticWait();                                               // Wait for conversion if parasitic //
      uint8_t  _MaxThermometers;                                              // Devices fit (EEPROM-ReserveRom)  //
      uint32_t _ConvStartTime;                                                // Conversion start time            //
      bool     _LastCommandWasConvert=false;                                  // Unset when other commands issued //
      IO_REG_TYPE bitmask;                                                    //                                  //
      volatile IO_REG_TYPE *baseReg;                                          //                                  //
      unsigned char ROM_NO[8];                                                // global search state              //
      uint8_t  LastDiscrepancy;                                               //                                  //
      uint8_t  LastFamilyDiscrepancy;                                         //                                  //
      uint8_t  LastDeviceFlag;                                                //                                  //
      void     reset_search();                                                // 1-Wire reset search status       //
      uint8_t  reset(void);                                                   // Reset the 1-Wire microLAN state  //
      void     write_bit(uint8_t v);                                          // Write 1 bit on 1-Wire microLAN   //
      uint8_t  read_bit(void);                                                // Read 1 bit from 1-Wire microLAN  //
      void     write_byte(uint8_t v, uint8_t power  = 0 );                    // Write a byte to 1-Wire microLAN  //
      uint8_t  read_byte();                                                   // Read a byte from 1-Wire microLAN //
      void     select(const uint8_t rom[8]);                                  // Select a give 1-Wire device      //
      uint8_t  search(uint8_t *newAddr);                                      // Search 1-Wire for next device    //
  }; // of DSFamily class definition                                          //                                  //
#endif                                                                        //----------------------------------//
