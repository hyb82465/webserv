#include "HttpRequest.hpp"
#include "RequestParser.hpp"
#include <sstream>
#include <iostream>

RequestParser::RequestParser()
{}

RequestParser::~RequestParser()
{}

HttpStatus RequestParser::parseRequestLine(const std::string &line, HttpRequest &request)
{
    std::stringstream ss(line);
    if (!(ss >> request._method))
        return HTTP_BAD_REQUEST;
    if (!(ss >> request._path))
        return HTTP_BAD_REQUEST;
    if (!(ss >> request._version))
        return HTTP_BAD_REQUEST;
    std::string extra;
    if (ss >> extra)
        return HTTP_BAD_REQUEST;
    if (request._version != "HTTP/1.1")
        return HTTP_VERSION_NOT_SUPPORTED;
    if (request._method != "GET"
        && request._method != "POST"
        && request._method != "DELETE")
        return HTTP_BAD_REQUEST;
    return HTTP_OK;
}

HttpStatus RequestParser::parseHeaders(const std::string &headers, HttpRequest &request)
{
    std::size_t start = 0;
    while (start < headers.size())
    {
        std::size_t end = headers.find("\r\n", start);
        std::string line;
        if (end == std::string::npos)
            line = headers.substr(start);
        else
            line = headers.substr(start, (end - start));
        std::size_t colon = line.find(":");
        if (colon == std::string::npos)
            return HTTP_BAD_REQUEST;
        std::string key = line.substr(0, colon);
        if (key.empty())
            return HTTP_BAD_REQUEST;
        std::string value = line.substr(colon + 1);
        while (!value.empty() && value[0] == ' ')
            value.erase(0, 1);
        request._headers[key] = value;
        if (end == std::string::npos)
            break ;
        start = end + 2;
    }
    return HTTP_OK;
}

HttpStatus RequestParser::parseBody(const std::string &body, HttpRequest &request)
{
    request._body = body;
    return HTTP_OK;
}

ParseResult RequestParser::parse(const std::string &raw, HttpRequest &request)
{
    std::size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return PARSE_INCOMPLETE;

    std::size_t lineEnd = raw.find("\r\n");
    if (lineEnd == std::string::npos)
        return PARSE_ERROR;
    std::string requestLine = raw.substr(0, lineEnd);
    request._status = parseRequestLine(requestLine, request);
    if (request._status != HTTP_OK)
        return PARSE_ERROR;

    std::size_t headerStart = lineEnd + 2;
    std::string headers = raw.substr(headerStart, (headerEnd - headerStart));
    request._status = parseHeaders(headers, request);
    if (request._status != HTTP_OK)
        return PARSE_ERROR;

    std::map<std::string, std::string>::iterator it =
        request._headers.find("Content-Length");
    if (it == request._headers.end())
    {
        request._body = "";
        request._status = HTTP_OK;
        return PARSE_COMPLETE;
    }
    std::stringstream ss(it->second);
    std::size_t len;
    if (!(ss >> len))
    {
        request._status = HTTP_BAD_REQUEST;
        return PARSE_ERROR;
    }
    std::string extra;
    if (ss >> extra)
    {
        request._status = HTTP_BAD_REQUEST;
        return PARSE_ERROR;
    }
    std::size_t bodyStart = headerEnd + 4;
    std::size_t bodySize = raw.size() - bodyStart;
    if (bodySize < len)
        return PARSE_INCOMPLETE;
    std::string body = raw.substr(bodyStart, len);
    request._status = parseBody(body, request);
    if (request._status != HTTP_OK)
        return PARSE_ERROR;

    return PARSE_COMPLETE;
}