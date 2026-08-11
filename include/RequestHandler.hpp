#ifndef REQUESTHANDLER_HPP
# define REQUESTHANDLER_HPP

# include "HttpRequest.hpp"
# include "HttpResponse.hpp"

class RequestHandler
{
public:
    RequestHandler();
    ~RequestHandler();

    HttpResponse handle(const HttpRequest &request);
};

#endif