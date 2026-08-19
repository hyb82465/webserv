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
    const ServerConfig &_server;

    // forbidden
    RequestHandler();
    RequestHandler(const RequestHandler &other);
    RequestHandler &operator=(const RequestHandler &other);

    // response
    HttpResponse autoindexResponse(
        const std::string &path,
        const std::string &requestPath);
    HttpResponse badRequest(); // 400
    HttpResponse forbidden(); //403
    HttpResponse notFound(); // 404
    HttpResponse methodNotAllowed(); // 405
    HttpResponse payloadTooLarge(); // 413
    HttpResponse notImplemented(); // 501
    HttpResponse internalServerError(); // 500
    HttpResponse versionNotSupported(); // 505

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
    std::string buildPath(const LocationConfig *location, const std::string &requestPath);
    std::string getRoot(const LocationConfig *location) const;
    std::string getIndex(const LocationConfig *location) const;
    bool getAutoindex(const LocationConfig *location) const;
    bool isMethodAllowed(const LocationConfig *location, const std::string &requestMethod);
    bool hasParentTraversal(const std::string &path);
    bool isSafeFilename(const std::string &filename);

    HttpResponse handleGet(const HttpRequest &request, const LocationConfig *location);
    HttpResponse handlePost(const HttpRequest &request, const LocationConfig *location);
    HttpResponse handleDelete(const HttpRequest &request, const LocationConfig *location);
public:
    RequestHandler(const ServerConfig &config);
    ~RequestHandler();

    HttpResponse handle(const HttpRequest &request);
    HttpResponse handleError(HttpStatus status);
};

#endif