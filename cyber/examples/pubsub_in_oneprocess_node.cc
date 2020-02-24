
#include <iostream>
#include <thread>
#include <fstream>
#include <memory>

#include "cyber/cyber.h"
#include "cyber/time/rate.h"
#include "cyber/time/time.h"
#include "cyber/proto/unit_test.pb.h"

using apollo::cyber::Rate;
using apollo::cyber::Time;
using apollo::cyber::proto::Needata;
using apollo::cyber::Node;
using namespace apollo::cyber;
using namespace std;


uint8_t file_name = 0;
int num = 0;

void MessageCallback(const std::shared_ptr<Needata>& msg) {
//    uint32_t time = Time::Now().ToNanosecond() - msg->timestamp();
//    double d_time = static_cast<double>(time)/1000000.0;
//    std::ofstream fout("/dev/shm/result"+to_string(file_name)+".txt",ios::in|ios::out|ios::app);
//    fout<< setw(10)<<to_string(msg->seq())<<","
//      <<to_string(d_time)<<","
//      <<to_string(msg->id())<<"\r\n";
//    fout.close();
   std::cout<<"callback1 content_size:"<<msg->content().size()<<std::endl;

   ++num;
   if ((num == 200) || (msg->id() == 200))
    {
        exit(0);
    }
}

void MessageCallback2(const std::shared_ptr<Needata>& msg) {
//    uint32_t time = Time::Now().ToNanosecond() - msg->timestamp();
//    double d_time = static_cast<double>(time)/1000000.0;
//    std::ofstream fout("/dev/shm/result2"+to_string(file_name)+".txt",ios::in|ios::out|ios::app);
//    fout<< setw(10)<<to_string(msg->seq())<<","
//      <<to_string(d_time)<<","
//      <<to_string(msg->id())<<"\r\n";
//    fout.close();
   std::cout<<"callback2 content_size:"<<msg->content().size()<<std::endl;

   ++num;
   if ((num == 200) || (msg->id() == 200))
    {
        exit(0);
    }
}


int main(int argc, char *argv[]) 
{
   // init cyber framework
   apollo::cyber::Init(argv[0]);
   int data_size_opt = atoi(argv[1]);
   int listener_num = atoi(argv[2]);
   std::cout<<"not used listener_num:"<<listener_num<<std::endl;

   // create listener
   auto listener_node1 = apollo::cyber::CreateNode("listener_node1");
   auto listener1 = listener_node1->CreateReader<Needata>("channel/chatter", MessageCallback);
//    auto listener_node2 = apollo::cyber::CreateNode("listener_node2");
   auto listener2 = listener_node1->CreateReader<Needata>("channel/chatter", MessageCallback2);

    // create talker
   auto talker_node = apollo::cyber::CreateNode("talker_node");
   auto talker = talker_node->CreateWriter<Needata>("channel/chatter");
   char* data = nullptr;
   size_t len = 0;
    try
    {
        switch (data_size_opt)
        {
        // 20 byte
        case 1:
           data = new char[21];len = 20;
        break;
        // 5kbyte
        case 2:
           data = new char[5001];len=5000;
        break;
        // 5Mbyte
        case 3:
           data = new char[5000001];len=5000000;
        break;
        //50Mbyte
        case 4:
          data = new char[50000001]();len=50000000;
        break;
        default:
        break;
        }
    }
    catch (const std::bad_alloc& e)
    {
        cerr << "Allocation failed for buffer" << e.what() << endl;
    }  
   for(size_t i=0;i<len;i++){
      data[i]='a';
   }
   data[len]='\0';
    Rate rate(4.0);
    int id = 200;
    for (int i =0; i<= id;i++)
    {
       std::cout<<"talker say:"<<i<<std::endl;

       auto msg = std::make_shared<Needata>();
       msg->set_timestamp(Time::Now().ToNanosecond());
       msg->set_seq(0);
       msg->set_id(i);
       msg->set_content(data);
       talker->Write(msg);
       rate.Sleep();
    }
   cout << "200 sequence over!" <<endl;

   delete []data;

   apollo::cyber::WaitForShutdown();
   std::cout<<"exit"<<std::endl;
}
