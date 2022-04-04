#  Copyright (c) 2021 Arm Limited. All rights reserved.
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

HERE="$(dirname "$0")"
set -e
record=".patches-applied"
touch "$record"
while read patch; do
    name="$(basename "$patch")"
    if ! grep "$name" "$record" >/dev/null; then
        patch -p1 -fi "$patch"
        echo "$name" | tee -a "$record"
    fi
done < <(find "$HERE" -iname '*.patch' | sort -n)
