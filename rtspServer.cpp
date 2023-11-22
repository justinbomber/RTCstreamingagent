#include "rtspServer.h"
#include <iostream>

RTSPServerManager::RTSPServerManager() {
}

RTSPServerManager::~RTSPServerManager() {
}

// TODO : need control to stop server
void RTSPServerManager::startserver(const int serverport, 
                                    portNumBits const udpport, 
                                    const std::string udpip,
                                    const std::string urlname,
                                    portNumBits const httptunnelingport)
                                    {
  OutPacketBuffer::maxSize = 300000;
  UsageEnvironment* env;
  Boolean reuseFirstSource = False;
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  env = BasicUsageEnvironment::createNew(*scheduler);
  // OutPacketBuffer::maxSize = 3000000;

  UserAuthenticationDatabase* authDB = NULL;
  RTSPServer* rtspServer = RTSPServer::createNew(*env, serverport, authDB);

  if (rtspServer == NULL) {
    *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
    exit(1);
  }

  char const* descriptionString = "Session streamed by \"testOnDemandRTSPServer\"";
  char const* streamName = urlname.c_str();
  char const* inputAddressStr = udpip.c_str();
  portNumBits const inputPortNum = udpport;

  ServerMediaSession* sms
    = ServerMediaSession::createNew(*env, streamName, streamName,
            descriptionString);
  sms->addSubsession(H264UDPServerMediaSubsession
          ::createNew(*env, inputAddressStr, inputPortNum));
  rtspServer->addServerMediaSession(sms);

  *env << "\n\"" << streamName << "\" stream, from a UDP Elementary Stream input source \n\t(";
  if (inputAddressStr != NULL) {
    *env << "IP multicast address " << inputAddressStr << ",";
  } else {
    *env << "unicast;";
  }
  *env << " port " << inputPortNum << ")\n";
  announceURL(rtspServer, sms);


  if (rtspServer->setUpTunnelingOverHTTP(httptunnelingport)) {
    *env << "\n(We use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling.)\n";
  } else {
    *env << "\n(RTSP-over-HTTP tunneling is not available.)\n";
  }

  // char eventLoopWatchVariable = 0;
  // env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
  env->taskScheduler().doEventLoop();
  return;
}
