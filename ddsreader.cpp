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

void appendToM3U8File(std::string m3u8name, const std::string &directory, const std::string &newString)
{
    std::string filename = directory + m3u8name + ".m3u8";

    // 打開已存在的文件用於追加
    std::ofstream outFile(filename, std::ios::app);

    // 檢查文件是否成功打開
    if (!outFile)
    {
        std::cerr << "無法打開文件" << std::endl;
        return;
    }
    outFile << "#EXTINF:1.000000,\n";
    outFile << newString << "\n";
    outFile << "#EXT-X-DISCONTINUITY\n";

    // 關閉文件
    outFile.close();
}

void saveAsH264File(const std::vector<uint8_t> &data, int num, std::string filepath)
{
    auto savefiletime = std::chrono::high_resolution_clock::now();
    auto savefileepoch = std::chrono::duration_cast<std::chrono::milliseconds>(
        savefiletime.time_since_epoch()
    ).count();
    std::cout << "=======================" << std::endl;
    std::cout << "start save h264 file time --->>>" << savefileepoch << std::endl;
    std::cout << "+++++++++++++++++++++++" << std::endl;

    std::string filename = filepath + "sample-" + std::to_string(num) + ".h264";
    std::cout << "save as h264 file: " << filename << std::endl;
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile.is_open())
        return;
    outFile.write(reinterpret_cast<const char *>(data.data()), data.size());
    outFile.close();

    savefiletime = std::chrono::high_resolution_clock::now();
    savefileepoch = std::chrono::duration_cast<std::chrono::milliseconds>(
        savefiletime.time_since_epoch()
    ).count();
    std::cout << "=======================" << std::endl;
    std::cout << "finish save h264 file time --->>>" << savefileepoch << std::endl;
    std::cout << "+++++++++++++++++++++++" << std::endl;
}

void delete_all_files(const std::filesystem::path &path)
{
    for (const auto &entry : std::filesystem::directory_iterator(path))
    {
        if (entry.is_regular_file())
        {
            std::filesystem::remove(entry.path());
        }
        std::filesystem::remove_all(entry.path());
    }
}

