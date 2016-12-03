# DS-Family library
*Arduino* library for using any/all of the "DS" Family of Maxim Integrated 1-Wire thermometers.  This library is specifically geared towards installations with several devices, particularly where the number of
attached devices is not known at design time. Each 1-Wire device has a unique 8 Byte ROM code which is used to address the device, so keeping 16 device addresses in memory can use a significant amount of available
RAM. This library uses the available Atmel EEPROM memory to store the 8-Byte addresses and reference to the devices is done via an integer index rather than device number.

##Supported Devices
The following Maxim Integrated DS-Family 1-Wire thermometers are supported:

<table>
 <th>
  DS-Family<br>Device
 </th>
 <tr>
  <td>DS1822</td>
 </tr>
 <tr>
  <td>DS1825</td>
 </tr>
 <tr>
  <td>DS18B20</td>
 </tr>
 <tr>
  <td>DS1820</td>
 </tr>
 <tr>
  <td>DS18S20</td>
 </tr>
 <tr>
  <td>DS28EA00</td>
 </tr>
</table>

## Temperature measurements
The DS-Family of devices have either fixed or variable levels of precision, ranging from 9 to 12 bits. All of the devices return a signed 16-bit integer result. Each unit returned equates to 0.0625°C, for example a temperature value of "325" equates to 20.3125°C. To avoid floating point arithmetic on the Atmel processors, multiply by 625 and then divide by 100, giving an integer "2031" which would be the temperature Celsius times 100; or hectodegrees.  The DS1820/DS18S20 models normally return values in 0.5°C increments but the library modifies those results so that all thermometers report the same way; the values for the DS1820/DS18S20 are just less accurate at 9bits precision.

## Calibration
The library can enhance the accuracy of readings with multiple devices by using the 2 user bytes to store measurement offset values. The calibration process assumes that all the thermometers are at the same temperature (there are various methods of doing this, for instance putting all the thermometers in sealed baggie inside a sealed plastic container for a long period of time and then measuring) and computes the mean temperature of all devices and then applies a positive or negative offset to each thermometer in order to achieve a common reading. Standard deviation of measurements is significantly improved when using calibrated thermometers.

## 1-Wire Library
The Maxim DS-Family of thermometers use the 1-Wire microLAN protocol. There is an excellent library for 1-Wire, written by Paul Stoffregen and located at http://www.pjrc.com/teensy/td_libs_OneWire.html. 
There is also an informative page at http://playground.arduino.cc/Learning/OneWire describing how to use the library. As there are parts of the code that are unnecessary for this DS implementation and 
to make this library useable without having to download other libraries or components, the code from version 2.0 (extracted 2016-11-23), has been included.

Details are available at the [GitHub DSFamily Wiki](https://github.com/SV-Zanshin/DSFamily/wiki)


![Zanshin Logo](https://www.sv-zanshin.com/images/gif/zanshinkanjitiny.gif) <img src="https://www.sv-zanshin.com/images/gif/zanshintext.gif" width="75"/>
