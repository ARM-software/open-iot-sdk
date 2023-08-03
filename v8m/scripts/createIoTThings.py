#!/usr/bin/python3

#  Copyright (c) 2023 Arm Limited. All rights reserved.
#  SPDX-License-Identifier: Apache-2.0
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

import os
import sys
import time
import traceback as tb

# JSON is used for policy creation
import json
from ast import literal_eval
import boto3
import botocore
import click
import logging

DEFAULT_LOG_LEVEL = "warning"
DEFAULT_BUILD_DIR = "build"
DEFAULT_OTA_BINARY = "keyword_signed_update.bin"
DEFAULT_OTA_BINARY_BUILD_DIR = "examples/keyword"
DEFAULT_OTA_UPDATE_SIGNATURE_FILENAME = "update-signature.txt"
OTA_JOB_NAME_PREFIX = "AFR_OTA-"

AWS_REGION = os.getenv("AWS_REGION")

if not AWS_REGION:
    raise ValueError("AWS_REGION is not set in environment")

iot = boto3.client("iot", AWS_REGION)
s3 = boto3.client("s3", AWS_REGION)
iam = boto3.client("iam", AWS_REGION)
sts_client = boto3.client("sts", AWS_REGION)


@click.group()
def cli():
    pass


def read_whole_file(path, mode="r"):
    with open(path, mode) as fp:
        return fp.read()


class Flags:
    def __init__(
        self,
        bucket_name="default",
        role_name="default",
        build_dir=DEFAULT_BUILD_DIR,
        ota_binary=DEFAULT_OTA_BINARY,
        ota_binary_build_dir=DEFAULT_OTA_BINARY_BUILD_DIR,
        ota_update_signature_filename=DEFAULT_OTA_UPDATE_SIGNATURE_FILENAME,
    ):
        self.BUILD_DIR = build_dir
        self.AWS_ACCOUNT_ID = boto3.client("sts").get_caller_identity().get("Account")
        self.AWS_ACCOUNT_ARN = boto3.client("sts").get_caller_identity().get("Arn")
        self.THING_NAME = None
        self.THING_ARN = None
        self.POLICY_NAME = None
        self.OTA_BUCKET_NAME = bucket_name
        self.OTA_ROLE_NAME = role_name
        self.OTA_ROLE_ARN = None
        self.OTA_BINARY = ota_binary
        self.OTA_BINARY_FILE_PATH = os.path.join(
            self.BUILD_DIR, ota_binary_build_dir, self.OTA_BINARY
        )
        self.UPDATE_ID = None
        self.OTA_UPDATE_PROTOCOLS = ["MQTT"]
        self.OTA_UPDATE_TARGET_SELECTION = "SNAPSHOT"
        self.bucket = None
        self.file = None
        self.thing = None
        self.policy = None
        self.keyAndCertificate = None
        self.certificate = None
        self.privateKey = None
        self.publicKey = None
        self.endPointAddress = None
        self.role = None
        self.POLICY_DOCUMENT = {
            "Version": "2012-10-17",
            "Statement": [
                {
                    "Effect": "Allow",
                    "Action": [
                        "iot:Publish",
                        "iot:Receive",
                        "iot:Subscribe",
                        "iot:Connect",
                    ],
                    "Resource": [
                        "arn:aws:iot:" + AWS_REGION + ":" + self.AWS_ACCOUNT_ID + ":*"
                    ],
                }
            ],
        }
        self.ASSUME_ROLE_POLICY_DOCUMENT = {
            "Version": "2012-10-17",
            "Statement": [
                {
                    "Effect": "Allow",
                    "Principal": {
                        "AWS": self.AWS_ACCOUNT_ARN,
                        "Service": [
                            "iot.amazonaws.com",
                            "s3.amazonaws.com",
                            "iam.amazonaws.com",
                        ],
                    },
                    "Action": ["sts:AssumeRole"],
                }
            ],
        }
        self.IAM_OTA_PERMISSION = {
            "Version": "2012-10-17",
            "Statement": [
                {
                    "Effect": "Allow",
                    "Action": ["iam:GetRole", "iam:PassRole"],
                    "Resource": "arn:aws:iam::"
                    + self.AWS_ACCOUNT_ID
                    + ":role/"
                    + self.OTA_ROLE_NAME,
                }
            ],
        }
        self.S3_OTA_PERMISSION = {
            "Version": "2012-10-17",
            "Statement": [
                {
                    "Effect": "Allow",
                    "Action": ["s3:GetObjectVersion", "s3:GetObject", "s3:PutObject"],
                    "Resource": ["arn:aws:s3:::" + self.OTA_BUCKET_NAME + "/*"],
                }
            ],
        }
        self.OTA_UPDATE_FILES = [
            {
                "fileName": "non_secure image",
                "fileLocation": {
                    "s3Location": {
                        "bucket": self.OTA_BUCKET_NAME,
                        "key": self.OTA_BINARY,
                    }
                },
                "codeSigning": {
                    "customCodeSigning": {
                        "signature": {
                            "inlineDocument": bytearray(
                                read_whole_file(
                                    os.path.join(
                                        self.BUILD_DIR,
                                        ota_binary_build_dir,
                                        ota_update_signature_filename,
                                    )
                                ).strip(),
                                "utf-8",
                            )
                        },
                        "certificateChain": {"certificateName": "0"},
                        "hashAlgorithm": "SHA256",
                        "signatureAlgorithm": "RSA",
                    },
                },
            }
        ]


def set_log_level(loglevel):
    logging.basicConfig(level=loglevel.upper())


class StdCommand(click.core.Command):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.params.insert(
            0,
            click.core.Option(
                ("--log_level",),
                help="Provide logging level. Example --log_level debug, default="
                + DEFAULT_LOG_LEVEL,
                default=DEFAULT_LOG_LEVEL,
            ),
        )

    def invoke(self, ctx):
        set_log_level(ctx.params["log_level"])
        super().invoke(ctx)


# same as the above, but with the default log level set at info, so that listing functions do print by default
class ListingCommand(click.core.Command):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.params.insert(
            0,
            click.core.Option(
                ("--log_level",),
                help="Provide logging level. Example --log_level debug, default="
                + "info",
                default="info",
            ),
        )

    def invoke(self, ctx):
        set_log_level(ctx.params["log_level"])
        super().invoke(ctx)


def _does_thing_exist(thing_name):
    try:
        iot.describe_thing(thingName=thing_name)
    except botocore.exceptions.ClientError as ex:
        if ex.response["Error"]["Code"] == "ResourceNotFoundException":
            return False
        else:
            raise ex
    return True


def _does_policy_exist(policy_name):
    try:
        iot.get_policy(policyName=policy_name)
    except botocore.exceptions.ClientError as ex:
        if ex.response["Error"]["Code"] == "ResourceNotFoundException":
            return False
        else:
            raise ex
    return True


def _does_job_exist(job_name):
    try:
        iot.describe_job(jobId=job_name)
    except botocore.exceptions.ClientError as ex:
        if ex.response["Error"]["Code"] == "ResourceNotFoundException":
            return False
        else:
            raise ex
    return True


