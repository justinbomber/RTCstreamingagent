// UserStructs.h
#ifndef COMMONSTRUCT_H
#define COMMONSTRUCT_H

#include <string>
#include <vector>
#include <rti/rti.hpp>
#include <map>

struct UserTask
{
  std::string token;
  std::string username;
  std::string path;
  std::vector<std::string> ai_type;
  std::string partition_device;
  bool query_type;
  std::int64_t starttime;
  std::int64_t endtime;
  std::string resolution;
  bool activate;
  bool threadcontroll;
  bool operator==(const UserTask &other) const
  {
    return token == other.token
        && username == other.username
        && path == other.path
        && ai_type == other.ai_type
        && partition_device == other.partition_device
        && query_type == other.query_type
        && starttime == other.starttime
        && endtime == other.endtime
        && resolution == other.resolution
        && activate == other.activate
        && threadcontroll == other.threadcontroll;
  }
};
struct UserDevice
{
  std::string token;
  std::string partition_device;
  bool operator<(const UserDevice &other) const
  {
    if (token < other.token)
      return true;
    if (token > other.token)
      return false;
    return partition_device < other.partition_device;
  }
    bool operator==(const UserDevice &other) const
  {
    return token == other.token && partition_device == other.partition_device;
  }
};
struct VideoStream {
    std::string source_id;
    std::string destination;
    int64_t unix_time;
    int32_t nanoseconds;
    int16_t format_code;
    uint16_t session_number;
    uint8_t flag;
    uint32_t sequence_number;
    int32_t frame_bytes;
    std::vector<uint8_t> frame;
};
struct PlayH264 {
    std::uint8_t query_type;
    std::string source;
    int64_t unix_time;
    int32_t nanoseconds;
    int16_t format_code;
    uint8_t flag;
    uint32_t sequence_number;
    int32_t frame_bytes;
    std::vector<uint8_t> frame;
};

//Define global map
extern std::map<UserDevice, UserTask> taskmanager;

// Define ddscam tp_videostream topic and qos
extern dds::domain::DomainParticipant ddscam_participant;
extern dds::core::QosProvider ddscam_qos;
extern const dds::core::xtypes::DynamicType &mytypeVideoStream;
extern dds::topic::Topic<dds::core::xtypes::DynamicData> topicVideoStream;

// Define paas tp_playh264 topic and qos
extern dds::domain::DomainParticipant paas_participant;
extern dds::core::QosProvider paas_qos;
extern const dds::core::xtypes::DynamicType &mytypePlayH264;
extern dds::topic::Topic<dds::core::xtypes::DynamicData> topicPlayH264;

extern const dds::core::xtypes::DynamicType &mytype;
extern dds::topic::Topic<dds::core::xtypes::DynamicData> topicQuery;



#endif // COMMONSTRUCT_H
