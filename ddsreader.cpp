#include <iostream>
#include <thread>
#include <fstream>
#include <rti/rti.hpp>
#include "ddsreader.h"
#include "ddswriter.h"

DDSReader::~DDSReader() {
}

DDSReader::DDSReader() {
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

void DDSReader::videostream_reader(std::map<UserDevice, UserTask> &taskmanager,
                                   UserDevice userdevice,
                                   bool & threadcontroll,
                                   dds::domain::DomainParticipant & ddscam_participant,
                                   std::string filepath)
{
    std::cout << "in reader-----------<<<<<<<<<<<<<" << std::endl;
    std::string partition = taskmanager[userdevice].partition_device;
    // Create a QosProvider (or use the default one)
    dds::core::QosProvider qos_provider("./ddscamxml/ddscam_qos.xml");
    // // Get the type called MyType
    const dds::core::xtypes::DynamicType &mytypeVideoStream = qos_provider.extensions().type("DdsCam::VideoStream");
    // Create Subscriber include partition of partitionDevice.
    dds::sub::Subscriber sub(ddscam_participant);
    dds::sub::qos::SubscriberQos subQos = sub.qos(); 
    auto &curPartition = subQos.policy<dds::core::policy::Partition>();
    std::vector<std::string> partitionNames = curPartition.name();
    partitionNames.push_back(partition);
    curPartition.name(partitionNames);
    sub.qos(subQos << curPartition);
    dds::topic::Topic<dds::core::xtypes::DynamicData> topicVideoStream(ddscam_participant, "Tp_VideoStream", mytypeVideoStream);

    std::string querycond = "source MATCH '" + taskmanager[userdevice].partition_device + "'";
    // Create the DataReader
    dds::sub::DataReader<dds::core::xtypes::DynamicData> reader(sub, topicVideoStream);
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
    while (threadcontroll)
    {
        // Read/take samples normally
        dds::sub::LoanedSamples<dds::core::xtypes::DynamicData> samples = reader.select().condition(cond).take();

        for (auto sample : samples)
        {
            if (sample.info().valid())
            {
                if (!threadcontroll)
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

                // 合成frame
                if(videoStream.flag == 1){
                    if (count == -1){
                        count++;
                        headframebuf = {};
                        bodyframebuf = {};
                        headframebuf.insert(headframebuf.end(), videoStream.frame.begin(), videoStream.frame.end());
                        continue;
                    }
                    headframebuf.insert(headframebuf.end(), bodyframebuf.begin(), bodyframebuf.end());
                    saveAsH264File(headframebuf, count, filepath);
                    headframebuf = {};
                    bodyframebuf = {};
                    headframebuf.insert(headframebuf.end(), videoStream.frame.begin(), videoStream.frame.end());
                    count++;
                } else {
                    bodyframebuf.insert(bodyframebuf.end(), videoStream.frame.begin(), videoStream.frame.end());
                }
            }
        }
    }
}

void DDSReader::playh264_reader(std::map<UserDevice, UserTask> &taskmanager,
                                UserDevice userdevice,
                                bool & threadcontroll,
                                dds::domain::DomainParticipant & paas_participant,
                                std::string filepath)
{
    // partition : device + user
    std::string partition = taskmanager[userdevice].partition_device + taskmanager[userdevice].token;
    dds::core::QosProvider qos_provider("./paasxml/pass_qos.xml");
    const dds::core::xtypes::DynamicType &mytypePlayH264 = qos_provider.extensions().type("Paas::Cam::PlayH264");

    dds::sub::Subscriber sub(paas_participant);
    dds::sub::qos::SubscriberQos subQos = sub.qos();
    auto &curPartition = subQos.policy<dds::core::policy::Partition>();
    std::vector<std::string> partitionNames = curPartition.name();
    partitionNames.push_back(partition);
    curPartition.name(partitionNames);
    sub.qos(subQos << curPartition);
    dds::topic::Topic<dds::core::xtypes::DynamicData> topic(paas_participant, "Tp_PlayH264", mytypePlayH264);

    std::string typeflag;
    if (taskmanager[userdevice].query_type)
        typeflag = "0x01";
    else
        typeflag = "0x00";
        

    std::string querycond = "source MATCH '" + taskmanager[userdevice].partition_device + "'";
                            + " AND query_type MATCH " + typeflag;
    // Create the DataReader
    dds::sub::DataReader<dds::core::xtypes::DynamicData> reader(sub, topic);
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
    while (threadcontroll)
    {
        // Read/take samples normally
        dds::sub::LoanedSamples<dds::core::xtypes::DynamicData> samples = reader.select().condition(cond).take();
        for (auto sample : samples)
        {
            if (sample.info().valid())
            {
                if (!threadcontroll){
                    DDSWriter ddswriter;
                    ddswriter.query_writer(taskmanager[userdevice].token, 
                                            taskmanager[userdevice].ai_type, 
                                            taskmanager[userdevice].partition_device, 
                                            taskmanager[userdevice].query_type, 
                                            taskmanager[userdevice].starttime, 
                                            taskmanager[userdevice].endtime, 
                                            0, std::ref(paas_participant));
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
