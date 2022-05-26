# Overview
Welcome to Arm's Open-IoT-SDK, where you have access to a wide variety of IoT targeted software applications and components ready to run on Arm based platforms.  This software framework accelerates intelligent, connected IoT product design by allowing developers to focus on what really matters – innovation and differentiation. 

The Open-IoT-SDK contains Total-Solutions applications that allow the user to explore and evaluate not only Arm IP, and the [Open-CMSIS-CDI API's](https://github.com/Open-CMSIS-Pack/open-cmsis-cdi-spec), but several other Arm tools as well.  It is strongly recommended to explore these pages thoroughly as you will find information on the code itself, the different ways to build and execute the applications as well as the different tools that are available to you, such as Arm Virtual Hardware. 

The Open-IoT-SDK located within **GitHub** is a read-only downstream Arm® project mirror of the upstream project located in [GitLab:Open-IoT-SDK](https://gitlab.arm.com/iot/open-iot-sdk/examples).  Any enhancement requests or bugs identified for this repository will need to be submitted to the upstream [GitLab:Open-IoT-SDK](https://gitlab.arm.com/iot/open-iot-sdk/examples) project.”

Below is a list of software topics to explore and utilize:

* Source Code
  * [Arm Total Solution applications](#Arm-Total-Solutions)
  * [System Setup and other things a developer needs to know](#things-to-know)
* Developer Tools
  * [Continuous Integration](#Continuous-Integration)
  * [Arm Virtual Hardware](#Arm-Virtual-Hardware)
  * [ML Embedded Software Evaluation Kit](#ML-Embedded-Eval-Kit)
  * [ML Inference Advisor](#ML-Inference-Advisor)
* [More applications highlighting ARM Hardware-IP](#Other-Resources)
* [Future Enhancements](#Future-Enhancements)

The goal of this repository is to provide you the developer with as many software choices as possible to run on Arm IP without downloading a huge monolithic codebase. This repository utilizes a multi-repository architecture stored in various locations across the internet. All the source code located in this GitHub repository is a subset of repositories stored and maintained within the [GitLab:Open-IoT-SDK](https://gitlab.arm.com/iot/open-iot-sdk) framework. 

The Total-Solutions applications you have access to here, when built, will pull down only the required software components that are needed thus saving time and hard disk space. To further explore the next level of source code, you can go to the home of the [GitLab:Open-IoT-SDK](https://gitlab.arm.com/iot/open-iot-sdk) framework.

<br>

# Arm Total Solutions
[Arm IoT Total Solutions](https://www.arm.com/solutions/iot/total-solutions-iot) provides a complete solution designed for specific use-cases containing everything needed to streamline the design process and accelerate product development.  To accelerate your software development needs, each Total Solutions brings together:

* Hardware IP
* CSP cloud connectivity middleware
* Real-time OS support
* Machine learning (ML) models
* Advanced tools such as Arm Virtual Hardware
* Application specific reference code 
* 3rd Party support from the world’s largest IoT ecosystem

THe [FreeRTOS](https://freertos.org/) kernel is already included in all application's and over time more RTOS kernel abstractions will be added demonstrating the usability of the Open-CMSIS-CDI RTOS interfaces.

Refer to our initial Total Solutions below, ATS-Keyword and ATS-Blinky for more information.   

<br>

## ATS-Keyword
This repo contains Arm's first [IoT Total Solution](https://www.arm.com/solutions/iot/total-solutions-iot), "Key Word Spotting" that runs on Arm's newer platforms and processors from Armv8-M onwards.  It provides general-purpose compute and ML workload use-cases, including an ML-based keyword spotting example that leverages the DS-CNN model from the [Arm Model Zoo](https://github.com/ARM-software/ML-zoo). This application also demonstrates various Cloud-connected functionality, such as Over-the-Air update and Security concepts via [TrustedFirmware-M](https://www.trustedfirmware.org/projects/tf-m/).  The source code for this project is located within the [ATS-Keyword](./examples/ats-keyword) folder.

<br>

## ATS-Blinky
ATS-Blinky is a General-Purpose-Compute application that demonstrates blinking LEDs using Arm Virtual Hardware on Arm's more established processors based on Armv6-M and Armv7-M.    To start exploring this baseline application, head on over to the [ATS-Blinky](./examples/ats-blinky) project.

<br>

## Things to know
To build the Total-Solutions applications, there are some thing you need to know. If you want to tryout Arm Virtual hardware, proceed to the application that best fits your needs.  You will find all the instructions needed to build the source code to work with the Arm Virtual Hardware platform.

If you are more of a **Do-It-Yourself** type of developer, wanting to know how to setup your local build environment, perform the build yourself and see what options are available; head on over to the [Open-IoT-SDK developer page](https://gitlab.arm.com/iot/open-iot-sdk/sdk/-/tree/main/docs).

| Build Environments       | Status                    |
|----------------------    |---------------------------|
| Linux                    | **Now supported**         |
| Windows (WSL)*           | Coming soon               |
| Windows (DOS)*           | Coming soon               |
| uVision (MDK)            | Coming soon               |
| Keil Studio              | Coming soon               |

### Known limitations

- *Currently if your Windows system has WSL installed, you must use WSL for all builds.
- Arm compiler 6 is the only compiler full supported, GCC support is in Beta status.
- Accuracy of the ML detection running alongside cloud connectivity depends on the performance of the AVH server instance used. We recommend to use at least a t3.medium instance, though c5.large is preferred, when running on AWS.
- Arm Virtual Hardware subsystem simulation is not time/cycle accurate. Performances will differ depending on the performances of the host machine.

<br>

# Open-CMSIS-CDI
The Open-CMSIS-CDI standard is one the of the foundations of IoT Total Solutions and is designed to solve common industry problems, reduce barriers to deployment and enable scale across the Arm Cortex-M ecosystem. Open-CMSIS-CDI will define a common set of interfaces targeting cloud-service-to-device functionality to enable major IoT stacks to run across as broad a range of Arm-based MCUs as possible, with minimal porting effort.

Open IoT-SDK is the reference implementation of the Centauri standards, [Open-CMSIS-CDI](https://github.com/Open-CMSIS-Pack/open-cmsis-cdi-spec) and [PSA](https://www.psacertified.org/). It is a software framework that demonstrates the capabilities that a Centauri-compliant device should have, including all the relevant APIs. This reference implementation provides a foundation for developers to build products upon in a scalable, consistent manner. 

Click [here](https://github.com/Open-CMSIS-Pack/open-cmsis-cdi-spec) learn more.

<br>

# Open-CMSIS-Pack
The Open-CMSIS-Pack project delivers the infrastructure to integrate and manage software components and improve code reuse across embedded and IoT projects.  Work is underway to integrate this technology into the Open-IoT-SDK and the Total-Solutions applications and is coming soon.

Click [here](https://github.com/Open-CMSIS-Pack/Open-CMSIS-Pack-Spec) to learn more.

<br>

# Continuous Integration
Each Total Solution application has been built and verified using a continuous integration process.  Inside each Total Solution application folder there are examples for CI systems such as GitHub.  GitLab example files are coming soon!

<br>

# Arm Virtual Hardware
Arm Virtual Hardware (AVH) helps software developers build Arm-based intelligent applications faster.  AVH delivers models of Arm-based processors, systems, and boards for application developers and SoC designers to build and test software without hardware. It runs as a simple application in the cloud and is ideal for testing using modern agile software development practices such as continuous integration and continuous development CI/CD (DevOps) workflows. 

Click [here](https://www.arm.com/products/development-tools/simulation/virtual-hardware) to learn more.

<br>

# ML Embedded Eval Kit
The Arm ML Embedded Evaluation Kit , is an open-source repository enabling users to quickly build and execute machine learning software for evaluating and exercising Arm Cortex-M CPUs and Arm Ethos-U55/U65 NPUs.

With the ML Eval Kit you can run inferences on either a custom neural network on Ethos-U microNPU or using available ML applications such as [Image classification](https://review.mlplatform.org/plugins/gitiles/ml/ethos-u/ml-embedded-evaluation-kit/+/c930ad9dc189d831ac77f716df288f70178d4c10/docs/use_cases/img_class.md), [Keyword spotting (KWS)](https://review.mlplatform.org/plugins/gitiles/ml/ethos-u/ml-embedded-evaluation-kit/+/c930ad9dc189d831ac77f716df288f70178d4c10/docs/use_cases/kws.md), [Automated Speech Recognition (ASR)](https://review.mlplatform.org/plugins/gitiles/ml/ethos-u/ml-embedded-evaluation-kit/+/c930ad9dc189d831ac77f716df288f70178d4c10/docs/use_cases/asr.md), [Anomaly Detection](https://review.mlplatform.org/plugins/gitiles/ml/ethos-u/ml-embedded-evaluation-kit/+/c930ad9dc189d831ac77f716df288f70178d4c10/docs/use_cases/ad.md), and [Person Detection](https://review.mlplatform.org/plugins/gitiles/ml/ethos-u/ml-embedded-evaluation-kit/+/HEAD/docs/use_cases/visual_wake_word.md) all using [Arm Virtual Hardware](#Arm-Virtual-Hardware).


Click [here](https://review.mlplatform.org/plugins/gitiles/ml/ethos-u/ml-embedded-evaluation-kit/) to learn more.

<br>

# Other Resources

| Repository                                                                                                    | Description                                                                                                                            |
|---------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------|
| [Arm AI Ecosystem Catalog](https://www.arm.com/why-arm/partner-ecosystem/ai-ecosystem-catalog)                | Connects you to the right partners, enabling you to build the next generation of AI solutions                                          |
| [Arm IoT Ecosystem Catalog](https://www.arm.com/why-arm/partner-ecosystem/iot-ecosystem-catalog)              | Explore Arm IoT Ecosystem partners who can help transform an idea into a secure, market-leading device.                                |
| [Arm ML Model Zoo](https://github.com/ARM-software/ML-zoo)                                                    | A collection of machine learning models optimized for Arm IP.                                                                          |
| [Arm Virtual Hardware Documentation](https://mdk-packs.github.io/VHT-TFLmicrospeech/overview/html/index.html) | Documentation for [Arm Virtual Hardware](https://www.arm.com/products/development-tools/simulation/virtual-hardware)                   |
| [Arm Virtual Hardware source code](https://github.com/ARM-software/VHT)                                       | Source code of[Arm Virtual Hardware](https://www.arm.com/products/development-tools/simulation/virtual-hardware)                       |
| [FreeRTOS Documentation](https://freertos.org/a00104.html#getting-started)                                    | Documentation for FreeRTOS.                                                                                                        |
| [FreeRTOS source code](https://github.com/FreeRTOS)                                                           | Source code of FreeRTOS.                                                                                                           |
| [AWS IoT](https://docs.aws.amazon.com/iot/latest/developerguide/what-is-aws-iot.html)                         | Documentation for AWS IoT.                                                                                                             |
| [Trusted Firmware-M Documentation](https://tf-m-user-guide.trustedfirmware.org/)                              | Documentation for Trusted Firmware-M.                                                                                                  |
| [Trusted Firmware-M Source code](https://git.trustedfirmware.org/TF-M/trusted-firmware-m.git)                 | Source code of Trusted Firmware-M.                                                                                                     |
| [Mbed Crypto](https://github.com/ARMmbed/mbedtls)                                                             | Mbed Crypto source code.                                                                                                               |
| [MCU Boot](https://github.com/mcu-tools/mcuboot)                                                              | MCU Boot source code.                                                                                                                  |
| [ml-embedded-evaluation-kit](https://review.mlplatform.org/plugins/gitiles/ml/ethos-u/ml-embedded-evaluation-kit) | ML Embedded eval kit source code                                                                                                   |
| Support                                                                                                       | A [community.arm.com](http://community.arm.com/) forum exists for users to post queries.                                               |
| |

# Future Enhancements
- [AWS Partner Device Catalog Listing](https://devices.amazonaws.com/) - leveraging Arm Virtual Hardware

<br>


# License and contributions

The software is provided under the Apache-2.0 license. All contributions to software and documents are licensed by the contributing organization under the same license model as the software/document itself (ie. inbound == outbound licensing). Arm Total Solutions applications may reuse software already licensed under another license, provided the license is permissive in nature and compatible with Apache v2.0.

Folders containing files under different permissive license than Apache 2.0 are listed in the LICENSE file.

Please see the **CONTRIBUTING.md** file in each folder for more information.

## Security issues reporting

If you find any security vulnerabilities, please do not report it in the GitLab issue tracker. Instead, send an email to the security team at arm-security@arm.com stating that you may have found a security vulnerability in the IoT Total Solution Keyword Spotting project.

More details can be found at [Arm Developer website](https://developer.arm.com/support/arm-security-updates/report-security-vulnerabilities).

