#include "postgresConnector.h"
#include <iostream>

PostgresConnector::PostgresConnector()
{

}

PostgresConnector::~PostgresConnector()
{
    try {
        if (this->connection != nullptr) {  // 檢查 this->connection 是否為 nullptr
            if (this->connection->is_open()) this->connection->disconnect();
            delete this->connection;
            this->connection = nullptr;
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}


bool PostgresConnector::open(std::string dbName, std::string username, std::string password, std::string hostAddress, int port)
{
    bool result = false;

    try
    {
        std::stringstream ss;
        ss << "dbname=" << dbName
           << " user=" << username
           << " password=" << password
           << " hostaddr=" << hostAddress
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
    catch(const std::exception& e)
    {
        result = false;
        std::cerr << e.what() << '\n';
    }
    
    return result;
}

bool PostgresConnector::close()
{
    bool result = false;

    try
    {
        if (this->connection->is_open()) this->connection->disconnect();

        result = true;
    }
    catch(const std::exception& e)
    {
        result = false;
        std::cerr << e.what() << '\n';
    }

    return result;
}

bool PostgresConnector::executeNoResultset(std::string command)
{
    bool result = false;

    pqxx::work tran(*this->connection);
    try
    {
        tran.exec(command);
        tran.commit();

        result = true;
    }
    catch(const std::exception& e)
    {
        tran.abort();
        result = false;
        std::cerr << e.what() << '\n';
    }
    
    return result;
}

pqxx::result PostgresConnector::executeResultset(std::string command, bool useTransaction)
{
    pqxx::result result;

    try
    {
        if (useTransaction)
        {
            pqxx::work tran(*this->connection);
            try
            {
                result = tran.exec(command);
                tran.commit();
            }
            catch(const std::exception& e)
            {
                tran.abort();
                std::cerr << e.what() << '\n';
            }
        }
        else
        {
            pqxx::nontransaction nonTran(*this->connection);
            result = nonTran.exec(command);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    return result;
}

