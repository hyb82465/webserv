#include "ConfigParser.hpp"
#include "Tokenizer.hpp"
#include "TokenStream.hpp"
#include "LocationConfig.hpp"
#include "Utils.hpp"

#include <fstream>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>

ConfigParser::ConfigParser() {}
ConfigParser::~ConfigParser() {}

ListenConfig ConfigParser::parseListenValue(const std::string &value)
{
    ListenConfig listen;

    size_t colon = value.find(':');

    if (colon == std::string::npos)
    {
		// printf("debug 1\n");

        listen.host = "0.0.0.0";

        char *endPtr;
        long portValue = std::strtol(value.c_str(), &endPtr, 10);
        if (*endPtr != '\0' )
            throw std::runtime_error("Invalid Value: " + portValue);
        if (portValue < 1 || portValue > 65535)
            throw std::out_of_range("Port Value out of range");
   
        listen.port = static_cast<int>(portValue);

        return listen;
    }

    std::string host = value.substr(0, colon);
    std::string portStr = value.substr(colon + 1);
    if (host.empty())
        throw std::runtime_error("Missing interface in listen");
    
    char *endPtr;
    long portValue = std::strtol(portStr.c_str(), &endPtr, 10);
    if (*endPtr != '\0' )
            throw std::runtime_error("Invalid Value: " + portStr);
    if (portValue < 1 || portValue > 65535)
            throw std::out_of_range("Port Value out of range");
    
    listen.host = host;
    listen.port = static_cast<int>(portValue);
    return listen;
}
void ConfigParser::parseListen(TokenStream &tokens, ServerConfig &config)
{
    std::string port = tokens.consume();

    tokens.expect(";");
		// printf("debug 1\n");

    ListenConfig listen = parseListenValue(port);
    config.listens.push_back(listen);
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
void ConfigParser::parseClientMaxBodySize(TokenStream &tokens, ServerConfig &config)
{
    std::string size = tokens.consume();
    tokens.expect(";");
    char *endPtr;
    long sizeValue = std::strtol(size.c_str(), &endPtr, 10);
    if (*endPtr != '\0')
        throw std::runtime_error("Invalid Value: " + size);
    if (sizeValue < 0)
        throw std::out_of_range("Client Max Body Size Value cannot be negative.");
    config.client_max_body_size = static_cast<size_t>(sizeValue);
}
void ConfigParser::parseErrorPage(TokenStream &tokens, ServerConfig &config)
{
    std::string num = tokens.consume();
    std::string path = tokens.consume();
    tokens.expect(";");

    char *endPtr;

    long numValue = std::strtol(num.c_str(), &endPtr, 10);
    if (*endPtr != '\0')
        throw std::runtime_error("Invalid Value: " + num);
    if (numValue < 100 || numValue > 599)
        throw std::out_of_range("Invalid Error Page Number.");
    config.error_pages[static_cast<int>(numValue)] = path;
}

ServerConfig ConfigParser::parseServer(TokenStream &tokens)
{
    ServerConfig config;

    tokens.expect("server");
    tokens.expect("{");

    while (tokens.hasNext() && tokens.peek() != "}")
    {
        std::string token = tokens.peek();
        if (tokens.match("listen"))
        {
            // printf("debug1\n"); 
            parseListen(tokens, config);
           // printf("debug 1\n");
        }
        
        else if (tokens.match("root"))
        {
            // printf("debug2\n"); 
            parseRoot(tokens, config);
        }
        else if (tokens.match("index"))
        {
            parseIndex(tokens, config);
            // printf("debug3\n");

        }
        else if (tokens.match("client_max_body_size"))
        {
            parseClientMaxBodySize(tokens, config);
            // printf("debug4\n");
        }
        else if (tokens.match("error_page"))
        {
            parseErrorPage(tokens, config);
            // printf("debug5\n");
        }
        else if (tokens.peek() == "location")
        { 

            config.locations.push_back(parseLocation(tokens));
        }
        else
            throw std::runtime_error("Unexpected token: " + token);
    }
    tokens.expect("}");
    return config;
}
void ConfigParser::parseLocationRoot(TokenStream &tokens, LocationConfig &location)
{
    tokens.consume(); 
    location.root = tokens.peek();
    tokens.consume();
    tokens.expect(";");
    // printf("debug6\n");

}
void ConfigParser::parseMethods(TokenStream &tokens, LocationConfig &location)
{
    tokens.consume();
    while (tokens.hasNext() && tokens.peek() != ";")
    {
        std::string method = tokens.peek();
        // printf("debug7: %s\n", method.c_str());
        if (method != "GET" && method != "POST" && method != "DELETE")
            throw std::runtime_error("Invalid HTTP method: " + method);
        location.methods.push_back(method);
        tokens.consume();
    }
    tokens.expect(";");
}
void ConfigParser::parseAutoindex(TokenStream &tokens, LocationConfig &location)
{
    tokens.consume();
    std::string autoindexValue = tokens.consume();
    tokens.expect(";");
    if (autoindexValue == "on")
        location.autoindex = true;
    else if (autoindexValue == "off")
        location.autoindex = false;
    else
    {
        throw std::runtime_error("Invalid autoindex value: " + autoindexValue);
    }
}
void ConfigParser::parseUploadStore(TokenStream &tokens, LocationConfig &location)
{
    tokens.consume();
    location.upload_store = tokens.consume();
    tokens.expect(";");
}

void ConfigParser::parseRedirect(TokenStream &tokens, LocationConfig &location)
{
    tokens.consume();

    char *endPtr;
    std::string strcode = tokens.consume();
    std::string url = tokens.consume();
    long statusValue = std::strtol(strcode.c_str(), &endPtr, 10);
    if (*endPtr != '\0')
        throw std::runtime_error("Invalid redirect status: " +strcode);
    if (statusValue < 300 || statusValue > 399)
        throw std::runtime_error("Invalid redirect status code: " + strcode);
    if (url == ";")
        throw std::runtime_error("Redirect URL cannot be empty.");

    location.redirectCode = static_cast<int>(statusValue);
    location.redirectUrl = url;
    
    tokens.expect(";");
}

void ConfigParser::parseLocationIndex(TokenStream &tokens, LocationConfig &location)
{
    tokens.consume();
    location.index= tokens.consume();
    tokens.expect(";");
}

void ConfigParser::parseCgi(TokenStream &tokens, LocationConfig &location)
{
    tokens.consume();
    std::string extension = tokens.consume();
    std::string executable = tokens.consume();
    tokens.expect(";");
    if (extension.empty())
        throw std::runtime_error("CGI extension cannot be empty");
    if (executable.empty())
        throw std::runtime_error("CGI executable cannot be empty");
    if (extension[0] != '.')
        throw std::runtime_error("CGI extension must start with '.':" + extension);
    
    location.cgi[extension] = executable;
}
LocationConfig ConfigParser::parseLocation(TokenStream &tokens)
{
    LocationConfig location;

    tokens.expect("location");
    location.path = tokens.consume();
    tokens.expect("{");

    while (tokens.hasNext() && tokens.peek() != "}")
    {
        std::string token = tokens.peek();

        if (token == "methods")
        {
            parseMethods(tokens, location);
            // printf("debug5: %s\n", location.methods[0].c_str());
        }
        else if (token == "root")
        {
            parseLocationRoot(tokens, location);
            // printf("debug6: %s\n", location.root.c_str());

        }
        else if (token == "autoindex")
        {
            parseAutoindex(tokens, location);
        }
        else if (token == "index")
        {
            parseLocationIndex(tokens, location);
        }
        else if (token == "upload_store")
        {
            parseUploadStore(tokens, location);
        }
        else if (token == "return")
        {
            parseRedirect(tokens, location);
        }
        else if (token == "cgi")
        {
            parseRedirect(tokens, location);
        }
        else
            throw std::runtime_error("Unexpected token in location block: " + token);
    }
    tokens.expect("}");
    return location;
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
    
    // print tokens for debugging
    // std::cout << "Tokens: " << std::endl;
    // for (size_t i = 0; i < tokens.size(); ++i)
    // {
    //     std::cout << tokens[i] << std::endl;
    // }
		// printf("debug 1\n");

    std::vector<ServerConfig> servers;
    TokenStream tokenStream(tokens);
		// printf("debug 2\n");

    while (tokenStream.hasNext())
    {
        servers.push_back(parseServer(tokenStream));
    }   
    return servers;
}


