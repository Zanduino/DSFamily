/*! @file DSFamily.cpp
 @section DSFamilycpp_intro_section Description

Arduino Library class to access the DS Family of 1-wire temperature sensors\n\n
See main library header file for details
*/
#include "DSFamily.h"  // Include the header definition
/***************************************************************************************************
** Declare constants used in the class, but ones that are not visible as public or private class  **
** components                                                                                     **
***************************************************************************************************/
const uint8_t  DS18B20_FAMILY{0x28};         ///< The Family byte for DS18B20
const uint8_t  DS18S20_FAMILY{0x10};         ///< The family byte for DS18S20
const uint8_t  DS28EA00_FAMILY{0x42};        ///< The family byte for DS28EA00
const uint8_t  DS1825_FAMILY{0x3B};          ///< The family byte for DS1825
const uint8_t  DS1822_FAMILY{0x22};          ///< The family byte for DS1822
const uint8_t  DS_START_CONVERT{0x44};       ///< Command to start conversion
const uint8_t  DS_READ_SCRATCHPAD{0xBE};     ///< Command to read the temperature
const uint8_t  DS_READ_POWER_SUPPLY{0xB4};   ///< Command to read the temperature
const uint8_t  DS_WRITE_SCRATCHPAD{0x4E};    ///< Write to the DS scratchpad
const uint8_t  DS_COPY_SCRATCHPAD{0x48};     ///< Copy the contents of scratchpad
const uint8_t  DS_SKIP_ROM{0xCC};            ///< Skip the ROM address on 1-Wire
const uint8_t  DS_SELECT_ROM{0x55};          ///< Select the ROM address on 1-Wire
const uint8_t  DS_SEARCH{0xF0};              ///< Search the 1-Wire for devices
const int16_t  DS_BAD_TEMPERATURE{0xFC90};   ///< Bad measurement value, -55°C
const uint8_t  DS_MAX_NV_CYCLE_TIME{100};    ///< Max ms taken to write NV memory
const uint8_t  DS_USER_BYTE_1{2};            ///< The 2nd scratchpad byte
const uint8_t  DS_USER_BYTE_2{3};            ///< The 3rd scratchpad byte
const uint8_t  DS_CONFIG_BYTE{4};            ///< The 4th scratchpad byte
const uint16_t DS_12b_CONVERSION_TIME{750};  ///< Max ms taken to convert @ 12bits
const uint16_t DS_11b_CONVERSION_TIME{375};  ///< Max ms taken to convert @ 11bits
const uint16_t DS_10b_CONVERSION_TIME{188};  ///< Max ms taken to convert @ 10bits
const uint16_t DS_9b_CONVERSION_TIME{94};    ///< Max ms taken to convert @ 9bits

