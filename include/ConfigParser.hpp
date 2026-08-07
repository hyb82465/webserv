#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

#include <string>
#include <vector>
#include "ServerConfig.hpp"

class ConfigParser
{
    public:
        ConfigParser();
        ~ConfigParser();
        //ConfigParser(const ConfigParser &other);
        //ConfigParser &operator=(const ConfigParser &other);

        std::vector<ServerConfig> parse(const std::string &filename);
    
    private:
        std::string getValue(const std::string &line, ServerConfig &config);
        
        ServerConfig parseServer(std::ifstream &file);

        void parseListen(const std::string &line, ServerConfig &config);
        void parseRoot(const std::string &line, ServerConfig &config);
        void parseIndex(const std::string &line, ServerConfig &config);
};

#endif