def _does_bucket_exist(bucket_name):
    try:
        s3.get_bucket_location(Bucket=bucket_name)
    except botocore.exceptions.ClientError as ex:
        if ex.response["Error"]["Code"] == "NoSuchBucket":
            return False
        else:
            raise ex
    return True


def _does_role_exist(iam_role_name):
    try:
        iam.get_role(RoleName=iam_role_name)
    except botocore.exceptions.ClientError as ex:
        if ex.response["Error"]["Code"] == "NoSuchEntity":
            return False
        else:
            raise ex
    return True


# Write to credential files
def _write_credentials(flags: Flags, credentials_path):
    fileDir = os.path.dirname(os.path.realpath("__file__"))
    keyFileTemplate = os.path.join(
        fileDir, "scripts/templates", "aws_clientcredential_keys_template.h"
    )
    credentialFileTemplate = os.path.join(
        fileDir, "scripts/templates", "aws_clientcredential_template.h"
    )
    keyFile = os.path.join(fileDir, credentials_path, "aws_clientcredential_keys.h")
    credentialFile = os.path.join(fileDir, credentials_path, "aws_clientcredential.h")
    with open(keyFileTemplate, "r") as file:
        filedata = file.read()
        rawdata = repr(filedata)
    if r'"client certificate\\n"\\\n' in rawdata:
        rawdata = rawdata.replace(
            r'"-----BEGIN CERTIFICATE-----\\n"\\\n    "client certificate\\n"\\\n    "-----END CERTIFICATE-----"\n\n\n',
            flags.certificate,
        )
    if r'"client private key\\n"\\\n' in rawdata:
        rawdata = rawdata.replace(
            r'"-----BEGIN RSA PRIVATE KEY-----\\n"\\\n    "client private key\\n"\\\n    "-----END RSA PRIVATE KEY-----"\n\n\n',
            flags.privateKey,
        )
    if r'"client public key\\n"\\\n' in rawdata:
        rawdata = rawdata.replace(
            r'"-----BEGIN PUBLIC KEY-----\\n"\\\n    "client public key\\n"\\\n    "-----END PUBLIC KEY-----"\n\n',
            flags.publicKey,
        )
    with open(keyFile, "w") as file:
        filedata = literal_eval(rawdata)
        file.write(filedata)
    with open(credentialFileTemplate, "r") as file:
        filedata = file.read()
        rawdata = repr(filedata)
    if r'"endpointid.amazonaws.com"' in rawdata:
        rawdata = rawdata.replace(
            r"endpointid.amazonaws.com", flags.endPointAddress["endpointAddress"]
        )
    if r'"thingname"' in rawdata:
        rawdata = rawdata.replace(r"thingname", flags.THING_NAME)
    with open(credentialFile, "w") as file:
        filedata = literal_eval(rawdata)
        file.write(filedata)


# Format certificate i.e adding double quotations and next line
def _parse_credentials(flags: Flags):
    flags.certificate = repr(flags.keyAndCertificate["certificatePem"]).replace(
        r"'", r'"'
    )
    flags.certificate = flags.certificate.replace(r"\n", r'\\n"\\\n    "')
    flags.certificate = flags.certificate.replace(
        r'"-----END CERTIFICATE-----\\n"\\\n    ""',
        r'"-----END CERTIFICATE-----"\n\n\n',
    )
    flags.privateKey = repr(flags.keyAndCertificate["keyPair"]["PrivateKey"]).replace(
        r"'", r'"'
    )
    flags.privateKey = flags.privateKey.replace(r"\n", r'\\n"\\\n    "')
    flags.privateKey = flags.privateKey.replace(
        r'"-----END RSA PRIVATE KEY-----\\n"\\\n    ""',
        r'"-----END RSA PRIVATE KEY-----"\n\n\n',
    )
    flags.publicKey = repr(flags.keyAndCertificate["keyPair"]["PublicKey"]).replace(
        r"'", r'"'
    )
    flags.publicKey = flags.publicKey.replace(r"\n", r'\\n"\\\n    "')
    flags.publicKey = flags.publicKey.replace(
        r'"-----END PUBLIC KEY-----\\n"\\\n    ""', r'"-----END PUBLIC KEY-----"\n\n\n'
    )


def _create_credentials(flags: Flags, credentials_path):
    try:
        # KeyAndCertificate is Dictionary
        flags.keyAndCertificate = iot.create_keys_and_certificate(setAsActive=True)
        flags.endPointAddress = iot.describe_endpoint(endpointType="iot:Data-ATS")
        _parse_credentials(flags)
        _write_credentials(flags, credentials_path)
        logging.info(
            "New certificate ARN: " + flags.keyAndCertificate["certificateArn"]
        )
    except Exception as e:
        logging.error("failed creating a certificate")
        print_exception(e)
        cleanup_aws_resources(flags)
        raise e
    return True


# Create the credentials if needed, or set up to use existing credentials
def _get_credential_arn(flags: Flags, use_existing_certificate_arn, credentials_path):
    if use_existing_certificate_arn == "":
        if not _create_credentials(flags, credentials_path):
            cleanup_aws_resources(flags)
            exit(1)
        certificate_arn = flags.keyAndCertificate["certificateArn"]
    else:
        certificate_arn = use_existing_certificate_arn
        flags.endPointAddress = iot.describe_endpoint(endpointType="iot:Data-ATS")
    return certificate_arn


# Create test thing with policy attached.
def _create_thing(flags: Flags, Name, certificate_arn):
    try:
        if _does_thing_exist(flags.THING_NAME):
            # Then there's already a thing with that name.
            logging.warning("The thing named " + Name + " already exist")
            return False

        flags.thing = iot.create_thing(thingName=flags.THING_NAME)
        logging.info("Created thing:" + flags.THING_NAME)

        iot.attach_thing_principal(
            thingName=flags.THING_NAME,
            principal=certificate_arn,
        )
        logging.info(
            "Attached Certificate: "
            + flags.endPointAddress["endpointAddress"]
            + " to Thing: "
            + flags.THING_NAME
        )
    except botocore.exceptions.ClientError as ex:
        if ex.response["Error"]["Code"] == "ResourceNotFoundException":
            logging.error("No certificate found with the ARN: " + certificate_arn)
            return False
    except Exception as e:
        logging.error("failed creating a thing")
        print_exception(e)
        cleanup_aws_resources(flags)
        raise e
    return True


@cli.command(cls=StdCommand)
@click.option(
    "--thing_name", prompt="Enter Thing Name", help="Name of Thing to be created."
)
@click.option(
    "--use_existing_certificate_arn",
    default="",
    help="Use the provided certificate ARN instead of creating a new one",
)
@click.option(
    "-a",
    "--credentials_path",
    default="bsp/default_credentials",
    show_default=True,
    help="The path where the credentials are stored. If not specified, credentials in bsp/default_credentials are updated",
)
@click.option(
    "--build_dir",
    help="Override the default build directory",
    default=DEFAULT_BUILD_DIR,
)
@click.option(
    "--ota_binary",
    help="Override the default kws OTA file used",
    default=DEFAULT_OTA_BINARY,
)
@click.option(
    "--ota_binary_build_dir",
    help="Override the default OTA build directory",
    default=DEFAULT_OTA_BINARY_BUILD_DIR,
)
@click.pass_context
def create_thing_only(
    ctx,
    thing_name,
    use_existing_certificate_arn,
    credentials_path,
    build_dir,
    ota_binary,
    ota_binary_build_dir,
    log_level,
):
    ctx.flags = Flags(
        build_dir=build_dir,
        ota_binary=ota_binary,
        ota_binary_build_dir=ota_binary_build_dir,
    )
    ctx.flags.THING_NAME = thing_name
    certificate_arn = _get_credential_arn(
        ctx.flags, use_existing_certificate_arn, credentials_path
    )

    if not _create_thing(ctx.flags, thing_name, certificate_arn):
        cleanup_aws_resources(ctx.flags)
        ctx.exit(1)
    ctx.exit(0)


