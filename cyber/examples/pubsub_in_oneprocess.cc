#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <chrono>

#include "cyber/common/global_data.h"
#include "cyber/cyber.h"
#include "cyber/init.h"
#include "cyber/node/reader.h"
#include "cyber/node/writer.h"
#include "cyber/proto/unit_test.pb.h"
#include "cyber/time/time.h"

apollo::cyber::Time g_start;
std::chrono::time_point<std::chrono::high_resolution_clock> g_send_time;

double sum_time = 0;

struct NetOutput
{
    std::vector<unsigned char> data;
};


void test_case1(char** argv)
{
    using namespace std;
    using namespace chrono;
    using namespace apollo::cyber;

	apollo::cyber::Init(argv[0]);

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    const uint32_t width = 1280;
    const uint32_t height = 640;
    std::string g_netout_topic = "net_out_TOPIC";

    std::shared_ptr<Writer<NetOutput> > g_netout_writer = nullptr;
    std::shared_ptr<Reader<NetOutput> > g_netout_reader = nullptr;

    proto::RoleAttributes attr;
    attr.set_node_name("netout_reader");
	attr.set_channel_name(g_netout_topic);
	auto channel_id_netout = common::GlobalData::RegisterChannel(attr.channel_name());
	attr.set_channel_id(channel_id_netout);
	g_netout_reader = std::make_shared<Reader<NetOutput>>(attr, [&] (const shared_ptr<NetOutput>& msg) {
            // apollo::cyber::Duration delta = Time::Now() - g_start;
            // double sec = delta.ToSecond() * 1000.0;
            // sum_time += sec;
            // AINFO << "##recv netout msg: " << (uint32_t)msg->data[0] << "  delta time: " << sec;

            AINFO << "##recv netout msg: " << (uint32_t)msg->data[std::stoi(argv[1]) - 1]; 
            auto recv_time = chrono::high_resolution_clock::now();
            auto delta = std::chrono::duration_cast<std::chrono::microseconds>(recv_time - g_send_time);
            AINFO << "  delta time: " << delta.count() / 1000.0 << " ms";
            sum_time += delta.count();
		}
	);
	if (!g_netout_reader->Init()) printf("netout reader init failed.\n");

	attr.set_node_name("netout_writer");
	g_netout_writer = std::make_shared<Writer<NetOutput>>(attr);
	if (!g_netout_writer->Init()) printf("netout writer init failed.\n");

    for (uint32_t k=0; k<256; k++)
    {
        this_thread::sleep_for(std::chrono::milliseconds(std::stoi(argv[2])));
        auto netout_data = std::make_shared<NetOutput>();
        netout_data->data.resize(std::stoi(argv[1]));
        // memcpy(&(netout_data->data[0]), k, std::stoi(argv[1]));
        for (uint32_t i=0; i<std::stoi(argv[1]); i++)
            netout_data->data[i] = (unsigned char)k;

        // g_start = Time::Now();
        AINFO << "##pub netout msg: " << (uint32_t)k; 
        g_send_time = chrono::high_resolution_clock::now();
        g_netout_writer->Write(netout_data);
    }

	assert(g_netout_reader->HasWriter());
	assert(g_netout_writer->HasReader());

    sum_time /= 256000.0f;
    AINFO << "average time:" << sum_time;
    std::cout << "job done" << std::endl; 

    apollo::cyber::WaitForShutdown();

    g_netout_reader->Shutdown();
	g_netout_writer->Shutdown();

    cout << "endddddddd" << endl;
}


void test_case2(char** argv)
{
    using namespace std;
    using namespace apollo::cyber;

    proto::RoleAttributes attr;
    attr.set_node_name("writer");
    attr.set_channel_name("messaging");
    auto channel_id = common::GlobalData::RegisterChannel(attr.channel_name());
    attr.set_channel_id(channel_id);

    Writer<proto::UnitTest> writer(attr);
    assert(writer.Init());

    std::mutex mtx;
    std::vector<proto::UnitTest> recv_msgs;
    attr.set_node_name("reader_a");
    Reader<proto::UnitTest> reader_a(
        attr, [&](const std::shared_ptr<proto::UnitTest>& msg) {
        std::lock_guard<std::mutex> lck(mtx);
        cout << "reader_a recv a msg" << endl;
        recv_msgs.emplace_back(*msg);
        });
    assert(reader_a.Init());

    attr.set_node_name("reader_b");
    Reader<proto::UnitTest> reader_b(
        attr, [&](const std::shared_ptr<proto::UnitTest>& msg) {
        std::lock_guard<std::mutex> lck(mtx);
        cout << "reader_b recv a msg" << endl;
        recv_msgs.emplace_back(*msg);
        });
    assert(reader_b.Init());

    auto msg = std::make_shared<proto::UnitTest>();
    msg->set_class_name("WriterReaderTest");
    msg->set_case_name("messaging");

    writer.Write(msg);
    std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(500));
    cout << "writer sent a msg" << endl;

    assert(recv_msgs.size() == 2);

    assert(writer.HasReader());
    assert(reader_a.HasWriter());

    apollo::cyber::WaitForShutdown();

    writer.Shutdown();
    reader_a.Shutdown();
    reader_b.Shutdown();

    cout << "enddd" << endl;
}

int main(int argc, char** argv) 
{
    AINFO << "test case running...";
    AINFO << "msg size: " << std::stoi(argv[1]);
    AINFO << "msg interval: " << std::stoi(argv[2]) << " ms";
    test_case1(argv);

    return 0;
}