/*! @file DSFamily.h

 @mainpage Arduino Library Header to access the DS Family of 1-wire temperature sensors

 @section intro_section Description

The goal of this class is to simplify using multiple DSFamily 1-Wire thermometers from an Arduino. Particularly
when the number of attached devices is unknown, quite a bit of valuable variable memory is consumed when
allocating an array of {n} times the 8 Byte (64bit) Unique address; e.g. 128Bytes of storage would normally
be reserved if there can be up to 16 1-Wire devices.\n\n

This class stores this address information at the end of the EEPROM memory, optionally reserving space at the
beginning of EEPROM for other applications. Thus the maximum number of devices that can be processed depends upon 
the space reserved and the space available on the given Atmel processor. While the number of write-cycles to 
EEPROM should be limited to 10,000; the order of the 1-Wire devices is deterministic and the EEPROM.h library 
calls will check to make sure that no writes are performed if the data hasn't changed, so using EEPROM for this 
functionality is not an issue as few, if any, writes are done after the program has been run with a given 
configuration.\n\n

Access to the devices is done with an index number rather than the 64-Bit unique address, simplifying using the 
device. Several methods are built into the library to simplify basic operations on multiple thermometers, 
including allowing one of the thermometer readings to be ignored - important if one of the devices is placed
elsewhere - i.e. one thermometer is on the board to measure ambient temperature, or one thermometer is placed
directly on the evaporator plate in a refrigerator or freezer and is thus much colder than the others.\n\n

While the DS Family of thermometers are quite accurate, there can still be significant variations between readings. 
The class contains a calibration routine which assumes that all of the devices are at the same temperature and 
will use the DS-Family's 2 User-definable bytes to store offset calibration information which ensures a significant 
improvement in accuracy.\n\n

The Maxim DSFamily of thermometers use the 1-Wire microLAN protocol. There is an excellent library for 1-Wire,
written by Paul Stoffregen and located at http://www.pjrc.com/teensy/td_libs_OneWire.html. There is also an
informative page at http://playground.arduino.cc/Learning/OneWire describing how to use the library. As there
are parts of the code that are unnecessary for this DS implementation and in order to make this library self-
sufficient, the code from version 2.0 (extracted 2016-11-23), has been included in this library.

@section license GNU General Public License v3.0
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation, either version 3 of the License, or (at your
option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details. You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

@section author Author

Written by Arnd\@SV-Zanshin

@section versions Changelog

Version| Date       | Developer                     | Comments
------ | ---------- | ----------------------------- | --------
1.0.5  | 2019-01-26 | https://github.com/SV-Zanshin | Issue #4 - converted documentation to doxygen

1.0.8  | 2019-01-27 | https://github.com/SV-Zanshin | Issue #4 - Convert to  Doxygen format
1.0.7  | 2018-06-26 | https://github.com/SV-Zanshin | Packaging  and documentation changes, optimized EEPROM
1.0.6   |2018-06-25 | https://github.com/SV-Zanshin | Documentation changes
1.0.5  | 2017-07-31 | https://github.com/SV-Zanshin | Only function prototypes may have default values as this
                                                      may cause compiler errors.
1.0.4  | 2016-12-29 | https://github.com/SV-Zanshin | Added error loop to Read1WireScratchpad(), corrected
                                                      DS18S20 call in ReadDeviceTemp() to avoid false temps
1.0.3  | 2016-12-16 | https://github.com/SV-Zanshin | Added optional CalibrationTemp to Calibrate function
1.0.2  | 2016-12-03 | https://github.com/SV-Zanshin | Added optional ReadDeviceTemp "WaitSwitch", minimized waits
1.0.1  | 2016-12-02 | https://github.com/SV-Zanshin | Added delays for ReadDeviceTemp() and when a parasitic device
                                                      is present
1.0.0  | 2016-12-01 | https://github.com/SV-Zanshin | Initial release
1.0.b5 | 2016-11-30 | https://github.com/SV-Zanshin | Moved 1-Wire calls to private, refactored some of the calls
1.0.b4 | 2016-11-29 | https://github.com/SV-Zanshin | Included sections of the 1-Wire library, see above
1.0.b3 | 2016-11-23 | https://github.com/SV-Zanshin | Refactored class naming DS18B20 to support multiple types
1.0.b2 | 2016-11-14 | https://github.com/SV-Zanshin | Made ScanForDevices return the number of devices found
1.0.b1 | 2016-11-10 | https://github.com/SV-Zanshin | Added SRAM template functions to read/write efficiently
*/

