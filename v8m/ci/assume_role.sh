#!/bin/bash

#  Copyright (c) 2021 Arm Limited. All rights reserved.
#  SPDX-License-Identifier: Apache-2.0

apt update -y
apt install jq -y
aws sts get-caller-identity
aws sts assume-role --role-arn ${CI_USER_PRIVILEGED_AWS_ROLE} --role-session-name AWSCLI-Session > /tmp/creds 2>&1

export AWS_ACCESS_KEY_ID=$(jq -r '.Credentials.AccessKeyId' /tmp/creds)
export AWS_SECRET_ACCESS_KEY=$(jq -r '.Credentials.SecretAccessKey' /tmp/creds)
export AWS_SESSION_TOKEN=$(jq -r '.Credentials.SessionToken' /tmp/creds)
aws sts get-caller-identity
