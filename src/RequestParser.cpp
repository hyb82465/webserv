#include "HttpRequest.hpp"
#include "RequestParser.hpp"
#include "Utils.hpp"
#include <limits>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <iostream>

RequestParser::RequestParser()
{}

RequestParser::~RequestParser()
{}

HttpStatus RequestParser::parseRequestLine(
    const std::string &line,
    HttpRequest &request)
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

HttpStatus RequestParser::parseHeaders(
    const std::string &headers,
    HttpRequest &request)
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
        const std::string allowedSymbols = "!#$%&'*+-.^_`|~";
        for (std::size_t i = 0; i < key.size(); ++i)
        {
            unsigned char c = static_cast<unsigned char>(key[i]);
            bool isLetterOrDigit =
                (c >= 'a' && c <= 'z')
                || (c >= 'A' && c <= 'Z')
                || (c >= '0' && c <= '9');
            bool isAllowedSymbol = 
                allowedSymbols.find(static_cast<char>(c)) != std::string::npos;
            if (!isLetterOrDigit && !isAllowedSymbol)
                return HTTP_BAD_REQUEST;
        }
        key = Utils::toLower(key);
        std::string value = line.substr(colon + 1);
        for (std::size_t i = 0; i < value.size(); ++i)
        {
            unsigned char c = static_cast<unsigned char>(value[i]);
            if ((c < 32 && c != '\t') || c == 127)
                return HTTP_BAD_REQUEST;
        }
        value = Utils::trim(value);
        if (request._headers.find(key) != request._headers.end())
        {
            if (key == "host"
                || key == "content-length"
                || key == "transfer-encoding")
            {
                return HTTP_BAD_REQUEST; 
            }
        }
        request._headers[key] = value;
        if (end == std::string::npos)
            break ;
        start = end + 2;
    }
    return HTTP_OK;
}

StageResult RequestParser::parseRequestLineStage(
    const std::string &buffer,
    RequestState &state)
{
    const std::size_t MAX_REQUEST_LINE_SIZE = 8192; // 8kb
    if (state.pos > buffer.size())
    {
        state.request._status = HTTP_BAD_REQUEST;
        return STAGE_ERROR;
    }
    std::size_t lineEnd = buffer.find("\r\n", state.pos);
    if (lineEnd == std::string::npos)
    {
        if (buffer.size() - state.pos > MAX_REQUEST_LINE_SIZE)
        {
            state.request._status = HTTP_URI_TOO_LONG;
            return STAGE_ERROR;
        }
        return STAGE_INCOMPLETE;
    }
    std::size_t requestLineSize = lineEnd - state.pos + 2;
    if (requestLineSize > MAX_REQUEST_LINE_SIZE)
    {
        state.request._status = HTTP_URI_TOO_LONG;
        return STAGE_ERROR;
    }
    std::string line = buffer.substr(state.pos, lineEnd - state.pos);
    state.request._status = parseRequestLine(line, state.request);
    if (state.request._status != HTTP_OK)
        return STAGE_ERROR;
    state.pos = lineEnd + 2;
    state.stage = STAGE_HEADERS;
    return STAGE_OK;
}

