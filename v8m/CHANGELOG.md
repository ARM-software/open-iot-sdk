# v2023.07 (2023-07-13)

## Changes

* examples: Add Azure Device Update to keyword and speech examples.
* build: Set CMAKE_SYSTEM_PROCESSOR based on TS_TARGET.
* cmake: Replace custom PRJ_DIR with built-in CMAKE_SOURCE_DIR.
* examples: Remove MDH direct dependency and usage of CMSIS Driver APIs
* cs310: Fix timing for AWS device certification
* docs: Update the category and version for component in LICENSE.md
* script: add scripts to ease the creation of thing, policies and most things around them.
* open-iotsdk: Use updated mcu-driver-hal.
* build: Add support for GCC.
* ml: Add support to run examples without Ethos NPU.
* docs: Removing CODEOWNERS file as it will be resolved by Gitlab groups
* toolchains: Use updated toolchain files.
* iotsdk: Update to latest version
  * Include and Update to TF-M V1.8.0
  * Remove bespoke BSP for Corstone-310 and transition to TM-F default BSP
  * Support for latest CMSIS
* tf-m: Set all config options in TFM_CMAKE_ARGS as required by the latest Open IoT SDK.
* test: Update tests to use pyedmgr.
* ci: Use pyedmgr in CI
* coremqtt-agent: Replace with the Open IoT SDK's component
* app: Improve cloud connection stability

  - Pull patches to cloud clients from the SDK
  - Improve RTOS configuration
  - Fix logging timestamp
  - Rework ML processing start and stop
* cmake: Use tfm_s_signed.bin from TF-M build instead of signing the secure image by ourselves.
* examples: Remove duplicated audio files.
* keyword: Fix incorrect mapping of results.
* keyword: Fix and simplify ML state processing in blink task.
* keyword: Add support for preloading the audio clip in ROM.
* cmake: Do not copy compile_commands.json into the root directory.
* speech: Add support for preloading audio clip in ROM.
* cmake: Update toolchain files.
* aws: Reduce the log verbosity from debug to info to improve readability and CI performance.
* ci: Update the developer-tools to have Black Duck scan available.
* ci: Enable the Black Duck scan as part of quality-check.
* ami: Update environment setup instruction


# v2023.04 (2023-04-13)

## Changes

* open-iot-sdk: Update version to pull TF-M v1.7 and align examples with it
* ci: Enable possible tpip violation warning messages in Merge Requests.
* docs: Remove trailing space on main README.md
* bsp: Move platform NS ER_DATA regions to ISRAM0 from QSPI to prevent overlappings
* cmsis: Apply the new macros available in threadx-cdi-port for mapping ThreadX priorities to CMSIS RTOS
* mlia: update MLIA to 0.6.0
* examples: Remove padding of update images.
* Update Open IoT SDK to the latest version
* bsp: Replace AN547 with AN552 for Corstone 300


# v2023.01 (2023-01-19)

## Changes

* changelog: Add towncrier news fragments and configuration
* ci: Import template jobs
  ci: Enable pipeline in main branch
* ci: Add explicit stage for `sync-public`
* license: Add components information

  LICENSE.md does not only list name of components and their
  licenses but also more detailed information like version,
  url-origin.
* gitlab-ci: Fetch submodules for the pipeline (developer-tools are needed)
* ci: Add public sync from public Gitlab to internal GitLab
* cmake: Fix the issue that cmsis-rtx accidentally gets linked when threadx-cdi-port is the intended RTOS implementation.
* blinky: Fix tfm_ns_interface_init failing to create a mutex due to uninitialized ThreadX kernel.
* codeowners: Update the list of code owners to reflect the expanded scope of this repository.
* pre-commit: Add gitlint
* rtos: Raise blink thread's priority to fix "Failed to send blink_event message to ui_msg_queue".
* pre-commit: Add black to format Python scripts and apply recommended format changes.
* ml: Update support for ml-embedded-evaluation-kit.
* cloud: Add an option `--endpoint AZURE_NETXDUO` to use NetX Duo as the network stack to connect to Azure IoT Hub.
* mlia: update MLIA to 0.5.0
* examples: Fix coding style with includes sorting enabled
* ci: Create azure device dynamically
* doc: Update architecture diagram
* ci: Enable autobot


This changelog should be read in conjunction with release notes provided
for a specific release version.