void transferdata(const std::string &targetFolder, 
                UserTask &usertask, 
                const std::string &inputfolder)
{
    int count = 0;
    bool should_out = 0;
    while(usertask.threadcontroll){
        int file_count = 1;
        for (const auto &entry : std::filesystem::directory_iterator(inputfolder))
            if (entry.is_regular_file())
                ++file_count;
        if (file_count > 0)
        {
            if (!should_out){
                auto nowws = std::chrono::high_resolution_clock::now();
                auto receivedwsepoch = std::chrono::duration_cast<std::chrono::milliseconds>(
                                        nowws.time_since_epoch())
                                        .count();
                std::cout << "=======================" << std::endl;
                std::cout << "start transfer h264 to ts data --->>>" << receivedwsepoch << std::endl;
                std::cout << "+++++++++++++++++++++++" << std::endl;
            }
            std::string cmdline;
            appendToM3U8File(usertask.path, targetFolder, "\'sample-" + std::to_string(count) + ".ts\'");
            std::string filefix = "sample-" + std::to_string(count);
            std::filesystem::path targetfile = inputfolder + filefix + ".h264";
            cmdline = "ffmpeg -i " + inputfolder + filefix + ".h264 -preset ultrafast -c:v libx264 -c:a aac -s 1920x1080 " +
                                targetFolder + filefix + ".ts && mv " +
                                targetFolder + filefix + ".ts " +
                                targetFolder + "\"\'" + filefix + ".ts\'\"";
            system(cmdline.c_str());
            std::filesystem::remove(targetfile);
            count++;
            if (!should_out){
                auto nowws = std::chrono::high_resolution_clock::now();
                auto receivedwsepoch = std::chrono::duration_cast<std::chrono::milliseconds>(
                                        nowws.time_since_epoch())
                                        .count();
                std::cout << "=======================" << std::endl;
                std::cout << "finish transfer h264 to ts data --->>>" << receivedwsepoch << std::endl;
                std::cout << "+++++++++++++++++++++++" << std::endl;
                should_out = true;
            }
        } else {
            if(should_out)
                break;
            else
                continue;
        }
    }
    while (usertask.threadcontroll){
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    std::filesystem::remove_all(inputfolder);
    std::filesystem::remove_all(targetFolder);
}

void DDSReader::videostream_reader(UserTask &usertask,
                                   std::string filepath,
                                   std::uint64_t port)
{

    auto nowreaderstart = std::chrono::high_resolution_clock::now();
    auto startreaderepoch = std::chrono::duration_cast<std::chrono::milliseconds>(
        nowreaderstart.time_since_epoch()
    ).count();
    std::cout << "=======================" << std::endl;
    std::cout << "start video staream reader time --->>>" << startreaderepoch << std::endl;
    std::cout << "+++++++++++++++++++++++" << std::endl;

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
    int time_duration = 0;
    if (usertask.resolution == "1080"){
        time_duration = 500;
    } else {
        time_duration = 1500;
    }

    bool GotKeyFrame = false;
    bool first = true;
    bool first480 = true;

    while (usertask.threadcontroll)
    {
        // Read/take samples normally
        dds::sub::LoanedSamples<dds::core::xtypes::DynamicData> samples = reader.select().take();
        now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() >= time_duration)
        {
            // usertask.threadcontroll = false;
            break;
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

                if (usertask.resolution == "480") {
                    if (first480){
                        auto nowsedframe = std::chrono::high_resolution_clock::now();
                        auto sendframeepoch = std::chrono::duration_cast<std::chrono::milliseconds>(
                            nowsedframe.time_since_epoch()
                        ).count();
                        std::cout << "=======================" << std::endl;
                        std::cout << videoStream.sequence_number << "<--- start transfer 480 frame time --->>>" << sendframeepoch << std::endl;
                        std::cout << "+++++++++++++++++++++++" << std::endl;
                    }
                    if (!GotKeyFrame) {
                        if (videoStream.flag == 1)
                            GotKeyFrame = true;
                        else {
                            std::cout << "Not Key Frame" << std::endl;
                            continue;
                        }
                    }
                    if (!h264480decoder.convertH264(videoStream.frame, frame264))
                    {
                            std::cerr << "Error converting 480P\n";
                            first480 = true;
                    }
                    else{
                        if (first480){
                            auto nowsedframe = std::chrono::high_resolution_clock::now();
                            auto sendframeepoch = std::chrono::duration_cast<std::chrono::milliseconds>(
                                nowsedframe.time_since_epoch()
                            ).count();
                            std::cout << "=======================" << std::endl;
                            std::cout << videoStream.sequence_number << "<---finish transfer 480 time --->>>" << sendframeepoch << std::endl;
                            std::cout << "+++++++++++++++++++++++" << std::endl;
                            first480 = false;
                        }
                    }
                } else {
                    frame264 = videoStream.frame;
                }

                sendLargeData(sock, frame264.data(), frame264.size(), addr);
                if (first){
                    usertask.videocontroll = true;
                    auto nowsedframe = std::chrono::high_resolution_clock::now();
                    auto sendframeepoch = std::chrono::duration_cast<std::chrono::milliseconds>(
                        nowsedframe.time_since_epoch()
                    ).count();
                    std::cout << "=======================" << std::endl;
                    std::cout << "send first frame time --->>>" << sendframeepoch << std::endl;
                    std::cout << "+++++++++++++++++++++++" << std::endl;
                    first = false;
                }
                start = std::chrono::steady_clock::now();
            }
        }
    }
    if (usertask.resolution == "480"){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        json_obj["partition_device"] = usertask.partition_device;
        json_obj["type"] = "disconnected";
        commonstruct.write(json_obj.dump());
    }
}

