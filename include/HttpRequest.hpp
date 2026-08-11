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
    std::string _query;
    std::string _version;
    std::map<std::string, std::string> _headers;
    std::string _body;
    HttpStatus _status;
public:
    HttpRequest();
    HttpRequest(const HttpRequest &other);
    HttpRequest &operator=(const HttpRequest &other);
    ~HttpRequest();

    const std::string &getMethod() const;
    const std::string &getPath() const;
    const std::string &getQuery() const;
    const std::string &getVersion() const;
    std::string getHeader(const std::string &key) const;
    const std::string &getBody() const;
    HttpStatus getStatus() const;
};

#endif