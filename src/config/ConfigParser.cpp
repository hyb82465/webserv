#include "ConfigParser.hpp"
#include "Tokenizer.hpp"
#include "TokenStream.hpp"
#include "Utils.hpp"

#include <fstream>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>

ConfigParser::ConfigParser() {}
ConfigParser::~ConfigParser() {}

ServerConfig ConfigParser::parseServer(TokenStream &tokens)
{
    ServerConfig config;

    tokens.expect("server");
    tokens.expect("{");

    while (tokens.hasNext() && tokens.peek() != "}")
    {
        std::string token = tokens.peek();

        if (token == "}")
        {
            tokens.consume(); // consume the closing brace
            return config;
        }

        if (token == "listen")
        {
            tokens.consume(); // consume the 'listen' token
            std::string value = tokens.consume(); // consume the port value
            parseListen(value, config);
            tokens.expect(";"); // expect a semicolon after the value
        }
        else if (token == "root")
        {
            tokens.consume(); // consume the 'root' token
            std::string value = tokens.consume(); // consume the root value
            parseRoot(value, config);
            tokens.expect(";"); // expect a semicolon after the value
        }
        else if (token == "index")
        {
            tokens.consume(); // consume the 'index' token
            std::string value = tokens.consume(); // consume the index value
            parseIndex(value, config);
            tokens.expect(";"); // expect a semicolon after the value
        }
        else
        {
            throw std::runtime_error("Unexpected token: " + token);
        }
    }
    while (std::getline(file, line))
    {
        line = Utils::trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        if (line == "}")
            return config;

        parseListen(line, config);
        parseRoot(line, config);
        parseIndex(line, config);
    }
    throw std::runtime_error("Missing '}' for server block ");
}

std::vector<ServerConfig> ConfigParser::parse(const std::string &filename)
{
    std::ifstream file(filename.c_str());

    if (!file.is_open())
        throw std::runtime_error("Failed to open configuration file: " + filename);
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string fileContent = buffer.str();
    file.close();

    Tokenizer tokenizer;
    std::vector<std::string> tokens = tokenizer.getTokens(fileContent);
    
    //print tokens for debugging
    std::cout << "Tokens: " << std::endl;
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        std::cout << tokens[i] << std::endl;
    }           
    std::vector<ServerConfig> servers;
    


    return servers;
}

std::string ConfigParser::getValue(const std::string &line)
{  
        std::istringstream iss(line);

        std::string keyword;
        std::string value;

        iss >> keyword;

        if (!(iss >> value))
        {
            throw std::runtime_error("Missing Value");
        }   

        if (!value.empty() && value[value.size() - 1] == ';')
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
    
    std::string value = getValue(line);
    
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
    
    config.root = getValue(line);
}

void ConfigParser::parseIndex(const std::string &line, ServerConfig &config)
{
    if(line.find("index") != 0)
        return;
    
    config.index = getValue(line);
}