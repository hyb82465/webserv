#ifndef REQUESTSTATE_HPP
# define REQUESTSTATE_HPP

# include "HttpRequest.hpp"
# include <cstddef>

enum StageResult
{
    STAGE_OK,
    STAGE_INCOMPLETE,
    STAGE_ERROR
};

enum ParseResult
{
    PARSE_COMPLETE,
    PARSE_INCOMPLETE,
    PARSE_ERROR
};

enum ParseStage
{
    STAGE_REQUEST_LINE,
    STAGE_HEADERS,
    STAGE_CONTENT_BODY,
    STAGE_CHUNK_SIZE,
    STAGE_CHUNK_DATA,
    STAGE_DONE
};

struct RequestState
{
    HttpRequest request;
    ParseStage stage;
    std::size_t pos;
    std::size_t contentLength;
    std::size_t chunkSize;

    RequestState()
        : stage(STAGE_REQUEST_LINE),
          pos(0),
          contentLength(0),
          chunkSize(0)
    {
    }

    void reset()
    {
        request = HttpRequest();
        stage = STAGE_REQUEST_LINE;
        pos = 0;
        contentLength = 0;
        chunkSize = 0;
    }
};

#endif