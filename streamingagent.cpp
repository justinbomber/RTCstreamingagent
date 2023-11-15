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
std::mutex mtx;

CommonStruct commonstruct;
// 定義taskmanager
void resortmap(UserDevice userdevice, UserTask usertask, std::map<UserDevice, UserTask> &taskmanager)
{
  for (auto it = taskmanager.begin(); it != taskmanager.end(); ++it){
    if (it->first.partition_device == userdevice.partition_device){
      it->second.threadcontroll = false;
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
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

  // 定義websocket
  net::io_context ioc;
  PostgresConnector postgresinstance;
  sub_thread sub;
  tcp::resolver resolver(ioc);
  websocket::stream<tcp::socket> ws(ioc);
  auto const results = resolver.resolve(commonstruct.websocketip, commonstruct.websocketport);
  auto ep = boost::asio::connect(ws.next_layer(), results);
  ws.handshake(commonstruct.websocketip, "/" + commonstruct.websocketpath);
  std::cout << "Websocket Server Connection Success !" << std::endl;

  // 連線至websocket server
  std::string ipaddr = commonstruct.local_serverip;

  while (true)
  {
    commonstruct.local_udpport++;
    UserDevice userdevice;
    UserTask usertask;
    // 收到client request
    beast::flat_buffer buffer;
    ws.read(buffer);
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
      usertask.timestampnow = userdevice.timestampnow;
    } catch (std::exception& e) {
      pqxxController pqc1;
      boost::property_tree::ptree jsonObject;
      jsonObject.put("token", usertask.token);
      jsonObject.put("type", "error occured");
      jsonObject.put("message", e.what());
      std::string inifile_text = pqc1.ptreeToJsonString(jsonObject);
      ws.write(net::buffer(inifile_text));
    }

    resortmap(userdevice, usertask, taskmanager);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
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
      ws.write(net::buffer(outputurl));
    }
  }
  ws.close(websocket::close_code::normal);
  return 0;
}