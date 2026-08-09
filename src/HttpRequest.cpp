#include "HttpRequest.hpp"

HttpRequest::HttpRequest() : _status(HTTP_OK)
{}

HttpRequest::~HttpRequest()
{}

HttpStatus HttpRequest::getStatus() const
{
    return _status;
}