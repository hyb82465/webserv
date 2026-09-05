#ifndef REQUESTPARSER_HPP
# define REQUESTPARSER_HPP

# include "HttpStatus.hpp"
# include "RequestState.hpp"
# include <string>

class HttpRequest;

class RequestParser
{
private:
    HttpStatus parseRequestLine(const std::string &line, HttpRequest &request);
    HttpStatus parseHeaders(const std::string &headers, HttpRequest &request);

    StageResult parseHeadersStage(const std::string &buffer, RequestState &state, std::size_t maxBodySize);
    StageResult parseContentBodyStage(const std::string &buffer, RequestState &state);
    StageResult parseChunkSizeStage(const std::string &buffer, RequestState &state, std::size_t maxBodySize);
    StageResult parseChunkDataStage(const std::string &buffer, RequestState &state);

    StageResult parseCurrentStage(const std::string &buffer, RequestState &state, std::size_t maxBodySize);
public:
    RequestParser();
    ~RequestParser();

    StageResult parseRequestLineStage(const std::string &buffer, RequestState &state);
    ParseResult parse(const std::string &buffer, RequestState &state, std::size_t maxBodySize);
};

#endif