DSFamily_Class::DSFamily_Class(const uint8_t OneWirePin, const uint8_t ReserveRom)
    : ConversionMillis(DS_12b_CONVERSION_TIME), _MaxThermometers((E2END - ReserveRom) / 8) {
  /*!
    @brief     Class constructor
    @details   Class Constructor instantiates the class and uses the initializer list to also
               instantiate the 1-Wire microLAN on the defined pin and to set the maximum number of
               thermometers that the system can store in EEPROM. The latter is dynamic because it
               depends upon which Atmel processor is being used, as each one has different amount of
               EEPROM space available
    @param[in] OneWirePin 1-Wire microLAN pin number
    @param[in] ReserveRom (Optional) Number of bytes of ROM space to reserve, used to calculate
    _MaxThermometers
  */
  pinMode(OneWirePin, INPUT);            // Make the 1-Wire pin an input
  bitmask = PIN_TO_BITMASK(OneWirePin);  // Set the bitmask
  baseReg = PIN_TO_BASEREG(OneWirePin);  // Set the base register
  reset_search();                        // Reset the search status
}  // of class constructor
DSFamily_Class::~DSFamily_Class() {
  /*!
    @brief   Class destructor
    @details Currently empty and unused
  */
}
uint8_t DSFamily_Class::ScanForDevices() {
  /*!
    @brief   Use the standardized 1-Wire microLAN search mechanism to discover all DS devices
    @details Each device has a unique 8-byte ROM address, which is stored at the end of program's
             EEPROM. Since each Atmel chip has a different amount of memory, and the class
             constructor allows the user to specify a number of bytes to reserve at the the
             beginning of the EEPROM memory the maximum number of devices that can be processed by
             the class is variable. After each device is discovered the resolution is set to the
             maximum value and a conversion is initiated. This is done as soon as possible since a
             conversion at maximum resolution takes up to 750ms
    @return number of devices found
  */
  uint8_t tempTherm[8];
  _LastCommandWasConvert = false;
  reset_search();  // Reset the search status
  ThermometersFound = 0;
  while (search(tempTherm))  // Use the 1-Wire "search" method
  {
    if (tempTherm[0] == DS18B20_FAMILY || tempTherm[0] == DS18S20_FAMILY ||
        tempTherm[0] == DS28EA00_FAMILY || tempTherm[0] == DS1822_FAMILY ||
        tempTherm[0] == DS1825_FAMILY) {
      EEPROM.put(E2END - ((ThermometersFound + 1) * 8),
                 tempTherm);                       // Write thermometer data to EEPROM
      SetDeviceResolution(ThermometersFound, 12);  // Set to maximum resolution
      if (ThermometersFound <= _MaxThermometers) ThermometersFound++;  // Increment if we have room
    }                                // of if-then we have a DS18x20 family device
  }                                  // of while there are still new devices on the 1-Wire bus
  reset();                           // Reset the 1-Wire bus
  write_byte(DS_SKIP_ROM);           // Send Skip ROM code
  write_byte(DS_READ_POWER_SUPPLY);  // Send command to read power supply
  Parasitic = !read_bit();           // Read the power status from bus
  DeviceStartConvert();              // Start conversion for all devices
  return (ThermometersFound);        // return number of devices detected
}  // of method ScanForDevices
boolean DSFamily_Class::Read1WireScratchpad(const uint8_t deviceNumber, uint8_t buffer[9]) {
  /*!
    @brief     read the scratchpad contents from a given DS device
    @param[in] deviceNumber 1-Wire device number
    @param[in] buffer 8-byte scratchpad contents from device
    @return    "true" if successful otherwise "false"
  */
  _LastCommandWasConvert = false;            // Set switch to false
  bool    CRCStatus      = false;            // default to a bad reading
  uint8_t ErrorCounter   = 0;                // Count number of bad readings
  while (!CRCStatus && ErrorCounter++ < 10)  // Loop until good read or overflow
  {
    SelectDevice(deviceNumber);      // Reset the 1-wire, address device
    write_byte(DS_READ_SCRATCHPAD);  // Request device to send Scratchpad contents
    for (uint8_t i = 0; i < 9; i++) {
      buffer[i] = read_byte();
    }                                          // for-next read each scratchpad byte
    CRCStatus = crc8(buffer, 8) == buffer[8];  // Check to see if result is valid
  }                    // of loop until good read or number of errors exceeded
  return (CRCStatus);  // Return false if bad CRC checksum
}  // of method Read1WireScratchpad()
int16_t DSFamily_Class::ReadDeviceTemp(const uint8_t deviceNumber, const bool raw) {
  /*!
    @brief   return the current temperature value for a given device number
    @details All devices except the DS18S20 return raw values in 0.0625°C increments, so the 0.5°C
             increments of the DS18S20 are converted to the same scale as the other devices. A check
             is done to see if there are still conversion(s) being done and a delay is made until
             any conversions have time to complete. We only store the value for conversion start
             time, so the delay might be for another devices and might not be necessary, but the
             alternative is to store the conversion times for each device which would potentially
             consume a lot  of available memory
   @param[in] deviceNumber 1-Wire device number
   @param[in] raw (Optional, default "false") If set to "true" then the raw reading is returned,
             otherwise the compensated calibrated value is returned
   @return Temperature reading in device units
  */
  uint8_t dsBuffer[9];
  int16_t temperature = DS_BAD_TEMPERATURE;  // Default return is error value
  if (Parasitic || !_LastCommandWasConvert)  // Wait a fixed time in parasite or
  {
    if ((_ConvStartTime + ConversionMillis) > millis()) {
      delay(millis() - (_ConvStartTime + ConversionMillis));  // Wait for conversion to finish
    }                                                         // if-then-else conversion is active
  } else {
    if (_LastCommandWasConvert) {
      while (read_bit() == 0)
        ;  // Loop until bit goes high after conversion has finished
    }      // if-then last command was conversion
  }        // if-then-else parasitic or conversion active
  if (deviceNumber < ThermometersFound &&
      Read1WireScratchpad(deviceNumber, dsBuffer))  // Successful read from device
  {
    if (ROM_NO[0] == DS1822_FAMILY)  // If DS1822 then temp is different
    {
      temperature = ((dsBuffer[1] << 8) | dsBuffer[0]) << 3;    // get the raw reading and apply
      temperature = (temperature & 0xFFF0) + 12 - dsBuffer[6];  // value from "count remain" byte
    } else {
      temperature = (dsBuffer[1] << 8) | dsBuffer[0];  // Results come in 2s complement
    }                                                  // if-then-else a DS1822
    if ((dsBuffer[2] ^ dsBuffer[3]) == 0xFF &&
        !raw)  // Apply any calibration offset if raw is not true
    {
      temperature += (int8_t)dsBuffer[2];
    }  // if-then calibrated device and raw is not true
  }    // of if-then the read was successful
  return (temperature);
}  // of method ReadDeviceTemp()
void DSFamily_Class::DeviceStartConvert(const uint8_t deviceNumber, const bool WaitSwitch) {
  /*!
    @brief     Start the sampling and conversion on a device
    @details   At maximum resolution this conversion can take 750ms. If the optional deviceNumber
               is not specified then all device conversions are started at the same time. If the
               optional WaitSwitch parameter is set to "true" then call doesn't return until the
               conversion has completed
    @param[in] deviceNumber 1-Wire device number
    @param[in] WaitSwitch (Optional, default "false"). When "true" the call doesn't return until
               measurements have completed
  */
  ParasiticWait();                // Wait for conversion to complete if necessary
  if (deviceNumber == UINT8_MAX)  // if default for all devices
  {
    reset();                  // Reset 1-wire network
    write_byte(DS_SKIP_ROM);  // Tell all devices to listen
  } else {
    SelectDevice(deviceNumber);
  }                                   // if-then-else all devices or just one
  write_byte(DS_START_CONVERT);       // Initiate temperature conversion
  _ConvStartTime         = millis();  // Store start time of conversion
  _LastCommandWasConvert = true;      // Set switch to true
  if (WaitSwitch)                     // Don't return until finished
  {
    if (Parasitic) {
      ParasiticWait();  // wait a fixed period when in parasite mode
    } else {
      while (read_bit() == 0)
        ;  // Wait until read bit goes high when conversion finished
    }      // if-then-else Parasitic
  }        // if-then Waitswitch set
}  // of method DeviceStartConvert
void DSFamily_Class::Calibrate(const uint8_t iterations, const int16_t CalTemp) {
  /*!
   @brief     Calibrate all thermometers
   @details   Each DS has a persistent 2 user bytes which can be both read and updated.  These can
              be used for triggering alarms, but in this application we are going to set these to
              provide a calibration offset value so that all DS devices on the 1-wire can be set to
              accurately show the same temperature. This only works when all of the thermometers are
              at the same temperature, which can be done by various methods. What temperature is
              used for the calibration is unimportant, although a calibration at typical operating
              temperatures makes the most sense.\n\n The calibration method used here is quite
              simple and straightforward. First, all devices are measured for a period of time
              defined in MEASUREMENT_ITERATIONS. The average of all readings is computed and that
              value is assumed to be the correct and accurate temperature reading. Each
              thermometer's offset to this standard value is computed and is written to the two user
              bytes.\n\n In order to ensure that the correct values are used at runtime, the values
              are written to the two user bytes so that XOR'ing them together always results in a
              value of 0xFF.\n\n The CalTemp optional parameter specifies the calibration
              temperature that all thermometers are to be adjusted to. This temperature is a signed
              integer in hectodegrees Celsius, so a temperature of "28.12" would be "2812".
   @param[in] iterations Number of calibration iterations to perform. The higher the value the more
              accurate the calibration is.
   @param[in] CalTemp (Optional) When specified, the given temperature is assumed to be the correct
              one and all thermometers are calibrated to that temperature, otherwise the average
              reading is used as the calibrated temperature completed
  */
  const uint8_t DS_MAX_THERMOMETERS{32};            // Specify a maximum number here
  int64_t       stats1[DS_MAX_THERMOMETERS] = {0};  // store statistics per device
  int64_t       tempSum{0};                         // Stores interim values
  int8_t        offset{0};                          // Stores the computed offset value
  uint8_t       ThermometersUsed =
      min(DS_MAX_THERMOMETERS, ThermometersFound);  // Use the lower of the 2 values
  _LastCommandWasConvert = false;
  for (uint8_t i = 0; i < iterations; i++)  // Loop to get a good sampling
  {
    for (uint8_t x = 0; x < ThermometersUsed; x++)  // process each thermometer
    {
      stats1[x] += ReadDeviceTemp(x, true);  // read raw temperature, no offset
    }                                        // of for each thermometer loop
    DeviceStartConvert();                    // Start conversion on all devices
    delay(ConversionMillis);                 // Wait to complete measurements
  }                                          // of for loop
  for (uint8_t i = 0; i < ThermometersUsed; i++) {
    tempSum += stats1[i];  // Add value to standard dev comps
  }                        // for-next each thermometer
  if (CalTemp == INT16_MAX) {
    tempSum = (CalTemp * 10) / 625;  // Use specific target temperature
  } else {
    tempSum = tempSum / iterations / ThermometersUsed;  // Use computed average temperature
  }  // if-then-else use specified calibration temperature
  for (uint8_t i = 0; i < ThermometersUsed; i++)  // Loop for every thermometer found
  {
    offset = tempSum - (stats1[i] / iterations);  // Compute offset value from mean
    SetDeviceCalibration(i, offset);              // Set the new user bytes 1&2
  }                                               // of for-next all thermometers
}  // of method CalibrateDS18B20
void DSFamily_Class::SetDeviceCalibration(const uint8_t deviceNumber, const int8_t offset) {
  /*!
    @brief     Set the user bytes 1 and 2 to the calibration computed
    @details   The calibration value is written to register 1 and register is written as the XOR'd
               value of the the first register. This is done to check for an actual calibration
               value rather than some other value that the user might have written to the device
    @param[in] deviceNumber 1-Wire device number
    @param[in] offset Calibration value to set
  */
  uint8_t dsBuffer[9];
  _LastCommandWasConvert = false;
  Read1WireScratchpad(deviceNumber, dsBuffer);  // Read from the device scratchpad
  SelectDevice(deviceNumber);                   // Reset 1-wire, address device
  write_byte(DS_WRITE_SCRATCHPAD);              // Write scratchpad, send 3 bytes
  write_byte(offset);                           // Write user Byte 1
  write_byte(offset ^ 0xFF);                    // Write the XOR'd value user Byte 1
  write_byte(dsBuffer[DS_CONFIG_BYTE]);         // Set configuration register back
  write_byte(DS_COPY_SCRATCHPAD);               // Copy scratchpad values to NV memory
  delay(DS_MAX_NV_CYCLE_TIME);                  // Give the DS18x20 time to process
}  // of method SetDeviceCalibration()
int8_t DSFamily_Class::GetDeviceCalibration(const uint8_t deviceNumber) {
  /*!
    @brief     Return the calibration setting from the device
    @details   If the calibration register 1 is equal to the XOR value of register 2 then it is
               assumed that a valid calibration has been set and that is returned
    @param[in] deviceNumber 1-Wire device number
    @return    calibration offset. If the calibration offset is invalid then INT8_MIN is returned
  */
  int8_t  offset = INT8_MIN;                    // Default to an invalid value
  uint8_t dsBuffer[9];                          // Temporary scratchpad buffer
  _LastCommandWasConvert = false;               // Set switch to false
  Read1WireScratchpad(deviceNumber, dsBuffer);  // Read from the device scratchpad
  if ((dsBuffer[2] ^ dsBuffer[3]) == 0xFF) {
    offset = (int8_t)dsBuffer[2];
  }  // if-then a valid calibration
  return (offset);
}  // of method GetDeviceCalibration()
void DSFamily_Class::SelectDevice(const uint8_t deviceNumber) {
  /*!
   @brief     reset the 1-Wire microLAN and select the device number specified
   @param[in] deviceNumber 1-Wire device number
  */
  ParasiticWait();  // Wait for conversion if necessary
  for (uint8_t i = 0; i < 8; i++) {
    ROM_NO[i] = EEPROM.read(i + E2END - ((deviceNumber + 1) * 8));  // Read the EEPROM byte
  }                // for-next each byte of the buffer
  reset();         // Reset 1-wire communications
  select(ROM_NO);  // Select only current device
}  // of method SelectDevice()
void DSFamily_Class::GetDeviceROM(const uint8_t deviceNumber, uint8_t ROMBuffer[8]) {
  /*!
    @brief      return the 8-byte ROM address buffer
    @param[in]  deviceNumber 1-Wire device number
    @param[out] ROMBuffer 8-byte ROM address buffer of device
  */
  _LastCommandWasConvert = false;
  for (uint8_t i = 0; i < 8; i++) {
    ROMBuffer[i] = EEPROM.read(i + E2END - ((deviceNumber + 1) * 8));
  }  // for-next each byte in the buffer
}  // of method GetDeviceROM()
int16_t DSFamily_Class::MinTemperature(uint8_t skipDeviceNumber) {
  /*!
    @brief      reads all current device temperatures and returns the lowest value
    @details    If the optional skipDeviceNumber is specified then that device number is skipped;
                this is used when one of the thermometers is out-of-band - i.e. if it is attached to
                an evaporator plate and reads much lower than the others.
    @param[in]  skipDeviceNumber Device number to skip, defaults to no skipped device
    @return Minimum temperature
  */
  int16_t deviceTemp;
  int16_t minimumTemp = INT16_MAX;  // Starts at highest possible value
  for (uint8_t i = 0; i < ThermometersFound; i++) {
    deviceTemp = ReadDeviceTemp(i);  // retrieve the temperature reading
    if (i != skipDeviceNumber && deviceTemp < minimumTemp)
      minimumTemp = deviceTemp;  // set if value is less than minimum and not the skip device
  }                              // of for-next each thermometer
  return (minimumTemp);
}  // of method MinTemperature
int16_t DSFamily_Class::MaxTemperature(uint8_t skipDeviceNumber) {
  /*!
    @brief      reads all current device temperatures and returns the highest value
    @details    If the optional skipDeviceNumber is specified then that device number is skipped;
                this is used when one of the thermometers is out-of-band - i.e. if it is attached to
                a heat source plate and reads much higher than the others.
    @param[in]  skipDeviceNumber Device number to skip, defaults to no skipped device
    @return     Maximum temperature
  */
  int16_t deviceTemp;
  int16_t maximumTemp = INT16_MIN;  // Starts at lowest possible value
  for (uint8_t i = 0; i < ThermometersFound; i++) {
    deviceTemp = ReadDeviceTemp(i);  // retrieve the temperature reading
    if (i != skipDeviceNumber && deviceTemp > maximumTemp) {
      maximumTemp = deviceTemp;
    }  // if-then not skipped device and greater than current max
  }    // of for-next each thermometer
  return (maximumTemp);
}  // of method MaxTemperature
int16_t DSFamily_Class::AvgTemperature(const uint8_t skipDeviceNumber) {
  /*!
    @brief      reads all current device temperatures and returns the average value
    @details    If the optional skipDeviceNumber is specified then that device number is skipped;
                this is used when one of the thermometers is out-of-band - i.e. if it is attached to
                a heat source or evaporator plate and reads much lower or higher than the others.
    @param[in]  skipDeviceNumber Device number to skip, defaults to no skipped device
    @return     Average temperature
  */
  int16_t AverageTemp = 0;
  for (uint8_t i = 0; i < ThermometersFound; i++) {
    if (i != skipDeviceNumber) AverageTemp += ReadDeviceTemp(i);  // add temperature to the sum
  }                                                               // of for-next each thermometer
  if (skipDeviceNumber == UINT8_MAX) {
    AverageTemp = AverageTemp / ThermometersFound;  // Divide by number of thermometers
  } else {
    AverageTemp =
        AverageTemp / (ThermometersFound - 1);  // Divide by number of thermometers minus 11
  }                                             // if-then a device was skipped
  return (AverageTemp);
}  // of method AvgTemperature
void DSFamily_Class::SetDeviceResolution(const uint8_t deviceNumber, uint8_t resolution) {
  /*!
    @brief      set the resolution of the DS devices to 9, 10, 11 or 12 bits
    @details    Lower resolution results in a faster conversion time. The global ConversionMillis is
                set on the assumption that all devices are set to the same resolution\n\n
                   Value Resolution Conversion\n
                   ===== ========== ==========\n
                       9  0.5°C      93.75ms\n
                      10  0.25°C    187.5 ms\n
                      11  0.125°C   375   ms\n
                      12  0.0625°C  750   ms
   @param[in] deviceNumber 1-Wire device number
   @param[in] resolution Device resolution in bits: 9, 10, 11 or 12
 */
  uint8_t dsBuffer[9];
  _LastCommandWasConvert = false;                          // Set switch to false
  if (resolution < 9 || resolution > 12) resolution = 12;  // Default to full resolution
  switch (resolution) {
    case 12:
      ConversionMillis = DS_12b_CONVERSION_TIME;
      break;
    case 11:
      ConversionMillis = DS_11b_CONVERSION_TIME;
      break;
    case 10:
      ConversionMillis = DS_10b_CONVERSION_TIME;
      break;
    case 9:
      ConversionMillis = DS_9b_CONVERSION_TIME;
      break;
    default:
      ConversionMillis = DS_12b_CONVERSION_TIME;
  }                                             // of switch statement for precision
  resolution = (resolution - 9) << 5;           // Shift resolution bits over
  Read1WireScratchpad(deviceNumber, dsBuffer);  // Read device scratchpad
  SelectDevice(deviceNumber);                   // Reset 1-wire, address device
  write_byte(DS_WRITE_SCRATCHPAD);              // Write scratchpad, send 3 bytes
  write_byte(dsBuffer[DS_USER_BYTE_1]);         // Restore the old user byte 1
  write_byte(dsBuffer[DS_USER_BYTE_2]);         // Restore the old user byte 2
  write_byte(resolution);                       // Set configuration register
  write_byte(DS_COPY_SCRATCHPAD);               // Copy scratchpad to NV memory
  delay(DS_MAX_NV_CYCLE_TIME);                  // Give the DS18x20 time to process
}  // of method SetDeviceResolution
uint8_t DSFamily_Class::GetDeviceResolution(const uint8_t deviceNumber) {
  /*!
    @brief      Get the device resolution
    @param[in]  deviceNumber 1-Wire device number
    @return number of bits resolution (9, 10, 11 or 12)
  */
  uint8_t resolution, dsBuffer[9];
  _LastCommandWasConvert = false;                    // Set switch to false
  Read1WireScratchpad(deviceNumber, dsBuffer);       // Read from the device scratchpad
  resolution = (dsBuffer[DS_CONFIG_BYTE] >> 5) + 9;  // get bits 6&7 from the config byte
  return (resolution);
}  // of method GetDeviceResolution()
float DSFamily_Class::StdDevTemperature(const uint8_t skipDeviceNumber) {
  /*!
    @brief      reads all current device temperatures and returns the standard deviation
    @details    If the optional skipDeviceNumber is specified then that device number is skipped;
                this is used when one of the thermometers is out-of-band and should be ignored
    @param[in]  skipDeviceNumber Device number to skip, defaults to no skipped device
    @return Floating point standard deviation
  */
  float   StdDev      = 0;
  int16_t AverageTemp = AvgTemperature(skipDeviceNumber);  // Compute the average
  for (uint8_t i = 0; i < ThermometersFound; i++) {
    if (i != skipDeviceNumber)
      StdDev += sq(AverageTemp - ReadDeviceTemp(i));  // add squared variance delta to sum
  }                                                   // of for-next each thermometer
  if (skipDeviceNumber == UINT8_MAX) {
    StdDev = StdDev / ThermometersFound;
  } else {
    StdDev = StdDev / (ThermometersFound - 1);
  }
  StdDev = sqrt(StdDev);  // compute the square root
  return (StdDev);
}  // of method StdDevTemperature
void DSFamily_Class::reset_search() {
  /*!
    @brief      Reset the 1-Wire search
    @details    Used to start a search again at the beginning of the 1-Wire microLAN
  */
  LastDiscrepancy       = {0};
  LastDeviceFlag        = {false};
  LastFamilyDiscrepancy = {0};
  for (int i = 7;; i--) {
    ROM_NO[i] = 0;
    if (i == 0) break;
  }  // of for-next each ROM byte
}  // of method reset_search
uint8_t DSFamily_Class::reset(void) {
  /*!
    @brief      Perform the 1-wire reset function
    @details    Wait up to 250uS for the bus to come high, if it doesn't then it is broken or
                shorted and we return a 0; Returns 1 if a device asserted a presence pulse, 0
                otherwise
  */
  IO_REG_TYPE               mask       = bitmask;  // Set the bitmask
  volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;  // point to the base register
  uint8_t                   r;                     // Return value
  uint8_t                   retries = 125;         // Maximum number of retries
  noInterrupts();                                  // Disable interrupts for now
  DIRECT_MODE_INPUT(reg, mask);                    // Send to register
  interrupts();                                    // Enable interrupts again
  do                                               // wait until the wire is high...
  {
    if (--retries == 0) return 0;     // wire is high, so return
    delayMicroseconds(2);             // Wait a bit
  } while (!DIRECT_READ(reg, mask));  // wait until the wire is high...
  noInterrupts();                     // Disable interrupts for now
  DIRECT_WRITE_LOW(reg, mask);
  DIRECT_MODE_OUTPUT(reg, mask);  // drive output low
  interrupts();                   // Enable interrupts again
  delayMicroseconds(480);         // Wait 480 microseconds
  noInterrupts();                 // Disable interrupts for now
  DIRECT_MODE_INPUT(reg, mask);   // allow it to float
  delayMicroseconds(70);          // Wait 70 microseconds
  r = !DIRECT_READ(reg, mask);    // Read the status
  interrupts();                   // Enable interrupts again
  delayMicroseconds(410);         // Wait again
  return r;                       // return the result
}  // of method reset()
void DSFamily_Class::write_bit(uint8_t v) {
  /*!
    @brief      Write a bit to 1-wire
    @details    Port & bit is used to cut lookup time and provide more certain timing
    @param[in]  v Only the LSB is used as the bit to write to 1-Wire
  */
  IO_REG_TYPE               mask       = bitmask;  // Register mask
  volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;  // Register
  if (v & 1)                                       // If writing a "1"
  {
    noInterrupts();  // Disable interrupts for now
    DIRECT_WRITE_LOW(reg, mask);
    DIRECT_MODE_OUTPUT(reg, mask);  // drive output low
    delayMicroseconds(10);          // Wait
    DIRECT_WRITE_HIGH(reg, mask);   // drive output high
    interrupts();                   // Enable interrupts again
    delayMicroseconds(55);          // Wait
  } else {
    noInterrupts();  // Disable interrupts for now
    DIRECT_WRITE_LOW(reg, mask);
    DIRECT_MODE_OUTPUT(reg, mask);  // drive output low
    delayMicroseconds(65);          // Wait
    DIRECT_WRITE_HIGH(reg, mask);   // drive output high
    interrupts();                   // Enable interrupts again
    delayMicroseconds(5);           // Wait
  }                                 // of if-then we have a "true" to write
}  // of method write_bit()
uint8_t DSFamily_Class::read_bit(void) {
  /*!
    @brief      Read a bit from 1-wire
    @details    Port & bit is used to cut lookup time and provide more certain timing
    @return     single bit where only the LSB is used as the bit that was read
  */
  IO_REG_TYPE               mask       = bitmask;  // Register mask
  volatile IO_REG_TYPE *reg IO_REG_ASM = baseReg;  // Register
  uint8_t                   r;                     // Return bit
  noInterrupts();                                  // Disable interrupts for now
  DIRECT_MODE_OUTPUT(reg, mask);
  DIRECT_WRITE_LOW(reg, mask);
  delayMicroseconds(3);          // Wait
  DIRECT_MODE_INPUT(reg, mask);  // let pin float, pull up will raise it up again
  delayMicroseconds(10);         // Wait
  r = DIRECT_READ(reg, mask);
  interrupts();           // Enable interrupts again
  delayMicroseconds(53);  // Wait
  return r;               // Return result
}  // of method read_bit()
void DSFamily_Class::write_byte(uint8_t v, uint8_t power) {
  /*!
    @brief      Write a byte to 1-wire
    @details    The writing code uses the active drivers to raise the pin high, if you need power
                after the write (e.g. DS18S20 in parasite power mode) then set 'power' to 1,
                otherwise the pin will go tri-state at the end of the write to avoid heating in a
                short or other mishap
    @param[in]  v Byte to write
    @param[in]  power Boolean switch set to "true" when powered and "false" in parasite mode
  */
  uint8_t bitMask;
  for (bitMask = 0x01; bitMask; bitMask <<= 1) {
    write_bit((bitMask & v) ? 1 : 0);  // Write bits until empty
  }                                    // for-next each bit
  if (!power) {
    noInterrupts();
    DIRECT_MODE_INPUT(baseReg, bitmask);
    DIRECT_WRITE_LOW(baseReg, bitmask);
    interrupts();
  }  // of if-then we have parasite mode
}  // of method write_byte()
uint8_t DSFamily_Class::read_byte() {
  /*!
    @brief      Read a byte from 1-wire
    @return     byte read from 1-Wire bus
  */
  uint8_t bitMask;
  uint8_t r = 0;
  for (bitMask = 0x01; bitMask; bitMask <<= 1) {
    if (read_bit()) r |= bitMask;  // For each bit in the byte read bit into correct position
  }                                // of for-next each bit in the byte
  return r;
}  // of method read_byte()
void DSFamily_Class::select(const uint8_t rom[8]) {
  /*!
    @brief      Do a 1-Wire ROM Select
    @param[in]  8-Byte ROM buffer
  */
  write_byte(DS_SELECT_ROM);  // Send Select ROM code
  for (uint8_t i = 0; i < 8; i++) {
    write_byte(rom[i]);  // Send the ROM address bytes
  }                      // for-next each byte in ROM buffer
}  // of method select()
uint8_t DSFamily_Class::search(uint8_t *newAddr) {
  /*!
    @brief      Search the 1-Wire microLAN using the Dallas Semiconductor search algorith and code
    @details    Perform a search. If this function returns a '1' then it has enumerated the next
                device and you may retrieve the ROM from the OneWire::address variable. If there
                are no devices, no further devices, or something horrible happens in the middle of
                the enumeration then a 0 is returned.  If a new device is found then its address is
                copied to newAddr.  Use DSFamily_Class::reset_search() to start over.
    @param[in]  newAddr  8-Byte ROM Buffer
    @return     TRUE - device found, ROM number in ROM_NO buffer, FALSE - device not found, end of
                search
  */
  uint8_t       id_bit_number, last_zero, rom_byte_number, search_result, id_bit, cmp_id_bit;
  unsigned char rom_byte_mask, search_direction;
  id_bit_number   = 1;  // initialize values for searching
  last_zero       = 0;
  rom_byte_number = 0;
  rom_byte_mask   = 1;
  search_result   = 0;
  if (!LastDeviceFlag)  // if last call was not the last one
  {
    if (!reset()) {
      LastDiscrepancy       = 0;
      LastDeviceFlag        = false;
      LastFamilyDiscrepancy = 0;
      return false;
    }                       // of if-then we have a reset
    write_byte(DS_SEARCH);  // issue the search command
    do                      // loop to do the search
    {
      id_bit     = read_bit();  // read a bit
      cmp_id_bit = read_bit();  // and then the complement
      if ((id_bit == 1) && (cmp_id_bit == 1))
        break;  // check for no devices on 1-wire
      else {
        if (id_bit != cmp_id_bit)
          search_direction = id_bit;  // all devices coupled have 0 or 1
        else {
          // bit write value for search if this discrepancy is before the Last Discrepancy on a
          // previous next then pick the same as last time
          if (id_bit_number < LastDiscrepancy) {
            search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
          } else {
            // if equal to last pick then 1, if not then pick 0
            search_direction = (id_bit_number == LastDiscrepancy);
          }                           // if-then-else
          if (search_direction == 0)  // if 0 was picked then record its position in LastZero
          {
            last_zero = id_bit_number;
            if (last_zero < 9) LastFamilyDiscrepancy = last_zero;
          }                         // of if-then search direction is 0
        }                           // of if-then-else  all devices have 0 or 1
        if (search_direction == 1)  // set or clear the bit in the ROM
        {
          ROM_NO[rom_byte_number] |= rom_byte_mask;  // byte rom_byte_number with mask
        } else {
          ROM_NO[rom_byte_number] &= ~rom_byte_mask;
        }
        write_bit(search_direction);  // serial number search direction
        id_bit_number++;              // increment the byte counter
        rom_byte_mask <<= 1;          // id_bit_number & shift rom_byte_mask
        if (rom_byte_mask ==
            0)  // if the mask is 0 then go to new  SerialNum byte rom_byte_number and reset mask
        {
          rom_byte_number++;
          rom_byte_mask = 1;
        }                         // of if-then mask is 0
      }                           // of if-then-else device still found
    }                             // of loop until all devices found
    while (rom_byte_number < 8);  // loop until through ROM bytes 0-7

    if (!(id_bit_number < 65))  // if the search was successful then search successful so set
    {
      LastDiscrepancy = last_zero;
      if (LastDiscrepancy == 0) LastDeviceFlag = true;  // check for last device
      search_result = true;
    }                                // of if-then search was successful
  }                                  // of if-then there are still devices to be found
  if (!search_result || !ROM_NO[0])  // if the search was successful then
  {
    LastDiscrepancy       = 0;  // Set counters so next 'search' will be like a first
    LastDeviceFlag        = false;
    LastFamilyDiscrepancy = 0;
    search_result         = false;
  } // of if-then no device found
  for (int i = 0; i < 8; i++) 
    newAddr[i] = ROM_NO[i];  // Copy result buffer
  return search_result;
}  // of method search()
uint8_t DSFamily_Class::crc8(const uint8_t *addr, uint8_t len) {
  /*!
    @brief      Compute the 8 bit crc of the returned buffer
    @details    This method uses the iterative method, which is slower than the default 1-Wire
                table lookup but that uses 255 bytes of scant program memory.
    @param[in]  addr  Pointer to buffer
    @param[in]  len  Length of buffer
    @return     computed crc8
  */
  uint8_t crc = 0;
  while (len--) {
    uint8_t inbyte = *addr++;
    for (uint8_t i = 8; i; i--) {
      uint8_t mix = (crc ^ inbyte) & 0x01;
      crc >>= 1;
      if (mix) crc ^= 0x8C;
      inbyte >>= 1;
    }  // of for-next each byte in the ROM
  }    // of while data still be to computed
  return crc;
}  // of method crc8()

void DSFamily_Class::ParasiticWait() {
  /*!
    @brief      Wait when parasitically powered devices are converting
    @details    Any parasitically device needs to have a strong power pullup on the 1-Wire data line
                during conversion. This means that the whole 1-Wire microLAN is effectively blocked
                during the rather lengthy conversion time; since using the bus would cause the
                parasitically powered device to abort conversion. Therefore this function will wait
                until the last conversion request has had enough time to complete. The wait time
                might be unnecessary, but since we only track the last conversion start rather than
                track each device independently this is the best we can do.
  */
  if (Parasitic && ((_ConvStartTime + ConversionMillis) > millis())) {
    delay((_ConvStartTime + ConversionMillis) - millis());  // then delay until it is finished
  }  // of if-then we have a parasitic device on the 1-Wire
}  // of method ParasiticWait()
