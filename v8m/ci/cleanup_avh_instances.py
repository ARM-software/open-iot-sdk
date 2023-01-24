#!/usr/bin/env python3
#  Copyright (c) 2022 Arm Limited. All rights reserved.
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

import gitlab
import boto3


def get_instance_name(instance):
    for tag in instance.tags:
        if tag["Key"] == "Name":
            return tag["Value"]
    else:
        return None


def main(args):
    if len(args) != 1:
        raise ValueError(
            "Invalid script usage!\n"
            f"Usage: {os.path.basename(__file__)} <avh-instance-name-prefix>"
            "Example:\n"
            f"{os.path.basename(__file__)} 'orta-ami-testing-'"
        )
    avh_instance_name_prefix = args[0]

    gitlab_client = gitlab.Gitlab(
        os.environ["CI_SERVER_URL"],
        private_token=os.environ["CI_PROJECT_READ_ACCESS_TOKEN"],
    )
    gitlab_project = gitlab_client.projects.get(os.environ["CI_PROJECT_PATH"])

    ec2_resource = boto3.resource("ec2", region_name="eu-west-1")
    avh_ec2_instances = ec2_resource.instances.filter(
        Filters=[
            {"Name": "tag:AVH_CLI", "Values": ["true"]},
            {"Name": "tag:Name", "Values": [f"{avh_instance_name_prefix}*"]},
            {"Name": "instance-state-name", "Values": ["pending", "running"]},
        ]
    )

    instance_name_map = {
        get_instance_name(instance): instance for instance in avh_ec2_instances
    }

    instance_names_to_terminate = []
    for instance_name in instance_name_map.keys():
        job_id = int(instance_name[len(avh_instance_name_prefix) :])
        job = gitlab_project.jobs.get(job_id)
        if job and job.status != "running":
            instance_names_to_terminate.append(instance_name)

    if len(instance_names_to_terminate) == 0:
        print("There is no AVH instance to terminate.")
        return

    print(f"Terminating {len(instance_names_to_terminate)} AVH instances:")

    for instances_name in instance_names_to_terminate:
        instance = instance_name_map[instances_name]
        print(f"{instance_name}' (id: {instance.id})")
        instance.terminate()


if __name__ == "__main__":
    main(sys.argv[1:])
