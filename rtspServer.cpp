#include "rtspServer.h"

RTSPServerManager::RTSPServerManager() {
  UsageEnvironment* env;
  Boolean reuseFirstSource = False;
}

RTSPServerManager::~RTSPServerManager() {
  // 在此進行清理，例如釋放內存或關閉文件
}

std::string RTSPServerManager::getURL() {
  return url;
}


// TODO : need control to stop server
void RTSPServerManager::startserver(const int serverport, portNumBits const udpport, const std::string urlname) {
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  env = BasicUsageEnvironment::createNew(*scheduler);

  UserAuthenticationDatabase* authDB = NULL;
  RTSPServer* rtspServer = RTSPServer::createNew(*env, serverport, authDB);

  if (rtspServer == NULL) {
    *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
    exit(1);
  }

  char const* descriptionString = "Session streamed by \"testOnDemandRTSPServer\"";
  char const* streamName = urlname.c_str();
  char const* inputAddressStr = "239.255.42.42";
  portNumBits const inputPortNum = udpport;

  ServerMediaSession* sms = ServerMediaSession::createNew(*env, streamName, streamName, descriptionString);
  sms->addSubsession(H264UDPServerMediaSubsession::createNew(*env, inputAddressStr, inputPortNum));
  rtspServer->addServerMediaSession(sms);

  *env << "\n\"" << streamName << "\" stream, from a UDP Elementary Stream input source \n\t(";
  if (inputAddressStr != NULL) {
    *env << "IP multicast address " << inputAddressStr << ",";
  } else {
    *env << "unicast;";
  }
  *env << " port " << inputPortNum << ")\n";
  url = announceURL(rtspServer, sms);

  if (rtspServer->setUpTunnelingOverHTTP(80) || rtspServer->setUpTunnelingOverHTTP(8000) || rtspServer->setUpTunnelingOverHTTP(8080)) {
    *env << "\n(We use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling.)\n";
  } else {
    *env << "\n(RTSP-over-HTTP tunneling is not available.)\n";
  }

  env->taskScheduler().doEventLoop();
  return;
}
