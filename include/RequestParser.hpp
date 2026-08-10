#ifndef REQUESTPARSER_HPP
# define REQUESTPARSER_HPP

# include "HttpStatus.hpp"
# include <string>

class HttpRequest;

enum ParseResult
{
    PARSE_COMPLETE,
    PARSE_INCOMPLETE,
    PARSE_ERROR
};

class RequestParser
{
private:
    HttpStatus parseRequestLine(const std::string &line, HttpRequest &request);
    HttpStatus parseHeaders(const std::string &headers, HttpRequest &request);
    ParseResult parseContentLengthBody(const std::string &body, HttpRequest &request);
    ParseResult parseChunkedBody(const std::string &body, HttpRequest &request);
public:
    RequestParser();
    ~RequestParser();

    ParseResult parse(const std::string &raw, HttpRequest &request);
};

#endif