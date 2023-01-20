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
