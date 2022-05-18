# Overview

This repo contains Arm's first [IoT Total Solution](https://www.arm.com/solutions/iot/total-solutions-iot).  It provides a general-purpose compute use-case as well as a Machine-Learning workload use-case, the Keyword Spotting application, that leverages the DS-CNN model from the [Arm Model Zoo](https://github.com/ARM-software/ML-zoo).

The software supports multiple configurations of the Arm Corstone™-300 series subsystems, incorporating the Cortex-M55 processor and Arm [Ethos™-U55](https://www.arm.com/products/silicon-ip-cpu/ethos/ethos-u55) microNPU.  This total solution provides the complex, non differentiated secure platform software on behalf of the ecosystem, thus enabling you to focus on your next killer app.

The two applications available within this folder for you to evaluate are "keyword" and "blinky".

<br>

**TL;DR:**
Jump to the [Quick Start](#Quick-Start) instructions below to build and execute the applications with Arm Virtual Hardware.  Or if you want to setup your local system to build the software locally, **do-it-yourself** information is documented [here](../../README.md#things-to-know).

<br>

## Keyword Spotting application

The keyword spotting application runs the DS-CNN model on top of [FreeRTOS](https://freertos.org/). It detects keywords and informs the user of which keyword has been spotted. The audio data to process are injected at run time using the AVH [Virtual Streaming Interface](https://arm-software.github.io/AVH/main/simulation/html/group__arm__cmvp.html).

The application connects to [AWS IoT](https://docs.aws.amazon.com/iot/latest/developerguide/what-is-aws-iot.html) cloud to publish recognized keywords. AWS IoT cloud is also used for OTA firmware updates. These firmware updates are securely applied using [Trusted Firmware-M](https://tf-m-user-guide.trustedfirmware.org/). For specific Keyword Spotting application information, click [here](#Keyword-Spotting-Application-specific-information).

<br>

![Key word detection architecture overview](./resources/Keyword-detection-overview.png)

<br>

## Blinky application

The blinky application demonstrates blinking LEDs and is a great starting point when creating your own project and using Arm Virtual Hardware. This application is purposely simple to speed you on your way.  Remove or keep the blinking led, your choice, and add the source code that brings your project to life.

<br>

# Quick Start
Follow these simple steps to build and execute either the keyword spotting or blinky application within **Arm Virtual Hardware**.

* [Launch Arm Virtual Hardware system](../../TS_Instructions.md#Launch-Arm-Virtual-Hardware-Instance)
* [Build and execute](#Build-and-execute-the-application)
* [Setting up Cloud connectivity](../../TS_Instructions.md#Setting-up-AWS-connectivity)
* [Enabling OTA firmware update from the Cloud](../../TS_Instructions.md#OTA-firmware-update)
* [Terminating Arm Virtual Hardware](../../TS_Instructions.md#Terminate-Arm-Virtual-Hardware-Instance)

<br>

# Build and execute the application
Once you have performed the [Launch Arm Virtual Hardware system](../../TS_Instructions.md#Launch-Arm-Virtual-Hardware-Instance) step you will have an open console that you will use to build and execute the application.   If you are coming back to this step after some time and just need to re-open a console, refer to the [Console connection instructions](../../TS_Instructions.md#Connect-a-terminal-to-the-instance).

To update the application, a set of scripts is included to setup the environment,
build applications, run them and test them. These scripts are currently designed to be executed within the AVH AMI.

To build, run and launch a test of the blinky application, replace `kws` by `blinky`.

1. Clone the repository in the AMI:

    ```sh
    git clone https://github.com/ARM-software/open-iot-sdk.git
    ```

2. Depending on which application you would like to build, insert the name below. For these instructions 

    ```sh
    cd open-iot-sdk/examples/ats-keyword
    ```

3. Synchronize git submodules, setup ML and apply required patches:

    ```sh
    ./ats.sh bootstrap
    ```

4. Install additional python dependencies required to run tests and sign binaries:

    ```sh
    pip3 install click imgtool pytest cbor intelhex
    export PATH=$PATH:/home/ubuntu/.local/bin
    ```

5. Update cmake:

    ```sh
    sudo snap refresh cmake --channel=latest/stable
    ```

6. Build the kws application:

    ```sh
    ./ats.sh build kws
    ```

7. Run the kws application:

    ```sh
    ./ats.sh run kws
    ```

8. Launch the kws integration tests:

  * kws
    ```sh
    pytest -s kws/tests/test_ml.py
    ```

  * blinky
    ```sh
    pytest -s blinky/tests/test_blink.py
    ```


# Keyword Spotting application specific information

## Updating audio data input file

The audio data streamed into the Arm Virtual Hardware is read from the file `test.wav` located at the root of the repository. It can be replaced with another audio file with the following configuration:
- Format: PCM
- Audio channels: Mono
- Sample rate: 16 bits
- Sample rate: 16kHz

## Runtime details

The Key Word application demonstrates machine learning running on the non-secure side of the Armv8-M processor.

Depending on the keyword (shown in italics below) that is recognized, LEDs of the system are turned ON or OFF:

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

The system can be connected to the Cloud and broadcast the ML inference results
to the cloud in an MQTT topic or receive OTA update.

Details of the Cloud configuration can be found in the following [README](../../TS_Instructions.md).

<br>

# Blinky application specific information
There is none.  Its simple on purpose!

<br>

# Source code overview

- `bsp`: Arm Corstone™-300 subsystem platform code and AWS configurations.
- `lib`: Middleware used by IoT Total Solution.
  - `lib/mcuboot`: MCUboot bootloader.
  - `lib/tf-m`: Trusted Firmware M: implementing [Platform Security Architecture](https://www.arm.com/architecture/psa-certified) for Armv8-M.
  - `lib/mbedcrypto`: Mbed TLS and PSA cryptographic APIs.
  - `lib/ml-embedded-evaluation-kit`: Arm® ML embedded evaluation kitr Ethos NPU. It includes [TensorFlow](https://www.tensorflow.org/)
  - `lib/amazon_freertos`: AWS FreeRTOS distribution.
  - `libVHT`: Virtual streaming solution for Arm Virtual Hardware.
- `blinky`: Blinky application.
  - `blinky/main_ns.c`: Entry point of the blinky application
- `kws`: Keyword detection application.
  - `kws/source/main_ns.c`: Entry point of the kws application.
  - `kws/source/blinky_task.c`: Blinky/UX thread of the application.
  - `kws/source/ml_interface.c`: Interface between the virtual streaming solution and tensor flow.

<br>

# ML Model Replacement

All the ML models supported by the [ML Embedded Eval Kit](https://review.mlplatform.org/plugins/gitiles/ml/ethos-u/ml-embedded-evaluation-kit/) from the [Arm Model Zoo](https://github.com/ARM-software/ML-zoo) are available to use in your this application codebase, depending on memory requirements of course. The first step to use another module is to generate sources files from its labels and `.tflite` model.

```sh
# Enter the ml example repository
cd lib/ml-embedded-evaluation-kit/

ML_GEN_SRC="generated/<model name>/src"
ML_GEN_INC="generated/<model name>/include"

mkdir -p $ML_GEN_SRC
mkdir -p $ML_GEN_INC

./lib/ml-embedded-evaluation-kit/resources_downloaded/env/bin/python3 scripts/py/gen_labels_cpp.py \
    --labels_file resources/<model name>/labels/<label file>.txt \
    --source_folder_path $ML_GEN_SRC \
    --header_folder_path $ML_GEN_INC \
    --output_file_name <model name>_labels
./resources_downloaded/env/bin/python3 scripts/py/gen_model_cpp.py \
    --tflite_path resources_downloaded/<model name>/<model>.tflite \
    --output_dir $ML_GEN_SRC
```

Models available are present in `./lib/ml-embedded-evaluation-kit/resources_downloaded`.
Pre-integrated source code is available from the `ML Embedded Eval Kit` and can be browsed from `./lib/ml-embedded-evaluation-kit/source/use_case`.

Integrating a new model means integrating its source code and requires update of the build files.

<br>

# License and contributions

The software is provided under the Apache-2.0 license. All contributions to software and documents are licensed by contributors under the same license model as the software/document itself (ie. inbound == outbound licensing). ATS-Keyword may reuse software already licensed under another license, provided the license is permissive in nature and compatible with Apache v2.0.

Folders containing files under different permissive license than Apache 2.0 are listed in the [LICENSE](./LICENCE.md) file.

<br>

# Security issues reporting

If you find any security vulnerabilities, please do not report it in the GitLab issue tracker. Instead, send an email to the security team at arm-security@arm.com stating that you may have found a security vulnerability in the IoT Total Solution Keyword Spotting project.

More details can be found at [Arm Developer website](https://developer.arm.com/support/arm-security-updates/report-security-vulnerabilities).
