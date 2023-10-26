#include <boost/asio.hpp>
#include <time.h>
#include <boost/beast.hpp>
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
  std::cout << "in resort map" << std::endl;
  if (usertask.resolution == "1080")
  {
    // 刪除所有包含該 token 的 key-value pair
    for (auto it = taskmanager.begin(); it != taskmanager.end();)
    {
      if (it->first.token == userdevice.token)
        it->second.threadcontroll = false;
      else
        ++it;
    }
  }
  bool tokenExists = false;
  for (const auto& pair : taskmanager) 
  {
    if (pair.first.token == userdevice.token)
    {
      tokenExists = true;
      break;
    }
  }
  // 若 resolution 為 480 或者該 token 尚未出現在 map 中，則直接添加。
  if (usertask.resolution == "480" || !tokenExists)
  {
    // TODO: should lock
    taskmanager[userdevice] = usertask;
  }
}

void print_help()
{
  std::cout << "使用方法:\n";
  std::cout << "  -i, --ip <IP地址>     設定 IP 地址\n";
  std::cout << "  -p, --port <端口>     設定端口\n";
  std::cout << "  -h, --help            顯示幫助訊息\n";
  exit(0);
}

void signalHandler(int signum)
{
  globalthread = false;
  for (std::map<UserDevice, UserTask>::iterator it = taskmanager.begin(); it != taskmanager.end(); ++it)
  {
    it->second.threadcontroll = false;
  }
  std::cout << "捕獲到Ctrl+C信號（" << signum << "），清理資源..." << std::endl;
  exit(signum);
}

int main(int argc, char *argv[])
{
  // 定義退出訊號
  signal(SIGINT, signalHandler);

  // 定義websocket
  net::io_context ioc;
  PostgresConnector postgresinstance;
  sub_thread sub;
  tcp::resolver resolver(ioc);
  websocket::stream<tcp::socket> ws(ioc);
  std::cout << "連接成功" << std::endl;

  std::string ip_address;
  std::string port;

  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];
    if ((arg == "-i") || (arg == "--ip"))
    {
      if (i + 1 < argc)
        ip_address = argv[++i];
      else
      {
        std::cerr << "--ip 選項需要一個值\n";
        return 1;
      }
    }
    else if ((arg == "-p") || (arg == "--port"))
    {
      if (i + 1 < argc)
        port = argv[++i];
      else
      {
        std::cerr << "--port 選項需要一個值\n";
        return 1;
      }
    }
    else if ((arg == "-h") || (arg == "--help"))
    {
      print_help();
      return 0;
    }
    else
    {
      std::cerr << "未知選項: " << arg << "\n";
      print_help();
      return 1;
    }
  }

  // 連線至websocket server
  auto const results = resolver.resolve("10.1.1.104", "8011");
  auto ep = boost::asio::connect(ws.next_layer(), results);
  ws.handshake("10.1.1.104", "/ddsagent");

  while (true)
  {
    UserDevice userdevice;
    UserTask usertask;
    // 收到client request
    beast::flat_buffer buffer;
    std::cout << " ++++++++++listening on " << ip_address << ":" << port << std::endl;
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

    // for (auto it = taskmanager.begin(); it != taskmanager.end();)
    // {
    //   if (it->first.partition_device == userdevice.partition_device)
    //     it->second.threadcontroll = false;
    //   else
    //     ++it;
    // }
    taskmanager[userdevice] = usertask;
    // resortmap(userdevice, task, std::ref(taskmanager));

    if (!usertask.path.empty())
    {
      sub_thread instance;
      std::string outputurl;
      outputurl = instance.sub_thread_task(std::ref(taskmanager[userdevice]));
      ws.write(net::buffer(outputurl));
      std::cout << "start thread finished !!!!!!!!!!!!" << std::endl;
    }
    sleep(20);
    taskmanager[userdevice].threadcontroll = false;
  }
  ws.close(websocket::close_code::normal);
  // while (true)
  // {
  //   sleep(1);
  // }
  // -------------------------
  return 0;
}