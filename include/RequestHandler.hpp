#ifndef REQUESTHANDLER_HPP
# define REQUESTHANDLER_HPP

# include "HttpRequest.hpp"
# include "HttpResponse.hpp"
# include <string>

class RequestHandler
{
    bool hasParentTraversal(const std::string &path);
    HttpResponse notFound();
    HttpResponse forbidden();
    std::string getMimeType(const std::string &path);

    HttpResponse handleGet(const HttpRequest &request, const std::string &root);
    HttpResponse handlePost(const HttpRequest &request, const std::string &root);
    // HttpResponse handleDelete(const HttpRequest &request);
public:
    RequestHandler();
    ~RequestHandler();

    HttpResponse handle(const HttpRequest &request);
};

#endif