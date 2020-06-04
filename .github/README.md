# DS-Family library<br>[![Build Status](https://travis-ci.org/SV-Zanshin/DSFamily.svg?branch=master)](https://travis-ci.org/SV-Zanshin/DSFamily) [![DOI](https://www.zenodo.org/badge/75320780.svg)](https://www.zenodo.org/badge/latestdoi/75320780) [![arduino-library-badge](https://www.ardu-badge.com/badge/DSFamily.svg?)](https://www.ardu-badge.com/DSFamily)  [![Doxygen](https://github.com/SV-Zanshin/BME680/blob/master/Images/Doxygen-complete.svg)](https://sv-zanshin.github.io/DSFamily/html/index.html) [![Wiki](https://github.com/SV-Zanshin/BME680/blob/master/Images/Documentation-wiki.svg)](https://github.com/SV-Zanshin/DSFamily/wiki)
<img src="https://github.com/SV-Zanshin/DSFamily/blob/master/Images/DS18B20.jpg" width="175" align="right"/> *Arduino* library for using any/all of the "DS" Family of Maxim Integrated 1-Wire thermometers.  This library is specifically geared towards installations with several devices, particularly where the number of
attached devices is not known at design time. Each 1-Wire device has a unique 8 Byte ROM code which is used to address the device, so keeping 16 device addresses in memory can use a significant amount of available
RAM. This library uses the available Atmel EEPROM memory to store the 8-Byte addresses and reference to the devices is done via an integer index rather than device number.

## Supported Arduino hardware
Due to the use of EEPROM to store the 1-Wire DS thermometer information at runtime, currently only the AVR-Family of processors are supported. Work is ongoing to support the ESP32 and ESP8266 plaforms which have EEPROM emulation. If enough interest is present, a solution for the other common platforms can be implemented using "normal" memory.

## Supported Maxim DS-Devices
The following Maxim Integrated DS-Family 1-Wire thermometers are supported:

<table>
 <th>DS-Family Device</th>
 <th>Datasheet</th>
 <tr>
  <td>DS1822</td>
  <td><a href="https://datasheets.maximintegrated.com/en/ds/DS1822.pdf">DS1822 Datasheet</a></td>
 </tr>
 <tr>
  <td>DS1825</td>
  <td><a href="https://datasheets.maximintegrated.com/en/ds/DS1825.pdf">DS1825 Datasheet</a></td>
 </tr>
 <tr>
  <td>DS18B20</td>
  <td><a href="http://datasheets.maximintegrated.com/en/ds/DS18B20.pdf">DS18B20 Datasheet</a></td>
 </tr>
 <tr>
  <td>DS1820</td>
  <td><a href="https://datasheets.maximintegrated.com/en/ds/DS18S20.pdf">DS1820 Datasheet</a></td>
 </tr>
 <tr>
  <td>DS18S20</td>
  <td><a href="https://datasheets.maximintegrated.com/en/ds/DS18S20.pdf">DS18S20 Datasheet</a></td>
 </tr>
 <tr>
  <td>DS28EA00</td>
  <td><a href="https://datasheets.maximintegrated.com/en/ds/DS28EA00.pdf">DS28EA00 Datasheet</a></td>
 </tr>
</table>

## Temperature measurements
The DS-Family of devices have either fixed or variable levels of precision, ranging from 9 to 12 bits. All of the devices return a signed 16-bit integer result. Each unit returned equates to 0.0625°C, for example a temperature value of "325" equates to 20.3125°C. To avoid floating point arithmetic on the Atmel processors, multiply by 625 and then divide by 100, giving an integer "2031" which would be the temperature Celsius times 100; or centidegrees.  The DS1820/DS18S20 models normally return values in 0.5°C increments but the library modifies those results so that all thermometers report the same way; the values for the DS1820/DS18S20 are just less accurate at 9bits precision.

## Calibration
The library can enhance the accuracy of readings with multiple devices by using the 2 user bytes to store measurement offset values. The calibration process assumes that all the thermometers are at the same temperature (there are various methods of doing this, for instance putting all the thermometers in sealed baggie inside a sealed plastic container for a long period of time and then measuring) and computes the mean temperature of all devices and then applies a positive or negative offset to each thermometer in order to achieve a common reading. Standard deviation of measurements is significantly improved when using calibrated thermometers.

## 1-Wire Library
The Maxim DS-Family of thermometers use the 1-Wire microLAN protocol. There is an excellent library for 1-Wire, written by Paul Stoffregen and located at http://www.pjrc.com/teensy/td_libs_OneWire.html. 
There is also an informative page at http://playground.arduino.cc/Learning/OneWire describing how to use the library. As there are parts of the code that are unnecessary for this DS implementation and 
to make this library useable without having to download other libraries or components, the code from version 2.0 (extracted 2016-11-23), has been included.

## Documentation
In addition to the [GitHub DSFamily Wiki](https://github.com/SV-Zanshin/DSFamily/wiki), the library and example programs utiliez Doxygen documentation, whose output can be found at [Doxygen Documentation](https://sv-zanshin.github.io/DSFamily/html/index.html)  

![Zanshin Logo](https://www.sv-zanshin.com/r/images/site/gif/zanshinkanjitiny.gif) <img src="https://www.sv-zanshin.com/r/images/site/gif/zanshintext.gif" width="75"/>
