#include "commonstruct.h"


// void initialize(){
//     AppConfig *appconfig = new AppConfig();
//     boost::property_tree::ptree jsonObject;
//     if(appconfig->getObjectValue("postgres", jsonObject)){
//         postgreshost = jsonObject.get<std::string>("host");
//         postgresport = jsonObject.get<int>("port");
//         postgresdb = jsonObject.get<std::string>("dbname");
//         postgresuser = jsonObject.get<std::string>("username");
//         postgrespassword = jsonObject.get<std::string>("password");
//     }
//     if (appconfig->getObjectValue("websocket", jsonObject)){
//         websocketip = jsonObject.get<std::string>("ip");
//         websocketport = jsonObject.get<int>("port");
//         websocketpath = jsonObject.get<std::string>("path");
//     }
//     if (appconfig->getObjectValue("ddscam", jsonObject)){
//         ddscam_qos = jsonObject.get<std::string>("qos");
//         ddscam_typedef = jsonObject.get<std::string>("typedef");
//     }
//     if (appconfig->getObjectValue("paas", jsonObject)){
//         paas_qos = jsonObject.get<std::string>("qos");
//         paas_typedef = jsonObject.get<std::string>("typedef");
//     }
//     if (appconfig->getObjectValue("local", jsonObject)){
//         local_serverip = jsonObject.get<std::string>("serverip");
//         local_rootpath = jsonObject.get<std::string>("hisrootpath");
//         local_domainid = jsonObject.get<int>("ddsdomainid");
//         local_udpip = jsonObject.get<std::string>("udpip");
//         local_udpport = jsonObject.get<int>("udpport");
//     }
    
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
// }



