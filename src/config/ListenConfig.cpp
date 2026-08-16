#include "ListenConfig.hpp"

ListenConfig::ListenConfig():host("0.0.0.0"),port(8080){}

ListenConfig::ListenConfig(const std::string &host, int port):host(host), port(port){}
ListenConfig::~ListenConfig(){}

ListenConfig::ListenConfig(const ListenConfig &other):host(other.host), port(other.port){}
ListenConfig& ListenConfig::operator=(const ListenConfig &other)
{
    if (this != &other)
    {
        host = other.host;
        port = other.port;
    }
    return *this;
}

const std::string &ListenConfig::getHost()const
{
    return host;
}
int ListenConfig::getPort() const
{
    return port;
}

void ListenConfig::setHost(const std::string &host)
{
    this->host = host;
}
void ListenConfig::setPort(int port)
{
    this->port = port;
}