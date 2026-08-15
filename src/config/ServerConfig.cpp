#include "ServerConfig.hpp"

ServerConfig :: ServerConfig() : listens(), root("./www"), index("index.html"), client_max_body_size(1000000), error_pages(), locations()  
{
    error_pages[400] = "./www/errors/400.html";
    error_pages[403] = "./www/errors/403.html";
    error_pages[404] = "./www/errors/404.html";
    error_pages[405] = "./www/errors/405.html";
    error_pages[413] = "./www/errors/413.html";
    error_pages[500] = "./www/errors/500.html";
}
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

const std::vector<ListenConfig> &
ServerConfig::getListens() const
{
    return listens;
}

const std::string &
ServerConfig::getRoot() const
{
    return root;
}

const std::string &
ServerConfig::getIndex() const
{
    return index;
}

size_t ServerConfig::getClientMaxBodySize() const
{
    return client_max_body_size;
}

const std::map<int, std::string> &
ServerConfig::getErrorPages() const
{
    return error_pages;
}

const std::vector<LocationConfig> &
ServerConfig::getLocations() const
{
    return locations;
}

void ServerConfig::setRoot(
    const std::string &root)
{
    this->root = root;
}

void ServerConfig::setIndex(
    const std::string &index)
{
    this->index = index;
}

void ServerConfig::setClientMaxBodySize(
    size_t size)
{
    this->client_max_body_size = size;
}

void ServerConfig::addListen(
    const ListenConfig &listen)
{
    listens.push_back(listen);
}

void ServerConfig::addListen(
    const ListenConfig &listen)
{
    listens.push_back(listen);
}

void ServerConfig::addErrorPage(
    int code,
    const std::string &path)
{
    error_pages[code] = path;
}