# Create a policy
def _create_policy(flags: Flags, Name, certificate_arn):

    try:
        flags.POLICY_NAME = Name
        flags.policy = iot.create_policy(
            policyName=flags.POLICY_NAME,
            policyDocument=json.dumps(flags.POLICY_DOCUMENT),
        )

        iot.attach_principal_policy(
            policyName=flags.POLICY_NAME,
            principal=certificate_arn,
        )
        logging.info(
            "Attached Policy: "
            + flags.POLICY_NAME
            + " to Certificate: "
            + flags.endPointAddress["endpointAddress"]
        )
    except botocore.exceptions.ClientError as ex:
        if ex.response["Error"]["Code"] == "ResourceAlreadyExistsException":
            logging.warning("The policy named " + Name + " already exist")
            return False
        elif ex.response["Error"]["Code"] == "ResourceNotFoundException":
            logging.error("No certificate found with the ARN: " + certificate_arn)
            return False
        else:
            logging.error("failed creating a policy")
            print_exception(ex)
            cleanup_aws_resources(flags)
            raise ex
    except Exception as e:
        logging.error("failed creating a policy")
        print_exception(e)
        cleanup_aws_resources(flags)
        raise e
    return True


@cli.command(cls=StdCommand)
@click.option(
    "--policy_name", prompt="Enter Policy Name", help="Name of Policy to be created."
)
@click.option(
    "--thing_name",
    help="If you create a new certificate, you must name the thing this credential will be attached to.",
)
@click.option(
    "--use_existing_certificate_arn",
    default="",
    help="Use the provided certificate ARN instead of creating a new one",
)
@click.option(
    "-a",
    "--credentials_path",
    default="bsp/default_credentials",
    show_default=True,
    help="The path where the credentials are stored. If not specified, credentials in bsp/default_credentials are updated",
)
@click.option(
    "--build_dir",
    help="Override the default build directory",
    default=DEFAULT_BUILD_DIR,
)
@click.option(
    "--ota_binary",
    help="Override the default kws OTA file used",
    default=DEFAULT_OTA_BINARY,
)
@click.option(
    "--ota_binary_build_dir",
    help="Override the default OTA build directory",
    default=DEFAULT_OTA_BINARY_BUILD_DIR,
)
@click.pass_context
def create_policy_only(
    ctx,
    policy_name,
    thing_name,
    use_existing_certificate_arn,
    credentials_path,
    build_dir,
    ota_binary,
    ota_binary_build_dir,
    log_level,
):
    ctx.flags = Flags(
        build_dir=build_dir,
        ota_binary=ota_binary,
        ota_binary_build_dir=ota_binary_build_dir,
    )
    if use_existing_certificate_arn == "":
        # then we create a new credential, and require a thing name
        if thing_name == "":
            ctx.flags.THING_NAME = thing_name
        else:
            logging.error(
                'No thing name specified. Use an existing certificate with "--use_existing_certificate_arn" or create a new one by specifying a thing name with "--thing_name"'
            )
            ctx.exit(1)

    certificate_arn = _get_credential_arn(
        ctx.flags, use_existing_certificate_arn, credentials_path
    )

    if not _create_policy(ctx.flags, policy_name, certificate_arn):
        cleanup_aws_resources(ctx.flags)
        ctx.exit(1)
    ctx.exit(0)


# Creating AWS IoT firmware update job
def _create_aws_bucket(flags: Flags, Name):
    if _does_bucket_exist(Name):
        logging.warning("The bucket named " + Name + " already exist")
        return False
    # checking existance and creating can conflict if run too close and throw an error
    time.sleep(1)
    try:
        flags.OTA_BUCKET_NAME = Name
        flags.bucket = s3.create_bucket(
            Bucket=flags.OTA_BUCKET_NAME,
            ACL="private",
            CreateBucketConfiguration={"LocationConstraint": AWS_REGION},
        )
        logging.info("Created bucket: " + flags.OTA_BUCKET_NAME)
        s3.put_bucket_versioning(
            Bucket=flags.OTA_BUCKET_NAME, VersioningConfiguration={"Status": "Enabled"}
        )
    except Exception as e:
        logging.error("failed creating an aws bucket")
        print_exception(e)
        cleanup_aws_resources(flags)
        raise e
    return True


@cli.command(cls=StdCommand)
@click.option(
    "--bucket_name", prompt="Enter Bucket Name", help="Name of Bucket to be created."
)
@click.option(
    "--build_dir",
    help="Override the default build directory",
    default=DEFAULT_BUILD_DIR,
)
@click.option(
    "--ota_binary",
    help="Override the default kws OTA file used",
    default=DEFAULT_OTA_BINARY,
)
@click.option(
    "--ota_binary_build_dir",
    help="Override the default OTA build directory",
    default=DEFAULT_OTA_BINARY_BUILD_DIR,
)
@click.pass_context
def create_bucket_only(
    ctx, bucket_name, build_dir, ota_binary, ota_binary_build_dir, log_level
):
    ctx.flags = Flags(
        build_dir=build_dir,
        ota_binary=ota_binary,
        ota_binary_build_dir=ota_binary_build_dir,
    )
    if not _create_aws_bucket(ctx.flags, bucket_name):
        cleanup_aws_resources(ctx.flags)
        ctx.exit(1)
    ctx.exit(0)


