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

#include "cyber/examples/proto/examples.pb.h"

#include "cyber/cyber.h"

bool s_rdy = false;
std::future<int> s_f;
int async_func(int& val) {
  std::future_status status = s_f.wait_for(std::chrono::milliseconds(1));
  if (status != std::future_status::ready) return -1;
  else  {
    val = s_f.get();
    return 0;
  }
}

void MessageCallback(
  const std::shared_ptr<apollo::cyber::examples::proto::Chatter>& msg) {

  if (!s_rdy) {
    int value = 0;
    if (async_func(value) == 0) {
      s_rdy = true;
      AINFO << "calc done: " << value;
    } else {
      AINFO << "calc not done";
      apollo::cyber::USleep(5000 * 1000);   // 主动yield
    }
  }

  AINFO << "Received message seq-> " << msg->seq();
  AINFO << "msgcontent->" << msg->content();
}

int main(int argc, char* argv[]) {
  // init cyber framework
  apollo::cyber::Init(argv[0]);
  // create listener node
  auto listener_node = apollo::cyber::CreateNode("listener");
  // create listener
  auto listener =
      listener_node->CreateReader<apollo::cyber::examples::proto::Chatter>(
          "channel/chatter", MessageCallback);

  s_f = std::async(std::launch::async, []() {
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    return 8;
    });

  apollo::cyber::WaitForShutdown();
  return 0;
}
