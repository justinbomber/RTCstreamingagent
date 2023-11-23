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
void resortmap(UserDevice userdevice, UserTask usertask, std::map<UserDevice, UserTask> &taskmanager)
{
  for (auto it = taskmanager.begin(); it != taskmanager.end(); ++it){
    if (it->first.partition_device == userdevice.partition_device){
      it->second.threadcontroll = false;
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
  exit(signum);
}

int main(int argc, char *argv[]){
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
    if (buffer.size() == 0){
      continue;
    }
    std::string received = beast::buffers_to_string(buffer.data());
    std::cout << "Received: " << received << std::endl;
    auto json_obj = nlohmann::json::parse(received);
    
    try {
      // 設定key
      userdevice.partition_device = json_obj["partition_device"].get<std::string>();
      userdevice.timestampnow = std::time(0);

      // 設定value
      usertask.token = json_obj["token"].get<std::string>();
      usertask.username = json_obj["username"].get<std::string>();
      usertask.partition_device = json_obj["partition_device"].get<std::string>();
      usertask.path = json_obj["path"].get<std::string>();
      usertask.threadcontroll = true;
    } catch (std::exception& e) {
      pqxxController pqc1;
      boost::property_tree::ptree jsonObject;
      jsonObject.put("token", usertask.token);
      jsonObject.put("type", "error occured");
      jsonObject.put("message", e.what());
      std::string inifile_text = pqc1.ptreeToJsonString(jsonObject);
      commonstruct.write(inifile_text);
    }

    resortmap(userdevice, usertask, std::ref(taskmanager));
    // std::this_thread::sleep_for(std::chrono::milliseconds(300));
    taskmanager[userdevice] = usertask;

    if (!usertask.path.empty())
    {
      sub_thread instance;
      std::string outputurl;
      outputurl = instance.sub_thread_task(std::ref(taskmanager[userdevice]), 
                  commonstruct.local_udpport, 
                  commonstruct.local_udpip, 
                  ipaddr, 
                  commonstruct.local_rootpath);
      commonstruct.write(outputurl);
    }
  }
  commonstruct.disconnect();
  return 0;
}
