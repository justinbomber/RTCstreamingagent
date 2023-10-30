
#include "H264UDPServerMediaSubsession.hh"
#include "H264VideoRTPSink.hh"
#include "BasicUDPSource.hh"
#include "SimpleRTPSource.hh"
#include "SimpleRTPSink.hh"
#include "GroupsockHelper.hh"
#include "H264VideoStreamDiscreteFramer.hh"
#include "H264VideoStreamFramer.hh"


H264UDPServerMediaSubsession*
H264UDPServerMediaSubsession::createNew(UsageEnvironment& env,
						  char const* inputAddressStr, Port const& inputPort) {
  return new H264UDPServerMediaSubsession(env, inputAddressStr, inputPort);
}

H264UDPServerMediaSubsession
::H264UDPServerMediaSubsession(UsageEnvironment& env,
                                         char const* inputAddressStr, Port const& inputPort)
  : OnDemandServerMediaSubsession(env, True/*reuseFirstSource*/),
    fInputPort(inputPort), fInputGroupsock(NULL) {
  fInputAddressStr = strDup(inputAddressStr);
}

H264UDPServerMediaSubsession::
~H264UDPServerMediaSubsession() {
  delete fInputGroupsock;
  delete[] (char*)fInputAddressStr;
}

FramedSource* H264UDPServerMediaSubsession
::createNewStreamSource(unsigned/* clientSessionId*/, unsigned& estBitrate) {
  estBitrate = 5000; // kbps, estimate
  // estBitrate = 10000; // kbps, estimate

  if (fInputGroupsock == NULL) {
    // Create a 'groupsock' object for receiving the input stream:
    struct sockaddr_storage inputAddress;
    if (fInputAddressStr == NULL) {
      inputAddress = nullAddress();
    } else {
      NetAddressList inputAddresses(fInputAddressStr);
      if (inputAddresses.numAddresses() == 0) return NULL;
      copyAddress(inputAddress, inputAddresses.firstAddress());
    }
    fInputGroupsock = new Groupsock(envir(), inputAddress, fInputPort, 255);
  }

  // return H264VideoStreamFramer::createNew(envir(), BasicUDPSource::createNew(envir(), fInputGroupsock), False);
  return H264VideoStreamFramer::createNew(envir(), BasicUDPSource::createNew(envir(), fInputGroupsock), False);
}

RTPSink* H264UDPServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* /*inputSource*/) {
  return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
