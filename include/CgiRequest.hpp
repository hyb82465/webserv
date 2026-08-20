#ifndef CGIREQUEST_HPP
#define CGIREQUEST_HPP

#include <string>

struct CgiRequest
{
    std::string method;
    std::string queryString;
    std::string contentType;
    std::string body;

    std::string serverProtocol;
    std::string serverName;
    std::string serverPort;

    std::string scriptName;
    std::string scriptFilename;
    std::string pathInfo;

    std::string remoteAddr;
};

#endif