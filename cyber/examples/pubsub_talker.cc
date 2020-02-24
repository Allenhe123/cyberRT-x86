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
#include "cyber/time/rate.h"
#include "cyber/time/time.h"

#include <memory>

using apollo::cyber::Rate;
using apollo::cyber::Time;
using apollo::cyber::examples::proto::Chatter;

// argv[1] -- msg size
// argv[2] -- send interval   millsecond
int main(int argc, char *argv[]) {
  if (argc < 3) {
    AINFO << "cmd param num must be more than 3!";
    return -1;
  }

  AINFO << "the battle begin...";

  int32_t msgsize = std::stoi(argv[1]) - 24;
  int32_t interval = std::stoi(argv[2]);

  // init cyber framework
  apollo::cyber::Init(argv[0]);
  // create talker node
  auto talker_node = apollo::cyber::CreateNode("talker");
  // create talker
  auto talker = talker_node->CreateWriter<Chatter>("channel/chatter");
  Rate rate((uint64_t)interval * 1000000);

  while (apollo::cyber::OK()) {
    static uint64_t seq = 0;
    auto msg = std::make_shared<Chatter>();
    msg->set_seq(seq);

    auto func = [](char* p) {delete []p;};
    std::unique_ptr<char, decltype(func)> pp(new char[msgsize], func);
    memset(pp.get(), (char)seq, msgsize);

    msg->set_content(pp.get());
    msg->set_timestamp(Time::Now().ToNanosecond());
    msg->set_lidar_timestamp(msgsize); // bytes size

    talker->Write(msg);
    AINFO << "talker sent a message! " << seq;
    seq++;
    rate.Sleep();

    if (seq == 128) {
      AINFO << "send completed...";
      break;
    }
  }

  apollo::cyber::WaitForShutdown();

  return 0;
}
