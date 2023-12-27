#include "sub_thread.h"
#include <iostream>
#include "ddsreader.h"
#include "ddswriter.h"
#include "H264UDPServerMediaSubsession.hh"
#include "commonstruct.h"

using namespace std;

sub_thread::sub_thread()
{
}

sub_thread::~sub_thread()
{
}

void createM3U8File(const std::string &directory, const std::string &m3u8name)
{
    std::string filename = directory + m3u8name + ".m3u8";

    // 打開一個新文件用於寫入
    std::ofstream outFile(filename);

    if (!outFile.is_open())
    {
        std::cerr << "無法創建或打開文件" << std::endl;
        return;
    }

    // 寫入指定的字符串
    outFile << "#EXTM3U\n";
    outFile << "#EXT-X-VERSION:3\n";
    // outFile << "#EXT-X-START:TIME-OFFSET=-1,PRECISE=YES\n";
    outFile << "#EXT-X-TARGETDURATION:5\n";
    outFile << "#EXT-X-MEDIA-SEQUENCE:0\n";

    // 關閉文件
    outFile.close();
}

bool checkmetadata(std::string source, int startTime, int endTime)
{
    PostgresConnector postgresConnector;
    bool isOpen = postgresConnector.open("paasdb", "dds_paas", "postgres", "10.1.1.200", 5433);

    if (isOpen)
        std::cout << "Opened database successfully." << std::endl;
    else
        std::cout << "Database cannot be opened." << std::endl;

    std::string command =
        "select content_id, file_name, has_pre_ai from "
        "tb_cam_ipfs_controller_meta"
        " where source = '" +
        source + "' and unix_time_start >= " + std::to_string(startTime) +
        " and unix_time_end <= " + std::to_string(endTime) +
        " and status = '5'" +
        " order by source, unix_time_start, unix_time_end";
    pqxx::result ipfsRows = postgresConnector.executeResultset(command);
    int count = 0;
    bool closedb = postgresConnector.close();
    for (auto const &row : ipfsRows)
    {
        count++;
    }

    if (count > 0)
        return true;
    else
        return false;
}

int find_available_port(int start_port, int socket_type, const char *ip_address = "0.0.0.0")
{
    int sock = socket(AF_INET, socket_type, 0);
    if (sock == -1)
    {
        std::cerr << "無法創建 socket\n";
        return -1;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_address);

    for (int port = start_port; port < 65535; ++port)
    {
        addr.sin_port = htons(port);
        if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0)
        {
            close(sock);
            return port;
        }
    }

    close(sock);
    return -1;
}

void executeCommand(const std::string &cmd, bool &flag) {
    pid_t pid = fork();  // 創建子進程
    if (pid == 0) {
        // 子進程執行 cmd
        execl("/bin/sh", "sh", "-c", cmd.c_str(), (char *)NULL);
        exit(127); // 只有當 execl 出錯時才會執行這裡
    } else if (pid > 0) {
        // 父進程
        while (flag) {
            int status;
            waitpid(pid, &status, WNOHANG); // 非阻塞檢查子進程狀態
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                // 如果子進程已經結束，則退出循環
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 短暫休眠以減少 CPU 使用
        }
        if (!flag) {
            // 如果 flag 被設置為 false，則殺死子進程
            kill(pid, SIGKILL);
        }
    } else {
        // fork 失敗
        std::cerr << "Failed to fork process to execute command." << std::endl;
    }
}

void transferRTSPtom3u8(UserTask &usertask, const std::string &outputpath, int timewindow) {
    std::string cmdline = "ffmpeg -i " + usertask.rtsp_url + " -c copy -f hls" +
                          " -hls_time" + std::to_string(timewindow) + " -hls_list_size 0 -hls_flags discont_start+program_date_time" +
                          " -hls_flags append_list -hls_segment_filename \"" + outputpath + "sample_%%d.ts'\"" +
                          " '" + outputpath + usertask.path + ".m3u8'";
    if (usertask.threadcontroll) {
        std::thread ffmpegThread(executeCommand, std::ref(cmdline), std::ref(usertask.threadcontroll));
        ffmpegThread.join(); // 等待線程結束
    } else {
        return;
    }
}

