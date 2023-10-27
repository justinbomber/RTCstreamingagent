/*
Author: Philip Chang

easy to use progreSQL
*/
#ifndef pqxxController_hpp
#define pqxxController_hpp
#include <pqxx/pqxx>
#include <string>
// #include <rti/rti.hpp>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include "appConfig.h"

class pqxxController
{
public:
    // Connection parameters for Postgres
    std::string APP_CONFIG = "app.json";
    std::string DB_HOST = "host";
    std::string DB_PORT = "port";
    std::string DB_NAME = "dbname";
    std::string DB_USERNAME = "username";
    std::string DB_PASSWORD = "password";
    std::string host = "10.1.1.200";
    std::string dbName = "paasdb";
    std::string username = "dds_paas";
    std::string password = "postgres";
    int port = 5433;
    pqxxController(std::string pc_host, std::string pc_dbName, std::string pc_username, std::string pc_password, int pc_port);
    pqxxController();
    ~pqxxController();
    bool open();
    bool close();
    pqxx::result get_ai_type_intime(std::string camera, int starttime, int endtime, std::string ai_type);
    void get_app_configuration();
    boost::property_tree::ptree get_all_ai_type_intime(std::string camera, int starttime, int endtime);

    boost::property_tree::ptree get_multitag_ai_type_intime(std::string camera, int starttime, int endtime, std::string *user_tag_type, int user_tag_type_length);
    inline std::string ptreeToJsonString(const boost::property_tree::ptree &tree)
    {
        std::stringstream ss;
        boost::property_tree::write_json(ss, tree, false);
        return ss.str();
    }
    pqxx::connection *connection = nullptr;
};

