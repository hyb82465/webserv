#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

#include <string>
#include <fstream>
#include <vector>
#include "ServerConfig.hpp"

class ConfigParser
{
    public:
        ConfigParser();
        ~ConfigParser();
        //ConfigParser(const ConfigParser &other);
        //ConfigParser &operator=(const ConfigParser &other);

        std::vector<ServerConfig> parse(TokenStream &tokens);
    
    private:
        std::string getValue(const std::string &line);
        
        ServerConfig parseServer(std::ifstream &file);

        void parseListen(const std::string &line, ServerConfig &config);
        void parseRoot(const std::string &line, ServerConfig &config);
        void parseIndex(const std::string &line, ServerConfig &config);
};

#endif