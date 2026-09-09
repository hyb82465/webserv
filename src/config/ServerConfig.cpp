#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
    : listens(),
      root("./www"),
      index("index.html"),
      client_max_body_size(1024 * 1024),
      error_pages(),
      locations()
{}

ServerConfig::~ServerConfig()
{}

ServerConfig::ServerConfig(const ServerConfig &other) 
    : listens(other.listens),
      root(other.root),
      index(other.index),
      client_max_body_size(other.client_max_body_size),
      error_pages(other.error_pages),
      locations(other.locations)
{}

ServerConfig &ServerConfig::operator=(const ServerConfig &other)
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

const std::vector<ListenConfig> &ServerConfig::getListens() const
{
    return listens;
}

const std::string &ServerConfig::getRoot() const
{
    return root;
}

const std::string &ServerConfig::getIndex() const
{
    return index;
}

size_t ServerConfig::getClientMaxBodySize() const
{
    return client_max_body_size;
}

const std::map<int, std::string> &ServerConfig::getErrorPages() const
{
    return error_pages;
}

const std::vector<LocationConfig> &ServerConfig::getLocations() const
{
    return locations;
}

void ServerConfig::setRoot(const std::string &root)
{
    this->root = root;
}

void ServerConfig::setIndex(const std::string &index)
{
    this->index = index;
}

void ServerConfig::setClientMaxBodySize(size_t size)
{
    this->client_max_body_size = size;
}

void ServerConfig::addListen(const ListenConfig &listen)
{
    listens.push_back(listen);
}

void ServerConfig::addErrorPage(int code, const std::string &path)
{
    error_pages[code] = path;
}

void ServerConfig::addLocation(const LocationConfig &location)
{
    locations.push_back(location);
}

void ServerConfig::applyDefaultsToLocations()
{
    for (size_t i = 0; i < locations.size(); ++i)
    {
        if (locations[i].getRoot().empty())
            locations[i].setRoot(root);
        if (locations[i].getIndex().empty())
            locations[i].setIndex(index);
    }
}