#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

#include <string>
#include "LocationConfig.hpp"

class ServerConfig
{
    public:
        ServerConfig();
        ~ServerConfig();
        ServerConfig(const ServerConfig &other);
        ServerConfig &operator=(const ServerConfig &other);

        int port;
        std::string root;
        std::string index;
        std::vector<LocationConfig> locations;
};

#endif 