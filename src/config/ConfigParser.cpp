#include "ServerConfig.hpp"
#include "ConfigParser.hpp"
#include "Tokenizer.hpp"
#include "TokenStream.hpp"
#include "LocationConfig.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cstddef>
#include <stdexcept>
#include <limits>
#include <set>
#include <map>

ConfigParser::ConfigParser()
{}

ConfigParser::~ConfigParser()
{}

std::size_t ConfigParser::parseBodySizeValue(TokenStream &tokens)
{
    std::string value = tokens.consume();
    tokens.expect(";");
    if (value.empty())
        throw std::runtime_error("client_max_body_size cannot be empty");
    std::size_t result = 0;
    for (std::size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] < '0' || value[i] > '9')
            throw std::runtime_error("Invalid client_max_body_size: " + value);
        std::size_t digit = static_cast<std::size_t>(value[i] - '0');
        if (result > (std::numeric_limits<std::size_t>::max() - digit) / 10)
            throw std::out_of_range("client_max_body_size is too large");
        result = result * 10 + digit;
    }
    return result;
}

ServerConfig ConfigParser::parseServer(TokenStream& tokens)
{
    ServerConfig config;
    std::set<std::string> seen;

    tokens.expect("server");
    tokens.expect("{");

    while (tokens.hasNext() && tokens.peek() != "}")
    {
        std::string token = tokens.peek();
        bool singleValue = 
            token == "root"
            || token == "index"
            || token == "client_max_body_size";
        if (singleValue && !seen.insert(token).second)
            throw std::runtime_error("Duplicate directive in server: " + token);
        if (tokens.match("listen"))
            parseListen(tokens, config);
        else if (tokens.match("root"))
            parseRoot(tokens, config);
        else if (tokens.match("index"))
            parseIndex(tokens, config);
        else if (tokens.match("client_max_body_size"))
            parseServerClientMaxBodySize(tokens, config);
        else if (tokens.match("error_page"))
            parseErrorPage(tokens, config);
        else if (tokens.match("location"))
            config.addLocation(parseLocation(tokens));
        else
            throw std::runtime_error("Unexpected token: " + token);
    }
    tokens.expect("}");
    // apply server config to location when location no cofig.
    config.applyDefaultsToLocations();
    return config;
}

ListenConfig ConfigParser::parseListenValue(const std::string& value)
{
    ListenConfig listen;
    size_t colon = value.find(':');
    if (colon == std::string::npos)
        throw std::runtime_error("Invalid listen address");

    std::string host = value.substr(0, colon);
    std::string portStr = value.substr(colon + 1);
    if (host.empty())
        throw std::runtime_error("Missing interface in listen");

    char* endPtr;
    long portValue = std::strtol(portStr.c_str(), &endPtr, 10);
    if (*endPtr != '\0')
        throw std::runtime_error("Invalid Value: " + portStr);
    if (portValue < 1 || portValue > 65535)
        throw std::out_of_range("Port Value out of range");

    listen.setHost(host);
    listen.setPort(static_cast<int>(portValue));
    return listen;
}

void ConfigParser::parseListen(TokenStream& tokens, ServerConfig& config)
{
    std::string port = tokens.consume();
    tokens.expect(";");
    ListenConfig listen = parseListenValue(port);
    config.addListen(listen);
}

void ConfigParser::parseRoot(TokenStream& tokens, ServerConfig& config)
{
    config.setRoot(tokens.consume());
    tokens.expect(";");
}

void ConfigParser::parseIndex(TokenStream& tokens, ServerConfig& config)
{
    config.setIndex(tokens.consume());
    tokens.expect(";");
}

void ConfigParser::parseErrorPage(TokenStream& tokens, ServerConfig& config)
{
    std::string num = tokens.consume();
    std::string path = tokens.consume();
    tokens.expect(";");

    char* endPtr;

    long numValue = std::strtol(num.c_str(), &endPtr, 10);
    if (*endPtr != '\0')
        throw std::runtime_error("Invalid Value: " + num);
    if (numValue < 100 || numValue > 599)
        throw std::out_of_range("Invalid Error Page Number.");
    int code = static_cast<int>(numValue);
    const std::map<int, std::string> &errorPages = config.getErrorPages();
    if (errorPages.find(code) != errorPages.end())
        throw std::runtime_error("Duplicate error_page status code: " + num);
    config.addErrorPage(code, path);
}

void ConfigParser::parseServerClientMaxBodySize(
    TokenStream& tokens,
    ServerConfig& config)
{
    config.setClientMaxBodySize(parseBodySizeValue(tokens));
}

