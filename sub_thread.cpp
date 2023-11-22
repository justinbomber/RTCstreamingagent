#include "sub_thread.h"
#include <iostream>
#include "ddsreader.h"
#include "ddswriter.h"
#include "H264UDPServerMediaSubsession.hh"
#include "commonstruct.h"

using namespace std;
std::mutex mtx;
std::condition_variable cv;
bool hasfile = false;

sub_thread::sub_thread()
{
}

sub_thread::~sub_thread()
{
}

int find_available_port(int start_port, int socket_type, const char* ip_address = "0.0.0.0") {
    int sock = socket(AF_INET, socket_type, 0);
    if (sock == -1) {
        std::cerr << "無法創建 socket\n";
        return -1;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_address);

    for (int port = start_port; port < 65535; ++port) {
        addr.sin_port = htons(port);
        if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            close(sock);
            return port;
        }
    }

    close(sock);
    return -1;
}

std::string sub_thread::sub_thread_task(UserTask & usertask, 
                                        portNumBits udpport,
                                        std::string udpip,
                                        std::string ipaddr,
                                        std::string rootpath)
{

    std::string path = usertask.path;
    std::string token = usertask.token;
    std::string partition_device = usertask.partition_device;
    std::string username = usertask.username;

    std::time_t timestampnow = std::time(0);

    DDSReader ddsreader;
    DDSWriter ddswriter;
    RTSPServerManager rtspservermanager;
    nlohmann::json json_obj;
    // 影片存放根目錄
    std::string catchinput = rootpath + "/catchinput/" + partition_device + "/" + username + "/" + std::to_string(timestampnow) + "/";
    std::string catchoutput = rootpath + "/catchoutput/" + partition_device + "/" + username + "/" + std::to_string(timestampnow) + "/";

    
    // gen port num start from 8554
    int serverport = 8554;
    // portNumBits udpport = 1250;
    portNumBits httptunnelingport = 8000;
    serverport = find_available_port(serverport, SOCK_STREAM);
    httptunnelingport = find_available_port(httptunnelingport, SOCK_STREAM);
    auto videostream_func = std::bind(&DDSReader::videostream_reader, &ddsreader,
                                      std::ref(usertask),
                                      catchinput,
                                      udpport);
    auto rtspserverfunc = std::bind(&RTSPServerManager::startserver, &rtspservermanager, 
                                    serverport, udpport, udpip,
                                    usertask.partition_device + "/" + usertask.token,
                                    httptunnelingport);

    // Read VideoStream topic;
    std::thread readerthread(videostream_func);
    readerthread.detach();

    std::thread rtspthread(rtspserverfunc);
    rtspthread.detach();

    json_obj["url"] = "rtsp://" + ipaddr + ":" + std::to_string(serverport) + "/" + usertask.partition_device + "/" + usertask.username;
    int time_duration = 0;
    if (usertask.resolution == "1080"){
        time_duration = 500;
    } else {
        time_duration = 1500;
    }
    auto start = std::chrono::steady_clock::now();
    while (!usertask.videocontroll)
    {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() >= time_duration){
            json_obj["url"] = "None";
            break;
        }
        // start = std::chrono::steady_clock::now();
    }
    json_obj["token"] = token;
    json_obj["path"] = path;
    json_obj["type"]= "video";
    std::cout << json_obj << std::endl;
    hasfile = false;

    // 序列化 JSON 對象為字符串
    std::string json_str = json_obj.dump();
    // sleep(1);
    // return json_str;
    return json_str;
}
