#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

#include <string>
#include "ServerConfig.hpp"

class ConfigParser
{
    public:
        ConfigParser();
        ~ConfigParser();
        ConfigParser(const ConfigParser &other);
        ConfigParser &operator=(const ConfigParser &other);

        ServerConfig parse(const std::string &filename);
};

#endif