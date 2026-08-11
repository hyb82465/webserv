#ifndef HTTPRESPONSE_HPP
# define HTTPRESPONSE_HPP

# include "HttpStatus.hpp"
# include <map>
# include <string>

class HttpResponse
{
private:
    std::map<std::string, std::string> _headers;
    std::string _body;
    HttpStatus _status;

    std::string statusToString() const;
public:
    HttpResponse();
    HttpResponse(const HttpResponse &other);
    HttpResponse &operator=(const HttpResponse &other);
    ~HttpResponse();

    void setStatus(HttpStatus status);
    void setHeader(const std::string &key, const std::string &value);
    void setBody(const std::string &body);
    std::string getResponse() const;
};

#endif