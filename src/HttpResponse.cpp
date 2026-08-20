#include "HttpResponse.hpp"
#include <sstream>

std::string HttpResponse::statusToString() const
{
    switch (_status)
    {
        case HTTP_OK:
            return "200 OK";
        case HTTP_CREATED:
            return "201 Created";
        case HTTP_MOVED_PERMANENTLY:
            return "301 Moved Permanently";
        case HTTP_FOUND:
            return "302 Found";
        case HTTP_SEE_OTHER:
            return "303 See Other";
        case HTTP_TEMPORARY_REDIRECT:
            return "307 Temporary Redirect";
        case HTTP_PERMANENT_REDIRECT:
            return "308 Permanent Redirect";
        case HTTP_BAD_REQUEST:
            return "400 Bad Request";
        case HTTP_FORBIDDEN:
            return "403 Forbidden";
        case HTTP_NOT_FOUND:
            return "404 Not Found";
        case HTTP_METHOD_NOT_ALLOWED:
            return "405 Method Not Allowed";
        case HTTP_PAYLOAD_TOO_LARGE:
            return "413 Payload Too Large";
        case HTTP_INTERNAL_SERVER_ERROR:
            return "500 Internal Server Error";
        case HTTP_NOT_IMPLEMENTED:
            return "501 Not Implemented";
        case HTTP_VERSION_NOT_SUPPORTED:
            return "505 Version Not Supported";
        default:
            return "500 Internal Server Error";
    }
}

HttpResponse::HttpResponse() : _status(HTTP_OK)
{}

HttpResponse::HttpResponse(const HttpResponse &other)
    : _headers(other._headers),
      _body(other._body),
      _status(other._status)
{}

HttpResponse &HttpResponse::operator=(const HttpResponse &other)
{
    if (this != &other)
    {
        _headers = other._headers;
        _body = other._body;
        _status = other._status;
    }
    return *this;
}

HttpResponse::~HttpResponse()
{}

void HttpResponse::setStatus(HttpStatus status)
{
    _status = status;
}

void HttpResponse::setHeader(const std::string &key, const std::string &value)
{
    _headers[key] = value;
}

void HttpResponse::setBody(const std::string &body)
{
    _body = body;
}

std::string HttpResponse::getResponse() const
{
    std::string response;
    response += "HTTP/1.1 ";
    response += statusToString();
    response += "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
         it != _headers.end();
         ++it)
    {
        response += it->first;
        response += ": ";
        response += it->second;
        response += "\r\n";
    }
    std::stringstream ss;
    ss << _body.size();
    response += "Content-Length: ";
    response += ss.str();
    response += "\r\n";
    response += "\r\n";
    response += _body;
    return response;
}