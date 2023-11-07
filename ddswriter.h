#ifndef DDSWRITER_H
#define DDSWRITER_H

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <rti/rti.hpp>
#include "sub_thread.h"
#include "commonstruct.h"

class DDSWriter {
    private:
        // 加入splitString方法
        struct Result {
            std::string position_x;
            std::string position_y;
            std::string source;
        };

    public:
        ~DDSWriter();
        DDSWriter();
        // DDSWriter (sub_thread& sub_thread) : sub_thread_(sub_thread) {};
        void query_writer(const std::string & username,
                        const std::vector<std::string> & ai_type,
                        const std::string & partition_device,
                        bool query_type,
                        const std::int64_t & starttime,
                        const std::int64_t & endtime,
                        const std::string & token,
                        const std::string & path,
                        uint8_t activate);
        Result splitString(const std::string& input);
};

#endif // DDSWRITER_H





