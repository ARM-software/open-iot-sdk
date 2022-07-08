# Introduction

[Arm IoT Total Solutions](https://www.arm.com/solutions/iot/total-solutions-iot) provides a complete solution designed for specific use-cases, leaving developers to focus on what really matters - innovation and differentiation across diverse and varied use cases. It has everything needed to simplify the design process and streamline product development, including hardware IP, software, real-time OS support, machine learning (ML) models, advanced tools such as the new Arm Virtual Hardware, application specific reference code and support from the world's largest IoT ecosystem.

# Overview

This repo contains Arm's first [IoT Total Solution](https://www.arm.com/solutions/iot/total-solutions-iot), "Keyword Detection".  It provides general-purpose compute and ML workload use-cases, including an ML-based keyword recognition example that leverages the DS-CNN model from the [Arm Model Zoo](https://github.com/ARM-software/ML-zoo).

The software supports multiple configurations of the Arm Corstone-300 subsystem, incorporating the Cortex-M55 processor and Arm Ethos-U55 microNPU.  This total solution provides the complex, non differentiated secure platform software on behalf of the ecosystem, thus enabling you to focus on your next killer app.

## Keyword detection application

The keyword detection application runs the DS-CNN model on top of [FreeRTOS](https://freertos.org/a00104.html#getting-started). It detects keywords and inform the user of which keyword has been spotted. The audio data to process are injected at run time using the [Arm Virtual Hardware](https://www.arm.com/virtual-hardware) audio driver.

The Keyword application connects to [AWS IoT](https://docs.aws.amazon.com/iot/latest/developerguide/what-is-aws-iot.html) cloud to publish recognised keywords. AWS IoT cloud is also used for OTA firmware updates. These firmware updates are securely applied using [Trusted Firmware-M](https://tf-m-user-guide.trustedfirmware.org/). For more information, refer to the keyword detection [Readme](./kws/README.md).

![Key word detection architecture overview](./resources/Keyword-detection-overview.png)

## Blinky application

The blinky application demonstrate blinking LEDs using Arm Virtual Hardware. FreeRTOS is already included in the application to kickstart new developments.

# Quick Start

Follow these simple steps to build and execute the code example's application within **Arm Virtual Hardware**.

* [Launch Arm Virtual Hardware system](#launch-arm-virtual-hardware-instance)
* [Build and execute](#build-and-execute-the-application)
* [Setting up AWS Cloud connectivity](#setting-up-aws-connectivity)
* [Enabling OTA firmware update from the AWS Cloud](#ota-firmware-update)
* [Setting up Azure Cloud connectivity](#setting-up-azure-connectivity)
* [Terminating Arm Virtual Hardware](#terminate-arm-virtual-hardware-instance)

# Launch Arm Virtual Hardware Instance

There are 2 ways to launch the **Arm Virtual Hardware Instance**, choose one that best fits your work style.  For first timers, we recommend using option 1.
1. [AWS Web Console launch](#launch-using-aws-web-console)
2. [Local Terminal launch](#launch-using-a-local-terminal)

## Launch Using AWS Web Console
To utilize the Arm Virtual Hardware, you will need to create an [AWS Account](https://aws.amazon.com/premiumsupport/knowledge-center/create-and-activate-aws-account/) if you don't already have one.

### Launching the instance in EC2 [(AWS on getting started)](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/EC2_GetStarted.html)

1. Go to [EC2](https://console.aws.amazon.com/ec2/v2/) in the AWS Web Console.
1. Select **Launch Instance** which will take you to a wizard for launching the instance.
    > Arm Virtual Hardware for Corstone-300 is available as a public beta on AWS Marketplace. To help you get started, AWS are offering more than 100 hours of free AWS EC2 CPU credits for the first 1,000 qualified users.
    Click here to find out more: https://www.arm.com/company/contact-us/virtual-hardware.

     * **Step 1: Create a Name for your Instance** - To clearly identify the instance you are about to create you will need to apply a descriptive name.  It can be as simple as "J. Doe's AVH Instance".

     * **Step 2: Choose an [Amazon Machine Image (AMI)](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/AMIs.html)**
        * In the Search box, type `Arm Virtual Hardware` and then hit "enter" to find the item called <ins>"Arm Virtual Hardware" - "By Arm"</ins>.
          > Select: **Arm Virtual Hardware By Arm | Version 1.2.3**
          * NOTE: If you do not see the expected items, make sure the <ins>**AWS Marketplace AMIs**</ins> tab is selected.
        * Click on "Select" for that item. This image contains all the software necessary to build and run the Arm IoT Total Solutions.
          * This will raise a subscription page/pop-up titled, **Arm Virtual Hardware**.
          * You will note that the subscription is free from Arm, but <ins>AWS does charge for the costs of the instances themselves according to the pricing chart provided.</ins>
        * You must select "Continue" if you want to move forward.

     * **Step 3: Choose an Instance Type** - Select one of the instance types from the list.
        * We recommend the **c5.large**.
        * **Important:** Charges accrue while the instance is running and to a lesser degree when stopped.
        * Terminating the instance stops any charges from occurring.

     * **Step 4: Key pair (login)**
       * To ensure easy connection when using SSH from a local terminal, it is recommended to create a key pair at this time.
       * Click on **Create new key pair**
       * Enter a descriptive name, e.g. My_Key_Pair_AVH_us-west-2 or simply **MyKeyPair**
         *  To keep track of all the different things you create, we recommend adding your active region to the name.
       * Using the defaults options is fine.
       * Click on **Create key pair**
       * A private key is downloaded, place this in a location you will not forget as it is used later.
       * Once saved, you must fix the permissions of the file wherever you just stored it:
       ```sh
            chmod 400 MyKeyPair.pem
        ```

     * **Step 5: Configure storage** - To ensure enough disk drive space to contain the entire build image.  Set the amount of storage to "1x **24** GiB".

     * **Final Step:** From here you may select **Review and Launch** to move directly to the launch page or continue to configure instance details if you need to set any custom settings for this instance.

### Selecting the instance
Once you complete the wizard by initiating the instance launch you will see a page that allows you to navigate directly to the new instance. You may click this link or go back to your list of instances and find the instance through that method.

Whichever way you choose, find your new instance and select its instance ID to open the page to manage the instance.

### Connecting to the instance:
1. Select the instance you want.
2. Select **Connect** to open an SSH terminal session to the instance in your browser.
3. Ensure the User name field is set to `ubuntu`.
4. Select the **Connect** button to open the session.
   * This will put you in a browser window where you will have an SSH terminal window ready for your input.

You are now ready to build, click [here](#build-and-execute-the-application) to skip to the build instructions.

## Launch Using a local terminal
The instructions in this section, allow you to create and connect to an instance of the Arm Virtual Hardware AMI.  You will be using either you local PC or a server that is under your control.

1. Install [AWS CLI 2](https://docs.aws.amazon.com/cli/latest/userguide/install-cliv2.html) on your machine.

2. [Configure](https://docs.aws.amazon.com/cli/latest/userguide/cli-configure-quickstart.html) the access key, secret key and region that AWS CLI will use. If your organization uses AWS Single Sign-On, the [configuration process](https://docs.aws.amazon.com/cli/latest/userguide/cli-configure-sso.html) is slightly different. Make sure the region selected matches the region of the SSO service.

3. Create a new key pair if you have not done so already above.

```sh
aws ec2 create-key-pair --key-name MyKeyPair
```

4. When AWS CLI display the new key pair. Save the key material in a `.pem` file. The file permission must be set to `400`.

```sh
chmod 400 MyKeyPair.pem
```

5. Launch a new instance with the key pair created. The key pair can be reused to create new instances.

```sh
./scripts/vht_cli.py -k MyKeyPair start
```

### Connecting to the instance:

1. Get the IP of the instance started

```sh
./scripts/vht_cli.py -k MyKeyPair status
```

2. Connect to the instance using SSH and the private key saved locally.
  * e.g. ssh -i .ssh/MyKeyPair.pem ubuntu@\<**Public IPv4 DNS**\>
    * Look in your instance page at [EC2](https://console.aws.amazon.com/ec2/v2/) AWS Web Console for the **Public IPv4 DNS** value of your Instance.

```sh
ssh -i .ssh/MyKeyPair.pem ubuntu@ec2-xxx.xxx.xxx.xxx.eu-west-1.compute.amazonaws.com
```

# Re-connect a terminal to the instance
These instructions are only necessary if you have an existing instance and for some reason have terminated your console and need to reconnect for ongoing development.

Choose your terminal connection type (AWS-Web-Console or Local-Console)
* AWS-Web-Console
   * Go to [EC2](https://console.aws.amazon.com/ec2/v2/) in the AWS Web Console.
   * Click on "Instances"
   * Find the instance you created earlier
   * Click on the instance
   * Select Connect to open an SSH terminal session to the instance in your browser.
     * Ensure the User name field is set to ubuntu.
   * Select the Connect button to open the session.
     * This will put you in a browser window where you will have an SSH terminal window ready for your input.
 * Local-Console
     * Open your favorite terminal program or linux shell application and connect to the AVH AMI instance:
     * AWS requires you to use a secure connection, using the instance certificate you downloaded earlier.
     * e.g. ssh -i .ssh/MyKeyPair.pem ubuntu@\<**Public IPv4 DNS**\>
       * Look in your instance page at [EC2](https://console.aws.amazon.com/ec2/v2/) AWS Web Console for the **Public IPv4 DNS** value of your Instance.

      Example
     ```sh
     ssh -i .ssh/MyKeyPair.pem ubuntu@ec2-xxx.xxx.xxx.xxx.eu-west-1.compute.amazonaws.com
     ```
If you do not know your instance value, refer to the **AWS-Web-Console** instructions just above to get this information.

<br>

# Build and execute the application

To update the application, a set of scripts is included to setup the environment,
build applications, run them and test them. These scripts must be executed in the AVH AMI.

## Connect

Open your favorite terminal program or linux shell application and connect to the AVH AMI instance.
* AWS requires you to use a secure connection, using the instance certificate you downloaded earlier.
* e.g. ssh -i .ssh/MyKeyPair.pem ubuntu@\<**Public IPv4 DNS**\>
  * Look in your instance page at [EC2](https://console.aws.amazon.com/ec2/v2/) AWS Web Console for the **Public IPv4 DNS** value of your Instance.

Example
```sh
ssh -i .ssh/MyKeyPair.pem ubuntu@ec2-xxx.xxx.xxx.xxx.eu-west-1.compute.amazonaws.com
```

## Prepare environment

Clone the repository in the AMI using the following command format:
* git clone \<keyword repository\> && cd \<keyword repository\>

Example:
```sh
git clone https://github.com/ARM-software/open-iot-sdk.git && cd open-iot-sdk/examples/ats-keyword
```


Synchronize git submodules, setup ML and apply required patches:

```sh
./ats.sh bootstrap
```

Install additional python dependencies required to run tests and sign binaries:

```sh
sudo apt install python3.8-venv
```

```sh
python3.8 -m pip install imgtool cbor2
```

```sh
python3.9 -m pip install imgtool cffi intelhex cbor2 cbor pytest click
```

Make python user packages visible in the shell.

```sh
export PATH=$PATH:/home/ubuntu/.local/bin
```

## Build

There are currently two applications available: `blink` and `kws`.
The `ats.sh` scripts takes the command `bootstrap`, `build` or `run` as first  parameter.
The second parameter is the name of the application to build or run.
Below we use `kws` as the name of the application, replace it with `blinky` to build that instead.

Build the kws application:

```sh
./ats.sh build kws
```

This will by default build the application in the `build` directory for the `Corstone-300` target using the `FreeRTOS` OS. This is equivalent to:

```sh
./ats.sh build kws --target Corstone-300 --rtos FREERTOS --path build
```

To build for Corstone-310 use `--target Corstone-310`. To build using the RTX RTOS implementation use `--rtos RTX`.

You can have multiple builds with different parameters side by side by changing the `--path` parameter to something unique to each build configuration. This speed up the re-build process when working with multiple targets and RTOS.

## Run

Run the kws application:

```sh
./ats.sh run kws
```

The `run` command can take the `--path` switch to run a particular build. It uses `build` directory by default.
This is equivalent to:

```sh
./ats.sh run kws --path build
```

## Integration tests

Launch the kws integration tests:
```sh
pytest -s kws/tests/test_ml.py
```

# Updating audio data

The audio data streamed into the Arm Virtual Hardware is read from the file `test.wav` located at the root of the repository. It can be replaced with another audio file with the following configuration:
- Format: PCM
- Audio channels: Mono
- Sample rate: 16 bits
- Sample rate: 16kHz

# Continuous integration setup

Each Total Solution application has been built and verified using a continuous integration process. Inside each Total Solution application folder there are examples for CI systems such as GitHub. GitLab example files are coming soon

# Setting up AWS connectivity

The Keyword Detection application will attempt to connect to AWS IOT and report ML inference results through an MQTT connection.

To connect to the AWS cloud service you will need to setup an IoT Thing and then set the AWS credentials of the IoT Thing within the Application. You will need to create an [AWS Account](https://aws.amazon.com/premiumsupport/knowledge-center/create-and-activate-aws-account/) if you don’t already have one.


## AWS account IoT setup

The instructions below will allow the Application to send messages to the Cloud via MQTT as well as enable an Over-the-Air update.

  > Note: Due to AWS restrictions, you must ensure that when logging into the [AWS IoT console](https://console.aws.amazon.com/iotv2/) you ensure that it is using the same **Region** as where you created your AMI instance.  This restriction is documented within the [MQTT Topic](https://docs.aws.amazon.com/iot/latest/developerguide/topics.html) page in the AWS documentation.

### Create an IoT thing for your device

1. Login to your account and browse to the [AWS IoT console](https://console.aws.amazon.com/iotv2/).
   * If this takes you to AWS Console, click on **Services** above and then click on **IoT Core**.
   * Ensure your **Region** is correct.
2. In the left navigation pane, choose **Manage**, and then choose **Things**.
   * These instructions assume you do not have any IoT things registered in your account.
   * Choose **Register** or **Create things**.
3. On the **Create things** page, choose **Create single thing**.
4. On the **Specify thing properties** page, type a **Thing name** for your thing (for example `MyThing_eu_west_1`), and then choose **Next**.
   * Adding the region name helps to remind you which region the thing and topic is attached to.
   * You will need to add the name later to your C code.
   * There is no need to add any **addition configuration** or **Device Shadow** information.
5. On the **Configure device certificate** page, choose **Auto-generate a new certificate** and then click **Next**.
6. Skip the **Attach policies to certificate page** for now.
   * You will attach a certificate to the Thing in a later step.
7. Download your all the keys and certificates by choosing the **Download** links for each.
   * Click on all the **Download** buttons and store these files in a secure location as you will use them later.
   * Make note of the certificate ID. You need it later to attach a policy to your certificate.
   * Click on **Done** once all items have downloaded.
8. AWS used to have you choose **Activate** to activate your certificate.
   * This is no longer an option.

### Create a policy and attach it to your thing

1. In the navigation pane of the AWS IoT console, choose **Secure**, and then choose **Policies**.
2. On the **Policies** page, choose **Create Policy**.
   * These instructions assume you do not have any **Policies** registered in your account,
3. On the **Create Policy** page
   * Enter a name for the policy.
   * In the **Policy document** box, enter 4 separate policies with the following values:
     * **Policy effect** - Choose **Allow** for all entries.
     * **Policy Action** - Use the values below, 1 for each entry.
       * **iot:Connect**
       * **iot:Publish**
       * **iot:Subscribe**
       * **iot:Receive**
   * The **Policy resource** field requries an **ARN**. Sometimes this box will be auto-filled with your credentials.
     * If no value exists, use the following format:    (arn:aws:iot:**region:account-id:\***)
       * region (e.g. eu-west-1)
       * account-id ... This is your **Acount ID Number**.
         * You can see this usually on the top right corner that shows your login name.
       * e.g. *arn:aws:iot:eu-west-1:012345678901:*
     * Replace the part, or add, after the last colon (`:`) with `*`.
       * e.g. *arn:aws:iot:eu-west-1:012345678901:\**
     * Click on **Create**.

<br>

   > NOTE - The examples in this document are intended for development environments only.  All devices in your production fleet must have credentials with privileges that authorize only intended actions on specific resources. The specific permission policies can vary for your use case. Identify the permission policies that best meet your business and security requirements.  For more information, refer to Example policies and Security Best practices of your Cloud-Service-Provider.

<br>

4. In the left navigation pane of the AWS IoT console, choose **Secure**, and then choose **Certificates**. You should see the certificate that you have created earlier.
   * Use the ID in the front of the certificate and key files that you downloaded earlier to identify your certificate.
   * Click on your certificate name to take you to your certificate.
6. Click on **Actions** -> **Attach policy** or look down the page and click on **Attach policies**.
7. In the **Attach policies to certificate(s)** window
   * Find your policy and click on it it.
     * Even though you may enable more than one policy, for now we use the single policy you created earlier.
   * Click **Attach policies**.
   * You will now see your policy listed on the Certificate page.

## Configure the application to connect to your AWS account
Now that you have created an AWS Thing and attached the certificates and policies to it, the representative values must be added to your application to ensure connectivity with your AWS account.

Within the application directory that you are using, edit the `bsp/default_credentials/aws_clientcredential.h` file and set values for specified user defines called out below.

`clientcredentialMQTT_BROKER_ENDPOINT`

* Set this to the Device data endpoint name of your amazon account.
* To find this go to the navigation pane of the [AWS IoT console](https://console.aws.amazon.com/iotv2/), choose **Settings** (bottom left hand corner).
* On the **Settings** page, in the **Device data endpoint** section of the page look for **Endpoint**.  (e.g. `a3xyzzyx-ats.iot.us-east-2.amazonaws.com`).
  * Note the region may be different than these instructions.  It should match where your thing and policy were created due to the MQTT Topic restrictions discussed above.

`clientcredentialIOT_THING_NAME`

* Set this to the name of the thing you set (e.g. MyThing).

Save and close the file.


Next insert the keys that are in the certificates you have downloaded when you created the thing. Edit the file `bsp/default_credentials/aws_clientcredential_keys.h` replacing the existing keys with yours.

`keyCLIENT_CERTIFICATE_PEM`

* Replace with contents from `<your-thing-certificate-unique-string>-certificate.pem.crt`.

`keyCLIENT_PRIVATE_KEY_PEM`

* Replace with contents from `<your-thing-certificate-unique-string>-private.pem.key`.

`keyCLIENT_PUBLIC_KEY_PEM`

* Replace with contents from `<your-thing-certificate-unique-string>-public.pem.key`.

Save all files and rebuild the application.

## Observing MQTT connectivity

To see messages being sent by the application:
1. Login to your account and browse to the [AWS IoT console](https://console.aws.amazon.com/iotv2/).
2. In the left navigation panel, choose **Manage**, and then choose **Things**.
3. Select the thing you created, and open the **Activity** tab. This will show the application connecting and subscribing to a topic.
4. Click on the **MQTT test client** button. This will open a new page.
5. Click on **Subscribe to a topic**.
6. In the **Subscription topic** field enter the topic name which is a concatenation of the name of your thing (set in `clientcredentialIOT_THING_NAME`) and `/ml/inference`
   * e.g. if you thing name is MyThing then it's `MyThing/ml/inference`
8. In the **MQTT payload display** combo box select `Display payloads as strings (more accurate)`
9. Click the **Subscribe** button. The messages will be shown below within this same page.

# OTA firmware update

Total-Solution applications that have CSP connectivity enabled may also have Over-The-Air (OTA) update functionality. The application will check for updates from the AWS Cloud at boot time to check if there is an update pending.  If an update is available, the application will stop ML processing, download the new firmware, and then apply the new firmware if the version number indicates the image is newer. To make such a version available you need to prepare the update binary (this is part of the build process) and create an OTA job on AWS.

## Creating updated firmware

As part of the application build process, an updated firmware image will be created that will only differ in version number. That is enough to demonstrate the OTA process using a newly created image.

If you want to add other changes you should copy the original binary elsewhere before running the build again with your changes as the same build directory is used for both.  This is to ensure you have the original binary to compare against any new version you build.

For example, the updated binary is placed in `build/kws/kws_signed_update.bin`. The updated binary is already signed and it is the file you will need to upload to the Amazon S3 bucket in the next section.

Upon completion of the build and signing process the <ins>signature string will be echoed to the terminal</ins>. This will be needed in the next step.

## Creating AWS IoT firmware update job

The instructions below use the keyword spotting name, kws, as an example.  Replace kws with the application name in the build instructions that you followed.

1. Follow the instructions at: [Create an Amazon S3 bucket to store your update](https://docs.aws.amazon.com/freertos/latest/userguide/dg-ota-bucket.html)
  * Use the default options wherever you have a choice.
  * For simplicity, use the same region for the bucket as where your Instance is located.
2. Follow the instructions at: [Create an OTA Update service role](https://docs.aws.amazon.com/freertos/latest/userguide/create-service-role.html)
3. Follow the instructions at: [Create an OTA user policy](https://docs.aws.amazon.com/freertos/latest/userguide/create-ota-user-policy.html)
4. Go to AWS IoT web interface and choose **Manage** and then **Jobs**
5. Click the create job button and select **Create FreeRTOS OTA update job**
6. Give it a name and click next
7. Select the device to update (the Thing you created in earlier steps)
8. Select `MQTT` transport only
9. Select **Use my custom signed file**
10. Select upload new file and select the signed update binary (`build/kws/kws_signed_update.bin`)
11. Select the S3 bucket you created in step 1. to upload the binary to
12. Paste the signature string that is echoed during the build of the example (it is also available in `build/kws/update-signature.txt`).
13. Select `SHA-256` and `RSA` algorithms.
14. For **Path name of code signing certificate on device** put in `0` (the path is not used)
15. For **Path name of file on device** put in `non_secure image`
16. As the role, select the OTA role you created in step 2.
17. Click next
18. Create an ID for you Job
19. Add a description
20. **Job type**, select *Your job will complete after deploying to the selected devices/groups (snapshot).*
21. Click next, your update job is ready and running - next time your application connects it will perform the update.

<br>

# Setting up Azure connectivity

The Keyword Detection application can connect to an Azure IoT Hub and report ML inference results through that connection.

To connect to the Azure IoT Hub cloud service you will need to setup a device and then set the device connection string within the Application. You will need to create an [Azure Account](https://azure.microsoft.com/) if you don’t already have one.


## Azure IoT Hub setup

To create a new Azure IoT Hub and one device within it through the web portal follow the [instructions](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-create-through-portal) provided by Azure.

## Configure the application to connect to your Azure IoT Hub

Now that you have created a device in your IoT Hub, the application must be configured to connect to the Azure IoT Hub with the credentials of the device created.

Within the application directory that you are using, edit the `bsp/default_credentials/iothub_credentials.h` file.

You must set the define `IOTHUB_DEVICE_CONNECTION_STRING` to the value of the device's `Primary Connection String`. This value can be [retrieved](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-create-through-portal#register-a-new-device-in-the-iot-hub) in the portal.


## Build the application to connect to your Azure IoT Hub

The application selects a cloud client (Aws or Azure) at build time. This is achieved by adding the flag `-e <AZURE|AWS>` to the build command line.
To build a version of kws connecting to the Azure cloud on Corstone-300 and using FreeRTOS, use the following command line:

```sh
./ats.sh build kws -e AZURE
```

## Monitoring messages sent to your Azure IoT Hub

The Azure web portal does not offer monitoring facilities to visualize packets received out of the box.
To monitor packets, you can use the [Azure IoT Tools](https://marketplace.visualstudio.com/items?itemName=vsciot-vscode.azure-iot-tools) VS Code extension and follow these [instructions](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-vscode-iot-toolkit-cloud-device-messaging#monitor-device-to-cloud-messages).

To monitor activity (connection, disconnection, ...) follow the [reference instructions](https://docs.microsoft.com/en-us/azure/iot-hub/monitor-iot-hub).

# Terminate Arm Virtual Hardware Instance

When you are done using the AMI instance at the end of the day, you need to make sure you shut it down properly or you may be charged for usage you did not actually use.  There are 2 ways to do this action (pick one):

## Stopping the instance in EC2
1. Go to [EC2](https://console.aws.amazon.com/ec2/v2/) in the AWS Web Console.
2. Select the instance to stop.
3. Click on `Instance state` and select `Stop Instance` in the drop down menu.

## Stopping the instance using a local terminal
Execute the following script located in the application repository.

```sh
./scripts/vht_cli.py -k MyKeyPair stop
```

# Source code overview

- `bsp`: Arm Corstone-300 subsystem platform code and AWS configurations.
- `lib`: Middleware used by IoT Total Solution.
  - `AWS`: OTA and PKCS11 integration with AWS IoT SDK.
  - `VHT`: Virtual streaming solution for Arm Virtual Hardware.
- `blinky`: Blinky application.
  - `blinky/main_ns.c`: Entry point of the blinky application
- `kws`: Keyword detection application.
  - `kws/source/main_ns.c`: Entry point of the kws application.
  - `kws/source/aws_demo.c`: AWS IoT specific code of the kws application.
  - `kws/source/azure_demo.c`: Azure IoT Hub specific code of the kws application.
  - `kws/source/blinky_task.c`: Blinky/UX thread of the application.
  - `kws/source/ml_interface.c`: Interface between the virtual streaming solution and tensor flow.
  - `kws/source/ethosu_platform_adaptation.c`: RTOS adapatation for the Ethos U55.
  - `kws/ota`: Connector between AWS OTA and the application.
- `mlia`: Integration the ML Inference Advisor, using a simple wrapper script.sh to install and run the tool on given models.

# ML Model Replacement

All the ML models supported by the [ML Embedded Eval Kit](All the models supported ) are available to applications. The first step to use another module is to generate sources files from its labels and `.tflite` model.

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

# ML Embedded Eval Kit

The Arm ML Embedded Evaluation Kit , is an open-source repository enabling users to quickly build and deploy embedded machine learning applications for Arm Cortex-M55 CPU and Arm Ethos-U55 NPU.

With ML Eval Kit you can run inferences on either a custom neural network on Ethos-U microNPU or using availble ML applications such as [Image classification](https://review.mlplatform.org/plugins/gitiles/ml/ethos-u/ml-embedded-evaluation-kit/+/c930ad9dc189d831ac77f716df288f70178d4c10/docs/use_cases/img_class.md), [Keyword spotting (KWS)](https://review.mlplatform.org/plugins/gitiles/ml/ethos-u/ml-embedded-evaluation-kit/+/c930ad9dc189d831ac77f716df288f70178d4c10/docs/use_cases/kws.md), [Automated Speech Recognition (ASR)](https://review.mlplatform.org/plugins/gitiles/ml/ethos-u/ml-embedded-evaluation-kit/+/c930ad9dc189d831ac77f716df288f70178d4c10/docs/use_cases/asr.md), [Anomaly Detection](https://review.mlplatform.org/plugins/gitiles/ml/ethos-u/ml-embedded-evaluation-kit/+/c930ad9dc189d831ac77f716df288f70178d4c10/docs/use_cases/ad.md), and [Person Detection](https://review.mlplatform.org/plugins/gitiles/ml/ethos-u/ml-embedded-evaluation-kit/+/HEAD/docs/use_cases/visual_wake_word.md) all using Arm Fixed Virtual Platform (FVP) available in Arm Virtual Hardware.

# Known limitations

- Arm compiler 6 is the only compiler supported.
- Accuracy of the ML detection running alongside cloud connectivity depends on the performance of the EC2 instance used. We recommend to use at least a t3.medium instance.
- Arm Corstone-300 subsystem simulation is not time accurate. Performances will differ depending on the performances of the host machine.


# Future Enhancements
- [AWS Partner Device Catalog Listing](https://devices.amazonaws.com/) (leveraging Arm Virtual Hardware)


# Other Resources

| Repository                                                                                                    | Description                                                                                                                      |
|---------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| [Arm AI Ecosystem Catalog](https://www.arm.com/why-arm/partner-ecosystem/ai-ecosystem-catalog)                | Connects you to the right partners, enabling you to build the next generation of AI solutions                                    |
| [Arm IoT Ecosystem Catalog](https://www.arm.com/why-arm/partner-ecosystem/iot-ecosystem-catalog)              | Explore Arm IoT Ecosystem partners who can help transform an idea into a secure, market-leading device.                          |
| [Arm ML Model Zoo](https://github.com/ARM-software/ML-zoo)                                                    | A collection of machine learning models optimized for Arm IP.                                                                    |
| [Arm Virtual Hardware Documentation](https://mdk-packs.github.io/VHT-TFLmicrospeech/overview/html/index.html) | Documentation for [Arm Virtual Hardware](https://www.arm.com/products/development-tools/simulation/virtual-hardware)             |
| [Arm Virtual Hardware source code](https://github.com/ARM-software/VHT)                                       | Source code of[Arm Virtual Hardware](https://www.arm.com/products/development-tools/simulation/virtual-hardware)                 |
| [FreeRTOS Documentation](https://freertos.org/a00104.html#getting-started)                                    | Documentation for FreeRTOS.                                                                                                      |
| [FreeRTOS source code](https://github.com/FreeRTOS)                                                           | Source code of FreeRTOS.                                                                                                         |
| [AWS IoT](https://docs.aws.amazon.com/iot/latest/developerguide/what-is-aws-iot.html)                         | Documentation for AWS IoT.                                                                                                       |
| [Trusted Firmware-M Documentation](https://tf-m-user-guide.trustedfirmware.org/)                              | Documentation for Trusted Firmware-M.                                                                                            |
| [Trusted Firmware-M Source code](https://git.trustedfirmware.org/TF-M/trusted-firmware-m.git)                 | Source code of Trusted Firmware-M.                                                                                               |
| [Mbed Crypto](https://github.com/ARMmbed/mbedtls)                                                             | Mbed Crypto source code.                                                                                                         |
| [MCU Boot](https://github.com/mcu-tools/mcuboot)                                                              | MCU Boot source code.                                                                                                            |
| [ml-embedded-evaluation-kit](https://review.mlplatform.org/plugins/gitiles/ml/ethos-u/ml-embedded-evaluation-kit) | ML Embedded eval kit source code                                                                                             |
| Support                                                                                                       | A [community.arm.com](http://community.arm.com/) forum exists for users to post queries.                                         |

# License and contributions

The software is provided under the Apache-2.0 license. All contributions to software and documents are licensed by contributors under the same license model as the software/document itself (e.g. inbound == outbound licensing). Open IoT SDK may reuse software already licensed under another license, provided the license is permissive in nature and compatible with Apache v2.0.

Folders containing files under different permissive license than Apache 2.0 are listed in the LICENSE file.

# Security

Information on security considerations for an end user application can be found [here](https://gitlab.arm.com/iot/open-iot-sdk/sdk/-/blob/main/docs/guidelines/Security.md).
## Security issues reporting

If you find any security vulnerabilities, please do not report it in the GitLab issue tracker. Instead, send an email to the security team at arm-security@arm.com stating that you may have found a security vulnerability in the IoT Total Solution Keyword Detection project.

More details can be found at [Arm Developer website](https://developer.arm.com/support/arm-security-updates/report-security-vulnerabilities).
