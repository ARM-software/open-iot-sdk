# KWS Example

The Keyword Spotting application demonstrates Machine Learning running in a [TrustedFirmware-M](https://www.trustedfirmware.org/projects/tf-m/) controlled secure/non-secure partition configuration on Arm Corstone-300 series platforms utilizing the [Ethos-U55](https://www.arm.com/products/silicon-ip-cpu/ethos/ethos-u55) microNPU.  The ML application executes within the non-secure side of the Armv8-M processor coordinating data flow to and from the [Ethos-U55](https://www.arm.com/products/silicon-ip-cpu/ethos/ethos-u55).

In this folder you will find all the source code needed that implements the Keyword spotting application.

## Presentation

The KWS example demonstrates machine learning running on the non-secure side on Corstone-300.

Depending on the keyword recognized, LEDs of the system are turned ON or OFF:

- LED1:
  - _Yes_: on
  - _No_: off
- LED2:
  - _Go_: on
  - _Stop_: off
- LED3:
  - _Up_: on
  - _Down_: off
- LED4:
  - _Left_: on
  - _Right_: off
- LED5:
  - _On_: on
  - _Off_: off

The sixth LED blinks at a regular interval to indicate that the system is alive and waits for input.

## Connection to AWS cloud

The system can be connected to the AWS cloud and broadcast the ML inference results
to the cloud in an MQTT topic or receive OTA update.

Details of the AWS configuration can be found in the main [README](../README.md)