void DDSReader::h2642ai_reader(UserTask &usertask,
                                std::string filepath,
                                std::string inputpath,
                                std::uint64_t port)
{
    auto nowreaderstart = std::chrono::high_resolution_clock::now();
    auto startreaderepoch = std::chrono::duration_cast<std::chrono::milliseconds>(
        nowreaderstart.time_since_epoch()
    ).count();
    std::cout << "=======================" << std::endl;
    std::cout << "start h264ai reader time --->>>" << startreaderepoch << std::endl;
    std::cout << "+++++++++++++++++++++++" << std::endl;

    // Set partition
    std::string partition = usertask.partition_device + "/" + usertask.username;
    dds::sub::Subscriber sub(paas_participant);
    dds::sub::qos::SubscriberQos subQos = sub.qos();
    auto &curPartition = subQos.policy<dds::core::policy::Partition>();
    std::vector<std::string> partitionNames = curPartition.name();
    partitionNames.push_back(partition);
    curPartition.name(partitionNames);
    sub.qos(subQos << curPartition);

    // Create the DataReader
    dds::sub::DataReader<dds::core::xtypes::DynamicData> reader(sub, topicH2642Ai);

    int count = -1;
    std::vector<uint8_t> bodyframebuf = {};
    std::vector<uint8_t> headframebuf = {};
    // std::vector<uint8_t> framebuffer = {};
    auto start = std::chrono::steady_clock::now();
    bool first = true;

    auto transfer_t = std::bind(&transferdata, 
                                filepath, 
                                std::ref(usertask), 
                                inputpath);
    std::thread transfer_thread(transfer_t);
    transfer_thread.detach();

    // Read the data sample
    while (usertask.threadcontroll)
    {
        // Read/take samples normally
        dds::sub::LoanedSamples<dds::core::xtypes::DynamicData> samples = reader.select().take();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() >= 5)
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

                H2642Ai h2642Ai;
                dds::core::xtypes::DynamicData &data = const_cast<dds::core::xtypes::DynamicData &>(sample.data());
                h2642Ai.query_type = data.value<std::uint8_t>("query_type");
                h2642Ai.source = data.value<std::string>("source");
                h2642Ai.unix_time = data.value<int64_t>("unix_time");
                h2642Ai.nanoseconds = data.value<int32_t>("nanoseconds");
                h2642Ai.format_code = data.value<int16_t>("format_code");
                h2642Ai.flag = data.value<uint8_t>("flag");
                h2642Ai.sequence_number = data.value<uint32_t>("sequence_number");
                h2642Ai.frame_bytes = data.value<int32_t>("frame_bytes");
                h2642Ai.frame = data.get_values<uint8_t>("frame");
                if (first){
                    auto nowaiframe = std::chrono::high_resolution_clock::now();
                    auto recivedaiframeepoch = std::chrono::duration_cast<std::chrono::milliseconds>(
                        nowaiframe.time_since_epoch()
                    ).count();
                    std::cout << "=======================" << std::endl;
                    std::cout << "recieved h264ai fiirst frame time --->>>" << recivedaiframeepoch << std::endl;
                    std::cout << "+++++++++++++++++++++++" << std::endl;
                    first = false;
                }

                if (h2642Ai.flag == 1)
                {
                    if (count == -1)
                    {
                        count++;
                        headframebuf = {};
                        bodyframebuf = {};
                        headframebuf.insert(headframebuf.end(), h2642Ai.frame.begin(), h2642Ai.frame.end());
                        continue;
                    }
                    headframebuf.insert(headframebuf.end(), bodyframebuf.begin(), bodyframebuf.end());
                    saveAsH264File(headframebuf, count, inputpath);
                    headframebuf = {};
                    bodyframebuf = {};
                    headframebuf.insert(headframebuf.end(), h2642Ai.frame.begin(), h2642Ai.frame.end());
                    count++;
                }
                else
                {
                    bodyframebuf.insert(bodyframebuf.end(), h2642Ai.frame.begin(), h2642Ai.frame.end());
                }
                start = std::chrono::steady_clock::now();
            }
        }
    }
}

