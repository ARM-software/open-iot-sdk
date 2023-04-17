# ML Inference Advisor

## Introduction

This tool is used to help AI developers design and optimize neural network
models for efficient inference on Arm targets by enabling performance analysis
and providing actionable advice early in the model development cycle. The final
advice can cover the operator list, performance analysis and suggestions for
model inference run on certain hardware before/after applying model optimization
(e.g. pruning, clustering, etc.).

## Backends

The ML Inference Advisor is designed to support multiple performance
estimators (backends) to generate performance analysis for individual
types of hardware.

MLIA currently supports the following backend platforms:
* Corstone-300 - see https://developer.arm.com/Processors/Corstone-300
* Corstone-310 - see https://github.com/ARM-software/open-iot-sdk

## Setup

To set up MLIA with the [Arm IoT Total Solutions](https://www.arm.com/solutions/iot/total-solutions-iot) environment,
the recommended way is to use the `ats.sh` frontend script:

```bash
../ats.sh build mlia
```
This command installs the latest released version of MLIA along with its dependencies, and sets up the Python
virtual environment for the subsequent commands.

## Usage via the frontend script

Once the environemnt is set up, you can use the frontend script, `ats.sh` for a quick MLIA test drive:

```bash
../ats.sh run mlia
```

This will run MLIA operator command on a bundled quantized DS-CNN-L model, which reports advice.
The output of this command is saved into `build/mlia/mlia-output/logs/mlia.log`.

## Using MLIA CLI directly

In order to access the full feature set of the ML Inference Advisor tool, you can use the CLI directly as follows.
First you need to activate the virtual environment set up by the `build` command above.
In the top level of the `keyword` repository, type:

```bash
source build/mlia/venv/bin/activate
```

After this you can run `mlia`:

```bash
mlia --help
```

# Reference

For more details, please refer to official documentation: https://pypi.org/project/mlia