LocationConfig ConfigParser::parseLocation(TokenStream& tokens)
{
    LocationConfig location;
    std::set<std::string> seen;

    location.setPath(tokens.consume());
    tokens.expect("{");

    while (tokens.hasNext() && tokens.peek() != "}")
    {
        std::string token = tokens.peek();
        bool singleValue =
            token == "methods"
            || token == "root"
            || token == "index"
            || token == "client_max_body_size"
            || token == "autoindex"
            || token == "upload_store"
            || token == "return";
        if (singleValue && !seen.insert(token).second)
            throw std::runtime_error("Duplicate directive in location: " + token);
        if (tokens.match("methods"))
            parseMethods(tokens, location);
        else if (tokens.match("root"))
            parseLocationRoot(tokens, location);
        else if (tokens.match("index"))
            parseLocationIndex(tokens, location);
        else if (tokens.match("client_max_body_size"))
            parseLocationClientMaxBodySize(tokens, location);
        else if (tokens.match("autoindex"))
            parseAutoindex(tokens, location);
        else if (tokens.match("upload_store"))
            parseUploadStore(tokens, location);
        else if (tokens.match("return"))
            parseRedirect(tokens, location);
        else if (tokens.match("cgi"))
            parseCgi(tokens, location);
        else
            throw std::runtime_error("Unexpected token in location block: " + tokens.peek());
    }
    tokens.expect("}");
    return location;
}

void ConfigParser::parseLocationRoot(TokenStream& tokens, LocationConfig& location)
{
    location.setRoot(tokens.consume());
    tokens.expect(";");
}

void ConfigParser::parseMethods(TokenStream& tokens, LocationConfig& location)
{
    if (!tokens.hasNext() || tokens.peek() == ";")
        throw std::runtime_error("methods requires at least one HTTP method");
    while (tokens.hasNext() && tokens.peek() != ";")
    {
        std::string method = tokens.consume();
        if (method != "GET" && method != "POST" && method != "DELETE")
            throw std::runtime_error("Invalid HTTP method: " + method);
        location.addMethod(method);
    }
    tokens.expect(";");
}

void ConfigParser::parseAutoindex(TokenStream& tokens, LocationConfig& location)
{
    std::string autoindexValue = tokens.consume();
    tokens.expect(";");
    if (autoindexValue == "on")
        location.setAutoindex(true);
    else if (autoindexValue == "off")
        location.setAutoindex(false);
    else
        throw std::runtime_error("Invalid autoindex value: " + autoindexValue);
}

void ConfigParser::parseUploadStore(TokenStream& tokens, LocationConfig& location)
{
    location.setUploadStore(tokens.consume());
    tokens.expect(";");
}

void ConfigParser::parseRedirect(TokenStream& tokens, LocationConfig& location)
{
    char* endPtr;
    std::string strcode = tokens.consume();
    std::string url = tokens.consume();
    long statusValue = std::strtol(strcode.c_str(), &endPtr, 10);
    if (*endPtr != '\0')
        throw std::runtime_error("Invalid redirect status: " + strcode);
    if (statusValue != 301
        && statusValue != 302
        && statusValue != 303
        && statusValue != 307
        && statusValue != 308)
        throw std::runtime_error("Unsupported redirect status code: " + strcode);
    if (url == ";")
        throw std::runtime_error("Redirect URL cannot be empty.");

    location.setRedirectCode(static_cast<int>(statusValue));
    location.setRedirectUrl(url);

    tokens.expect(";");
}

void ConfigParser::parseLocationIndex(TokenStream& tokens, LocationConfig& location)
{
    location.setIndex(tokens.consume());
    tokens.expect(";");
}

void ConfigParser::parseCgi(TokenStream& tokens, LocationConfig& location)
{
    std::string extension = tokens.consume();
    std::string executable = tokens.consume();
    tokens.expect(";");
    if (extension.empty())
        throw std::runtime_error("CGI extension cannot be empty");
    if (executable.empty())
        throw std::runtime_error("CGI executable cannot be empty");
    if (extension[0] != '.')
        throw std::runtime_error("CGI extension must start with '.':" + extension);
    const std::map<std::string, std::string> &cgi = location.getCgi();
    if (cgi.find(extension) != cgi.end())
        throw std::runtime_error("Duplicate CGI extension: " + extension);
    location.addCgi(extension, executable);
}

void ConfigParser::parseLocationClientMaxBodySize(
    TokenStream &tokens,
    LocationConfig &location)
{
    location.setClientMaxBodySize(parseBodySizeValue(tokens));
}

std::vector<ServerConfig> ConfigParser::parse(const std::string& filename)
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

    while (tokenStream.hasNext())
        servers.push_back(parseServer(tokenStream));

    return servers;
}