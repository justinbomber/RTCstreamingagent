// rtspServer.h

#ifndef RTSPSERVER_H
#define RTSPSERVER_H

#include "liveMedia.hh"
#include "GroupsockHelper.hh"
#include "announceURL.hh"
#include "H264UDPServerMediaSubsession.hh"
#include "BasicUsageEnvironment.hh"
#include "commonstruct.h"
#include <string>
#include <functional>
#include <chrono>

class RTSPServerManager
{
public:
    RTSPServerManager();  // 建構函數
    ~RTSPServerManager(); // 解構函數

    void startserver(const int serverport,
                     portNumBits const udpport,
                     const std::string udpip,
                     const std::string urlname,
                     portNumBits const httptunnelingport,
                     UserTask &usertask);
    //  bool &threadcontroll);

private:
    UsageEnvironment *env;
    Boolean reuseFirstSource;
};

#endif // RTSPSERVER_H
