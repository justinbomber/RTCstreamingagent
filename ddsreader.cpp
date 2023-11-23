#include <iostream>
#include <thread>
#include <fstream>
#include <rti/rti.hpp>
#include "ddsreader.h"
#include "ddswriter.h"
#include <GroupsockHelper.hh>
#include <chrono>



DDSReader::~DDSReader()
{
}

DDSReader::DDSReader()
{
}

const size_t MAX_PACKET_SIZE = 1472; // 最大UDP封包大小

void sendLargeData(int sock, const uint8_t *data, size_t dataSize, struct sockaddr_in &addr)
{
    OutPacketBuffer::maxSize = 300000;

    size_t totalSent = 0;

    while (totalSent < dataSize)
    {
        size_t toSend = std::min(dataSize - totalSent, MAX_PACKET_SIZE);

        if (sendto(sock, data + totalSent, toSend, 0, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            perror("sendto() failed");
            break;
        }

        totalSent += toSend;
    }
}

// todo :: clean file path
void DDSReader::videostream_reader(UserTask &usertask,
                                   std::string filepath,
                                   std::uint64_t port)
{
    int sock;
    struct sockaddr_in addr;
    const char *multicast_ip = "239.255.42.42";
    std::cout << "multicast_ip,prot: " << multicast_ip << ":" << port << std::endl;

    // create socket connection
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        std::cout << "socket failed" << std::endl;
        return;
    }

    // initialize
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

    int count = -1;
    std::vector<uint8_t> bodyframebuf = {};
    std::vector<uint8_t> headframebuf = {};
    PaaS::H264Converter h264480decoder;
    PaaS::FFmpegDecoder ffmpegdecode;
    if (!ffmpegdecode.initializeDecoder()) 
    {
        std::cout << "Error initializing decoder\n";
        return;
    }

    std::vector<uint8_t> frame264;
    auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    nlohmann::json json_obj;

    bool GotKeyFrame = false;
    bool first = true;
    int time_duration = 1500;

    while (usertask.threadcontroll)
    {
        // Read/take samples normally
        dds::sub::LoanedSamples<dds::core::xtypes::DynamicData> samples = reader.select().take();
        now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() >= time_duration)
        {
            usertask.threadcontroll = false;
            return;
        }

        for (auto sample : samples)
        {
            if (sample.info().valid())
            {
                if (!usertask.threadcontroll)
                    return;
                VideoStream videoStream;
                dds::core::xtypes::DynamicData &data = const_cast<dds::core::xtypes::DynamicData &>(sample.data());
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

                if (!GotKeyFrame) {
                    if (videoStream.flag == 1)
                        GotKeyFrame = true;
                    else {
                        std::cout << "Not Key Frame" << std::endl;
                        continue;
                    }
                }
                if (!h264480decoder.convertH264(videoStream.frame, frame264))
                        std::cerr << "Error converting 480P\n";
                sendLargeData(sock, frame264.data(), frame264.size(), addr);
                if (first){
                    usertask.videocontroll = true;
                }
                start = std::chrono::steady_clock::now();
            }
        }
    }
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() >= time_duration){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        json_obj["partition_device"] = usertask.partition_device;
        json_obj["type"] = "disconnected";
        commonstruct.write(json_obj.dump());
    }
}
