#include <time.h>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <iostream>
#include <thread>
#include "sub_thread.h"
#include "postgresConnector.h"
#include "ddsreader.h"
#include "ddswriter.h"
#include <nlohmann/json.hpp>
#include <csignal>
#include <unistd.h>
#include <set>
#include "commonstruct.h"
#include "pqxxController.hpp"
#include <mutex>
#include "appConfig.h"

// using namespace boost::asio;
// using namespace boost::beast;
using json = nlohmann::json;

using namespace std;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

bool globalthread = true;

CommonStruct commonstruct;


// 定義taskmanager
portNumBits resortmap(UserDevice userdevice, UserTask usertask, std::map<UserDevice, UserTask> &taskmanager)
{
    std::set<portNumBits> udpportset;
    portNumBits cacheudpport = commonstruct.local_udpport;
    DDSWriter ddswriter;
    std::cout << "in resort map" << std::endl;
    if (usertask.resolution == "1080")
    {
        for (auto it = taskmanager.begin(); it != taskmanager.end(); ++it)
        {
            if (it->first.token == userdevice.token)
            {
                it->second.threadcontroll = false;
                it->second.rtspcontroll = 1;

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }
    else if (usertask.resolution == "480")
    {
        for (auto it = taskmanager.begin(); it != taskmanager.end(); ++it)
        {
            if ((it->first.partition_device == userdevice.partition_device) && (it->first.token == userdevice.token))
            {
                it->second.threadcontroll = false;
                it->second.rtspcontroll = 1;
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
    }

    for (std::map<UserDevice, UserTask>::iterator it = taskmanager.begin(); it != taskmanager.end(); ++it)
    {
        if (it->second.token == userdevice.token && (it->second.ai_type.size() > 0 || !it->second.query_type))
        {
            ddswriter.query_writer(it->second.username,
                                   it->second.ai_type,
                                   it->second.partition_device,
                                   it->second.query_type,
                                   it->second.starttime,
                                   it->second.endtime,
                                   it->second.token,
                                   it->second.path,
                                   0);
        }
        if (it->second.threadcontroll == true)
        {
            udpportset.insert(it->second.udpport);
        }
    }
    // for (std::set<int>::iterator it = udpportset.begin(); it != udpportset.end(); ++it)
    while (udpportset.count(cacheudpport))
        cacheudpport++;
    return cacheudpport;
}

void signalHandler(int signum)
{
    globalthread = false;
    for (std::map<UserDevice, UserTask>::iterator it = taskmanager.begin(); it != taskmanager.end(); ++it)
    {
        it->second.threadcontroll = false;
        it->second.rtspcontroll = 1;
    }
    std::cout << "Capture \'Ctrl+C\' signal(" << signum << "), exiting..." << std::endl;
    exit(signum);
}

std::string removeSlash(const std::string &input)
{
    std::string result = input;
    result.erase(std::remove(result.begin(), result.end(), '/'), result.end());
    return result;
}

int main(int argc, char *argv[])
{
    // 定義退出訊號
    signal(SIGINT, signalHandler);
    commonstruct.connect();

    PostgresConnector postgresinstance;
    sub_thread sub;

    // 連線至websocket server

    while (true)
    {
        commonstruct.local_udpport++;
        UserDevice userdevice;
        UserTask usertask;

        std::string ipaddr = commonstruct.local_serverip;
        // 收到client request
        beast::flat_buffer buffer;
        buffer = commonstruct.read();
        if (buffer.size() == 0)
        {
            continue;
        }
        // cout timestamp in ms
        auto nowws = std::chrono::high_resolution_clock::now();
        auto receivedwsepoch = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   nowws.time_since_epoch())
                                   .count();
        std::cout << "=======================" << std::endl;
        std::cout << "Recieved websocket request --->>>" << receivedwsepoch << std::endl;
        std::cout << "+++++++++++++++++++++++" << std::endl;

        std::string received = beast::buffers_to_string(buffer.data());
        std::cout << "Received: " << received << std::endl;
        auto json_obj = nlohmann::json::parse(received);

        if (json_obj.contains("type"))
        {
            DDSWriter ddswriter;
            if (json_obj["type"] == "disconnect")
            {
                usertask.token = json_obj["token"].get<std::string>();
                for (auto it = taskmanager.begin(); it != taskmanager.end(); ++it)
                {
                    if (it->first.token == usertask.token)
                    {
                        ddswriter.query_writer(it->second.username,
                                               it->second.ai_type,
                                               it->second.partition_device,
                                               it->second.query_type,
                                               it->second.starttime,
                                               it->second.endtime,
                                               it->second.token,
                                               it->second.path,
                                               0);
                        it->second.threadcontroll = false;
                        it->second.rtspcontroll = 1;
                    }
                }
                continue;
            }
        }

        try
        {
            // 設定key
            userdevice.token = json_obj["token"].get<std::string>();
            userdevice.partition_device = json_obj["partition_device"].get<std::string>();
            userdevice.timestampnow = std::time(0);

            // 設定value
            usertask.token = json_obj["token"].get<std::string>();
            usertask.username = json_obj["username"].get<std::string>();
            usertask.ai_type = json_obj["ai_type"].get<std::vector<std::string>>();
            usertask.partition_device = json_obj["partition_device"].get<std::string>();
            usertask.query_type = json_obj["query_type"].get<std::int8_t>();
            usertask.starttime = json_obj["starttime"].get<std::int64_t>();
            usertask.endtime = json_obj["endtime"].get<std::int64_t>();
            usertask.path = removeSlash(json_obj["path"].get<std::string>());
            usertask.resolution = json_obj["resolution"].get<std::string>();
            usertask.activate = json_obj["activate"].get<bool>();
            usertask.threadcontroll = true;
            if (json_obj.contains("rtsp_url"))
                usertask.rtsp_url = json_obj["rtsp_url"].get<std::string>();
            else
                usertask.rtsp_url = "None";
        }
        catch (std::exception &e)
        {
            pqxxController pqc1;
            boost::property_tree::ptree jsonObject;
            jsonObject.put("token", usertask.token);
            jsonObject.put("type", "error occured");
            jsonObject.put("message", e.what());
            std::string inifile_text = pqc1.ptreeToJsonString(jsonObject);
            commonstruct.write(inifile_text);
        }

        portNumBits udpport;

        udpport = resortmap(userdevice, usertask, std::ref(taskmanager));
        usertask.udpport = udpport;
        taskmanager[userdevice] = usertask;

        sub_thread instance;
        std::string outputurl;
        outputurl = instance.sub_thread_task(std::ref(taskmanager[userdevice]),
                                             commonstruct.local_udpip,
                                             ipaddr,
                                             commonstruct.local_rootpath);

        if (usertask.rtsp_url != "None")
            commonstruct.write(outputurl);
        else if (usertask.ai_type.size() > 0 && usertask.query_type == 0)
        {
            boost::property_tree::ptree jsonObject;
            pqxxController pqc1;
            std::string *ai_type_array = &usertask.ai_type[0];
            jsonObject = pqc1.get_multitag_ai_type_intime(usertask.partition_device,
                                                          usertask.starttime,
                                                          usertask.endtime, ai_type_array,
                                                          usertask.ai_type.size());
            jsonObject.put("token", usertask.token);
            jsonObject.put("type", "ai_time");

            std::string inifile_text = pqc1.ptreeToJsonString(jsonObject);
            commonstruct.write(inifile_text);
        }
        if (usertask.ai_type.size() > 0)
            continue;
        else
        {
            commonstruct.write(outputurl);
            auto nowwrite = std::chrono::high_resolution_clock::now();
            auto writewsepoch = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    nowwrite.time_since_epoch())
                                    .count();
            std::cout << "=======================" << std::endl;
            std::cout << "response websocket request --->>>" << writewsepoch << std::endl;
            std::cout << "+++++++++++++++++++++++" << std::endl;
        }

    }
    commonstruct.disconnect();
    return 0;
}
