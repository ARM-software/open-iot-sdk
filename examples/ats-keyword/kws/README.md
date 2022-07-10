# KWS Example

The Keyword Spotting application demonstrates Machine Learning running in a [TrustedFirmware-M](https://www.trustedfirmware.org/projects/tf-m/) controlled secure/non-secure partition configuration utilizing the [Ethos-U55](https://www.arm.com/products/silicon-ip-cpu/ethos/ethos-u55) microNPU on the following Arm platforms:
* Corstone-300
* Corstone-310

The ML application executes within the non-secure side of the Armv8-M processor coordinating data flow to and from the [Ethos-U55](https://www.arm.com/products/silicon-ip-cpu/ethos/ethos-u55).

In this folder you will find all the source code needed that implements the Keyword spotting application.

## FVP Console output
When running the KWS application, you should see something similar to the output below.  Note, due to processing loads, audio file inputs,  the scoring numbers may be slightly different, the number of inferences may change or the actual words detected may change.

```sh
*** ML interface initialised
ML_HEARD_ON
INFO - For timestamp: 0.000000 (inference #: 0); label: on, score: 0.996094; threshold: 0.900000
INFO - For timestamp: 0.500000 (inference #: 1); label: on, score: 0.996094; threshold: 0.900000
INFO - For timestamp: 1.000000 (inference #: 2); label: on, score: 0.917969; threshold: 0.900000
ML_HEARD_OFF
INFO - For timestamp: 1.500000 (inference #: 3); label: off, score: 0.996094; threshold: 0.900000
ML UNKNOWN
INFO - For timestamp: 2.000000 (inference #: 4); label: <none>; threshold: 0.000000
INFO - For timestamp: 2.500000 (inference #: 5); label: <none>; threshold: 0.000000
ML_HEARD_GO
INFO - For timestamp: 3.000000 (inference #: 6); label: go, score: 0.984375; threshold: 0.900000
ML UNKNOWN
INFO - For timestamp: 3.500000 (inference #: 7); label: <none>; threshold: 0.000000
INFO - For timestamp: 4.000000 (inference #: 8); label: <none>; threshold: 0.000000
INFO - For timestamp: 4.500000 (inference #: 9); label: <none>; threshold: 0.000000
ML UNKNOWN
INFO - For timestamp: 5.000000 (inference #: 10); label: _silence_, score: 0.996094; threshold: 0.900000
INFO - For timestamp: 5.500000 (inference #: 11); label: _silence_, score: 0.996094; threshold: 0.900000
INFO - For timestamp: 6.000000 (inference #: 12); label: _silence_, score: 0.996094; threshold: 0.900000
^C
Stopping simulation...
```

## FVP Graphical output

The KWS example demonstrates machine learning running on the non-secure side of then Corstone FVP.

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

## Connection to AWS cloud

The system can be connected to the AWS cloud and broadcast the ML inference results
to the cloud in an MQTT topic or receive OTA update.

Details of the AWS configuration can be found in the main [README](../README.md)
