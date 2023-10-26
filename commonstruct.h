// UserStructs.h
#ifndef COMMONSTRUCT_H
#define COMMONSTRUCT_H

#include <string>
#include <vector>
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



#endif // COMMONSTRUCT_H