# Create OTA update service role
def _create_iam_role(flags: Flags, Name, permissions_boundary=None):
    try:
        flags.OTA_ROLE_NAME = Name
        if permissions_boundary == None:
            flags.role = iam.create_role(
                RoleName=flags.OTA_ROLE_NAME,
                AssumeRolePolicyDocument=json.dumps(flags.ASSUME_ROLE_POLICY_DOCUMENT),
            )["Role"]
        else:
            flags.role = iam.create_role(
                RoleName=flags.OTA_ROLE_NAME,
                AssumeRolePolicyDocument=json.dumps(flags.ASSUME_ROLE_POLICY_DOCUMENT),
                PermissionsBoundary=permissions_boundary,
            )["Role"]
        logging.info("Role Created")
        iam.attach_role_policy(
            RoleName=flags.OTA_ROLE_NAME,
            PolicyArn="arn:aws:iam::aws:policy/service-role/AWSIoTRuleActions",
        )
        iam.attach_role_policy(
            RoleName=flags.OTA_ROLE_NAME,
            PolicyArn="arn:aws:iam::aws:policy/service-role/AmazonFreeRTOSOTAUpdate",
        )
        iam.attach_role_policy(
            RoleName=flags.OTA_ROLE_NAME,
            PolicyArn="arn:aws:iam::aws:policy/service-role/AWSIoTLogging",
        )
        iam.attach_role_policy(
            RoleName=flags.OTA_ROLE_NAME,
            PolicyArn="arn:aws:iam::aws:policy/service-role/AWSIoTThingsRegistration",
        )
        logging.info("Attached all policies to Role")
        iam.put_role_policy(
            RoleName=flags.OTA_ROLE_NAME,
            PolicyName="IAM_PERMISSION",
            PolicyDocument=json.dumps(flags.IAM_OTA_PERMISSION),
        )
        iam.put_role_policy(
            RoleName=flags.OTA_ROLE_NAME,
            PolicyName="S3_PERMISSION",
            PolicyDocument=json.dumps(flags.S3_OTA_PERMISSION),
        )
        flags.OTA_ROLE_ARN = flags.role["Arn"]
        logging.info("Attached IAM and S3 Policy")
    except botocore.exceptions.ClientError as ex:
        if ex.response["Error"]["Code"] == "EntityAlreadyExists":
            logging.warning("The role named " + Name + " already exist")
            return False
        elif ex.response["Error"]["Code"] == "AccessDenied":
            if permissions_boundary != None:
                logging.error(
                    "Unauthorized operation. Your permission boundary "
                    + permissions_boundary
                    + " might not be enough to create a role"
                )
            else:
                logging.error(
                    "Unauthorized operation. You might need to add the permission boundary with --permissions_boundary arn:aws:iam::"
                    + flags.AWS_ACCOUNT_ID
                    + ":policy/<your_permission_boundary_policy>"
                )
            return False
        else:
            logging.error("failed creating an iam role")
            print_exception(ex)
            cleanup_aws_resources(flags)
            raise ex
    except Exception as e:
        logging.error("failed creating an iam role")
        print_exception(e)
        cleanup_aws_resources(flags)
        raise e
    return True


@cli.command(cls=StdCommand)
@click.option(
    "--iam_role_name",
    prompt="Enter IAM Role Name",
    help="Name of IAM Role to be created.",
)
@click.option(
    "--build_dir",
    help="Override the default build directory",
    default=DEFAULT_BUILD_DIR,
)
@click.option(
    "--ota_binary",
    help="Override the default kws OTA file used",
    default=DEFAULT_OTA_BINARY,
)
@click.option(
    "--ota_binary_build_dir",
    help="Override the default OTA build directory",
    default=DEFAULT_OTA_BINARY_BUILD_DIR,
)
@click.option(
    "--permissions_boundary",
    help="restricted users might need to use a permission boundary to create new iam roles.",
    default=None,
)
@click.pass_context
def create_iam_role_only(
    ctx,
    iam_role_name,
    build_dir,
    ota_binary,
    ota_binary_build_dir,
    permissions_boundary,
    log_level,
):
    ctx.flags = Flags("", iam_role_name, build_dir, ota_binary, ota_binary_build_dir)
    if not _create_iam_role(ctx.flags, iam_role_name, permissions_boundary):
        cleanup_aws_resources(ctx.flags)
        ctx.exit(1)
    ctx.exit(0)


def _wait_for_status(id, action, timeout=20, timeout_step=3):
    success_status = action.upper() + "_COMPLETE"
    failure_status = action.upper() + "_FAILED"
    for i in range(0, timeout, timeout_step):
        res = None
        try:
            res = iot.get_ota_update(otaUpdateId=id)
        except:
            logging.warning("Could not get ota update status for id " + id)
            return False
        else:
            status = res["otaUpdateInfo"]["otaUpdateStatus"]
            if status == success_status:
                return True
            elif status == failure_status:
                return False
            logging.debug("OTA update status:" + status)
        time.sleep(timeout_step)
    # on timeout
    return False


def _wait_for_job_deleted(job_name, timeout=10):
    job_status = None
    for i in range(0, timeout, 2):
        if not _does_job_exist(job_name):
            return True
        res = iot.describe_job(jobId=job_name)
        job_status = res["job"]["status"]
        if job_status == "DELETION_IN_PROGRESS" or job_status == "DELETE_IN_PROGRESS":
            # we need to wait for the deletion to finish before creating the new OTA
            time.sleep(2)
            continue
        else:
            return False
    # on timeout
    logging.debug(
        "wait_for_job_deleted timed out. The job " + job_name + "is still running"
    )
    return False


def _wait_for_ready_to_assume(role_arn, timeout=20):
    for i in range(0, timeout, 2):
        try:
            response = sts_client.assume_role(
                RoleArn=role_arn,
                RoleSessionName="check_assuming_role_permission",
            )
        except botocore.exceptions.ClientError as ex:
            if ex.response["Error"]["Code"] == "AccessDenied":
                # we likely just need to wait longer
                pass
            else:
                logging.warning(
                    "Failed to assume role: " + ex.response["Error"]["Message"]
                )
                return False
        except Exception as ex:
            logging.warning("Failed to assume role: " + str(ex))
            return False
        else:
            # no error reported? that mean we're free to assume the role
            return True
        time.sleep(2)
    # timing out
    return False


# Create update
def _create_aws_update(flags: Flags, ota_name):
    # The role might exist, but not be usable yet. Leave some time for it to propagate in AWS if needed
    if not _wait_for_ready_to_assume(flags.OTA_ROLE_ARN):
        logging.error("Failed to assume role to create an ota update")
        return False

    # check if there's a OTA job pending with the same name
    ota_job_name = OTA_JOB_NAME_PREFIX + ota_name
    if not _wait_for_job_deleted(ota_job_name):
        logging.error(
            "The job "
            + ota_job_name
            + " already exist. It needs to be deleted before trying to create another ota job with the same name"
        )
        return False

    try:
        flags.THING_ARN = iot.describe_thing(thingName=flags.THING_NAME)["thingArn"]

        # Push firmware to test bucket
        flags.file = open(flags.OTA_BINARY_FILE_PATH, "rb")
        s3.put_object(
            Bucket=flags.OTA_BUCKET_NAME, Key=flags.OTA_BINARY, Body=flags.file
        )
        flags.file.close()
        logging.info(
            "Added " + flags.OTA_BINARY + " to S3 bucket " + flags.OTA_BUCKET_NAME
        )

        # Create update job
        iot.create_ota_update(
            otaUpdateId=ota_name,
            targets=[flags.THING_ARN],
            protocols=flags.OTA_UPDATE_PROTOCOLS,
            targetSelection=flags.OTA_UPDATE_TARGET_SELECTION,
            files=flags.OTA_UPDATE_FILES,
            roleArn=flags.OTA_ROLE_ARN,
        )

        if not _wait_for_status(ota_name, "create"):
            logging.error("failed creating aws update.")
            res = iot.get_ota_update(otaUpdateId=ota_name)
            if res["otaUpdateInfo"]["otaUpdateStatus"] == "CREATE_FAILED":
                logging.error(
                    "Update failed to be created: "
                    + res["otaUpdateInfo"]["errorInfo"]["message"]
                )
            else:
                logging.error(
                    "Update failed to be created or timed out during creation. Last status: "
                    + res["otaUpdateInfo"]["otaUpdateStatus"]
                )
            return False
        flags.UPDATE_ID = ota_name
        logging.info("Created update " + flags.UPDATE_ID)
    except botocore.exceptions.ClientError as ex:
        if ex.response["Error"]["Code"] == "ResourceAlreadyExistsException":
            logging.warning("The ota update " + ota_name + " already exist")
            return True
        else:
            logging.error(
                "failed creating aws update: " + ex.response["Error"]["Message"]
            )
            return False
    except Exception as e:
        logging.error("failed creating aws update.")
        print_exception(e)
        cleanup_aws_resources(flags)
        raise e
    return True


