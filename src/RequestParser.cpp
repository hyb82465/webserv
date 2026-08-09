#include "HttpRequest.hpp"
#include "RequestParser.hpp"
#include <cctype>
#include <sstream>
#include <iomanip>
#include <iostream>

RequestParser::RequestParser()
{}

RequestParser::~RequestParser()
{}

std::string RequestParser::toLower(const std::string &str)
{
    std::string result = str;
    for (std::size_t i = 0; i < result.size(); ++i)
    {
        result[i] = static_cast<char>(
            std::tolower(
                static_cast<unsigned char>(result[i]))
        );
    }
    return result;
}

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
        key = toLower(key);
        std::string value = line.substr(colon + 1);
        while (!value.empty() && value[0] == ' ')
            value.erase(0, 1);
        if (request._headers.find(key) != request._headers.end())
        {
            if (key == "host" || key == "content-length")
                return HTTP_BAD_REQUEST;
        } 
        request._headers[key] = value;
        if (end == std::string::npos)
            break ;
        start = end + 2;
    }
    return HTTP_OK;
}

ParseResult RequestParser::parseContentLengthBody(const std::string &body, HttpRequest &request)
{
    request._body = body;
    request._status = HTTP_OK;
    return PARSE_COMPLETE;
}

ParseResult RequestParser::parseChunkedBody(const std::string &body, HttpRequest &request)
{
    std::size_t lineEnd = body.find("\r\n");
    if (lineEnd == std::string::npos)
        return PARSE_INCOMPLETE;
    std::string sizeStr = body.substr(0, lineEnd);
    if (sizeStr.empty())
    {
        request._status = HTTP_BAD_REQUEST;
        return PARSE_ERROR;
    }
    std::cout << "chunk size string: " << sizeStr << std::endl;
    std::size_t chunkSize;
    std::stringstream ss(sizeStr);
    if (!(ss >> std::hex >> chunkSize))
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
    return PARSE_INCOMPLETE;
}

ParseResult RequestParser::parse(const std::string &raw, HttpRequest &request)
{
    std::size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return PARSE_INCOMPLETE;

    // line
    std::size_t lineEnd = raw.find("\r\n");
    if (lineEnd == std::string::npos)
        return PARSE_ERROR;
    std::string requestLine = raw.substr(0, lineEnd);
    request._status = parseRequestLine(requestLine, request);
    if (request._status != HTTP_OK)
        return PARSE_ERROR;

    // headers
    std::size_t headerStart = lineEnd + 2;
    std::string headers = raw.substr(headerStart, (headerEnd - headerStart));
    request._status = parseHeaders(headers, request);
    if (request._status != HTTP_OK)
        return PARSE_ERROR;
    std::map<std::string, std::string>::iterator host =
        request._headers.find("host");
    if (host == request._headers.end() || host->second.empty())
    {
        request._status = HTTP_BAD_REQUEST;
        return PARSE_ERROR;
    }

    // body
    std::size_t bodyStart = headerEnd + 4;
    std::string body = raw.substr(bodyStart);
    std::map<std::string, std::string>::iterator contentLength =
            request._headers.find("content-length");
    std::map<std::string, std::string>::iterator transferEncoding =
            request._headers.find("transfer-encoding");
    if (contentLength != request._headers.end()
        && transferEncoding != request._headers.end())
    {
        request._status = HTTP_BAD_REQUEST;
        return PARSE_ERROR;
    }
    if (contentLength != request._headers.end())
    {
        const std::string &value = contentLength->second;
        if (value.empty())
        {
            request._status = HTTP_BAD_REQUEST;
            return PARSE_ERROR;
        }
        for (std::size_t i = 0; i < value.size(); ++i)
        {
            if (!std::isdigit(static_cast<unsigned char>(value[i])))
            {
                request._status = HTTP_BAD_REQUEST;
                return PARSE_ERROR;
            }
        }
        std::stringstream ss(value);
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
        std::size_t bodySize = raw.size() - bodyStart;
        if (bodySize < len)
            return PARSE_INCOMPLETE;
        return parseContentLengthBody(body, request);
    }
    else if (transferEncoding != request._headers.end())
    {
        if (transferEncoding->second != "chunked")
        {
            request._status = HTTP_BAD_REQUEST;
            return PARSE_ERROR;
        }
        return parseChunkedBody(body, request);
    }
    else
    {
        request._body = "";
        request._status = HTTP_OK;
        return PARSE_COMPLETE;
    }
}