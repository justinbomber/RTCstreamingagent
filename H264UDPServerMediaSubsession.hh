#ifndef _H264_TRANSPORT_UDP_SERVER_MEDIA_SUBSESSION_HH
#define _H264_TRANSPORT_UDP_SERVER_MEDIA_SUBSESSION_HH

#ifndef _ON_DEMAND_SERVER_MEDIA_SUBSESSION_HH
#include "OnDemandServerMediaSubsession.hh"
#endif

class H264UDPServerMediaSubsession: public OnDemandServerMediaSubsession {
public:
  static H264UDPServerMediaSubsession*
  createNew(UsageEnvironment& env,
	    char const* inputAddressStr, // An IP multicast address, or use "0.0.0.0" or NULL for unicast input
	    Port const& inputPort); // otherwise (default) the input stream is RTP/UDP
protected:
  H264UDPServerMediaSubsession(UsageEnvironment& env,
					 char const* inputAddressStr, Port const& inputPort);
      // called only by createNew();
  virtual ~H264UDPServerMediaSubsession();

protected: // redefined virtual functions
  virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate);
  virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
				    unsigned char rtpPayloadTypeIfDynamic,
				    FramedSource* inputSource);
protected:
  char const* fInputAddressStr;
  Port fInputPort;
  Groupsock* fInputGroupsock;
};

#endif