#include <boost/asio.hpp>
#include <time.h>
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
#include "commonstruct.h"
#include "pqxxController.hpp"

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
// 定義taskmanager
void resortmap(UserDevice userdevice, UserTask usertask, std::map<UserDevice, UserTask> &taskmanager)
{
  DDSWriter ddswriter;
  std::cout << "in resort map" << std::endl;
  if (usertask.resolution == "1080"){
    for (auto it = taskmanager.begin(); it != taskmanager.end();){
      if (it->first.token == userdevice.token){
        // pthread_t thread_id = std::hash<std::thread::id>()(it->second.thread_id);
        // std::map<UserDevice, UserTask>::iterator iter = it;
        // pthread_cancel(thread_id);
        it->second.threadcontroll = false;
        
        if (!it->second.query_type || it->second.ai_type.size() > 0){
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
        it++;
      }
      else
        ++it;
    }
  } else if (usertask.resolution == "480"){
    for (auto it = taskmanager.begin(); it != taskmanager.end();){
      if ((it->first.partition_device == userdevice.partition_device) && (it->first.token == userdevice.token)) {
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
        it++;
      }
      else
        ++it;
    }
  }
}

void signalHandler(int signum)
{
  globalthread = false;
  for (std::map<UserDevice, UserTask>::iterator it = taskmanager.begin(); it != taskmanager.end(); ++it)
  {
    it->second.threadcontroll = false;
  }
  std::cout << "Capture \'Ctrl+C\' signal(" << signum << "), exiting..." << std::endl;
  sleep(5);
  exit(signum);
}

int main(int argc, char *argv[]){
  // 定義退出訊號
  signal(SIGINT, signalHandler);

  // 定義websocket
  net::io_context ioc;
  PostgresConnector postgresinstance;
  sub_thread sub;
  tcp::resolver resolver(ioc);
  websocket::stream<tcp::socket> ws(ioc);
  std::cout << "Websocket Server Connection Success !" << std::endl;

  // 連線至websocket server
  auto const results = resolver.resolve("10.1.1.104", "8011");
  auto ep = boost::asio::connect(ws.next_layer(), results);
  ws.handshake("10.1.1.104", "/ddsagent");
  portNumBits udpport = 1250;
  std::string ipaddr = "10.1.1.128";

  while (true)
  {
    udpport++;
    UserDevice userdevice;
    UserTask usertask;
    // 收到client request
    beast::flat_buffer buffer;
    ws.read(buffer);
    std::string received = beast::buffers_to_string(buffer.data());
    std::cout << "Received: " << received << std::endl;
    auto json_obj = nlohmann::json::parse(received);

    // 設定key
    userdevice.token = json_obj["token"].get<std::string>();
    userdevice.partition_device = json_obj["partition_device"].get<std::string>();

    // 設定value
    usertask.token = json_obj["token"].get<std::string>();
    usertask.username = json_obj["username"].get<std::string>();
    usertask.ai_type = json_obj["ai_type"].get<std::vector<std::string>>();
    usertask.partition_device = json_obj["partition_device"].get<std::string>();
    usertask.query_type = json_obj["query_type"].get<std::int8_t>();
    usertask.starttime = json_obj["starttime"].get<std::int64_t>();
    usertask.endtime = json_obj["endtime"].get<std::int64_t>();
    usertask.path = json_obj["path"].get<std::string>();
    usertask.resolution = json_obj["resolution"].get<std::string>();
    usertask.activate = json_obj["activate"].get<bool>();
    usertask.threadcontroll = true;

    if (usertask.partition_device == "Cam003")
      usertask.partition_device = "CAM003";

    if (userdevice.token == "stopthread") {
      for(auto it = taskmanager.begin(); it != taskmanager.end(); ++it)
        {
          pthread_t thread_id = std::hash<std::thread::id>()(it->second.thread_id);
          pthread_cancel(thread_id);
          sleep(1);
        }
      continue;
    }

    resortmap(userdevice, usertask, std::ref(taskmanager));
    sleep(1);
    taskmanager[userdevice] = usertask;



    if (!usertask.path.empty())
    {
      sub_thread instance;
      std::string outputurl;
      outputurl = instance.sub_thread_task(std::ref(taskmanager[userdevice]), udpport, ipaddr);
      if (usertask.ai_type.size() > 0 && usertask.query_type == 0){
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
        ws.write(net::buffer(inifile_text));
        sleep(1);
      }
      if (usertask.ai_type.size() > 0 && usertask.query_type)
        continue;
      else
        ws.write(net::buffer(outputurl));
    }
  }
  ws.close(websocket::close_code::normal);
  return 0;
}