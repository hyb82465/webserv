#include "HttpRequest.hpp"

HttpRequest::HttpRequest() : _status(HTTP_OK)
{}

HttpRequest::HttpRequest(const HttpRequest &other)
    : _method(other._method),
      _path(other._path),
      _query(other._query),
      _version(other._version),
      _headers(other._headers),
      _body(other._body),
      _status(other._status)
{}

HttpRequest &HttpRequest::operator=(const HttpRequest &other)
{
    if (this != &other)
    {
        _method = other._method;
        _path = other._path;
        _query = other._query;
        _version = other._version;
        _headers = other._headers;
        _body = other._body;
        _status = other._status;
    }
    return *this;
}

HttpRequest::~HttpRequest()
{}

const std::string &HttpRequest::getMethod() const
{
    return _method;
}

const std::string &HttpRequest::getPath() const
{
    return _path;
}

const std::string &HttpRequest::getQuery() const
{
    return _query;
}

const std::string &HttpRequest::getVersion() const
{
    return _version;
}

std::string HttpRequest::getHeader(const std::string &key) const
{
    std::map<std::string, std::string>::const_iterator it =
        _headers.find(key);
    if (it == _headers.end())
        return "";
    return it->second;
}

const std::map<std::string, std::string> &HttpRequest::getHeaders() const
{
    return _headers;
}

const std::string &HttpRequest::getBody() const
{
    return _body;
}

HttpStatus HttpRequest::getStatus() const
{
    return _status;
}

void HttpRequest::swapBody(std::string &body)
{
    _body.swap(body);
}