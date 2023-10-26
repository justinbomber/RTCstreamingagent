#ifndef DDSREADER_H
#define DDSREADER_H

#include <iostream>
#include <rti/rti.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include "sub_thread.h"
#include "commonstruct.h"
#include "string"

class DDSReader {
    private:
        std::uint32_t last_sequence = 0;
        // std::queue<std::vector<uint8_t>> queue_;
        // std::mutex mtx_;
        // std::condition_variable condVar_;
        bool isShutdown_ = false;
        // sub_thread& sub_thread_;

    public:
        DDSReader();
        ~DDSReader();
        // DDSReader(sub_thread& sub_thread) : sub_thread_(sub_thread) {};
        void videostream_reader(UserTask &usertask,
                                std::string filepath);

        void playh264_reader(UserTask &usertask,
                            std::string filepath);



};

#endif // DDSREADER_H