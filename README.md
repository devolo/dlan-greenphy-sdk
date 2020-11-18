# devolo dLAN® Green PHY Module SDK

This project contains the SDK for the devolo [dLAN® Green PHY module](https://www.devolo.de/dlan-green-phy-module).

It is ideal for installation in all electrical devices of a smart home. From the heating system to the refrigerator - Green PHY enables seamless 
integration into the networked home. The open standard makes smart energy applications, sensor solutions and automation systems efficient to set up and 
use. Another advantage: The dLAN® Green PHY module is particularly energy-saving and - compared to conventional HomePlug solutions - has reduced energy 
consumption.

## QCA7000 Firmware
The module contains a QCA7000 GreenPHY processor and a LPC1758 MCU. This project mainly covers the SDK for the MCU, while the latest firmware (version 1.2.5) for the GreenPHY chip itself can be found in the directory QCA7000_GreenPHY_Firmware.

The following options are available within the QCA7000 devolo firmware package:
 *  iot-generic    IoT generic, optimized for performance: 50561 off (SLAC off)
 *  iot-conform    IoT over mains, optimized for conformity: 50561 on (SLAC off)
 *  emob-charger   e-mobility use as charging station: SLAC in EVSE mode (50561 off)
 *  emob-vehicle   e-mobility use as vehicle: SLAC in PEV mode (50561 off)

## LPC1758 Firmware
The SDK is based on [LPCOpen 
v2.10](https://www.nxp.com/products/developer-resources/software-development-tools/developer-resources-/lpcopen-libraries-and-examples/lpcopen-software-development-platform-lpc17xx:LPCOPEN-SOFTWARE-FOR-LPC17XX) 
and [FreeRTOS v10.4.1](http://www.freertos.org/).


![devolo dLAN® Green PHY 
module](https://www.codico.com/media/catalog/product/cache/c59ee43e27a3fd6035149ee08efef60b/i/m/image_3_4.jpg)

*The dLAN® Green PHY module*

## Features
* HTTP server with webinterface
* TCP/IP stack with many protocols ready to use, e.g. DNS, DHCP, ICMP
* Green PHY and fast ethernet controller
* Many interfaces, e.g. SPI, GPIO, I²C, UART, USB and CAN
* Compatible to [microBUS™ socket](https://www.mikroe.com/mikrobus/) and MikroElektronika [Click Boards](https://shop.mikroe.com/click)

## Requirements
* [dLAN® Green PHY module](https://www.devolo.de/dlan-green-phy-module) with a development board, e.g. the [dLAN® 
Green PHY eval board II](https://www.devolo.de/dlan-green-phy-eval-board-ii), or your own design
* [MCUXpresso IDE](https://www.nxp.com/products/developer-resources/run-time-software/mcuxpresso-software-and-tools/mcuxpresso-integrated-development-environment-ide-v10.0.2:MCUXpresso-IDE) *(tested for v11.1.0, not working with v11.2.1)*
* JTAG-Debugger, recommended [LPC-Link 
2](https://www.nxp.com/products/developer-resources/software-development-tools/developer-resources-/lpcopen-libraries-and-examples/lpc-link2:OM13054) 
with [ARM-JTAG-20-10 Adapter](https://www.olimex.com/Products/ARM/JTAG/ARM-JTAG-20-10/)
* Python3, [beautifulsoup4](https://pypi.python.org/pypi/beautifulsoup4), [css-html-js-minify](https://pypi.python.org/pypi/css-html-js-minify) *(to 
convert HTML files for the WebUI)*

## Usage
1. Download the repository, following the *Clone or download* button's instructions above.
2. Import the project into MCUXpresso:
   *  In MCUXpresso create a new workspace and open the *File->Import* dialogue.
   * Select *General/Existing Projects into Workspace* import wizard and click next.
   * If you downloaded the SDK as .zip, select it in the *Select archive file* input,
     if you got it by *git clone* use the *Select root directory* input. 
   * Leave all projects marked for import and click *Finish*.

3. Now you should be able to see the SDK's folders in the Project Explorer.
   Select the *FirmwareStandalone* project and click on the blue debug icon or use *Debug 'FirmwareStandalone' [Debug]* in the Quickstart Panel. 
4. Get the GreenPHY module's IP address from your local DHCP server and access it's WebUI in your Browser.

![greenphy-sdk-webui](https://user-images.githubusercontent.com/10745701/30339626-ecc350ca-97ef-11e7-96c5-5e3ad115d538.png)
*Example of the dLAN® Green PHY Module SDK's webinterface's status page*
