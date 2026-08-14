#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

#include <string>
#include "LocationConfig.hpp"
#include "ListenConfig.hpp"
#include <vector>
#include <map>

class ServerConfig
{
    public:
        ServerConfig();
        ~ServerConfig();
        ServerConfig(const ServerConfig &other);
        ServerConfig &operator=(const ServerConfig &other);

        std::vector<ListenConfig> listens;
        std::string root;
        std::string index;
        size_t client_max_body_size;
        std::map<int, std::string> error_pages;
        std::vector<LocationConfig> locations;
};

#endif 