#include <EEPROM.h>    // Access the AVR EEPROM memory
#if ARDUINO >= 100     // Include depending on version
  #include "Arduino.h"
#else                                       
  #include "WProgram.h"
  #include "pins_arduino.h" // for digitalPinToBitMask, etc.
#endif
#if defined(__AVR__) // Platform specific I/O definitions
  #define PIN_TO_BASEREG(OneWirePin)     (portInputRegister(digitalPinToPort(OneWirePin)))
  #define PIN_TO_BITMASK(OneWirePin)     (digitalPinToBitMask(OneWirePin))
  #define IO_REG_TYPE uint8_t
  #define IO_REG_ASM asm("r30")
  #define DIRECT_READ(base, mask)        (((*(base)) & (mask)) ? 1 : 0)
  #define DIRECT_MODE_INPUT(base, mask)  ((*((base)+1)) &= ~(mask))
  #define DIRECT_MODE_OUTPUT(base, mask) ((*((base)+1)) |= (mask))
  #define DIRECT_WRITE_LOW(base, mask)   ((*((base)+2)) &= ~(mask))
  #define DIRECT_WRITE_HIGH(base, mask)  ((*((base)+2)) |= (mask))
#elif defined(__MK20DX128__)
  #define PIN_TO_BASEREG(OneWirePin)     (portOutputRegister(OneWirePin))
  #define PIN_TO_BITMASK(OneWirePin)     (1)
  #define IO_REG_TYPE uint8_t
  #define IO_REG_ASM
  #define DIRECT_READ(base, mask)        (*((base)+512))
  #define DIRECT_MODE_INPUT(base, mask)  (*((base)+640) = 0)
  #define DIRECT_MODE_OUTPUT(base, mask) (*((base)+640) = 1)
  #define DIRECT_WRITE_LOW(base, mask)   (*((base)+256) = 1)
  #define DIRECT_WRITE_HIGH(base, mask)  (*((base)+128) = 1)
#elif defined(__SAM3X8E__)
  /*****************************************************************************************************************
  ** Arduino 1.5.1 may have a bug in delayMicroseconds() on Arduino Due. If you have trouble with OneWire on the  **
  ** Arduino Due, please check the status of delayMicroseconds() before reporting a bug in OneWire! See URL       **
  ** http://arduino.cc/forum/index.php/topic,141030.msg1076268.html#msg1076268 for details                        **
  *****************************************************************************************************************/
  #define PIN_TO_BASEREG(OneWirePin)      (&(digitalPinToPort(OneWirePin)->PIO_PER))
  #define PIN_TO_BITMASK(OneWirePin)      (digitalPinToBitMask(OneWirePin))
  #define IO_REG_TYPE uint32_t
  #define IO_REG_ASM
  #define DIRECT_READ(base, mask)         (((*((base)+15)) & (mask)) ? 1 : 0)
  #define DIRECT_MODE_INPUT(base, mask)   ((*((base)+5)) = (mask))
  #define DIRECT_MODE_OUTPUT(base, mask)  ((*((base)+4)) = (mask))
  #define DIRECT_WRITE_LOW(base, mask)    ((*((base)+13)) = (mask))
  #define DIRECT_WRITE_HIGH(base, mask)   ((*((base)+12)) = (mask))
  #ifndef PROGMEM
    #define PROGMEM
  #endif
  #ifndef pgm_read_byte
    #define pgm_read_byte(addr) (*(const uint8_t *)(addr))
  #endif