@cli.command(cls=StdCommand)
@click.option(
    "--thing_name", prompt="Enter Thing Name", help="Name of Thing to be created."
)
@click.option(
    "--policy_name", prompt="Enter Policy Name", help="Name of Policy to be created."
)
@click.option(
    "--bucket_name", prompt="Enter Bucket Name", help="Name of Bucket to be created."
)
@click.option(
    "--iam_role_name",
    prompt="Enter IAM Role Name",
    help="Name of IAM Role to be created.",
)
@click.option("--update_name", prompt="Enter Update ID: ", help="Update ID to create.")
@click.option(
    "--build_dir",
    help="Override the default build directory",
    default=DEFAULT_BUILD_DIR,
)
@click.option(
    "--ota_binary",
    help="Override the default kws OTA file used",
    default=DEFAULT_OTA_BINARY,
)
@click.option(
    "--ota_binary_build_dir",
    help="Override the default OTA build directory",
    default=DEFAULT_OTA_BINARY_BUILD_DIR,
)
@click.pass_context
def create_update_only(
    ctx,
    thing_name,
    policy_name,
    bucket_name,
    iam_role_name,
    update_name,
    build_dir,
    ota_binary,
    ota_binary_build_dir,
    log_level,
):
    ctx.flags = Flags(
        bucket_name, iam_role_name, build_dir, ota_binary, ota_binary_build_dir
    )

    # Need previously created things and policy to setup an OTA update job
    ctx.flags.THING_NAME = thing_name
    if not _does_thing_exist(thing_name):
        logging.error("no thing found with the name " + thing_name)
        ctx.exit(1)

    ctx.flags.POLICY_NAME = policy_name
    if not _does_policy_exist(policy_name):
        logging.error("no policy found with the name " + policy_name)
        ctx.exit(1)

    ctx.flags.OTA_BUCKET_NAME = bucket_name
    if not _does_bucket_exist(bucket_name):
        logging.error("no bucket found with the name " + bucket_name)
        ctx.exit(1)

    ctx.flags.OTA_ROLE_NAME = iam_role_name
    if not _does_role_exist(iam_role_name):
        logging.error("no iam role found with the name " + iam_role_name)
        ctx.exit(1)

    if not _create_aws_update(ctx.flags, update_name):
        cleanup_aws_resources(ctx.flags)
        ctx.exit(1)
    ctx.exit(0)


# Create thing and policy
@cli.command(cls=StdCommand)
@click.option(
    "--thing_name", prompt="Enter Thing Name", help="Name of Thing to be created."
)
@click.option(
    "--policy_name", prompt="Enter Policy Name", help="Name of Policy to be created."
)
@click.option(
    "--use_existing_certificate_arn",
    default="",
    help="Use the provided certificate ARN instead of creating a new one",
)
@click.option(
    "-a",
    "--credentials_path",
    default="bsp/default_credentials",
    show_default=True,
    help="The path where the credentials are stored. If not specified, credentials in bsp/default_credentials are updated",
)
@click.option(
    "--build_dir",
    help="Override the default build directory",
    default=DEFAULT_BUILD_DIR,
)
@click.option(
    "--ota_binary",
    help="Override the default kws OTA file used",
    default=DEFAULT_OTA_BINARY,
)
@click.option(
    "--ota_binary_build_dir",
    help="Override the default OTA build directory",
    default=DEFAULT_OTA_BINARY_BUILD_DIR,
)
@click.pass_context
def create_thing_and_policy(
    ctx,
    thing_name,
    policy_name,
    use_existing_certificate_arn,
    credentials_path,
    build_dir,
    ota_binary,
    ota_binary_build_dir,
    log_level,
):
    ctx.flags = Flags(
        build_dir=build_dir,
        ota_binary=ota_binary,
        ota_binary_build_dir=ota_binary_build_dir,
    )

    ctx.flags.THING_NAME = thing_name
    certificate_arn = _get_credential_arn(
        ctx.flags, use_existing_certificate_arn, credentials_path
    )

    if not _create_thing(ctx.flags, thing_name, certificate_arn):
        cleanup_aws_resources(ctx.flags)
        ctx.exit(1)
    if not _create_policy(ctx.flags, policy_name, certificate_arn):
        cleanup_aws_resources(ctx.flags)
        ctx.exit(1)
    ctx.exit(0)


# Create bucket and role for the OTA update job
@cli.command(cls=StdCommand)
@click.option(
    "--thing_name", prompt="Enter existing Thing Name", help="Name of Thing to be used."
)
@click.option(
    "--bucket_name", prompt="Enter Bucket Name", help="Name of Bucket to be created."
)
@click.option(
    "--iam_role_name",
    prompt="Enter IAM Role Name",
    help="Name of IAM Role to be created.",
)
@click.option("--update_name", prompt="Enter Update ID", help="Update ID to create.")
@click.option(
    "--build_dir",
    help="Override the default build directory",
    default=DEFAULT_BUILD_DIR,
)
@click.option(
    "--ota_binary",
    help="Override the default kws OTA file used",
    default=DEFAULT_OTA_BINARY,
)
@click.option(
    "--ota_binary_build_dir",
    help="Override the default OTA build directory",
    default=DEFAULT_OTA_BINARY_BUILD_DIR,
)
@click.option(
    "--permissions_boundary",
    help="restricted users might need to use a permission boundary to create new iam roles.",
    default=None,
)
@click.pass_context
def create_bucket_role_update(
    ctx,
    thing_name,
    bucket_name,
    iam_role_name,
    update_name,
    build_dir,
    ota_binary,
    ota_binary_build_dir,
    permissions_boundary,
    log_level,
):
    ctx.flags = Flags(
        bucket_name, iam_role_name, build_dir, ota_binary, ota_binary_build_dir
    )

    # Need previously created thing to setup an OTA update job
    ctx.flags.THING_NAME = thing_name
    if not _does_thing_exist(thing_name):
        logging.error("no thing found with the name " + thing_name)
        ctx.exit(1)

    if not _create_aws_bucket(ctx.flags, bucket_name):
        cleanup_aws_resources(ctx.flags)
        ctx.exit(1)

    if not _create_iam_role(ctx.flags, iam_role_name, permissions_boundary):
        cleanup_aws_resources(ctx.flags)
        ctx.exit(1)

    if not _create_aws_update(ctx.flags, update_name):
        cleanup_aws_resources(ctx.flags)
        ctx.exit(1)
    ctx.exit(0)


def print_exception(ex):
    x = tb.extract_stack()[1]
    logging.error(f"{x.filename}:{x.lineno}:{x.name}: {ex}")


