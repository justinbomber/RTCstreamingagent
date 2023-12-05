#ifndef DDSREADER_H
#define DDSREADER_H
#include <iostream>
#include <rti/rti.hpp>
#include <thread>
#include <cstring> // for std::memset
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // for close
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <queue>
#include "sub_thread.h"
#include "commonstruct.h"
#include "string"
#include "BasicUsageEnvironment.hh"
#include "liveMedia.hh"
#include "ddsStreamTransformer.hpp"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>

class DDSReader {
    private:
        std::uint32_t last_sequence = 0;
        bool isShutdown_ = false;

    public:
        DDSReader();
        ~DDSReader();
        // DDSReader(sub_thread& sub_thread) : sub_thread_(sub_thread) {};
        std::queue<std::vector<uint8_t>> framequeue;
        void videostream_reader(UserTask &usertask,
                                std::string filepath,
                                std::uint64_t port);

        void h2642ai_reader(UserTask &usertask,
                            std::string filepath,
                            std::uint64_t port);
        void playh264_reader(UserTask &usertask,
                            std::string filepath,
                            std::string inputpath,
                            std::uint64_t port);



};

#endif // DDSREADER_H