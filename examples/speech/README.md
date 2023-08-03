# Speech Recognition Example

The Speech recognition application demonstrates Machine Learning running in a [TrustedFirmware-M](https://www.trustedfirmware.org/projects/tf-m/) controlled secure/non-secure partition configuration utilizing the [Ethos-U55](https://www.arm.com/products/silicon-ip-cpu/ethos/ethos-u55) microNPU on the following Arm platforms:
* Corstone-300
* Corstone-310

The ML application executes within the non-secure side of the Armv8-M processor coordinating data flow to and from the [Ethos-U55](https://www.arm.com/products/silicon-ip-cpu/ethos/ethos-u55).

In this folder you will find all the source code needed that implements the Speech recognition application.

## Presentation

The speech example demonstrates machine learning running on the non-secure side on Corstone-300 or Corstone-310.

When a sentence is infered, it is printed on the terminal and sent to the configured cloud provider.

The sixth LED blinks at a regular interval to indicate that the system is alive and waits for input.

## Connection to commercial clouds

The system can be connected to the AWS IoT cloud and broadcast the ML inference results
to the cloud in an MQTT topic or receive OTA update.

The system can also be connected to the Azure IoT cloud and broadcast the ML inference result.
When using NetX Duo Azure IoT Middleware, it can also receive OTA update.

Details of the AWS and Azure configuration can be found in the main [README](../../README.md)

## ASR settings

Special DSP processing is applied for noise reduction to the audio in input before processing by tensorflow.
It is possible to remove that processing by removing the compile definition `ENABLE_DSP` from the `speech` target configuration.
