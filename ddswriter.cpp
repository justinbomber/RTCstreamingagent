#include <iostream>
#include <rti/rti.hpp>
#include "ddswriter.h"
#include <fstream>
// #include <dds/core/ddscore.hpp>
// #include <dds/pub/ddspub.hpp>
// #include <dds/sub/ddssub.hpp>

DDSWriter::~DDSWriter() {
}

DDSWriter::DDSWriter() {
    
}

DDSWriter::Result DDSWriter::splitString(const std::string& input) {
    Result result;

    // Locate the '/' character
    size_t slash_pos = input.find('/');
    if (slash_pos == std::string::npos) {
        std::cerr << "Cannot find '/' character!" << std::endl;
        return result; // Return an empty structure
    }

    // Extract position info from the beginning up to '/'
    std::string position = input.substr(0, slash_pos);
    // Extract source info after '/'
    result.source = input.substr(slash_pos + 1);

    // Locate the '-' character in position info
    size_t dash_pos = position.find('-');
    if (dash_pos == std::string::npos) {
        result.position_x = position;
    } else {
        result.position_x = position.substr(0, dash_pos);
        result.position_y = position.substr(dash_pos + 1);
    }

    return result;
}

std::vector<std::string> parseInput(const std::string& input) {
    std::vector<std::string> result;
    std::stringstream ss(input.substr(1, input.size() - 2));
    std::string token;
    while (std::getline(ss, token, ',')) {
        result.push_back(token);
    }
    return result;
}

void DDSWriter::query_writer(const std::string & username,
                            const std::vector<std::string> & ai_type,
                            const std::string & partition_device,
                            bool query_type,
                            const std::int64_t & starttime,
                            const std::int64_t & endtime,
                            const std::string & token,
                            const std::string & path,
                            uint8_t activate)
// TODO: partition_device need to seperate to source(uuid) and x y location.
{
    // Create a QosProvider (or use the default one)
    // TODO: add qos
    dds::pub::DataWriter<dds::core::xtypes::DynamicData> writer(dds::pub::Publisher(paas_participant), topicQuery);

    // Create a data sample and assign its values
    dds::core::xtypes::DynamicData sample(mytype);
    // Result result = splitString(partition_device);

    sample.value<unsigned char>("query_type", query_type);
    sample.value<std::string>("source", partition_device);
    sample.value<std::string>("partition_device", partition_device);
    sample.value<std::string>("partition_user", username);
    sample.value<std::int64_t>("unix_time_start", starttime);
    sample.value<std::int64_t>("unix_time_end", endtime);
    sample.value<std::uint8_t>("activate", activate);
    sample.value<std::string>("token", token);
    sample.value<std::string>("path", path);
    for (size_t i = 0; i < ai_type.size(); ++i) {
        std::stringstream ss;
        ss << "tag_objects[" << i << "].tag_object";
        std::string key = ss.str();
        sample.value<std::string>(key, ai_type[i]);
    }
    
    // Write the data sample
    int cc = 0;
    int target;
    if (activate == 0)
        target = 2;
    else
        target = 3;
    // while (true)
    while (cc < target)
    {
        writer.write(sample);
        auto nowwriteer = std::chrono::high_resolution_clock::now();
        auto writertopicepoch = std::chrono::duration_cast<std::chrono::milliseconds>(
            nowwriteer.time_since_epoch()
        ).count();
        std::cout << "=======================" << std::endl;
        std::cout << "finish sending tp_query " << cc << " time--->>>" << writertopicepoch << std::endl;
        std::cout << "+++++++++++++++++++++++" << std::endl;
        // Output a log
        std::cout << "Writer is writing a data sample. Value: " << sample;
        cc ++;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
