#ifndef REQUESTHANDLER_HPP
# define REQUESTHANDLER_HPP

# include "HttpRequest.hpp"
# include "HttpResponse.hpp"
# include <string>
# include <map>
#include <vector>

struct MultipartPart
{
    std::map<std::string, std::string> headers;
    std::string name;
    std::string filename;
    std::string data;
};

class RequestHandler
{
private:
    HttpResponse notFound();
    HttpResponse forbidden();
    HttpResponse internalServerError();

    std::string getMimeType(const std::string &path);
    std::string getBoundary(const HttpRequest &request);
    std::vector<MultipartPart> parseMultipart(
        const std::string &body,
        const std::string &boundary
    );
    bool writeFile(const std::string &path, const std::string &data);

    bool hasParentTraversal(const std::string &path);

    HttpResponse handleGet(const HttpRequest &request, const std::string &root);
    HttpResponse handlePost(const HttpRequest &request, const std::string &root);
    // HttpResponse handleDelete(const HttpRequest &request);
public:
    RequestHandler();
    ~RequestHandler();

    HttpResponse handle(const HttpRequest &request);
};

#endif