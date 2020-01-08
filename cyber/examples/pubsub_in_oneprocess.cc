#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "cyber/common/global_data.h"
#include "cyber/cyber.h"
#include "cyber/init.h"
#include "cyber/node/reader.h"
#include "cyber/node/writer.h"
#include "cyber/proto/unit_test.pb.h"


int main() 
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

    writer.Shutdown();
    reader_a.Shutdown();
    reader_b.Shutdown();

    cout << "enddd" << endl;

    return 0;
}