// UserStructs.h
#ifndef COMMONSTRUCT_H
#define COMMONSTRUCT_H

#include <string>
#include <vector>
#include <rti/rti.hpp>
#include <map>
#include <atomic>
#include "appConfig.h"
#include "liveMedia.hh"
#include <thread>

// void initialize();
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
  std::time_t timestampnow;
  // std::thread::id thread_id;
  pthread_t thread_id;
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
        && threadcontroll == other.threadcontroll
        && token == other.token
        && path == other.path;
  }
};
struct UserDevice
{
  std::string token;
  std::string partition_device;
  std::time_t timestampnow;
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
    return token == other.token && 
    partition_device == other.partition_device &&
    timestampnow == other.timestampnow;
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
struct H2642Ai {
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


class CommonStruct{

  public:
    CommonStruct();

    //Define global map
    // std::map<UserDevice, UserTask> taskmanager;

    std::string postgreshost;
    int postgresport;
    std::string postgresuser;
    std::string postgrespassword;
    std::string postgresdb;

    std::string websocketip;
    std::string websocketport;
    std::string websocketpath;

    std::string ddscam_qos_path;
    std::string ddscam_typedef_path;

    std::string paas_qos_path;
    std::string paas_typedef_path;

    std::string local_serverip;
    std::string local_rootpath;
    std::int32_t local_domainid;
    std::string local_udpip;
    portNumBits local_udpport;

};

extern CommonStruct commonstruct;
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
extern const dds::core::xtypes::DynamicType &mytypeH2642Ai;
extern dds::topic::Topic<dds::core::xtypes::DynamicData> topicH2642Ai;

extern const dds::core::xtypes::DynamicType &mytypePlayH264;
extern dds::topic::Topic<dds::core::xtypes::DynamicData> topicPlayH264;

extern const dds::core::xtypes::DynamicType &mytype;
extern dds::topic::Topic<dds::core::xtypes::DynamicData> topicQuery;


#endif // COMMONSTRUCT_H