def _list_generic(
    client,
    listing_function_name: str,
    item_type_name: str,
    dictionary_item_name: str,
    max_listed=10,
):
    paginator = client.get_paginator(listing_function_name)
    response_iterator = paginator.paginate()
    currently_listed = 0
    for page in response_iterator:
        if len(page.get(dictionary_item_name)) == 0 and currently_listed == 0:
            logging.info("No " + item_type_name + " found")
            return
        for item in page[dictionary_item_name]:
            if currently_listed >= max_listed:
                logging.info("List truncated for readability")
                return
            logging.info(item)
            currently_listed += 1
    logging.info("Listed " + str(currently_listed) + " " + item_type_name)


def _list_things(max_listed=float("inf")):
    _list_generic(iot, "list_things", "things", "things", max_listed)


@cli.command(cls=ListingCommand)
@click.option(
    "--max_listed",
    help="Will not print more than max_listed things",
    default=25,
)
@click.pass_context
def list_things(ctx, log_level, max_listed):
    _list_things(max_listed)
    ctx.exit(0)


def _list_policies(max_listed=float("inf")):
    _list_generic(iot, "list_policies", "policies", "policies", max_listed)


@cli.command(cls=ListingCommand)
@click.option(
    "--max_listed",
    help="Will not print more than max_listed policies",
    default=25,
)
@click.pass_context
def list_policies(ctx, log_level, max_listed):
    _list_policies(max_listed)
    ctx.exit(0)


def _list_iam_roles(max_listed=float("inf")):
    _list_generic(iam, "list_roles", "roles", "Roles", max_listed)


@cli.command(cls=ListingCommand)
@click.option(
    "--max_listed",
    help="Will not print more than max_listed roles",
    default=25,
)
@click.pass_context
def list_iam_roles(ctx, log_level, max_listed):
    _list_iam_roles(max_listed)
    ctx.exit(0)


def _list_ota_updates(max_listed=float("inf")):
    _list_generic(iot, "list_ota_updates", "ota updates", "otaUpdates", max_listed)


@cli.command(cls=ListingCommand)
@click.option(
    "--max_listed",
    help="Will not print more than max_listed ota updates",
    default=25,
)
@click.pass_context
def list_ota_updates(ctx, log_level, max_listed):
    _list_ota_updates(max_listed)
    ctx.exit(0)


def _list_jobs(max_listed=float("inf")):
    _list_generic(iot, "list_jobs", "jobs", "jobs", max_listed)


@cli.command(cls=ListingCommand)
@click.option(
    "--max_listed",
    help="Will not print more than max_listed jobs",
    default=25,
)
@click.pass_context
def list_jobs(ctx, log_level, max_listed):
    _list_jobs(max_listed)
    ctx.exit(0)


def _list_buckets(max_listed=float("inf")):
    # list_buckets cannot be paginated, so we can't use _list_generic()
    response = s3.list_buckets()
    if len(response.get("Buckets")) == 0:
        logging.info("No buckets found")
        return
    currently_listed = 0
    for item in response["Buckets"]:
        if currently_listed >= max_listed:
            logging.info("List truncated for readability")
            return
        logging.info(item)
        currently_listed += 1
    logging.info("Listed " + str(currently_listed) + " buckets")


@cli.command(cls=ListingCommand)
@click.option(
    "--max_listed",
    help="Will not print more than max_listed buckets",
    default=25,
)
@click.pass_context
def list_buckets(ctx, log_level, max_listed):
    _list_buckets(max_listed)
    ctx.exit(0)


def _list_certificates(max_listed=float("inf")):
    _list_generic(iot, "list_certificates", "certificates", "certificates", max_listed)


@cli.command(cls=ListingCommand)
@click.option(
    "--max_listed",
    help="Will not print more than max_listed certificates",
    default=25,
)
@click.pass_context
def list_certificates(ctx, log_level, max_listed):
    _list_certificates(max_listed)
    ctx.exit(0)


@cli.command(cls=ListingCommand)
@click.option(
    "--max_listed",
    help="Will not print more than max_listed item for each category",
    default=15,
)
@click.pass_context
def list_all(ctx, log_level, max_listed):
    logging.info("Things:")
    _list_things(max_listed)
    logging.info("\n\nPolicies:")
    _list_policies(max_listed)
    logging.info("\n\nRoles:")
    _list_iam_roles(max_listed)
    logging.info("\n\nOTA Updates:")
    _list_ota_updates(max_listed)
    logging.info("\n\nJobs:")
    _list_jobs(max_listed)
    logging.info("\n\nBuckets:")
    _list_buckets(max_listed)
    logging.info("\n\nCertificates:")
    _list_certificates(max_listed)
    ctx.exit(0)


def _has_certificate_anything_attached(certificate_arn):
    pass
    response = iot.list_principal_things(principal=certificate_arn)
    if len(response["things"]):
        return True
    response = iot.list_attached_policies(target=certificate_arn)
    if len(response["policies"]):
        return True
    return False


def _get_certificate_id_from_arn(certificate_arn):
    return certificate_arn.split("/")[-1]


def _delete_thing(thing_name, prune=False):
    if not _does_thing_exist(thing_name):
        logging.error("no thing found with the name " + thing_name)
        return False
    try:
        paginator = iot.get_paginator("list_thing_principals")
        response_iterator = paginator.paginate(thingName=thing_name)
        for page in response_iterator:
            if len(page.get("principals")) == 0:
                break
            for principal in page["principals"]:
                iot.detach_thing_principal(
                    thingName=thing_name,
                    principal=principal,
                )
                if prune and not _has_certificate_anything_attached(principal):
                    # if the certificate status is INACTIVE then calling _delete_certificate would do a reccursion loop
                    certificate_id = _get_certificate_id_from_arn(principal)
                    response = iot.describe_certificate(certificateId=certificate_id)
                    if response["certificateDescription"]["status"] == "ACTIVE":
                        if _delete_certificate(certificate_id):
                            logging.info("Deleted certificate: " + principal)
        iot.delete_thing(thingName=thing_name)
    except Exception as ex:
        logging.error("failed deleting the thing named " + thing_name)
        print_exception(ex)
        raise ex
    return True


@cli.command(cls=StdCommand)
@click.option(
    "--thing_name", prompt="Enter thing name", help="Name of thing to be deleted."
)
@click.option(
    "-p",
    "--prune-certificate",
    "prune",
    help="If the deleted thing was the last item attached to a certificate, delete the certificate as well.",
    is_flag=True,
)
@click.pass_context
def delete_thing(ctx, thing_name, log_level, prune):
    if _delete_thing(thing_name, prune):
        logging.info(thing_name + " deleted")
        ctx.exit(0)
    ctx.exit(1)


def _delete_policy(policy_name, prune=False):
    if not _does_policy_exist(policy_name):
        logging.error("no policy found with the name " + policy_name)
        return False
    try:
        paginator = iot.get_paginator("list_policy_principals")
        response_iterator = paginator.paginate(policyName=policy_name)
        for page in response_iterator:
            if page.get("principals") == []:
                break
            for principal in page["principals"]:
                iot.detach_principal_policy(
                    policyName=policy_name,
                    principal=principal,
                )
                if prune and not _has_certificate_anything_attached(principal):
                    # if the certificate status is INACTIVE then calling _delete_certificate would do a reccursion loop
                    certificate_id = _get_certificate_id_from_arn(principal)
                    response = iot.describe_certificate(certificateId=certificate_id)
                    if response["certificateDescription"]["status"] == "ACTIVE":
                        if _delete_certificate(certificate_id):
                            logging.info("Deleted certificate: " + principal)
        iot.delete_policy(policyName=policy_name)
    except Exception as ex:
        logging.error("failed deleting the policy named " + policy_name)
        print_exception(ex)
        raise ex
    return True


