#ifndef SUB_THREAD_H
#define SUB_THREAD_H

#include <string>
#include <queue>
#include <ctime>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include <map>
#include <fstream>
#include <rti/rti.hpp>
#include <filesystem>
#include "postgresConnector.h"
#include "commonstruct.h"
#include "rtspServer.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
namespace net = boost::asio;
namespace websocket = boost::beast::websocket;
using tcp = boost::asio::ip::tcp;

// 全局變數聲明
extern std::mutex mtx;
extern std::condition_variable cv;
extern bool foundFile;

class sub_thread {
private:
    std::atomic_bool should_close{false};
    bool isShutdown_ = false;
public:
    sub_thread();
    ~sub_thread();

    std::string sub_thread_task(UserTask & usertask, 
                                portNumBits udpport,
                                std::string udpip,
                                std::string ipaddr,
                                std::string rootpath);
};

#endif // YOURCLASS_H
