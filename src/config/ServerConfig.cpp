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