@cli.command(cls=StdCommand)
@click.option(
    "--policy_name", prompt="Enter policy name", help="Name of policy to be deleted."
)
@click.option(
    "-p",
    "--prune-certificate",
    "prune",
    help="If the deleted policy was the last item attached to a certificate, delete the certificate as well.",
    is_flag=True,
)
@click.pass_context
def delete_policy(ctx, policy_name, log_level, prune):
    if _delete_policy(policy_name, prune):
        logging.info(policy_name + " deleted")
        ctx.exit(0)
    ctx.exit(1)


def _delete_iam_role(iam_role_name, force_delete=False):
    try:
        role = boto3.resource("iam").Role(iam_role_name)
        if force_delete:
            policy_iterator = role.attached_policies.all()
            for policy in policy_iterator:
                logging.debug("Detached " + policy.arn)
                policy.detach_role(RoleName=iam_role_name)

            role_policy_iterator = role.policies.all()
            for role_policy in role_policy_iterator:
                logging.debug("Detached " + role_policy.name)
                role_policy.delete()
        role.delete()
    except botocore.exceptions.ClientError as ex:
        if ex.response["Error"]["Code"] == "NoSuchEntity":
            logging.warning("no iam role found with the name " + iam_role_name)
            return False
        if ex.response["Error"]["Code"] == "AccessDenied":
            logging.warning("Access denied or non existant role: " + iam_role_name)
            return False
        elif ex.response["Error"]["Code"] == "DeleteConflict":
            logging.warning(
                "Role have attached policies. Try re-running with the -f option to detach them first."
            )
            return False
        else:
            logging.error("failed deleting the iam role named " + iam_role_name)
            print_exception(ex)
            raise ex
    except Exception as ex:
        logging.error("failed deleting the iam role named " + iam_role_name)
        print_exception(ex)
        raise ex
    return True


@cli.command(cls=StdCommand)
@click.option(
    "--iam_role_name", prompt="Enter role name", help="Name of the role to be deleted."
)
@click.option(
    "-f",
    "--force-delete",
    "force_delete",
    help="Delete the role even if it have policies attached (detach them all first)",
    is_flag=True,
)
@click.pass_context
def delete_iam_role(ctx, iam_role_name, log_level, force_delete):
    if _delete_iam_role(iam_role_name, force_delete):
        logging.info(iam_role_name + " deleted")
        ctx.exit(0)
    ctx.exit(1)


def _delete_job(job_name, force_delete=False):
    try:
        iot.delete_job(jobId=job_name, force=force_delete)
    except botocore.exceptions.ClientError as ex:
        if ex.response["Error"]["Code"] == "ResourceNotFoundException":
            logging.warning("no job found with the name " + job_name)
            return False
        else:
            logging.error("failed deleting the job named " + job_name)
            print_exception(ex)
            raise ex
    except Exception as ex:
        logging.error("failed deleting the job named " + job_name)
        print_exception(ex)
        raise ex
    return True


@cli.command(cls=StdCommand)
@click.option(
    "--job_name", prompt="Enter role name", help="Name of the job to be deleted."
)
@click.option(
    "-f",
    "--force-delete",
    "force_delete",
    help="Delete the role even if it have policies attached (detach them all first)",
    is_flag=True,
)
@click.pass_context
def delete_job(ctx, job_name, log_level, force_delete):
    if _delete_job(job_name, force_delete):
        logging.info(job_name + " deleted")
        ctx.exit(0)
    ctx.exit(1)


def _delete_ota_update(ota_update_name, force_delete=False):
    # check if the OTA job is running
    ota_job_name = OTA_JOB_NAME_PREFIX + ota_update_name
    ota_job_exists = False
    if not _wait_for_job_deleted(ota_job_name):
        ota_job_exists = True
        if not force_delete:
            # we do have a job but don't force delete: we won't be able to delete the ota yet
            logging.warning(
                "The ota update is currently in progress. Try re-running with the -f option to delete it anyway."
            )
            return False

    # tries to delete the ota update
    try:
        if force_delete:
            if ota_job_exists:
                _delete_job(ota_job_name, True)
                logging.info("Deleting the OTA jobs. This may take a minute...")
                if not _wait_for_job_deleted(ota_job_name, 60):
                    logging.error(
                        "failed deleting the job update named " + ota_job_name
                    )
                    return False
            iot.delete_ota_update(
                otaUpdateId=ota_update_name, deleteStream=True, forceDeleteAWSJob=True
            )
        else:
            res = iot.get_ota_update(otaUpdateId=ota_update_name)
            status = res["otaUpdateInfo"]["otaUpdateStatus"]
            if status in ["CREATE_PENDING", "CREATE_IN_PROGRESS", "DELETE_IN_PROGRESS"]:
                logging.warning(
                    "The ota update is currently in progress. Try re-running with the -f option to delete it anyway."
                )
                return False
            elif status in ["CREATE_COMPLETE"]:
                iot.delete_ota_update(otaUpdateId=ota_update_name)
            elif status in ["CREATE_FAILED", "DELETE_FAILED"]:
                logging.warning(
                    'The ota update is in an error state "'
                    + status
                    + '". Try re-running with the -f option to delete it anyway.'
                )
                return False
            else:
                logging.warning(
                    'The ota update is in an unknown state: "'
                    + status
                    + '". Try re-running with the -f option to delete it anyway.'
                )
                return False
    except botocore.exceptions.ClientError as ex:
        if ex.response["Error"]["Code"] == "ResourceNotFoundException":
            logging.warning("no ota update found with the name " + ota_update_name)
            return False
        else:
            logging.error("failed deleting the ota update named " + ota_update_name)
            print_exception(ex)
            raise ex
    except Exception as ex:
        logging.error("failed deleting the ota update named " + ota_update_name)
        print_exception(ex)
        raise ex

    # check the deletion status
    timeout = 20
    timeout_step = 2
    for i in range(0, timeout, timeout_step):
        try:
            res = iot.get_ota_update(otaUpdateId=ota_update_name)
            status = res["otaUpdateInfo"]["otaUpdateStatus"]
        except botocore.exceptions.ClientError as ex:
            if ex.response["Error"]["Code"] == "ResourceNotFoundException":
                # no ota update found = sucessful deletion
                return True
            else:
                raise ex
        if status == "DELETE_IN_PROGRESS":
            time.sleep(timeout_step)
            continue
        else:
            message = res["otaUpdateInfo"].get("errorInfo")["message"]
            logging.error(
                "failed deleting the ota update named "
                + ota_update_name
                + "\n    Status: "
                + status
                + ", "
                + message
            )
            return False
    # on timeout
    return False


