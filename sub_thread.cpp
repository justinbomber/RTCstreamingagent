#include "sub_thread.h"
#include <iostream>
#include "ddsreader.h"
#include "ddswriter.h"

using namespace std;

sub_thread::sub_thread()
{
}

sub_thread::~sub_thread()
{
}

void appendToM3U8File(std::string m3u8name, const std::string &directory, const std::string &newString)
{
    std::string filename = directory + m3u8name + ".m3u8";

    // 打開已存在的文件用於追加
    std::ofstream outFile(filename, std::ios::app);

    // 檢查文件是否成功打開
    if (!outFile)
    {
        std::cerr << "無法打開文件" << std::endl;
        return;
    }
    outFile << "#EXTINF:1.000000,\n";
    outFile << newString;
    outFile << "#EXT-X-DISCONTINUITY\n";

    // 關閉文件
    outFile.close();
}

void transferH264(const std::string &targetFolder, bool &threadcontroll, std::string m3u8name, const std::string &inputfolder)
{
    while (threadcontroll)
    {
        for (const auto &entry : std::filesystem::directory_iterator(inputfolder))
        {
            if (entry.is_regular_file())
            {
                const std::string filename = entry.path().filename().string();
                const std::string extension = entry.path().extension().string();

                if (extension == ".h264")
                {
                    if (threadcontroll == false)
                        break;
                    const std::string filenameWithoutExt = entry.path().stem().string();
                    std::string cmdline = "ffmpeg -i " + inputfolder + filenameWithoutExt + ".h264 -c:v libx264 -c:a aac " +
                                          targetFolder + filenameWithoutExt + ".ts && mv " +
                                          targetFolder + filenameWithoutExt + ".ts " +
                                          targetFolder + "\"\'" + filenameWithoutExt + ".ts\'\"";
                    system(cmdline.c_str());
                    std::string m3u8input = "'" + filenameWithoutExt + ".ts'\n";
                    appendToM3U8File(m3u8name, targetFolder, m3u8input);

                    std::filesystem::remove(entry.path());
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
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
    outFile << "#EXT-X-START:TIME-OFFSET=-1,PRECISE=YES\n";
    outFile << "#EXT-X-TARGETDURATION:5\n";
    outFile << "#EXT-X-MEDIA-SEQUENCE:0\n";

    // 關閉文件
    outFile.close();
}

std::string sub_thread::sub_thread_task(std::map<UserDevice, UserTask> &taskmanager,
                                        const UserDevice userdevice,
                                        dds::domain::DomainParticipant &ddscam_participant,
                                        dds::domain::DomainParticipant &paas_participant)
{

    std::string path = taskmanager[userdevice].path;
    std::string username = taskmanager[userdevice].token;
    std::string partition_device = taskmanager[userdevice].partition_device;
    bool query_type = taskmanager[userdevice].query_type;
    std::int64_t starttime = taskmanager[userdevice].starttime;
    std::int64_t endtime = taskmanager[userdevice].endtime;
    std::string resolution = taskmanager[userdevice].resolution;
    std::vector<std::string> ai_type = taskmanager[userdevice].ai_type;

    DDSReader ddsreader;
    DDSWriter ddswriter;
    std::string rootpath = "../../vue3-video-play/public/ramdisk/";
    std::string catchinput = rootpath + "/catchinput/" + username + "/" + partition_device + "/";
    std::string catchoutput = rootpath + "/catchoutput/" + username + "/" + partition_device + "/";

    std::vector<std::filesystem::path> filevec;
    std::filesystem::path rootPath = rootpath;
    filevec.push_back(rootPath);
    std::filesystem::path inputPath = rootPath / "catchinput";
    filevec.push_back(inputPath);
    std::filesystem::path inputuserPath = rootpath + "/catchinput/" + username;
    filevec.push_back(inputuserPath);
    std::filesystem::path inputdevicePath = inputuserPath / partition_device;
    filevec.push_back(inputdevicePath);
    std::filesystem::path outputPath = rootPath / "catchoutput";
    filevec.push_back(outputPath);
    std::filesystem::path userPath = rootpath + "/catchoutput/" + username;
    filevec.push_back(userPath);
    std::filesystem::path devicePath = userPath / partition_device;
    filevec.push_back(devicePath);

    // 如果資料夾不存在則創建
    for (auto &path : filevec)
    {
        if (!std::filesystem::exists(path))
        {
            std::filesystem::create_directories(path);
            std::cout << "Created directory: " << path.string() << std::endl;
        }
    }
    std::string partition = partition_device + username;

    createM3U8File(catchoutput, path);
    auto videostream_func = std::bind(&DDSReader::videostream_reader, &ddsreader,
                                      std::ref(taskmanager),
                                      userdevice,
                                      std::ref(taskmanager[userdevice].threadcontroll),
                                      std::ref(ddscam_participant),
                                      catchinput);

    auto playh264_func = std::bind(&DDSReader::playh264_reader, &ddsreader,
                                   std::ref(taskmanager),
                                   userdevice,
                                   std::ref(taskmanager[userdevice].threadcontroll),
                                   std::ref(paas_participant),
                                   catchinput);
    auto transfunc = std::bind(&transferH264, catchoutput, std::ref(taskmanager[userdevice].threadcontroll), path, catchinput);
    // if (ai_type.size() == 0 && query_type)
    // {
    //     // Read VideoStreaming
    //     if (resolution == "1080")
    //     {
    //         std::thread readerthread(videostream_func);
    //         readerthread.detach();
    //     }
    //     else
    //     {
    //         // TODO: transfer to 480p
    //         std::thread readerthread(videostream_func);
    //         readerthread.detach();
    //     }
    //     std::thread transthread(transfunc);
    //     transthread.detach();
    //     // TODO: transfer frame to RTC server
    // }
    // else
    // {
    //     // Write Tp_Query
    //     ddswriter.query_writer(taskmanager[userdevice].token,
    //                            taskmanager[userdevice].ai_type,
    //                            taskmanager[userdevice].partition_device,
    //                            taskmanager[userdevice].query_type,
    //                            taskmanager[userdevice].starttime,
    //                            taskmanager[userdevice].endtime,
    //                            1, std::ref(paas_participant));
    //     if (ai_type.size() > 0 && query_type) // Sam, AI dds Agent
    //     {
    //         // Read playh264 topic;
            
    //         std::thread readerthread(playh264_func);
    //         readerthread.detach();
    //     }
    //     else if (ai_type.size() == 0 && !query_type) // lung, IPFS Agent
    //     {
    //         // TODO: ipfs controller for NCHC

    //         // Read playh264 topic;
    //         std::thread readerthread(playh264_func);
    //         readerthread.detach();
    //     }
    //     else if (ai_type.size() > 0 && !query_type) // lung, IPFS Agent
    //     {
    //         // TODO: ipfs controller for NCHC

    //         // Read AI tag
            
    //         boost::property_tree::ptree jsonObject;
    //         pqxxController pqc1;
    //         std::string *ai_type_array=&ai_type[0];
    //         jsonObject = pqc1.get_multitag_ai_type_intime(partition_device, starttime, endtime, ai_type_array,ai_type.size());
    //         //pqxx::result ai_timestamp = searchdatabase("tb_cam_pre_ai_meta", partition_device, starttime, endtime);

    //         // TODO: Read playh264 topic;
    //         std::thread readerthread(playh264_func);
    //         readerthread.detach();
    //     }
    //     std::thread transthread(transfunc);
    //     std::cout << ">>>>start trans thread" << std::endl;
    //     transthread.detach();
    //     // TODO: transfer frame to RTC server
    // }

    // 創建 JSON 對象
    nlohmann::json json_obj;
    json_obj["token"] = username;
    // TODO: change path
    json_obj["url"] = "http://10.1.1.128:8088/ramdisk/catchoutput/" + username + "/" + partition_device + "/" + path + ".m3u8";
    json_obj["path"] = path;
    json_obj["type"]="video";

    // 序列化 JSON 對象為字符串
    std::string json_str = json_obj.dump();
    sleep(3);
    // return json_str;
    return json_str;
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