void create_userfolder(std::string path, std::string partition_device, std::string username, std::string rootpath, std::time_t timestampnow)
{
    std::vector<std::filesystem::path> filevec;
    std::string catchoutput = rootpath + "/catchoutput/" + partition_device + "/" + username + "/" + std::to_string(timestampnow) + "/";
    std::string catchinput = rootpath + "/catchinput/" + partition_device + "/" + username + "/" + std::to_string(timestampnow) + "/";
    std::filesystem::path rootPath = rootpath;
    filevec.push_back(rootPath);
    std::filesystem::path inputPath = rootPath / "catchinput";
    filevec.push_back(inputPath);
    std::filesystem::path inputuserPath = rootpath + "/catchinput/" + partition_device;
    filevec.push_back(inputuserPath);
    std::filesystem::path inputdevicePath = inputuserPath / username;
    filevec.push_back(inputdevicePath);
    std::filesystem::path outputPath = rootPath / "catchoutput";
    filevec.push_back(outputPath);
    std::filesystem::path userPath = rootpath + "/catchoutput/" + partition_device;
    filevec.push_back(userPath);
    std::filesystem::path outputdevicePath = userPath / username;
    filevec.push_back(outputdevicePath);

    // create non existing directory
    for (auto &path : filevec)
    {
        if (!std::filesystem::exists(path))
        {
            std::filesystem::create_directories(path);
            std::cout << "Created directory: " << path.string() << std::endl;
        }
    }
    std::filesystem::path catchinputpath = inputdevicePath / std::to_string(timestampnow);
    std::filesystem::path catchoutputpath = outputdevicePath / std::to_string(timestampnow);
    std::filesystem::create_directories(catchinputpath);
    std::filesystem::create_directories(catchoutputpath);
    createM3U8File(catchoutput, path);
}

