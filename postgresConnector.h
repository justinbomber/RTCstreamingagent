#ifndef POSTGRES_CONNECTOR
#define POSTGRES_CONNECTOR

#include <pqxx/pqxx>
#include <string>

class PostgresConnector
{
public:
    PostgresConnector();
    ~PostgresConnector();

    bool open(std::string dbName, std::string username, std::string password, std::string hostAddress, int port);
    bool close();
    bool executeNoResultset(std::string command);
    pqxx::result executeResultset(std::string command, bool useTransaction=false);

    pqxx::connection *connection = nullptr;
};
#endif