# Blinky Example

This example demonstrates the integration of FreeRTOS's kernel and MCU-Driver-HAL into a simple
application.

This example supports the following targets:

- Cortex-M55: `ARM_AN552_MPS3`
- Cortex-M4: `ARM_AN386_MPS2`

## Building

```sh
cmake -B __build --toolchain=toolchains/toolchain-arm-none-eabi-gcc.cmake -DCMAKE_SYSTEM_PROCESSOR=cortex-m4 -DMDH_PLATFORM=ARM_AN386_MPS2 .
cmake --build __build -j
```

## Running

The application can be ran using the following command:

```sh
VHT_MPS2_Cortex-M4 blinky
```

Note that due to how the model optimises the execution, you may need to add `--parameter armcortexm4ct.scheduler_mode=1`
in order to get more accurate frequency of the interrupt generation.

## Known limitations

The armclang toolchain is not yet fully supported.

## License and contributions

The software is provided under the [Apache-2.0 license](./LICENSE-apache-2.0.txt). All contributions to software and documents are licensed by contributors under the same license model as the software/document itself (ie. inbound == outbound licensing). Open IoT SDK may reuse software already licensed under another license, provided the license is permissive in nature and compatible with Apache v2.0.

Folders containing files under different permissive license than Apache 2.0 are listed in the LICENSE file.

Please see [CONTRIBUTING.md](CONTRIBUTING.md) for more information.

## Security issues reporting

If you find any security vulnerabilities, please do not report it in the GitLab issue tracker. Instead, send an email to the security team at arm-security@arm.com stating that you may have found a security vulnerability in the Open IoT SDK.

More details can be found at [Arm Developer website](https://developer.arm.com/support/arm-security-updates/report-security-vulnerabilities).
