# Overview

This file contains the instructions to build and execute the Arm Total-Solutions applications using Arm Virtual Hardware with connectivity to Cloud Service providers.  For specific application instructions (when applicable) refer to the application specific documentation for more information.

<br>

# Quick Start

Follow these simple steps to build and execute the code example's application within **Arm Virtual Hardware**.

* [Launch Arm Virtual Hardware system](#Launch-Arm-Virtual-Hardware-Instance)
* [Build and execute](#Build-and-execute-the-application)
* [Setting up Cloud connectivity](#Setting-up-AWS-connectivity)
* [Enabling OTA firmware update from the Cloud](#OTA-firmware-update)
* [Terminating Arm Virtual Hardware](#Terminate-Arm-Virtual-Hardware-Instance)

# Launch Arm Virtual Hardware Instance

There are 2 ways to launch the **Arm Virtual Hardware Instance**, choose one that best fits your work style.  For first timers, we recommend using option 1.
1. [AWS Web Console launch](#launch-using-aws-web-console)
2. [Local Terminal launch](#launch-using-a-local-terminal)


## Launch Using AWS Web Console

To utilize the Arm Virtual Hardware, you will need to create an [AWS Account](https://aws.amazon.com/premiumsupport/knowledge-center/create-and-activate-aws-account/) if you don’t already have one.

### Launching the instance in EC2 [(AWS on getting started)](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/EC2_GetStarted.html)

1. Go to [EC2](https://console.aws.amazon.com/ec2/v2/) in the AWS Web Console.
1. Select **Launch Instance** which will take you to a wizard for launching the instance.

     * **Step 1: Create a Name for your Instance** - To clearly identify the instance you are about to create you will need to apply a descriptive name.  It can be as simple as "J. Doe's AVH Instance".

     * **Step 2: Choose an [Amazon Machine Image (AMI)](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/AMIs.html)**
        > Arm Virtual Hardware for Corstone-300 is available as a public beta on AWS Marketplace. To help you get started, AWS are offering more than 100 hours of free AWS EC2 CPU credits for the first 1,000 qualified users. Click here to find out more: https://www.arm.com/company/contact-us/virtual-hardware.

        * In the Search box, type `Arm Virtual Hardware` and then hit "enter" to find the item called <ins>"Arm Virtual Hardware" - "By Arm"</ins>.
          * NOTE: If you dont see the expected items, make sure the <ins>**AWS Marketplace AMIs**</ins> tab is selected.
        * Click on "Select" for that item. This image contains all the software necessary to build and run the Arm IoT Total Solutions.
          * This will raise a subscription page/pop-up titled, **Arm Virtual Hardware**.
          * You will note that the subscription is free from Arm, but <ins>AWS does charge for the costs of the instances themselves according to the pricing chart provided.</ins>
        * You must select "Continue" if you want to move forward.

     * **Step 3: Choose an Instance Type** - Select one of the instance types from the list. 
        * We recommend the **c5.large**. 
        * **Important:** Charges accrue while the instance is running and to a lesser degree when stopped.  
        * Terminating the instance stops any charges from occurring.

     * **Step 4: Key Pair Creation** - To securely connect to the instance you will need to create a key pair.
         * Click on "Generate new key pair"
         * Enter a name for the key pair.  **DO NOT USE SPACES**
           * Again its recommended to use a descriptive name such as J_D_Keys.
         * Stay with the defaults or feel free to change the type or format of the key.
         * Click on "Create key pair"
         * The page will automatically download or prompt you to save the PRIVATE key.
           * <ins>Place this in a secure location as you will need it later!</ins>

     * **Step 5: Configure storage** - To ensure enough disk drive space to contain the entire build image.  Set the amount of storage to "1x **24** GiB".


      * **Final Step:** From here you may select **Review and Launch** to move directly to the launch page or continue to configure instance details if you need to set any custom settings for this instance.


### Selecting the instance
Once you complete the wizard by initiating the instance launch you will see a page that allows you to navigate directly to the new instance. You may click this link or go back to your list of instances and find the instance through that method.

Whichever way you choose, find your new instance and select its instance ID to open the page to manage the instance.

**Note:** Sometimes it takes a few minutes for the Instance to spin up completely. To check the status go to Instances page, find your instance and look under the "Status Check" column, it should say something similar to "2/2 checks passed" instead of "initializing".

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


3. Create a new key pair.

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

```sh
ssh -i "MyKeyPair.pem" ubuntu@<instance ip address>
```

# Connect a terminal to the instance 
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
```sh
ssh ubuntu@<your-ec2-instance>
```
      * If you do not know your instance value, refer to the **AWS-Web-Console** instructions just above to get this information.

<br>

# Build and execute the application
Due to the ever increasing number of applications, specific instructions exist for each application.  Refer back to the application that you have selected, for build and execution instruction.

# Setting up AWS connectivity

All Total-Solution applications that have CSP connectivity enabled will attempt to connect to AWS IOT and report ML inference results through an MQTT connection. 

To connect to the AWS cloud service you will need to setup an IoT Thing and then set the AWS credentials of the IoT Thing within the Application. You will need to create an [AWS Account](https://aws.amazon.com/premiumsupport/knowledge-center/create-and-activate-aws-account/) if you don’t already have one.

## AWS account IoT setup

### Create an IoT thing for your device

1. Login to your account and browse to the [AWS IoT console](https://console.aws.amazon.com/iotv2/).
2. In the left navigation pane, choose **Manage**, and then choose **Things**.
3. If you do not have any IoT things registered in your account, the **You don’t have any things yet** page is displayed. If you see this page, choose **Register** a thing.
4. On the **Creating AWS IoT things** page, choose **Create a single thing**.
5. On the **Add your device to the thing registry** page, type a name for your thing (for example `MyThing`), and then choose **Next**. You will need to add the name later to your C code.
6. On the **Add a certificate for your thing** page, under **One-click certificate creation**, choose **Create certificate**.
7. Download your private key and certificate by choosing the **Download** links for each. Make note of the certificate ID. You need it later to attach a policy to your certificate.
8. Choose **Activate** to activate your certificate. Certificates must be activated prior to use.

### Create a policy and attach it to your thing

1. In the navigation pane of the AWS IoT console, choose **Secure**, and then choose **Policies**.
2. On the **Policies** page, choose **Create** (top right corner).
3. On the **Create a policy** page, enter a name for the policy. In the **Action** box, enter **iot:Connect**, **iot:Publish**, **iot:Subscribe**, **iot:Receive**. The **Resource ARN** box will be auto-filled with your credentials. Replace the part after the last colon (`:`) with `*`. Under **Effect**, check the **Allow** box. Click on **Create**.

   > NOTE – The examples in this document are intended for development environments only.  All devices in your production fleet must have credentials with privileges that authorize only intended actions on specific resources. The specific permission policies can vary for your use case. Identify the permission policies that best meet your business and security requirements.  For more information, refer to Example policies and Security Best practices of your Cloud-Service-Provider.

4. In the left navigation pane of the AWS IoT console, choose **Secure**, and then choose **Certificates**. You should see the certificate that you have created earlier.
5. Click on the three dots in the upper right corner of the certificate and choose **Attach policy**.
6. In the **Attach policies to certificate(s)** window, enable the policy that you have just created and click **Attach**.

## Configure the application to connect to your AWS account
Within the application directory you previously built the program code, edit the `bsp/default_credentials/aws_clientcredential.h` file and set values of specified user defines.

`clientcredentialMQTT_BROKER_ENDPOINT`

Set this to the Device data endpoint name of your amazon account. To find this go to the navigation pane of the [AWS IoT console](https://console.aws.amazon.com/iotv2/), choose **Settings**. On the **Settings** page, copy the name of your **Device data endpoint** (such as `a3xyzzyx.iot.us-east-2.amazonaws.com`).

`clientcredentialIOT_THING_NAME`

Set this to the name of the thing you set (e.g. MyThing).

Next insert the keys that are in the certificates you have downloaded when you created the thing. Edit the file `bsp/default_credentials/aws_clientcredential_keys.h` replacing the existing keys with yours.

`keyCLIENT_CERTIFICATE_PEM`

Replace with contents from `<your-thing-certificate-unique-string>-certificate.pem.crt`.

`keyCLIENT_PRIVATE_KEY_PEM`

Replace with contents from `<your-thing-certificate-unique-string>-private.pem.key`.

`keyCLIENT_PUBLIC_KEY_PEM`

Replace with contents from `<your-thing-certificate-unique-string>-public.pem.key`.

## Observing MQTT connectivity

To see messages being sent by the application:
1. Login to your account and browse to the [AWS IoT console](https://console.aws.amazon.com/iotv2/).
2. In the left navigation panel, choose **Manage**, and then choose **Things**.
3. Select the thing you created, and open the **Activity** tab. This will show the application connecting and subscribing to a topic.
4. Click on the **MQTT test client** button. This will open a new tab.
5. The tab **Subscribe to a topic** should be already selected. Open the **Additional configuration** rollout.
6. In the topic filter field enter the topic name which is a concatenation of the name of your thing (set in `clientcredentialIOT_THING_NAME`) and `/ml/inference` (e.g. if you thing name is MyThing then it's `MyThing/ml/inference`)
7. In the **MQTT payload display** combo box select `Display payloads as strings (more accurate)`
8. Click the **Subscribe** button. The messages will be shown below.

# OTA firmware update

Total-Solution applications that have CSP connectivity enabled may also have Over-The-Air (OTA) update functionality. The application will check for updates from the AWS Cloud at boot time to check if there is an update pending.  If an update is available, the application will stop ML processing, download the new firmware, and then apply the new firmware if the version number indicates the image is newer. To make such a version available you need to prepare the update binary (this is part of the build process) and create an OTA job on AWS.

## Creating updated firmware

As part of the application build process, an updated firmware image will be created that will only differ in version number. That is enough to demonstrate the OTA process using a newly created image. 

If you want to add other changes you should copy the original binary elsewhere before running the build again with your changes as the same build directory is used for both.  This is to ensure you have the original binary to compare against any new version you build.

For example, the updated binary is placed in `build/kws/kws_signed_update.bin`. The updated binary is already signed and is the file you will need to upload to the Amazon S3 bucket in the next section.

Upon completion of the build and signing process the <ins>signature string will be echoed to the terminal</ins>. This will be needed in the next step.

## Creating AWS IoT firmware update job

The instructions below use the keyword spotting name, kws, as an example.  Replace kws with the application name in the build instructions that you followed.

1. [Create an Amazon S3 bucket to store your update](https://docs.aws.amazon.com/freertos/latest/userguide/dg-ota-bucket.html)
2. [Create an OTA Update service role](https://docs.aws.amazon.com/freertos/latest/userguide/create-service-role.html)
3. [Create an OTA user policy](https://docs.aws.amazon.com/freertos/latest/userguide/create-ota-user-policy.html)
4. Go to AWS IoT web interface and choose **Manage** and then **Jobs**
5. Click the create job button and select **Create FreeRTOS OTA update job**
6. Give it a name and click next
7. Select the device to update (the Thing you created in earlier steps)
8. Select `MQTT` transport only
9. Select **Use my custom signed file**
10. Paste the signature string that is echoed during the build of the example (it is also available in `build/kws/update-signature.txt`).
11. Select `SHA-256` and `RSA` algorithms.
12. For **Path name of code signing certificate on device** put in `0` (the path is not used)
13. Select upload new file and select the signed update binary (`build/kws/kws_signed_update.bin`)
14. Select the S3 bucket you created in step 1. to upload the binary to
15. For **Path name of file on device** put in `non_secure image`
16. As the role, select the OTA role you created in step 2.
17. Click next
18. Click next, your update job is ready and running - next time your application connects it will perform the update.

<br>

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

