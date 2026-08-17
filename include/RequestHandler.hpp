#ifndef REQUESTHANDLER_HPP
# define REQUESTHANDLER_HPP

# include "HttpRequest.hpp"
# include "HttpResponse.hpp"
# include "ServerConfig.hpp"
# include "LocationConfig.hpp"
# include <string>
# include <map>
# include <vector>

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
    HttpResponse autoindexResponse(
        const std::string &path,
        const std::string &requestPath);
    HttpResponse badRequest(); // 400
    HttpResponse forbidden(); //403
    HttpResponse notFound(); // 404
    HttpResponse methodNotAllowed(); // 405
    HttpResponse internalServerError(); // 500

    std::string getMimeType(const std::string &path);
    std::string getBoundary(const HttpRequest &request);
    std::vector<MultipartPart> parseMultipart(
        const std::string &body,
        const std::string &boundary
    );
    bool writeFile(const std::string &path, const std::string &data);
    std::string generateAutoindex(const std::string &path, const std::string &requestPath);
    const LocationConfig *findLocation(
        const ServerConfig &server,
        const std::string &requestPath);
    std::string buildPath(const LocationConfig &location, const std::string &requestPath);
    bool hasParentTraversal(const std::string &path);

    HttpResponse handleGet(const HttpRequest &request, const LocationConfig &location);
    HttpResponse handlePost(const HttpRequest &request, const LocationConfig &location);
    HttpResponse handleDelete(const HttpRequest &request, const LocationConfig &location);
public:
    RequestHandler();
    ~RequestHandler();

    HttpResponse handle(
        const HttpRequest &request,
        const ServerConfig &server);
};

#endif