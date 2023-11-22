#include "commonstruct.h"
#include "appConfig.h"




std::map<UserDevice, UserTask> taskmanager;
std::int32_t domain_id = 66;
std::string ddscamqos = "ddscamxml/ddscam_qos.xml";
std::string paasqos = "paasxml/paas_qos.xml";
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;


CommonStruct::CommonStruct():resolver(ioc), ws(ioc)
{
    AppConfig cfg;
    if (cfg.openJsonFile("../app.json"))
    {
        // std::cout << "Opened app.json." << std::endl;
        boost::property_tree::ptree jsonObject;
        if (cfg.getObjectValue("websocket", jsonObject))
        {
            websocketip = jsonObject.get<std::string>("ip");
            websocketport = jsonObject.get<std::string>("port");
            websocketpath = jsonObject.get<std::string>("path");
        }
        if (cfg.getObjectValue("local", jsonObject))
        {
            local_serverip = jsonObject.get<std::string>("serverip");
            local_rootpath = jsonObject.get<std::string>("hisrootpath");
            local_domainid = jsonObject.get<std::int32_t>("ddsdomainid");
            local_udpip = jsonObject.get<std::string>("udpip");
            local_udpport = jsonObject.get<portNumBits>("udpport");
        }
        if (cfg.getObjectValue("ddscam", jsonObject))
        {
            ddscam_qos_path = jsonObject.get<std::string>("qos");
            ddscam_typedef_path = jsonObject.get<std::string>("typedef");
        }
        if (cfg.getObjectValue("paas", jsonObject))
        {
            paas_qos_path = jsonObject.get<std::string>("qos");
            paas_typedef_path = jsonObject.get<std::string>("typedef");
        }
    }
    domain_id = local_domainid;
    ddscamqos = ddscam_qos_path;
    paasqos = paas_qos_path;
}

void CommonStruct::disconnect() {
    if (!isConnected_) {
        return;
    }

    try {
        ws.close(websocket::close_code::normal);
        isConnected_ = false;
        std::cout << "WebSocket connection closed." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error closing WebSocket: " << e.what() << std::endl;
    }
}

void CommonStruct::reconnect() {
    sleep(1);
    try
    {
        if (ws.is_open()) {
            ws.close(websocket::close_code::normal);
        }
        connect();
        std::cout << "WebSocket reconnected." << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error reconnecting WebSocket: " << e.what() << std::endl;
    }
    
}

void CommonStruct::connect() {
    if (isConnected_) {
        return;
    }

    try {
        auto const results = resolver.resolve(websocketip, websocketport);
        auto ep = net::connect(ws.next_layer(), results);
        ws.handshake(websocketip, "/" + websocketpath);

        std::cout << "Websocket Server Connection Success !" << std::endl;
        isConnected_ = true;
    } catch (const std::exception& e) {
        std::cerr << "Connection Failed: " << e.what() << std::endl;
    }
}

void CommonStruct::write(const std::string& message) {
    try {
        ws.write(net::buffer(message));
    } catch (const beast::system_error& e) {
        std::cerr << "Write error: " << e.what() << std::endl;
        reconnect();
    }
}

beast::flat_buffer CommonStruct::read() {
    beast::flat_buffer buffer;
    try {
        ws.read(buffer);
    } catch (const beast::system_error& e) {
        std::cerr << "Read error: " << e.what() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        buffer.clear();
        reconnect();
    }
    return buffer;
}

// Define ddscam tp_videostream topic and qos
dds::domain::DomainParticipant ddscam_participant(domain_id);
dds::core::QosProvider ddscam_qos(ddscamqos);

const dds::core::xtypes::DynamicType &mytypeVideoStream = ddscam_qos.extensions().type("DdsCam::VideoStream");
dds::topic::Topic<dds::core::xtypes::DynamicData> topicVideoStream(ddscam_participant, "Tp_VideoStream", mytypeVideoStream);

// Define paas tp_playh264 topic and qos
dds::domain::DomainParticipant paas_participant(domain_id);
dds::core::QosProvider paas_qos(paasqos);
const dds::core::xtypes::DynamicType &mytypeH2642Ai = paas_qos.extensions().type("Paas::Cam::H264Type");
dds::topic::Topic<dds::core::xtypes::DynamicData> topicH2642Ai(paas_participant, "Tp_H2642Ai", mytypeH2642Ai);

const dds::core::xtypes::DynamicType &mytypePlayH264 = paas_qos.extensions().type("Paas::Cam::PlayH264");
dds::topic::Topic<dds::core::xtypes::DynamicData> topicPlayH264(paas_participant, "Tp_PlayH264", mytypePlayH264);

const dds::core::xtypes::DynamicType &mytype = paas_qos.extensions().type("Paas::Cam::Query");
dds::topic::Topic<dds::core::xtypes::DynamicData> topicQuery(paas_participant, "Tp_Query", mytype);




