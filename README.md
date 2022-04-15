# Overview
Welcome to Arm's Open-IoT-SDK, where you have access to a wide variety of IoT targeted software applications and components ready to run on Arm based platforms.  This software framework accelerates intelligent, connected IoT product design by allowing developers to focus on what really matters – innovation and differentiation. 

The Open-IoT-SDK located within **GitHub** is a read-only downstream Arm® project mirror of the upstream project located in [GitLab:Open-IoT-SDK](https://gitlab.arm.com/iot/open-iot-sdk/examples).  Any enhancement requests or bugs identified for this repository will need to be submitted to the upstream [GitLab:Open-IoT-SDK](https://gitlab.arm.com/iot/open-iot-sdk/examples) project.”

Below is a list of software topics to explore and utilize:

* Arm Total Solution reference applications
* Open-CMSIS-CDI API adaptation code and header files
* Developer Tools
  * Continuous Integration
  * Arm Virtual Hardware
* Various example applications highlighting ARM Hardware-IP

The goal of this repository is to provide you the developer with as many software choices as possible to run on Arm IP without downloading a huge monolithic codebase.  This repository utilizes a multi-repository architecture stored in various locations across the internet.  

The Total-Solutions applications you have access to here are built to pull down only the required software components that are needed thus saving time and hard disk space.  All the source code located in this GitHub repository is a READ-ONLY mirror of repositories within the Open-IoT-SDK framework that are stored and maintained [here](https://gitlab.arm.com/iot/open-iot-sdk) .  If you would like to submit changes, fixes or suggestions go to the specific top-level folder in this repository that contains the code you wish to change for more detailed instructions. 

To further explore the next level of source code, you can go to the home of the Open-IoT-SDK software framework, located [here](https://gitlab.arm.com/iot/open-iot-sdk).

<br>

# Arm Total Solutions
[Arm IoT Total Solutions](https://www.arm.com/solutions/iot/total-solutions-iot) provides a complete solution designed for specific use-cases containing everything needed to streamline the design process and accelerate product development.  To accelerate your software development needs, each Total Solutions brings together:

* Hardware IP
* csp cloud connectivity middleware
* real-time OS support
* machine learning (ML) models
* advanced tools such as the new Arm Virtual Hardware
* application specific reference code 
* 3rd Party support from the world’s largest IoT ecosystem

Refer to our initial Total Solutions below, ATS-Keyword and ATS-Speech for more information.   

<br>

## ATS-Keyword
This repo contains Arm's first [IoT Total Solution](https://www.arm.com/solutions/iot/total-solutions-iot), "Keyword Detection".  It provides general-purpose compute and ML workload use-cases, including an ML-based keyword recognition example that leverages the DS-CNN model from the [Arm Model Zoo](https://github.com/ARM-software/ML-zoo).

<br>

## ATS-Blinky
ATS-Blinky is a General-Purpose-Compute application that not only demonstrates blinking LEDs using Arm Virtual Hardware on Arm's more established processors.  AWS FreeRTOS is already included in the application to kickstart new developme

<br>

## Things to know!

| Build Environments       | Status                    |
|----------------------    |---------------------------|
| Linux                    | Now supported             |
| Windows (WSL)            | Now supported             |
| MDK                      | **Coming soon**           |
| Keil Studio              | **Coming soon**           |
| Windows (command prompt) | **Coming soon**           |
| |

<br>

# Open-CMSIS-CDI
The Open-CMSIS-CDI standard is one the of the foundations of IoT Total Solutions and is designed to solve common industry problems, reduce barriers to deployment and enable scale across the Arm Cortex-M ecosystem. 

Open IoT-SDK is the reference implementation of the Centauri standards, [Open-CMSIS-CDI](https://github.com/Open-CMSIS-Pack/open-cmsis-cdi-spec) and [PSA](https://www.psacertified.org/). It is a software framework that demonstrates the capabilities that a Centauri-compliant device should have, including all the relevant APIs. This reference implementation provides a foundation for developers to build products upon in a scalable, consistent manner. 

Open-CMSIS-CDI will define a common set of interfaces targeting cloud-service-to-device functionality to enable major IoT stacks to run across as broad a range of Arm-based MCUs as possible, with minimal porting effort.
[Learn More](https://github.com/Open-CMSIS-Pack/open-cmsis-cdi-spec)

<br>

# Open-CMSIS-Pack
The Open-CMSIS-Pack project delivers the infrastructure to integrate and manage software components and improve code reuse across embedded and IoT projects.  Work is underway to integrate this technology into the Total-Solutions Applications and is coming soon.

Click [here](https://github.com/Open-CMSIS-Pack/Open-CMSIS-Pack-Spec) to learn more.

<br>

# Continuous Integration
Each Total Solution application has been built and verified using a continuous integration process.  Insie each Total Solution application folder you will find instructions on how to setup a continuous integration pipeline.  There will be examples for various CI systems such as GitLab and GitHub.

<br>

# Arm Virtual Hardware
Arm Virtual Hardware (AVH) helps software developers build Arm-based intelligent applications faster.  AVH delivers models of Arm-based processors, systems, and boards for application developers and SoC designers to build and test software without hardware. It runs as a simple application in the cloud and is ideal for testing using modern agile software development practices such as continuous integration and continuous development CI/CD (DevOps) workflows. 

[Learn More](https://www.arm.com/products/development-tools/simulation/virtual-hardware)


<br>

# Other Resources

| Repository                                                                                                    | Description                                                                                                                            |
|---------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------|
| [Arm AI Ecosystem Catalog](https://www.arm.com/why-arm/partner-ecosystem/ai-ecosystem-catalog)                | Connects you to the right partners, enabling you to build the next generation of AI solutions                                          |
| [Arm IoT Ecosystem Catalog](https://www.arm.com/why-arm/partner-ecosystem/iot-ecosystem-catalog)              | Explore Arm IoT Ecosystem partners who can help transform an idea into a secure, market-leading device.                                |
| [Arm ML Model Zoo](https://github.com/ARM-software/ML-zoo)                                                    | A collection of machine learning models optimized for Arm IP.                                                                          |
| [Arm Virtual Hardware Documentation](https://mdk-packs.github.io/VHT-TFLmicrospeech/overview/html/index.html) | Documentation for [Arm Virtual Hardware](https://www.arm.com/products/development-tools/simulation/virtual-hardware)                   |
| [Arm Virtual Hardware source code](https://github.com/ARM-software/VHT)                                       | Source code of[Arm Virtual Hardware](https://www.arm.com/products/development-tools/simulation/virtual-hardware)                       |
| [AWS FreeRTOS Documentation](https://docs.aws.amazon.com/freertos/)                                           | Documentation for AWS FreeRTOS.                                                                                                        |
| [AWS FreeRTOS source code](https://github.com/aws/amazon-freertos)                                            | Source code of AWS FreeRTOS.                                                                                                           |
| [AWS IoT](https://docs.aws.amazon.com/iot/latest/developerguide/what-is-aws-iot.html)                         | Documentation for AWS IoT.                                                                                                             |
| [Trusted Firmware-M Documentation](https://tf-m-user-guide.trustedfirmware.org/)                              | Documentation for Trusted Firmware-M.                                                                                                  |
| [Trusted Firmware-M Source code](https://git.trustedfirmware.org/TF-M/trusted-firmware-m.git)                 | Source code of Trusted Firmware-M.                                                                                                     |
| [Mbed Crypto](https://github.com/ARMmbed/mbedtls)                                                             | Mbed Crypto source code.                                                                                                               |
| [MCU Boot](https://github.com/mcu-tools/mcuboot)                                                              | MCU Boot source code.                                                                                                                  |
| [ml-embedded-evaluation-kit](https://review.mlplatform.org/plugins/gitiles/ml/ethos-u/ml-embedded-evaluation-kit) | ML Embedded eval kit source code                                                                                                   |
| Support                                                                                                       | A [community.arm.com](http://community.arm.com/) forum exists for users to post queries.                                               |
| |



# License and contributions

The software is provided under the Apache-2.0 license. All contributions to software and documents are licensed by the contributing organization under the same license model as the software/document itself (ie. inbound == outbound licensing). Arm Total Solutions applications may reuse software already licensed under another license, provided the license is permissive in nature and compatible with Apache v2.0.

Folders containing files under different permissive license than Apache 2.0 are listed in the LICENSE file.

Please see the **CONTRIBUTING.md** file in each folder for more information.

## Security issues reporting

If you find any security vulnerabilities, please do not report it in the GitLab issue tracker. Instead, send an email to the security team at arm-security@arm.com stating that you may have found a security vulnerability in the IoT Total Solution Keyword Detection project.

More details can be found at [Arm Developer website](https://developer.arm.com/support/arm-security-updates/report-security-vulnerabilities).