StageResult RequestParser::parseHeadersStage(
    const std::string &buffer,
    RequestState &state,
    std::size_t maxBodySize)
{
    const std::size_t MAX_HEADERS_SIZE = 32768; // 32kb
    if (state.pos > buffer.size())
    {
        state.request._status = HTTP_BAD_REQUEST;
        return STAGE_ERROR;
    }
    if (buffer.size() - state.pos >= 2 && buffer.compare(state.pos, 2, "\r\n") == 0)
    {
        state.request._status = HTTP_BAD_REQUEST;
        return STAGE_ERROR;
    }
    std::size_t headerStart = state.pos;
    std::size_t headerEnd = buffer.find("\r\n\r\n", state.pos);
    if (headerEnd == std::string::npos)
    {
        if (buffer.size() - headerStart > MAX_HEADERS_SIZE)
        {
            state.request._status = HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE;
            return STAGE_ERROR;
        }
        return STAGE_INCOMPLETE;
    }
    std::size_t headersSize = headerEnd - headerStart + 4;
    if (headersSize > MAX_HEADERS_SIZE)
    {
        state.request._status = HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE;
        return STAGE_ERROR;
    }
    std::string headers = buffer.substr(headerStart, (headerEnd - headerStart));
    state.request._status = parseHeaders(headers, state.request);
    if (state.request._status != HTTP_OK)
        return STAGE_ERROR;
    std::map<std::string, std::string>::const_iterator connection =
        state.request._headers.find("connection");
    if (state.request._version == "HTTP/1.1")
    {
        // HTTP/1.1 default connection: keep-alive
        state.keepAlive = true;
        if (connection != state.request._headers.end()
            && Utils::toLower(connection->second) == "close")
            state.keepAlive = false;
    }
    std::map<std::string, std::string>::iterator host =
        state.request._headers.find("host");
    if (host == state.request._headers.end() || host->second.empty())
    {
        state.request._status = HTTP_BAD_REQUEST;
        return STAGE_ERROR;
    }
    state.pos = headerEnd + 4;

    std::map<std::string, std::string>::iterator contentLength =
            state.request._headers.find("content-length");
    std::map<std::string, std::string>::iterator transferEncoding =
            state.request._headers.find("transfer-encoding");
    if (contentLength != state.request._headers.end()
        && transferEncoding != state.request._headers.end())
    {
        state.request._status = HTTP_BAD_REQUEST;
        return STAGE_ERROR;
    }
    if (contentLength != state.request._headers.end())
    {
        const std::string &value = contentLength->second;
        if (value.empty())
        {
            state.request._status = HTTP_BAD_REQUEST;
            return STAGE_ERROR;
        }
        for (std::size_t i = 0; i < value.size(); ++i)
        {
            if (!std::isdigit(static_cast<unsigned char>(value[i])))
            {
                state.request._status = HTTP_BAD_REQUEST;
                return STAGE_ERROR;
            }
        }
        std::stringstream ss(value);
        if (!(ss >> state.contentLength))
        {
            state.request._status = HTTP_BAD_REQUEST;
            return STAGE_ERROR;
        }
        std::string extra;
        if (ss >> extra)
        {
            state.request._status = HTTP_BAD_REQUEST;
            return STAGE_ERROR;
        }
        if (maxBodySize != 0 && state.contentLength > maxBodySize)
        {
            state.request._status = HTTP_PAYLOAD_TOO_LARGE;
            return STAGE_ERROR;
        }
        state.stage = STAGE_CONTENT_BODY;
        return STAGE_OK;
    }
    else if (transferEncoding != state.request._headers.end())
    {
        // only accept "chunked"
        if (Utils::toLower(transferEncoding->second) != "chunked")
        {
            state.request._status = HTTP_BAD_REQUEST;
            return STAGE_ERROR;
        }
        state.stage = STAGE_CHUNK_SIZE;
        return STAGE_OK;
    }
    else
    {
        state.request._body = "";
        state.request._status = HTTP_OK;
        state.stage = STAGE_DONE;
        return STAGE_OK;
    }
    return STAGE_OK;
}

StageResult RequestParser::parseContentBodyStage(
    const std::string &buffer,
    RequestState &state)
{
    if (state.pos > buffer.size())
    {
        state.request._status = HTTP_BAD_REQUEST;
        return STAGE_ERROR;
    }
    if (buffer.size() - state.pos < state.contentLength)
        return STAGE_INCOMPLETE;
    state.request._body.assign(buffer, state.pos, state.contentLength);
    state.pos += state.contentLength;

    state.request._status = HTTP_OK;
    state.stage = STAGE_DONE;
    return STAGE_OK;
}

