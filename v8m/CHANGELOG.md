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
