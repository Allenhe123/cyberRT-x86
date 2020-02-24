/******************************************************************************
 * Copyright 2018 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#include "cyber/cyber.h"
#include "cyber/examples/proto/examples.pb.h"
#include "cyber/time/time.h"
using namespace std;

void MessageCallback(
    const std::shared_ptr<apollo::cyber::examples::proto::Chatter>& msg) {
    static double time_sum = 0;
    uint64_t bytes_size = msg->lidar_timestamp();
    auto diff = apollo::cyber::Time::Now().ToNanosecond() - msg->timestamp();
    AINFO << "recv msg-seq-> " << msg->seq() << " msg: " << (uint32_t)msg->content()[bytes_size - 1]
          << " delta-time: " << (double)diff / 1000000.0;
    time_sum += (double)diff / 1000000.0;
    if (msg->seq() == 127) {
      AINFO << "Average time: " << time_sum / 128.0;
    }

}

int main(int argc, char* argv[]) {
  AINFO << "the recv battle begin...";

  // init cyber framework
  apollo::cyber::Init(argv[0]);
  // create listener node
  auto listener_node = apollo::cyber::CreateNode("listener");
  // create listener
  auto listener =
      listener_node->CreateReader<apollo::cyber::examples::proto::Chatter>(
          "channel/chatter", MessageCallback);
  apollo::cyber::WaitForShutdown();
  return 0;
}
