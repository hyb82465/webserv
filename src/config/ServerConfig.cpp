#include "ServerConfig.hpp"

ServerConfig :: ServerConfig() : listens(), root("./www"), index("index.html"), client_max_body_size(1000000), error_pages(), locations()  {}
ServerConfig :: ~ServerConfig() {}
ServerConfig :: ServerConfig(const ServerConfig &other) : listens(other.listens), root(other.root), index(other.index), client_max_body_size(other.client_max_body_size), error_pages(other.error_pages), locations(other.locations) {}
ServerConfig &ServerConfig :: operator=(const ServerConfig &other)
{
    if (this != &other)
    {
        listens = other.listens;
        root = other.root;
        index = other.index;
        client_max_body_size = other.client_max_body_size;
        error_pages = other.error_pages;
        locations = other.locations;
    }
    return *this;
}