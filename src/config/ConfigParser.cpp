#include "ConfigParser.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>

ConfigParser::ConfigParser() {}
ConfigParser::~ConfigParser() {}
//ConfigParser::ConfigParser(const ConfigParser &other) {}
//ConfigParser &ConfigParser::operator=(const ConfigParser &other) { return *this; } 

// only open the file
ServerConfig ConfigParser::parse(const std::string &filename)
{
    ServerConfig config;

    std::ifstream file(filename.c_str());

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open configuration file: " + filename);
    }

    std::string line;
    int lineNumber = 1;
    
    std::cout << "Reading config ..." << std::endl;

    while (std::getline(file, line))
    {
        std::cout << "line " << lineNumber << ": " << line << std::endl;

        lineNumber++;
    }
    file.close();
    return config;
}