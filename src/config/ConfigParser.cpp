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

        if (tokens.match("listen"))
            parseListen(tokens, config);
        else if (tokens.match("root"))
            parseRoot(tokens, config);
        else if (tokens.match("index"))
            parseIndex(tokens, config);
        else
            throw std::runtime_error("Unexpected token: " + token);
    }
    tokens.expect("}");
    return config;
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
    std::vector<std::string> tokens = tokenizer.tokenize(fileContent);
    
    //print tokens for debugging
    std::cout << "Tokens: " << std::endl;
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        std::cout << tokens[i] << std::endl;
    }

    std::vector<ServerConfig> servers;
    TokenStream tokenStream(tokens);

    while (tokenStream.hasNext())
    {
        servers.push_back(parseServer(tokenStream));
    }   
    return servers;
}


void ConfigParser::parseListen(TokenStream &tokens, ServerConfig &config)
{
    std::string port = tokens.consume();

    tokens.expect(";");

    char *endPtr;
    long portValue = std::strtol(port.c_str(), &endPtr, 10);
    if (*endPtr != '\0' )
        throw std::runtime_error("Invalid Value: " + port);
    if (portValue < 1 || portValue > 65535)
        throw std::out_of_range("Port Value out of range");

    config.port = static_cast<int>(portValue);
}

void ConfigParser::parseRoot(TokenStream &tokens, ServerConfig &config)
{
    config.root = tokens.consume();
    tokens.expect(";");
}

void ConfigParser::parseIndex(TokenStream &tokens, ServerConfig &config)
{
    config.index = tokens.consume();
    tokens.expect(";");
}
