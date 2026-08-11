#include "ServerConfig.hpp"

ServerConfig :: ServerConfig() : port(8080), root("./www"), index("index.html"), locations()  {}
ServerConfig :: ~ServerConfig() {}
ServerConfig :: ServerConfig(const ServerConfig &other) : port(other.port), root(other.root), index(other.index), locations(other.locations) {}
ServerConfig &ServerConfig :: operator=(const ServerConfig &other)
{
    if (this != &other)
    {
        port = other.port;
        root = other.root;
        index = other.index;
        locations = other.locations;
    }
    return *this;
}