#include <iostream>
#include <thread>
#include <fstream>
#include <rti/rti.hpp>
#include "ddsreader.h"
#include "ddswriter.h"
#include <GroupsockHelper.hh>

DDSReader::~DDSReader() {
}

DDSReader::DDSReader() {

}

const size_t MAX_PACKET_SIZE = 1472;  // 最大UDP封包大小

void sendLargeData(int sock, const uint8_t* data, size_t dataSize, struct sockaddr_in& addr) {
    size_t totalSent = 0;

    while (totalSent < dataSize) {
        size_t toSend = std::min(dataSize - totalSent, MAX_PACKET_SIZE);
        
        if (sendto(sock, data + totalSent, toSend, 0, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("sendto() failed");
            break;
        }

        totalSent += toSend;
    }
}

void saveAsH264File(const std::vector<uint8_t>& data, int num, std::string filepath) {
    std::string filename = filepath + "sample-" + std::to_string(num) + ".h264";
    std::cout << "save as h264 file: " << filename << std::endl;
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile.is_open())
        return;
    outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
    outFile.close();
}



void DDSReader::videostream_reader(UserTask & usertask,
                                   std::string filepath,
                                   std::uint64_t port)
{
    int sock;
    struct sockaddr_in addr;
    const char *multicast_ip = "239.255.42.42";
    // port = 1250;  // 你可以更改此端口
    std::cout << "multicast_ip,prot: " << multicast_ip << ":" << port << std::endl;
    
    // 創建socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket() failed");
        return;
    }

    // 設定目的地址
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(multicast_ip);
    addr.sin_port = htons(port);

    // ddsreader
    std::string partition = usertask.partition_device;
    dds::sub::Subscriber sub(ddscam_participant);
    dds::sub::qos::SubscriberQos subQos = sub.qos(); 
    auto &curPartition = subQos.policy<dds::core::policy::Partition>();
    std::vector<std::string> partitionNames = curPartition.name();
    partitionNames.push_back(partition);
    curPartition.name(partitionNames);
    sub.qos(subQos << curPartition);

    dds::sub::DataReader<dds::core::xtypes::DynamicData> reader(sub, topicVideoStream);

    // count要改
    // int count = -1;
    int count = 0;
    std::vector<uint8_t> bodyframebuf = {};
    std::vector<uint8_t> headframebuf = {};
    while (usertask.threadcontroll)
    {
        // Read/take samples normally
        dds::sub::LoanedSamples<dds::core::xtypes::DynamicData> samples = reader.select().take();

        for (auto sample : samples)
        {
            if (sample.info().valid())
            {
                if (!usertask.threadcontroll)
                    return;
                VideoStream videoStream;
                dds::core::xtypes::DynamicData& data = const_cast<dds::core::xtypes::DynamicData&>(sample.data());
                videoStream.source_id = data.value<std::string>("source");
                videoStream.destination = data.value<std::string>("destination");
                videoStream.unix_time = data.value<int64_t>("unix_time");
                videoStream.nanoseconds = data.value<int32_t>("nanoseconds");
                videoStream.format_code = data.value<int16_t>("format_code");
                videoStream.session_number = data.value<uint16_t>("session_number");
                videoStream.flag = data.value<uint8_t>("flag");
                videoStream.sequence_number = data.value<uint32_t>("sequence_number");
                videoStream.frame_bytes = data.value<int32_t>("frame_bytes");
                videoStream.frame = data.get_values<uint8_t>("frame");
                
                sendLargeData(sock, videoStream.frame.data(), videoStream.frame.size(), addr);
            }
        }
    }
}

