# SD-Family library
*Arduino* library for using any/all of the "DS" Family of Maxim Integrated 1-Wire thermometers.  This library is specifically geared towards installations with several devices, particularly where the number of
attached devices is not known at design time. Each 1-Wire device has a unique 8 Byte ROM code which is used to address the device, so keeping 16 device addresses in memory can use a significant amount of available
RAM. This library uses the available Atmel EEPROM memory to store the 8-Byte addresses and reference to the devices is done via an integer index rather than device number.

##Supported Devices
The following Maxim Integrated DS-Family 1-Wire thermometers are supported:

DS1822
DS1825
DS18B20
DS18S20
DS28EA00

##1-Wire Library
The Maxim DS-Family of thermometers use the 1-Wire microLAN protocol. There is an excellent library for 1-Wire, written by Paul Stoffregen and located at http://www.pjrc.com/teensy/td_libs_OneWire.html. 
There is also an informative page at http://playground.arduino.cc/Learning/OneWire describing how to use the library. As there are parts of the code that are unnecessary for this DS implementation and 
to make this library useable without having to download other libraries or components, the code from version 2.0 (extracted 2016-11-23), has been included.

## Public methods
The DSFamily library has the following publicly accessible variables and functions:


###	Class instantiation. 
`DSFamily_Class DSFamily(OneWirePin [,ReserveROM]);`
The first parameter is the Arduino pin number on which the data line of the 1-Wire microLAN is attached.  The second parameter is optional and specifies how many bytes of EEPROM memory are to be reserved for other uses. The DSFamily class will use 8 Bytes of EEPROM per DS family device discovered, starting at the end of EEPROM and working towards the beginning. The reserved ROM bytes begin at address 0. The amount of EEPROM available for use is determined at compile-time as it depends upon which Arduino / Atmel processor is being used.
###	Variable “ThermometersFound” 
`uint8_t myThermometers = DSFamily.ThermometersFound;`.  This is set during class instantiation and should not be modified.
###	Variable “ConversionMillis” 
`uint16_t waitTime = DSFamily.ConversionMillis;`. This is set depending upon the precision setting as follows:
-			9 bits	=	94ms
-			10 bits	=	188ms
-			11 bits	=	375ms
-			12 bits	=	750ms
###	Variable “Parasitic”
`bool IsParasitic = DSFamily.Parasitic;`. Set to "true" if there is at least one parasitically powered DS-Family device, otherwise set to "false". This is set inside the class and should not be modified.
###	Function “ScanForDevices”
`DSFamily.ScanForDevices();`. Accepts no parameters and re-scans the current 1-Wire microLAN for DS Family devices. Any changes are reflected in the number of thermometers found and the values set in EEPROM.
###	Function “ReadDeviceTemp”
`int16_t temperature = DSFamily.ReadDeviceTemp(DeviceNumber [,true|false]);`. The DeviceNumber is the device index number starting at 0, and the optional second parameter returns the raw reading rather than the calibrated offset reading when set to “true”.  Note that the returned temperature is in device units. In order to get degrees Celsius this value must be multiplied by 0.0625.
###	Function “DeviceStartConvert”
`DSFamily.DeviceStartConvert([DeviceNumber])`. When called with no parameters, all of the DS-Family devices are triggered to start a conversion, when the parameter is specified only that device index number is addressed to start a conversion.
###	Function “Calibrate”
`DSFamily.Calibrate([iterations]);`. If no parameter is specified then the default conversion of 30 iterations is used, otherwise the parameter specifies how many iterations of temperature conversions are done to get a statistically relevant sampling.
The DS-Family of 1-Wire thermometers have 2 user bytes which are normally used to set temperature threshold alarms. When several thermometers are used it is possible to get greater accuracy by using these 2 bytes to store calibration offset information which is applied when reading temperatures. See the program code for a detailed description.
###	Function “GetDeviceCalibration”
`int8_t offset = DSFamily.GetDeviceCalibration(DeviceNumber);`. If the device specified is not calibrated, then the value of INT8_MIN is returned, otherwise the number of units (each denoting 0.0625C) of offset for that device is returned.
###	Function “SetDeviceCalibration”
`DSFamily.SetDeviceCalibration(DeviceNumber,Offset);`. The offset is a signed 8-bit number denoting how many device units (at 0.0625C each) of offset are to be applied. This is stored in user byte 1, and user byte 2 is set to the XOR’d value so that no inadvertent calibration readings can be made.
###	Functions “MinTemperature”, “MaxTemperature”, “AvgTemperature”, “StdDevTemperature”
All of these have the same functionality. If the optional parameter is specified, then that thermometer index number is ignored in the calculations; this is useful when one thermometer has a different reading/location from the others – for example if one is attached to the board, or attached somewhere much hotter or colder than the others. E.g. “int16_t avg = DSFamily.AvgTemperature();”
###	Function “SetDeviceResolution”
`DSFamily.SetDeviceResolution(11);`. Sets the device resolution to 9, 10, 11 or 12 bits. Any invalid values result in a 12 bit resolution setting. The resolution not only affects the accuracy but also the conversion times.
###	Function “GetDeviceResolution”
`uint8_t resolution = DSFamily.GetDeviceResolution(DeviceNumber);`. Returns the device’s resolution as 9, 10, 11 or 12 bits.
###	Function “GetDeviceROM”
`DSFamily.GetDeviceROM(DeviceNumber,ROMArray);`. where the ROMArray is defined as “uint8_t ROMArray[8]” and the 8 byte unique device address information for the specified index DeviceNumber is returned
###	Function “crc8”
`uint8_t crcvalue = DSFamily.crc8(ROMArray,sizeof(ROMArray);`. This uses the CRC generation algorithm as specified by Maxim Integrated and rather than using the fast array lookup mechanism which block 255 bytes of memory it uses the somewhat slower computational method.

![Zanshin Logo](https://www.sv-zanshin.com/images/gif/zanshincalligraphy.gif)
