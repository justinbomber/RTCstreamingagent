#include "sub_thread.h"
#include <iostream>
#include "ddsreader.h"
#include "ddswriter.h"
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
    outFile << "#EXT-X-START:TIME-OFFSET=-1,PRECISE=YES\n";
    outFile << "#EXT-X-TARGETDURATION:5\n";
    outFile << "#EXT-X-MEDIA-SEQUENCE:0\n";

    // 關閉文件
    outFile.close();
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


void transferH264(const std::string &targetFolder, UserTask &usertask, std::string m3u8name, const std::string &inputfolder)
{
    DDSWriter ddswriter;
    int traffic_status = 1;
    int last_taraffic_status = 1;
    while (usertask.threadcontroll)
    {
         
        int file_count = 0;
        for (const auto& entry : std::filesystem::directory_iterator(inputfolder))
            if (entry.is_regular_file()) 
                ++file_count;
        if (file_count > 1)
            traffic_status = 2;
        else
            traffic_status = 1;
        if(traffic_status != last_taraffic_status)
        {
            if (traffic_status == 2)
                ddswriter.query_writer(usertask.token, 
                                        usertask.ai_type, 
                                        usertask.partition_device, 
                                        usertask.query_type, 
                                        usertask.starttime, 
                                        usertask.endtime, 
                                        2);
            else
                ddswriter.query_writer(usertask.token, 
                                        usertask.ai_type, 
                                        usertask.partition_device, 
                                        usertask.query_type, 
                                        usertask.starttime, 
                                        usertask.endtime, 
                                        1);

            last_taraffic_status = traffic_status;
        }

        for (const auto &entry : std::filesystem::directory_iterator(inputfolder))
        {
            if (entry.is_regular_file())
            {
                const std::string extension = entry.path().extension().string();

                if (extension == ".h264")
                {
                    if (!usertask.threadcontroll)
                        return;
                    const std::string filenameWithoutExt = entry.path().stem().string();
                    std::string cmdline;
                    if(usertask.resolution == "480") 
                        cmdline = "ffmpeg -i " + inputfolder + filenameWithoutExt + ".h264 -preset ultrafast -c:v libx264 -c:a aac -s 854x480 " +
                                          targetFolder + filenameWithoutExt + ".ts && mv " +
                                          targetFolder + filenameWithoutExt + ".ts " +
                                          targetFolder + "\"\'" + filenameWithoutExt + ".ts\'\"";
                    else
                        cmdline = "ffmpeg -i " + inputfolder + filenameWithoutExt + ".h264 -preset ultrafast -c:v libx264 -c:a aac -s 1920x1080 " +
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

void delete_all_files(const std::filesystem::path& path) {
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (entry.is_regular_file()) {
            std::filesystem::remove(entry.path());
        }
    }
}

std::string sub_thread::sub_thread_task(UserTask & usertask)
{

    std::string path = usertask.path;
    std::string username = usertask.token;
    std::string partition_device = usertask.partition_device;
    bool query_type = usertask.query_type;
    std::int64_t starttime = usertask.starttime;
    std::int64_t endtime = usertask.endtime;
    std::string resolution = usertask.resolution;
    std::vector<std::string> ai_type = usertask.ai_type;

    DDSReader ddsreader;
    DDSWriter ddswriter;
    std::string rootpath = "../../work/paas/vue3-video-play/public/ramdisk";
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

    // create non existing directory
    for (auto &path : filevec)
    {
        if (!std::filesystem::exists(path))
        {
            std::filesystem::create_directories(path);
            std::cout << "Created directory: " << path.string() << std::endl;
        }
    }
    delete_all_files(inputdevicePath);
    delete_all_files(devicePath);
    createM3U8File(catchoutput, path);
    auto videostream_func = std::bind(&DDSReader::videostream_reader, &ddsreader,
                                      std::ref(usertask),
                                      catchinput);

    auto playh264_func = std::bind(&DDSReader::playh264_reader, &ddsreader,
                                   std::ref(usertask),
                                   catchinput);
    auto transfunc = std::bind(&transferH264, catchoutput, std::ref(usertask), path, catchinput);
    if (ai_type.size() == 0 && query_type)
    {
        // Read VideoStreaming
        if (resolution == "1080")
        {
            std::thread readerthread(videostream_func);
            readerthread.detach();
        }
        else
        {
            // TODO: transfer to 480p
            std::thread readerthread(videostream_func);
            readerthread.detach();
        }
        std::thread transthread(transfunc);
        transthread.detach();
        // TODO: transfer frame to RTC server
    }
    else
    {
        // Write Tp_Query
        ddswriter.query_writer(usertask.token, 
                                usertask.ai_type, 
                                usertask.partition_device, 
                                usertask.query_type, 
                                usertask.starttime, 
                                usertask.endtime, 
                                1);
        if (ai_type.size() > 0 && query_type) // Sam, AI dds Agent
        {
            // Read playh264 topic;
            std::thread readerthread(playh264_func);
            readerthread.detach();
        }
        else if (ai_type.size() == 0 && !query_type) // lung, IPFS Agent
        {
            // TODO: ipfs controller for NCHC

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

            // TODO: Read playh264 topic;
            std::thread readerthread(playh264_func);
            readerthread.detach();
        }
        std::thread transthread(transfunc);
        transthread.detach();
        // TODO: transfer frame to RTC server
    }

    // 創建 JSON 對象
    nlohmann::json json_obj;
    json_obj["token"] = username;
    // TODO: change path
    json_obj["url"] = "http://10.1.1.128:8088/ramdisk/catchoutput/" + username + "/" + partition_device + "/" + path + ".m3u8";
    json_obj["path"] = path;
    json_obj["type"]="video";

    // 序列化 JSON 對象為字符串
    std::string json_str = json_obj.dump();
    // sleep(3);
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
