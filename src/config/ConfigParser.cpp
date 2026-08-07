#include "ConfigParser.hpp"
#include "Utils.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>

ConfigParser::ConfigParser() {}
ConfigParser::~ConfigParser() {}

// only open the file
std::vector<ServerConfig> ConfigParser::parse(const std::string &filename)
{
    std::vector<ServerConfig> servers;
    
    std::ifstream file(filename.c_str());
    
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open configuration file: " + filename);
    }
    
    std::string line;
    bool isServerBlock = false;
    
    std::cout << "Reading config ..." << std::endl;

    while (std::getline(file, line))
    {
        line = Utils::trim(line);
        if(line.empty() || line[0] == '#')
            continue; 
        if (line == "server {")
        {
            if (isServerBlock)
                throw std::runtime_error("Nested server block: " + line);
            isServerBlock = true;
            continue;
        }
        if(line == "}")
        {
            if (!isServerBlock)
                throw std::runtime_error("Unexpected '}'" + line);
            isServerBlock = false;
            continue;
        }
        if (!isServerBlock)
            throw std::runtime_error("Unexpected line outside of server block: " + line );
        
        parseListen(line, config);
        parseRoot(line, config);
        parseIndex(line, config);
    
    }
    file.close();
    return config;
}

std::string ConfigParser::getValue(const std::string &line, ServerConfig &config)
{  
        std::istringstream iss(line);

        std::string keyword;
        std::string value;

        iss >> keyword;

        if (!(iss >> value))
        {
            throw std::runtime_error("Missing Value");
        }   

        if (value.empty() && value[value.size() - 1] == ';')
            value.erase (value.size() - 1);

        std::string extra;
        if (iss >> extra)
            throw std::runtime_error("too many arguments for " + keyword + ": " + extra);
        
        return value;
}
void ConfigParser::parseListen(const std::string &line, ServerConfig &config)
{
    if(line.find("listen") != 0)
        return;
    
    std::string value = getValue(line, config);
    
    char *endPtr;
    long port = std::strtol(value.c_str(), &endPtr, 10);
    if (*endPtr != '\0' )
        throw std::runtime_error("Invalid Value: " + value);
    
    if (port < 1 || port > 65535)
        throw std::out_of_range("Value out of range");
    config.port = static_cast<int>(port);
}

void ConfigParser::parseRoot(const std::string &line, ServerConfig &config)
{
    if(line.find("root") != 0)
        return;
    
    config.root = getValue(line, config);
}

void ConfigParser::parseIndex(const std::string &line, ServerConfig &config)
{
    if(line.find("index") != 0)
        return;
    
    config.index = getValue(line, config);
}