# Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import shlex, subprocess, time, os, socket, sys

command = os.environ.get("LATENCY_TEST_BIN")

# Best effort
subscriber_proc = subprocess.Popen([command, "subscriber", "--seed", str(os.getpid()), "--hostname"])
publisher_proc = subprocess.Popen([command, "publisher", "--seed", str(os.getpid()), "--hostname", "--export_csv"])

subscriber_proc.communicate()
publisher_proc.communicate()

# Reliable
subscriber_proc = subprocess.Popen([command, "subscriber", "-r", "reliable", "--seed", str(os.getpid()), "--hostname"])
publisher_proc = subprocess.Popen([command, "publisher", "-r", "reliable", "--seed", str(os.getpid()), "--hostname", "--export_csv"])

subscriber_proc.communicate()
publisher_proc.communicate()

quit()
