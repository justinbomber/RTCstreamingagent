#include "commonstruct.h"

//Define global map
std::map<UserDevice, UserTask> taskmanager;

// Define ddscam tp_videostream topic and qos
dds::domain::DomainParticipant ddscam_participant(66);
dds::core::QosProvider ddscam_qos("./ddscamxml/ddscam_qos.xml");
const dds::core::xtypes::DynamicType &mytypeVideoStream = ddscam_qos.extensions().type("DdsCam::VideoStream");
dds::topic::Topic<dds::core::xtypes::DynamicData> topicVideoStream(ddscam_participant, "Tp_VideoStream", mytypeVideoStream);

// Define paas tp_playh264 topic and qos
dds::domain::DomainParticipant paas_participant(66);
dds::core::QosProvider paas_qos("./paasxml/pass_qos.xml");
const dds::core::xtypes::DynamicType &mytypePlayH264 = paas_qos.extensions().type("Paas::Cam::PlayH264");
dds::topic::Topic<dds::core::xtypes::DynamicData> topicPlayH264(paas_participant, "Tp_PlayH264", mytypePlayH264);

const dds::core::xtypes::DynamicType &mytype = paas_qos.extensions().type("Paas::Cam::Query");
dds::topic::Topic<dds::core::xtypes::DynamicData> topicQuery(paas_participant, "Tp_Query", mytype);