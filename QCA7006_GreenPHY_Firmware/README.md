# devolo Green PHY module V2

This project contains the SDK only for the [devolo Green PHY module V2](https://business.devolo.de/green-phy-module-v2).

The devolo Green PHY module V2 differs from dLAN® Green PHY Module in that the MCU has been omitted.
The QCA7006 chip is a bridge between ethernet and HomePlugAV.
This makes the module even more energy-efficient and cost-effective than the previous one.

## QCA7006 Firmware
The module contains a QCA7006 GreenPHY processor. This directory contains the latest firmware (version 3.3.0) for the Green PHY chip.

The following options are available within the QCA7006 devolo firmware package:
 *  iot-generic    IoT generic, optimized for performance: 50561 off (SLAC off)
 *  iot-conform    IoT over mains, optimized for conformity: 50561 on (SLAC off)
 *  emob-charger   e-mobility use as charging station: SLAC in EVSE mode (50561 off)
 *  emob-vehicle   e-mobility use as vehicle: SLAC in PEV mode (50561 off)

![devolo Green PHY module V2](https://static.devolo.global/1050_0_0/Web-Content/DE/products/bs/green-phy-module/pictures/v2/devolo-Green-PHY-module-V2-4.png)

*The devolo Green PHY module V2*

## Features
* Green PHY and fast ethernet controller
* Interfaces: SPI, GPIO, ethernet

## Requirements
* [devolo Green PHY module V2](https://business.devolo.de/green-phy-module-v2) with a development board, e.g. the [dLAN® 
Green PHY eval board II](https://www.devolo.de/dlan-green-phy-eval-board-ii), or your own design
