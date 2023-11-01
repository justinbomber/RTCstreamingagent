// rtspServer.h

#ifndef RTSPSERVER_H
#define RTSPSERVER_H

#include "liveMedia.hh"
#include "GroupsockHelper.hh"
#include "announceURL.hh"
#include <string>
#include <functional>
#include "H264UDPServerMediaSubsession.hh"
#include "BasicUsageEnvironment.hh"

class RTSPServerManager {
public:
  RTSPServerManager();  // 建構函數
  ~RTSPServerManager();  // 解構函數

  std::string getURL();
  void startserver(const int serverport, 
                   portNumBits const udpport, 
                   const std::string urlname,
                   portNumBits const httptunnelingport);
                  //  bool &threadcontroll);

private:
  UsageEnvironment* env;
  Boolean reuseFirstSource;
  std::string url = "noURL";
};

#endif // RTSPSERVER_H
