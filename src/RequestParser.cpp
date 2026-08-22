#include "HttpRequest.hpp"
#include "RequestParser.hpp"
#include "Utils.hpp"
#include <cctype>
#include <sstream>
#include <iomanip>
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
    std::size_t queryPos = request._path.find('?');
    if (queryPos != std::string::npos)
    {
        request._query = request._path.substr(queryPos + 1);
        request._path = request._path.substr(0, queryPos);
    }
    else
        request._query = "";
    if (request._path.empty() || request._path[0] != '/')
        return HTTP_BAD_REQUEST;
    if (!Utils::decodeUri(request._path))
        return HTTP_BAD_REQUEST;
    if (request._version != "HTTP/1.1")
        return HTTP_VERSION_NOT_SUPPORTED;
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
        std::size_t colon = line.find(':');
        if (colon == std::string::npos)
            return HTTP_BAD_REQUEST;
        std::string key = line.substr(0, colon);
        if (key.empty())
            return HTTP_BAD_REQUEST;
        for (std::size_t i = 0; i < key.size(); i++)
        {
            if (key[i] == ' ' || key[i] == '\t')
                return HTTP_BAD_REQUEST;
        }
        key = Utils::toLower(key);
        std::string value = Utils::trim(line.substr(colon + 1));
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

ParseResult RequestParser::parseChunkedBody(const std::string &body, HttpRequest &request, std::size_t maxBodySize)
{
    request._body.clear();
    std::size_t pos = 0;
    while (true)
    {
        std::size_t lineEnd = body.find("\r\n", pos);
        if (lineEnd == std::string::npos)
            return PARSE_INCOMPLETE;
        std::string sizeStr = body.substr(pos, lineEnd - pos);
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
        if (chunkSize == 0)
        {
            if (body.size() < lineEnd + 4)
                return PARSE_INCOMPLETE;
            if (body[lineEnd + 2] != '\r' || body[lineEnd + 3] != '\n')
            {
                request._status = HTTP_BAD_REQUEST;
                return PARSE_ERROR;
            }
            request._status = HTTP_OK;
            return PARSE_COMPLETE;
        }
        if (maxBodySize != 0 
            && (request._body.size() > maxBodySize
            || chunkSize > maxBodySize - request._body.size()))
        {
            request._status = HTTP_PAYLOAD_TOO_LARGE;
            return PARSE_ERROR;
        }
        std::size_t dataStart = lineEnd + 2;
        if (chunkSize > body.size() - dataStart)
            return PARSE_INCOMPLETE;
        std::size_t dataEnd = dataStart + chunkSize;
        if (body.size() < dataEnd + 2)
            return PARSE_INCOMPLETE;
        if (body[dataEnd] != '\r' || body[dataEnd + 1] != '\n')
        {
            request._status = HTTP_BAD_REQUEST;
            return PARSE_ERROR;
        }
        std::string chunkData = body.substr(dataStart, chunkSize);
        request._body += chunkData;
        pos = dataEnd + 2;
    }
}

ParseResult RequestParser::parse(const std::string &raw, HttpRequest &request, std::size_t maxBodySize)
{
    std::size_t headerEnd = raw.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return PARSE_INCOMPLETE;

    // line
    std::size_t lineEnd = raw.find("\r\n");
    // would not happen
    if (lineEnd == std::string::npos)
    {
        request._status = HTTP_BAD_REQUEST;
        return PARSE_ERROR;
    }
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
        if (maxBodySize != 0 && len > maxBodySize)
        {
            request._status = HTTP_PAYLOAD_TOO_LARGE;
            return PARSE_ERROR;
        }
        std::size_t bodySize = raw.size() - bodyStart;
        if (bodySize < len)
            return PARSE_INCOMPLETE;
        return parseContentLengthBody(body.substr(0, len), request);
    }
    else if (transferEncoding != request._headers.end())
    {
        // only accept "chunked"
        if (Utils::toLower(transferEncoding->second) != "chunked")
        {
            request._status = HTTP_BAD_REQUEST;
            return PARSE_ERROR;
        }
        return parseChunkedBody(body, request, maxBodySize);
    }
    else
    {
        request._body = "";
        request._status = HTTP_OK;
        return PARSE_COMPLETE;
    }
}