std::string sub_thread::sub_thread_task(UserTask &usertask,
                                        portNumBits udpport,
                                        std::string udpip,
                                        std::string ipaddr,
                                        std::string rootpath)
{

    std::string path = usertask.path;
    std::string token = usertask.token;
    std::string partition_device = usertask.partition_device;
    bool query_type = usertask.query_type;
    std::int64_t starttime = usertask.starttime;
    std::int64_t endtime = usertask.endtime;
    std::string resolution = usertask.resolution;
    std::vector<std::string> ai_type = usertask.ai_type;
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

    auto playh264_func = std::bind(&DDSReader::playh264_reader, &ddsreader,
                                   std::ref(usertask),
                                   catchoutput,
                                   catchinput,
                                   udpport);
    auto h2642ai_func = std::bind(&DDSReader::h2642ai_reader, &ddsreader,
                                  std::ref(usertask),
                                  catchoutput,
                                  catchinput,
                                  udpport);
    // auto transfunc = std::bind(&checkfile, catchoutput, std::ref(usertask));
    auto rtpsserverfunc = std::bind(&RTSPServerManager::startserver, &rtspservermanager,
                                    serverport, udpport, udpip,
                                    usertask.partition_device + "/" + usertask.username,
                                    httptunnelingport);
    auto rtsptom3u8func = std::bind(&transferRTSPtom3u8, std::ref(usertask), catchoutput, 5);
    if (ai_type.size() == 0 && query_type)
    {
        // start rtps server
        std::thread rtpsserverthread(rtpsserverfunc);
        rtpsserverthread.detach();
        std::thread readerthread(videostream_func);
        readerthread.detach();
        json_obj["url"] = "rtsp://" + ipaddr + ":" + std::to_string(serverport) + "/" + usertask.partition_device + "/" + usertask.username;
        int time_duration = 0;
        if (usertask.resolution == "1080")
        {
            time_duration = 500;
        }
        else
        {
            time_duration = 1500;
        }
        auto start = std::chrono::steady_clock::now();
        while (!usertask.videocontroll)
        {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() >= time_duration)
            {
                json_obj["url"] = "None";
                break;
            }
            // start = std::chrono::steady_clock::now();
        }
    }
    else
    {
        // Write Tp_Query

        if (query_type) // Sam, AI dds Agent
        {
            json_obj["url"] = "rtsp://" + ipaddr + ":" + std::to_string(serverport) + "/" + usertask.partition_device + "/" + usertask.username;
            ddswriter.query_writer(usertask.username,
                                   usertask.ai_type,
                                   usertask.partition_device,
                                   usertask.query_type,
                                   usertask.starttime,
                                   usertask.endtime,
                                   usertask.token,
                                   usertask.path,
                                   1);
        }
        else
        // if (ai_type.size() == 0 && !query_type) // Sam, IPFS Agent
        {
            if (checkmetadata(usertask.partition_device, usertask.starttime, usertask.endtime))
            {
                create_userfolder(path, partition_device, username, rootpath, timestampnow);

                if (usertask.rtsp_url != "None")
                {
                    // Read h2642ai topic;
                    // std::thread readerthread(h2642ai_func);
                    // readerthread.detach();
                    std::thread tranferrtspthread(rtsptom3u8func);
                    tranferrtspthread.detach();
                } 
                else
                {
                    // Write Tp_Query
                    ddswriter.query_writer(usertask.username,
                                        usertask.ai_type,
                                        usertask.partition_device,
                                        usertask.query_type,
                                        usertask.starttime,
                                        usertask.endtime,
                                        usertask.token,
                                        usertask.path,
                                        1);
                    if (ai_type.size() == 0)
                    {
                        // Read playh264 topic;
                        std::thread readerthread(playh264_func);
                        readerthread.detach();
                    } else {
                        return "None";
                    }
                }

                auto start = std::chrono::steady_clock::now();
                while (usertask.threadcontroll)
                {
                    int file_count = 0;
                    for (const auto &entry : std::filesystem::directory_iterator(catchoutput))
                        if (entry.is_regular_file())
                            ++file_count;
                    auto now = std::chrono::steady_clock::now();
                    if (file_count > 1)
                    {
                        json_obj["url"] = "http://" + ipaddr + ":8080/ramdisk/catchoutput/" +
                                          // json_obj["url"] = "/public/ramdisk/catchoutput/" +
                                          partition_device + "/" +
                                          username + "/" +
                                          std::to_string(timestampnow) + "/" +
                                          path + ".m3u8";
                        break;
                    }
                    else
                    {
                        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() >= 5000)
                        {
                            json_obj["url"] = "None";
                            break;
                        }
                        continue;
                    }
                }
            }
            else
            {
                json_obj["url"] = "None";
            }
        }
    }
    json_obj["token"] = token;
    json_obj["path"] = path;
    json_obj["type"] = "video";
    std::cout << json_obj << std::endl;

    // 序列化 JSON 對象為字符串
    return json_obj.dump();
}

pqxx::result sub_thread::searchdatabase(const std::string &tablename,
                                        const std::string &source,
                                        const std::int64_t &starttime,
                                        const std::int64_t &endtime)
{
    pqxx::result result;
    std::string exetutestring = "SELECT * FROM " +
                                tablename + " WHERE " +
                                "source = '" + source +
                                "' AND starttime >= " + to_string(starttime) + " AND endtime <= " + to_string(endtime);
    PostgresConnector postgresinstance;
    if (postgresinstance.open("paasdb", "dds_paas", "postgres", "10.1.1.200", 5433))
    {
        cout << "open database successfully" << endl;
        result = postgresinstance.executeResultset(exetutestring);
    }
    else
        cout << "Can't open database." << endl;
    bool closedb = postgresinstance.close();
    for (auto const &row : result)
    {
        std::string source = row.at("source").as<std::string>(); // source is a device id
        std::string content_id = row.at("content_id").as<std::string>();
        std::string file_name = row.at("file_name").as<std::string>();
        int unix_time_start = row.at("unix_time_start").as<int>();
        int unix_time_end = row.at("unix_time_end").as<int>();
        int format_code = row.at("format_code").as<int>();
        int file_bytes = row.at("file_bytes").as<int>();
        std::string status = row.at("status").as<std::string>();
        std::cout << "test" << std::endl;
        std::cout << source << std::endl
                  << content_id << std::endl
                  << file_name << std::endl
                  << std::to_string(unix_time_start) << " " << std::to_string(unix_time_end) << std::endl;
    }
    return result;
}
