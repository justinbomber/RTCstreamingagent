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
dds::domain::DomainParticipant ddscam_participant(66);
dds::domain::DomainParticipant paas_participant(66);
std::map<UserDevice, UserTask> taskmanager;
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
  for (const auto &pair : taskmanager)
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
  auto const results = resolver.resolve("10.1.1.104", "8010");
  auto ep = boost::asio::connect(ws.next_layer(), results);
  ws.handshake("10.1.1.104", "/ddsagent");

  while (true)
  {
    UserDevice userdevice;
    UserTask task;
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
    // std::cout<<"atleasttohere"<<std::endl;
    task.token = json_obj["token"].get<std::string>();
    // std::cout<<"atleasttohere2"<<std::endl;
    task.username = json_obj["username"].get<std::string>();
    task.ai_type = json_obj["ai_type"].get<std::vector<std::string>>();
    task.partition_device = json_obj["partition_device"].get<std::string>();
    task.query_type = json_obj["query_type"].get<std::int8_t>();
    task.starttime = json_obj["starttime"].get<std::int64_t>();
    task.endtime = json_obj["endtime"].get<std::int64_t>();
    task.path = json_obj["path"].get<std::string>();
    task.resolution = json_obj["resolution"].get<std::string>();
    task.activate = json_obj["activate"].get<bool>();
    task.threadcontroll = true;

    // taskmanager[userdevice] = task;
    resortmap(userdevice, task, std::ref(taskmanager));

    if (!task.path.empty())
    {
      sub_thread instance;
      std::string outputurl;
      outputurl = instance.sub_thread_task(std::ref(taskmanager),
                                           userdevice,
                                           std::ref(ddscam_participant),
                                           std::ref(paas_participant));
      boost::property_tree::ptree jsonObject;
      pqxxController pqc1;
      std::string *ai_type_array = &task.ai_type[0];
      //std::cout << "ai_type_array: " << ai_type_array[0] << std::endl;
      //std::cout << "ai_type_array: " << ai_type_array[1] << std::endl;
      jsonObject = pqc1.get_multitag_ai_type_intime(task.partition_device, task.starttime, task.endtime, ai_type_array, task.ai_type.size());
      jsonObject.put("token", task.token);
      jsonObject.put("type", "ai_time");

      std::string inifile_text = pqc1.ptreeToJsonString(jsonObject);
      ws.write(net::buffer(inifile_text));
      ws.write(net::buffer(outputurl));
    }
  }
  ws.close(websocket::close_code::normal);
  // while (true)
  // {
  //   sleep(1);
  // }
  // -------------------------
  return 0;
}