void DDSReader::playh264_reader(UserTask &usertask,
                                std::string filepath,
                                std::uint64_t port)
{
    int sock;
    struct sockaddr_in addr;
    const char *multicast_ip = "239.255.42.42";
    std::cout << "multicast_ip,prot: " << multicast_ip << ":" << port << std::endl;
    // port = 1250;  // 你可以更改此端口
    
    // 創建socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket() failed");
        return;
    }

    // 設定目的地址
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(multicast_ip);
    addr.sin_port = htons(port);

    std::string partition = usertask.partition_device + usertask.username;
    bool query_type = usertask.query_type;
    dds::sub::Subscriber sub(paas_participant);
    dds::sub::qos::SubscriberQos subQos = sub.qos();
    auto &curPartition = subQos.policy<dds::core::policy::Partition>();
    std::vector<std::string> partitionNames = curPartition.name();
    partitionNames.push_back(partition);
    curPartition.name(partitionNames);
    sub.qos(subQos << curPartition);

    std::string typeflag;
    if (usertask.query_type)
        typeflag = "0x01";
    else
        typeflag = "0x00";
        

    std::string querycond = "source MATCH '" + usertask.partition_device + "'";
                            + " AND query_type MATCH " + typeflag;
    // Create the DataReader
    dds::sub::DataReader<dds::core::xtypes::DynamicData> reader(sub, topicPlayH264);
    dds::sub::cond::QueryCondition cond(
                    dds::sub::Query(reader, querycond),
                    dds::sub::status::DataState(
                        dds::sub::status::SampleState::any(),
                        dds::sub::status::ViewState::any(),
                        dds::sub::status::InstanceState::alive()));


    int count = -1;
    std::vector<uint8_t> bodyframebuf = {};
    std::vector<uint8_t> headframebuf = {};
    // Read the data sample
    while (usertask.threadcontroll)
    {
        // Read/take samples normally
        dds::sub::LoanedSamples<dds::core::xtypes::DynamicData> samples = reader.select().condition(cond).take();
        for (auto sample : samples)
        {
            if (sample.info().valid())
            {
                if (!usertask.threadcontroll){
                    DDSWriter ddswriter;
                    ddswriter.query_writer(usertask.token, 
                                            usertask.ai_type, 
                                            usertask.partition_device, 
                                            usertask.query_type, 
                                            usertask.starttime, 
                                            usertask.endtime, 
                                            0);
                    return;
                }
                PlayH264 playH264;
                dds::core::xtypes::DynamicData &data = const_cast<dds::core::xtypes::DynamicData &>(sample.data());
                playH264.query_type = data.value<std::uint8_t>("query_type");
                playH264.source = data.value<std::string>("source");
                playH264.unix_time = data.value<int64_t>("unix_time");
                playH264.nanoseconds = data.value<int32_t>("nanoseconds");
                playH264.format_code = data.value<int16_t>("format_code");
                playH264.flag = data.value<uint8_t>("flag");
                playH264.sequence_number = data.value<uint32_t>("sequence_number");
                playH264.frame_bytes = data.value<int32_t>("frame_bytes");
                playH264.frame = data.get_values<uint8_t>("frame");

                if (usertask.ai_type.size() > 0 && usertask.query_type)
                    sendLargeData(sock, playH264.frame.data(), playH264.frame.size(), addr);
                else{
                    if(playH264.flag == 1){
                        if (count == -1){
                            count++;
                            headframebuf = {};
                            bodyframebuf = {};
                            headframebuf.insert(headframebuf.end(), playH264.frame.begin(), playH264.frame.end());
                            continue;
                        }
                        headframebuf.insert(headframebuf.end(), bodyframebuf.begin(), bodyframebuf.end());
                        saveAsH264File(headframebuf, count, filepath);
                        headframebuf = {};
                        bodyframebuf = {};
                        headframebuf.insert(headframebuf.end(), playH264.frame.begin(), playH264.frame.end());
                        count++;
                    } else {
                        bodyframebuf.insert(bodyframebuf.end(), playH264.frame.begin(), playH264.frame.end());
                    }
                }
            }
        }
    }
    if (!usertask.threadcontroll){
        DDSWriter ddswriter;
        ddswriter.query_writer(usertask.token, 
                                usertask.ai_type, 
                                usertask.partition_device, 
                                usertask.query_type, 
                                usertask.starttime, 
                                usertask.endtime, 
                                0);
        return;
    }
}
