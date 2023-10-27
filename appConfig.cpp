#include "appConfig.h"
#include <iostream>

AppConfig::AppConfig()
{

}

AppConfig::~AppConfig()
{
    
}

bool AppConfig::getObjectValue(const std::string jsonKey, boost::property_tree::ptree &value)
{
    bool result = false;

    try
    {
        value = this->jsonDoc.get_child(jsonKey);
        result = true;
    }
    catch(const std::exception& e)
    {
        result = false;
        std::cerr << e.what() << '\n';
    }

    return result;
}

bool AppConfig::openJsonFile(const std::string jsonFile)
{
    bool result = false;

    try
    {
        boost::property_tree::read_json(jsonFile, this->jsonDoc);
        
        if (! this->jsonDoc.empty())
            result = true;
    }
    catch(const std::exception& e)
    {
        result = false;
        std::cerr << e.what() << '\n';
    }
    
    return result;
}

bool AppConfig::getStringValue(const std::string jsonKey, std::string &value)
{
    bool result = false;

    try
    {
        value = this->jsonDoc.get<std::string>(jsonKey);
        result = true;
    }
    catch(const std::exception& e)
    {
        result = false;
        std::cerr << e.what() << '\n';
    }

    return result;
}

bool AppConfig::getIntValue(const std::string jsonKey, int &value)
{
    bool result = false;

    try
    {
        value = this->jsonDoc.get<int>(jsonKey);
        result = true;
    }
    catch(const std::exception& e)
    {
        result = false;
        std::cerr << e.what() << '\n';
    }

    return result;
}
