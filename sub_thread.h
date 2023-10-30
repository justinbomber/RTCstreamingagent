#ifndef SUB_THREAD_H
#define SUB_THREAD_H

#include <string>
#include <queue>
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

class sub_thread {
    std::atomic_bool should_close{false};
private:
    bool isShutdown_ = false;
public:
    sub_thread();
    ~sub_thread();

    std::string sub_thread_task(UserTask & usertask, 
                                portNumBits udpport,
                                std::string ipaddr);
    pqxx::result searchdatabase(const std::string & tablename,
                                const std::string & source,
                                const std::int64_t & starttime,
                                const std::int64_t & endtime);
};

#endif // YOURCLASS_H
