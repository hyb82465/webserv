#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

# include "HttpStatus.hpp"
# include <string>
# include <map>

class RequestParser;

class HttpRequest
{
    friend class RequestParser;
private:
    std::string _method;
    std::string _path;
    std::string _version;
    std::map<std::string, std::string> _headers;
    std::string _body;

    HttpStatus _status;
public:
    HttpRequest();
    ~HttpRequest();

    HttpStatus getStatus() const;
};

#endif