@cli.command(cls=StdCommand)
@click.option(
    "--ota_update_name",
    prompt="Enter ota update name",
    help="Name of the ota update to be deleted.",
)
@click.option(
    "-f",
    "--force-delete",
    "force_delete",
    help="Delete the role even if it have policies attached (detach them all first)",
    is_flag=True,
)
@click.pass_context
def delete_ota_update(ctx, ota_update_name, log_level, force_delete):
    if _delete_ota_update(ota_update_name, force_delete):
        logging.info(ota_update_name + " deleted")
        ctx.exit(0)
    ctx.exit(1)


def _delete_bucket(bucket_name, force_delete=False):
    try:
        if force_delete:
            versioning = s3.get_bucket_versioning(Bucket=bucket_name)
            bucket = boto3.resource("s3").Bucket(bucket_name)
            if versioning.get("Status") == "Enabled":
                logging.debug("Removing bucket versioning")
                bucket.object_versions.delete()
            bucket.objects.delete()
        s3.delete_bucket(Bucket=bucket_name)
    except botocore.exceptions.ClientError as ex:
        if ex.response["Error"]["Code"] == "NoSuchBucket":
            logging.warning("no bucket found with the name " + bucket_name)
            return False
        elif ex.response["Error"]["Code"] == "BucketNotEmpty":
            logging.warning(
                "The bucket is not empty. Try re-running with the -f option to clean it first."
            )
            return False
        else:
            logging.error("failed deleting the bucket named " + bucket_name)
            print_exception(ex)
            raise ex
    except Exception as ex:
        logging.error("failed deleting the bucket named " + bucket_name)
        print_exception(ex)
        raise ex
    return True


@cli.command(cls=StdCommand)
@click.option(
    "--bucket_name",
    prompt="Enter bucket name",
    help="Name of the bucket to be deleted.",
)
@click.option(
    "-f",
    "--force-delete",
    "force_delete",
    help="Delete the role even if it have policies attached (detach them all first)",
    is_flag=True,
)
@click.pass_context
def delete_bucket(ctx, bucket_name, log_level, force_delete):
    if _delete_bucket(bucket_name, force_delete):
        logging.info(bucket_name + " deleted")
        ctx.exit(0)
    ctx.exit(1)


def _delete_certificate(certificate_id, force_delete=False):
    try:
        iot.update_certificate(
            certificateId=certificate_id,
            newStatus="INACTIVE",
        )

        # Try to delete any attached things and policies
        if force_delete:
            certificateArn = iot.describe_certificate(certificateId=certificate_id)[
                "certificateDescription"
            ]["certificateArn"]
            # find and delete the attached policies
            paginator = iot.get_paginator("list_attached_policies")
            response_iterator = paginator.paginate(target=certificateArn)
            for page in response_iterator:
                for policy in page["policies"]:
                    _delete_policy(policy["policyName"])
            # find and delete the attached things
            paginator = iot.get_paginator("list_principal_things")
            response_iterator = paginator.paginate(principal=certificateArn)
            for page in response_iterator:
                for thing_name in page["things"]:
                    _delete_thing(thing_name)

        # Try to delete the certificate
        iot.delete_certificate(
            certificateId=certificate_id,
            forceDelete=force_delete,
        )
    except botocore.exceptions.ClientError as ex:
        if ex.response["Error"]["Code"] == "ResourceNotFoundException":
            logging.warning("no certificate found with the name " + certificate_id)
            return False
        elif ex.response["Error"]["Code"] == "DeleteConflictException":
            logging.warning(
                "The certificate still have things or policies attached. Try re-running with the -f option to force detach them first"
            )
            return False
        else:
            logging.error("failed deleting the certificate named " + certificate_id)
            print_exception(ex)
            raise ex
    except Exception as ex:
        logging.error("failed deleting the certificate named " + certificate_id)
        print_exception(ex)
        raise ex
    return True


@cli.command(cls=StdCommand)
@click.option(
    "--certificate_id",
    prompt="Enter certificate id",
    help="Id of the certificate to be deleted.",
)
@click.option(
    "-f",
    "--force-delete",
    "force_delete",
    help="Delete the certificate even if it have policies or things attached. Those will be deleted as well.",
    is_flag=True,
)
@click.pass_context
def delete_certificate(ctx, certificate_id, log_level, force_delete):
    if _delete_certificate(certificate_id, force_delete):
        logging.info(certificate_id + " deleted")
        ctx.exit(0)
    ctx.exit(1)


def cleanup_aws_resources(
    flags: Flags,
):
    if flags.thing:
        try:
            has_deleted = _delete_thing(flags.THING_NAME)
        except Exception as ex:
            print_exception(ex)
        else:
            if has_deleted:
                logging.info("Deleted thing: " + flags.THING_NAME)
                flags.thing = None
            else:
                logging.warning("Failed to delete thing: " + flags.THING_NAME)
    if flags.policy:
        try:
            has_deleted = _delete_policy(flags.POLICY_NAME)
        except Exception as ex:
            print_exception(ex)
        else:
            if has_deleted:
                logging.info("Deleted policy: " + flags.POLICY_NAME)
                flags.policy = None
            else:
                logging.warning("Failed to delete policy: " + flags.POLICY_NAME)
    if flags.UPDATE_ID:
        try:
            has_deleted = _delete_ota_update(flags.UPDATE_ID, True)
        except Exception as ex:
            print_exception(ex)
        else:
            if has_deleted:
                logging.info("Deleted update" + flags.UPDATE_ID)
                flags.UPDATE_ID = None
            else:
                logging.warning("Failed to delete update: " + flags.UPDATE_ID)
    if flags.keyAndCertificate:
        try:
            has_deleted = _delete_certificate(
                flags.keyAndCertificate["certificateId"], True
            )
        except Exception as ex:
            print_exception(ex)
        else:
            if has_deleted:
                logging.info(
                    "Deleted certificate: " + flags.keyAndCertificate["certificateArn"]
                )
                flags.keyAndCertificate = None
            else:
                logging.warning(
                    "Failed to delete certificate: "
                    + flags.keyAndCertificate["certificateArn"]
                )
    if flags.bucket:
        try:
            has_deleted = _delete_bucket(flags.OTA_BUCKET_NAME, True)
        except Exception as ex:
            print_exception(ex)
        else:
            if has_deleted:
                logging.info("Deleted S3 bucket: " + flags.OTA_BUCKET_NAME)
                flags.bucket = None
            else:
                logging.warning("Failed to delete S3 bucket: " + flags.OTA_BUCKET_NAME)
    if flags.role:
        # We should not delete the role if the ota update have not been sucessfully deleted, or we'll lose our only way to delete the update
        if flags.UPDATE_ID == None:
            try:
                has_deleted = _delete_iam_role(flags.OTA_ROLE_NAME, True)
            except Exception as ex:
                print_exception(ex)
            else:
                if has_deleted:
                    logging.info("Deleted iam role: " + flags.OTA_ROLE_NAME)
                    flags.bucket = None
                else:
                    logging.warning("Failed to delete iam role: " + flags.OTA_ROLE_NAME)
        else:
            logging.warning(
                "Abort deleting the role because an OTA update still depends on it: "
                + flags.OTA_ROLE_NAME
            )


if __name__ == "__main__":
    cli()