#elif defined(__PIC32MX__)
  #define PIN_TO_BASEREG(OneWirePin)      (portModeRegister(digitalPinToPort(OneWirePin)))
  #define PIN_TO_BITMASK(OneWirePin)      (digitalPinToBitMask(OneWirePin))
  #define IO_REG_TYPE uint32_t
  #define IO_REG_ASM
  #define DIRECT_READ(base, mask)         (((*(base+4)) & (mask)) ? 1 : 0) // PORTX + 0x10
  #define DIRECT_MODE_INPUT(base, mask)   ((*(base+2)) = (mask))           // TRISXSET + 0x08
  #define DIRECT_MODE_OUTPUT(base, mask)  ((*(base+1)) = (mask))           // TRISXCLR + 0x04
  #define DIRECT_WRITE_LOW(base, mask)    ((*(base+8+1)) = (mask))         // LATXCLR  + 0x24
  #define DIRECT_WRITE_HIGH(base, mask)   ((*(base+8+2)) = (mask))         // LATXSET + 0x28
#else
  #error "Please define I/O register types here"
#endif
#ifndef DSFamily_h
  /** @brief  Guard code to prevent multiple definitions */
  #define DSFamily_h
/*!
* @class   DSFamily_Class
* @brief   Access the available DS-Family devices on the 1-Wire bus
*/  class DSFamily_Class
  {
    public: 
      DSFamily_Class(const uint8_t OneWirePin, const uint8_t ReserveRom = 0 );
      ~DSFamily_Class();
      uint16_t ConversionMillis;                                              ///< Current conversion milliseconds
      uint8_t  ThermometersFound = 0;                                         ///< Number of Devices  discovered
      bool     Parasitic         = true;                                      ///< One or more parasitic devices present

      uint8_t  ScanForDevices      ();
      int16_t  ReadDeviceTemp      (const uint8_t deviceNumber, const bool raw=false);
      void     DeviceStartConvert  (const uint8_t deviceNumber=UINT8_MAX, const bool WaitSwitch=false);
      void     Calibrate           (const uint8_t iterations=30, const int16_t CalTemp=INT16_MAX);
      int8_t   GetDeviceCalibration(const uint8_t deviceNumber);
      void     SetDeviceCalibration(const uint8_t deviceNumber, const int8_t  offset);
      int16_t  MinTemperature      (const uint8_t skipDeviceNumber=UINT8_MAX);
      int16_t  MaxTemperature      (const uint8_t skipDeviceNumber=UINT8_MAX);
      int16_t  AvgTemperature      (const uint8_t skipDeviceNumber=UINT8_MAX);
      float    StdDevTemperature   (const uint8_t skipDeviceNumber=UINT8_MAX);
      void     SetDeviceResolution (const uint8_t deviceNumber, uint8_t resolution);
      uint8_t  GetDeviceResolution (const uint8_t deviceNumber);
      void     GetDeviceROM        (const uint8_t deviceNumber, uint8_t ROMBuffer[8]);
      uint8_t  crc8                (const uint8_t *addr, uint8_t len);
    private:
      uint8_t  _MaxThermometers;             ///< Number of devices found/stord
      uint32_t _ConvStartTime;               ///< Conversion start time
      bool     _LastCommandWasConvert=false; ///< Unset when other commands issued
      IO_REG_TYPE bitmask;                   ///< Bitmask for 1-Wire IO
      volatile IO_REG_TYPE *baseReg;         ///< Base register
      unsigned char ROM_NO[8];               ///< global search state array
      uint8_t  LastDiscrepancy;              ///< 1-Wire internal value
      uint8_t  LastFamilyDiscrepancy;        ///< 1-Wire internal value
      uint8_t  LastDeviceFlag;               ///< 1-Wire internal value

      boolean  Read1WireScratchpad(const uint8_t deviceNumber, uint8_t bf[9]);
      void     SelectDevice(const uint8_t deviceNumber);
      void     ParasiticWait();
      void     reset_search();
      uint8_t  reset(void);
      void     write_bit(uint8_t v);
      uint8_t  read_bit(void);
      void     write_byte(uint8_t v, uint8_t power = 0 );
      uint8_t  read_byte();
      void     select(const uint8_t rom[8]);
      uint8_t  search(uint8_t *newAddr);
  }; // of DSFamily class definition
#endif