void DDSReader::playh264_reader(UserTask &usertask,
                                std::string filepath,
                                std::string inputpath,
                                std::uint64_t port)
{
    auto nowreaderstart = std::chrono::high_resolution_clock::now();
    auto startreaderepoch = std::chrono::duration_cast<std::chrono::milliseconds>(
        nowreaderstart.time_since_epoch()
    ).count();
    std::cout << "=======================" << std::endl;
    std::cout << "start playh264 reader time --->>>" << startreaderepoch << std::endl;
    std::cout << "+++++++++++++++++++++++" << std::endl;

    // Set partition
    std::string partition = usertask.partition_device + "/" + usertask.username;
    dds::sub::Subscriber sub(paas_participant);
    dds::sub::qos::SubscriberQos subQos = sub.qos();
    auto &curPartition = subQos.policy<dds::core::policy::Partition>();
    std::vector<std::string> partitionNames = curPartition.name();
    partitionNames.push_back(partition);
    curPartition.name(partitionNames);
    sub.qos(subQos << curPartition);

    // Create the DataReader
    dds::sub::DataReader<dds::core::xtypes::DynamicData> reader(sub, topicPlayH264);

    int count = -1;
    std::vector<uint8_t> bodyframebuf = {};
    std::vector<uint8_t> headframebuf = {};
    // std::vector<uint8_t> framebuffer = {};
    auto start = std::chrono::steady_clock::now();
    bool first = true;

    auto transfer_t = std::bind(&transferdata, 
                                filepath, 
                                std::ref(usertask), 
                                inputpath);
    std::thread transfer_thread(transfer_t);
    transfer_thread.detach();

    // Read the data sample
    while (usertask.threadcontroll)
    {
        // Read/take samples normally
        dds::sub::LoanedSamples<dds::core::xtypes::DynamicData> samples = reader.select().take();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() >= 5)
        {
            // usertask.threadcontroll = false;
            return;
        }
        for (auto sample : samples)
        {
            if (sample.info().valid())
            {
                if (!usertask.threadcontroll)
                    return;
                
                PlayH264 playh264;
                dds::core::xtypes::DynamicData &data = const_cast<dds::core::xtypes::DynamicData &>(sample.data());
                playh264.query_type = data.value<std::uint8_t>("query_type");
                playh264.source = data.value<std::string>("source");
                playh264.unix_time = data.value<int64_t>("unix_time");
                playh264.nanoseconds = data.value<int32_t>("nanoseconds");
                playh264.format_code = data.value<int16_t>("format_code");
                playh264.flag = data.value<uint8_t>("flag");
                playh264.sequence_number = data.value<uint32_t>("sequence_number");
                playh264.frame_bytes = data.value<int32_t>("frame_bytes");
                playh264.frame = data.get_values<uint8_t>("frame");
                if (first){
                    auto now264frame = std::chrono::high_resolution_clock::now();
                    auto recived264frameepoch = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now264frame.time_since_epoch()
                    ).count();
                    std::cout << "=======================" << std::endl;
                    std::cout << "recieved playh264 fiirst frame time --->>>" << recived264frameepoch << std::endl;
                    std::cout << "+++++++++++++++++++++++" << std::endl;
                    first = false;
                }

                if (playh264.flag == 1)
                {
                    if (count == -1)
                    {
                        count++;
                        headframebuf = {};
                        bodyframebuf = {};
                        headframebuf.insert(headframebuf.end(), playh264.frame.begin(), playh264.frame.end());
                        continue;
                    }
                    headframebuf.insert(headframebuf.end(), bodyframebuf.begin(), bodyframebuf.end());
                    saveAsH264File(headframebuf, count, inputpath);
                    headframebuf = {};
                    bodyframebuf = {};
                    headframebuf.insert(headframebuf.end(), playh264.frame.begin(), playh264.frame.end());
                    count++;
                }
                else
                {
                    bodyframebuf.insert(bodyframebuf.end(), playh264.frame.begin(), playh264.frame.end());
                }
                start = std::chrono::steady_clock::now();
            }
        }
    }
}
