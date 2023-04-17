Unless specifically indicated otherwise in a file, files are licensed under the Apache 2.0 license,
as can be found in: LICENSE-apache-2.0.txt

## Components

Each component should contain its own README file with license specified for its files. The original license text is included in those source files.

```json:table
{
    "fields": [
        "Component",
        "Path",
        "License",
        "Origin",
        "Category",
        "Version",
        "Security risk"
    ],
    "items": [
        {
            "Component": "Amazon FreeRTOS",
            "Path": "bsp/aws_configs, lib/AWS/aws_libraries",
            "License": "MIT",
            "Origin": "https://github.com/aws/amazon-freertos",
            "Category": "1",
            "Version": "v6.1.12_rel",
            "Security risk": "low"
        },
        {
            "Component": "coreMQTT-Agent",
            "Path": "lib/aws/coreMQTT-Agent",
            "License": "MIT",
            "Origin": "https://github.com/FreeRTOS/coreMQTT-Agent",
            "Category": "2",
            "Version": "3b743173ddc7ec00d3073462847db15580a56617",
            "Security risk": "low"
        },
        {
            "Component": "FreeRTOS OTA PAL PSA",
            "Path": "lib/AWS",
            "License": "MIT",
            "Origin": " https://github.com/Linaro/freertos-ota-pal-psa",
            "Category": "1",
            "Version": "8896ca67da50597f228828d21f90bb193cd8588d",
            "Security risk": "low"
        },
        {
            "Component": "FreeRTOS PKCS11 PSA",
            "Path": "lib/AWS",
            "License": "MIT",
            "Origin": " https://github.com/Linaro/freertos-pkcs11-psa",
            "Category": "1",
            "Version": "e137fb99e0980f4c35bbc6eff74e30505b3c08af",
            "Security risk": "low"
        },
        {
            "Component": "TF-M",
            "Path": "bsp/tf_m_targets",
            "License": "BSD-3-Clause",
            "Origin": " https://git.trustedfirmware.org/TF-M/trusted-firmware-m.git",
            "Category": "1",
            "Version": "TF-Mv1.6.0",
            "Security risk": "high"
        },
        {
            "Component": "portmacro.h",
            "Path": "bsp/freertos-config",
            "License": "MIT",
            "Origin": "https://github.com/FreeRTOS/FreeRTOS-Kernel",
            "Category": "1",
            "Version": "FreeRTOS Kernel V10.4.3",
            "Security risk": "low"
        },
        {
            "Component": "SpeexDSP",
            "Path": "lib/SpeexDSP",
            "License": "BSD-3-Clause",
            "Origin": "https://gitlab.xiph.org/xiph/speexdsp",
            "Category": "2",
            "Version": "738e17905e1ca2a1fa932ddd9c2a85d089f4e845",
            "Security risk": "low"
        },
        {
            "Component": "netxduo-config",
            "Path": "bsp/netxduo-config",
            "License": "Microsoft Software License Terms for Microsoft Azure RTOS",
            "Origin": "https://github.com/azure-rtos/netxduo",
            "Category": "2",
            "Version": "",
            "Security risk": "low"
        }
    ]
}
```
