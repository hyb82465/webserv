#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include <string>
#include <map>
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
    const ServerConfig &_server;

    // forbidden
    RequestHandler();
    RequestHandler(const RequestHandler &other);
    RequestHandler &operator=(const RequestHandler &other);

    // response
    HttpResponse badRequest();                                     // 400
    HttpResponse forbidden();                                      // 403
    HttpResponse notFound();                                       // 404
    HttpResponse methodNotAllowed(const LocationConfig *location); // 405
    HttpResponse payloadTooLarge();                                // 413
    HttpResponse uriTooLong();                                     // 414
    HttpResponse requestHeadersTooLarge();                         // 431
    HttpResponse internalServerError();                            // 500
    HttpResponse notImplemented();                                 // 501
    HttpResponse versionNotSupported();                            // 505
    HttpResponse errorResponse(HttpStatus status, const std::string &defaultBody);
    HttpResponse autoindexResponse(
        const std::string &path,
        const std::string &requestPath);
    HttpResponse redirect(HttpStatus status, const std::string &url);

    std::string getMimeType(const std::string &path);
    std::string getBoundary(const HttpRequest &request);
    bool parseMultipart(
        const std::string &body,
        const std::string &boundary,
        std::vector<MultipartPart> &parts);
    bool writeFile(const std::string &path, const std::string &data);
    std::string generateAutoindex(const std::string &path, const std::string &requestPath);
    const LocationConfig *findLocation(const ServerConfig &server, const std::string &requestPath) const;
    std::string getRoot(const LocationConfig *location) const;
    std::string getIndex(const LocationConfig *location) const;
    bool getAutoindex(const LocationConfig *location) const;
    bool isMethodAllowed(const LocationConfig *location, const std::string &requestMethod);
    bool hasParentTraversal(const std::string &path);
    bool isSafeFilename(const std::string &filename);
    std::string htmlEscape(const std::string &value);
    std::string urlEncodePath(const std::string &value);

    HttpResponse handleGet(const HttpRequest &request, const LocationConfig *location);
    HttpResponse handlePost(const HttpRequest &request, const LocationConfig *location);
    HttpResponse handleDelete(const HttpRequest &request, const LocationConfig *location);

public:
    RequestHandler(const ServerConfig &config);
    ~RequestHandler();

    const LocationConfig *getLocation(const HttpRequest &request) const;
    std::string buildPath(const LocationConfig *location, const std::string &requestPath);
    bool preCheck(
        const HttpRequest &request,
        const LocationConfig *location,
        HttpResponse &response);

    HttpResponse handleResolved(const HttpRequest &request);
    HttpResponse handleError(HttpStatus status);
};

#endif