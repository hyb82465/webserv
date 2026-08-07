#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

#include <string>

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
};

#endif 