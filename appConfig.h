#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

class AppConfig: boost::property_tree::ptree
{
public:
    AppConfig();
    ~AppConfig();

    bool openJsonFile(const std::string jsonFile);
    bool getObjectValue(const std::string jsonKey, boost::property_tree::ptree &value);
    bool getStringValue(const std::string jsonKey, std::string &value);
    bool getIntValue(const std::string jsonKey, int &value);

private:
    boost::property_tree::ptree jsonDoc;

};
#endif 