pqxxController::pqxxController()
{
    get_app_configuration();
}
pqxxController::pqxxController(std::string pc_host, std::string pc_dbName, std::string pc_username, std::string pc_password, int pc_port)
{
    host = pc_host;
    dbName = pc_dbName;
    username = pc_username;
    password = pc_password;
    port = pc_port;
}
pqxxController::~pqxxController()
{
    if (connection != NULL)
    {
        if (this->connection->is_open())
            this->connection->disconnect();

        delete this->connection;
        this->connection = nullptr;
    }
}
bool pqxxController::open()
{
    bool result = false;
    try
    {
        std::stringstream ss;
        ss << "dbname=" << dbName
           << " user=" << username
           << " password=" << password
           << " hostaddr=" << host
           << " port=" << std::to_string(port);
        this->connection = new pqxx::connection(ss.str());
        if (this->connection->is_open())
        {
            result = true;
            std::cout << "Open database successfully." << std::endl;
        }
        else
        {
            result = false;
            std::cout << "Can't open database." << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        result = false;
        std::cerr << e.what() << '\n';
    }

    return result;
}
bool pqxxController::close()
{
    bool result = false;
    try
    {
        if (this->connection->is_open())
            this->connection->disconnect();

        result = true;
        std::cout << "dcdb" << std::endl;
    }
    catch (const std::exception &e)
    {
        result = false;
        std::cerr << e.what() << '\n';
    }

    return result;
}
void pqxxController::get_app_configuration()
{
    AppConfig *cfg = new AppConfig();
    if (cfg->openJsonFile(APP_CONFIG))
    {
        std::cout << "Opened app.json." << std::endl;

        boost::property_tree::ptree jsonObject;
        if (cfg->getObjectValue("postgres", jsonObject))
        {
            std::cout << "Opened `postgres` configuration." << std::endl;

            host = jsonObject.get<std::string>(DB_HOST);
            port = jsonObject.get<int>(DB_PORT);
            dbName = jsonObject.get<std::string>(DB_NAME);
            username = jsonObject.get<std::string>(DB_USERNAME);
            password = jsonObject.get<std::string>(DB_PASSWORD);
        }
    }
    delete cfg;
    cfg = nullptr;
}
pqxx::result pqxxController::get_ai_type_intime(std::string camera, int starttime, int endtime, std::string ai_type)
{
    open();
    pqxx::result result;
    std::stringstream command;
    command << "SELECT min(unix_time) AS " << ai_type << " FROM public.tb_cam_pre_ai_meta WHERE "
            << "source = '" << camera << "' "
            << "AND unix_time >= " << std::to_string(starttime) << " "
            << "AND unix_time <= " << std::to_string(endtime) << " "
            << "AND tag_object LIKE '%" << ai_type << "%' "
            << "group by unix_time "
            << "ORDER BY " << ai_type << " ASC";
    if (true)
    {
        try
        {
            pqxx::work tran(*this->connection);
            try
            {
                result = tran.exec(command);
                tran.commit();
            }
            catch (const std::exception &e)
            {
                tran.abort();
                std::cerr << e.what() << '\n';
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
    }
    else
    {
        pqxx::nontransaction nonTran(*this->connection);
        result = nonTran.exec(command);
    }
    close();
    return result;
}
boost::property_tree::ptree pqxxController::get_all_ai_type_intime(std::string camera, int starttime, int endtime)
{
    boost::property_tree::ptree json_result;

    pqxxController pqc1;
    std::string tag_type[] = {
        "human",
        "bird",
        "cat",
        "dog",
        "light",
        "hydrant",
        "car",
        "bike",
        "motor",
        "plane",
        "bus",
        "train",
        "truck",
        "boat",
    };
    try
    {

        boost::property_tree::ptree lchildren;
        for (int i = 0; i < sizeof(tag_type) / sizeof(tag_type[0]); i++)
        {
            boost::property_tree::ptree children;
            // std::cout << tag_type[i] << std::endl;
            pqxx::result rows = pqc1.get_ai_type_intime(camera, starttime, endtime, tag_type[i]);
            boost::property_tree::ptree child;
            if (!rows.empty())
            {
                boost::property_tree::ptree lchild;
                int count = 0;
                for (auto const &row : rows)
                {
                    std::string source = row.at(tag_type[i]).as<std::string>(); // source is a device id
                    std::cout << source << " ";
                    lchild.put("", source);
                    child.push_back(std::make_pair("", lchild));
                }
                children.push_back(std::make_pair(tag_type[i], child));
            }
            lchildren.push_back(std::make_pair("", children));
        }

        json_result.add_child("ai_type", lchildren);
    }
    catch (const std::exception &e)
    {

        std::cerr << e.what() << '\n';
    }
    return json_result;
}
boost::property_tree::ptree pqxxController::get_multitag_ai_type_intime(std::string camera, int starttime, int endtime, std::string *user_tag_type, int user_tag_type_length)
{
    size_t tag_type_size = user_tag_type_length;

    boost::property_tree::ptree json_result;

    pqxxController pqc1;
    try
    {

        boost::property_tree::ptree lchildren;
        for (int i = 0; i < tag_type_size; i++)
        {
            boost::property_tree::ptree children;
            // std::cout << tag_type[i] << std::endl;
            pqxx::result rows = pqc1.get_ai_type_intime(camera, starttime, endtime, user_tag_type[i]);
            boost::property_tree::ptree child;
            if (!rows.empty())
            {
                boost::property_tree::ptree lchild;
                int lastvalue=0;
                int count=0;
                for (auto const &row : rows)
                {
                    std::string source = row.at(user_tag_type[i]).as<std::string>();
                    std::cout << source << " ";
                    long long int temp = stoi(source);
                    if (count != 0)
                    {
                        //int ttmep=stoi(lchild.end().);
                        if (temp-lastvalue<60)
                        {
                            continue;
                        }
                    }
                    lastvalue=temp;
                    count+=1;
                    lchild.put("", temp);
                    child.push_back(std::make_pair("", lchild));
                }
                children.push_back(std::make_pair(user_tag_type[i], child));
                std::cout << user_tag_type[i] << std::endl;
                lchildren.push_back(std::make_pair("", children));
            }
        }
        if (!lchildren.empty())
        {
            json_result.add_child("ai_type", lchildren);
        }
        else
        {
            json_result.put("ai_type", "");
            std::cout << "NULL";
        }
    }
    catch (const std::exception &e)
    {

        std::cerr << e.what() << '\n';
    }

    // boost::property_tree::write_json("test1.json", json_result);
    return json_result;
}

#endif