StageResult RequestParser::parseChunkSizeStage(
    const std::string &buffer,
    RequestState &state,
    std::size_t maxBodySize)
{
    const std::size_t MAX_CHUNK_SIZE_LINE = 1024;
    if (state.pos > buffer.size())
    {
        state.request._status = HTTP_BAD_REQUEST;
        return STAGE_ERROR;
    }
    std::size_t lineEnd = buffer.find("\r\n",state.pos);
    if (lineEnd == std::string::npos)
    {
        if (buffer.size() - state.pos > MAX_CHUNK_SIZE_LINE)
        {
            state.request._status = HTTP_BAD_REQUEST;
            return STAGE_ERROR;
        }
        return STAGE_INCOMPLETE;
    }   
    std::size_t lineSize = lineEnd - state.pos + 2;
    if (lineSize > MAX_CHUNK_SIZE_LINE)
    {
        state.request._status = HTTP_BAD_REQUEST;
        return STAGE_ERROR;
    }
    std::string sizeStr = buffer.substr(state.pos, lineEnd - state.pos);
    if (sizeStr.empty())
    {
        state.request._status = HTTP_BAD_REQUEST;
        return STAGE_ERROR;
    }
    std::size_t chunkSize = 0;
    const std::size_t maxSize = std::numeric_limits<std::size_t>::max();
    for (std::size_t i = 0; i < sizeStr.size(); ++i)
    {
        char c = sizeStr[i];
        std::size_t digit;
        if (c >= '0' && c <= '9')
            digit = static_cast<std::size_t>(c - '0');
        else if (c >= 'a' && c <= 'f')
            digit = static_cast<std::size_t>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F')
            digit = static_cast<std::size_t>(c - 'A' + 10);
        else
        {
            state.request._status = HTTP_BAD_REQUEST;
            return STAGE_ERROR;
        }
        if (chunkSize > (maxSize - digit) / 16)
        {
            state.request._status = HTTP_PAYLOAD_TOO_LARGE;
            return STAGE_ERROR;
        }
        chunkSize = chunkSize * 16 + digit;
    }

    std::size_t nextPos = lineEnd + 2;
    if (chunkSize == 0)
    {
        if (nextPos > buffer.size() || buffer.size() - nextPos < 2)
            return STAGE_INCOMPLETE;
        if (buffer[nextPos] != '\r' || buffer[nextPos + 1] != '\n')
        {
            state.request._status = HTTP_BAD_REQUEST;
            return STAGE_ERROR;
        }
        state.pos = nextPos + 2;
        state.request._status = HTTP_OK;
        state.stage = STAGE_DONE;
        return STAGE_OK;
    }
    if (maxBodySize != 0 
        && (state.request._body.size() > maxBodySize
        || chunkSize > maxBodySize - state.request._body.size()))
    {
        state.request._status = HTTP_PAYLOAD_TOO_LARGE;
        return STAGE_ERROR;
    }
    state.chunkSize = chunkSize;
    state.pos = nextPos;
    state.stage = STAGE_CHUNK_DATA;
    return STAGE_OK;
}

StageResult RequestParser::parseChunkDataStage(
    const std::string &buffer, 
    RequestState &state)
{
    if (state.pos > buffer.size())
    {
        state.request._status = HTTP_BAD_REQUEST;
        return STAGE_ERROR;
    }
    std::size_t available = buffer.size() - state.pos;
    if (state.chunkSize > available)
        return STAGE_INCOMPLETE;
    if (available - state.chunkSize < 2)
        return STAGE_INCOMPLETE;
    std::size_t dataEnd = state.pos + state.chunkSize;
    if (buffer[dataEnd] != '\r' || buffer[dataEnd + 1] != '\n')
    {
        state.request._status = HTTP_BAD_REQUEST;
        return STAGE_ERROR;
    }
    state.request._body.append(buffer, state.pos, state.chunkSize);
    state.pos = dataEnd + 2;
    state.chunkSize = 0;
    state.stage = STAGE_CHUNK_SIZE;
    return STAGE_OK;
}

StageResult RequestParser::parseCurrentStage(
    const std::string &buffer,
    RequestState &state,
    std::size_t maxBodySize)
{
    switch (state.stage)
    {
        case STAGE_REQUEST_LINE:
            return parseRequestLineStage(buffer, state);

        case STAGE_HEADERS:
            return parseHeadersStage(buffer, state, maxBodySize);

        case STAGE_CONTENT_BODY:
            return parseContentBodyStage(buffer, state);

        case STAGE_CHUNK_SIZE:
            return parseChunkSizeStage(buffer, state, maxBodySize);

        case STAGE_CHUNK_DATA:
            return parseChunkDataStage(buffer, state);

        default:
            state.request._status = HTTP_BAD_REQUEST;
            return STAGE_ERROR;
    }
}

ParseResult RequestParser::parse(
    const std::string &buffer,
    RequestState &state,
    std::size_t maxBodySize)
{
    while (state.stage != STAGE_DONE)
    {
        StageResult result = parseCurrentStage(buffer, state, maxBodySize);
        if (result == STAGE_INCOMPLETE)
            return PARSE_INCOMPLETE;
        if (result == STAGE_ERROR)
            return PARSE_ERROR;
        // STAGE_OK continue
    }
    return PARSE_COMPLETE;
}