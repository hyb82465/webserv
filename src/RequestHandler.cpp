#include "RequestHandler.hpp"
#include "HttpStatus.hpp"
#include "Utils.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <cstddef>

RequestHandler::RequestHandler()
{}

RequestHandler::~RequestHandler()
{}

// Temporary protection
bool RequestHandler::hasParentTraversal(const std::string &path)
{
    std::size_t start = 0;
    while (start < path.size())
    {
        std::size_t end = path.find('/', start);
        std::string part;
        if (end == std::string::npos)
            part = path.substr(start);
        else
            part = path.substr(start, end - start);
        if (part == "..")
            return true;
        if (end == std::string::npos)
            break ;
        start = end + 1;
    }
    return false;
}

HttpResponse RequestHandler::notFound()
{
    HttpResponse response;
    response.setStatus(HTTP_NOT_FOUND);
    response.setHeader("Content-Type", "text/plain");
    response.setHeader("Connection", "close");
    response.setBody("404 Not Found");
    return response;  
}

HttpResponse RequestHandler::forbidden()
 {
    HttpResponse response;
    response.setStatus(HTTP_FORBIDDEN);
    response.setHeader("Content-Type", "text/plain");
    response.setHeader("Connection", "close");
    response.setBody("403 Forbidden");
    return response;
}

std::string RequestHandler::getMimeType(const std::string &path)
{
    std::size_t lastDot = path.rfind('.');
    if (lastDot == std::string::npos)
        return "application/octet-stream";
    std::string ext = Utils::toLower(path.substr(lastDot + 1));
    if (ext == "html")
        return "text/html";
    if (ext == "txt")
        return "text/plain";
    if (ext == "css")
        return "text/css";
    if (ext == "js")
        return "application/javascript";
    if (ext == "png")
        return "image/png";
    if (ext == "jpg" || ext == "jpeg")
        return "image/jpeg";
    if (ext == "gif")
        return "image/gif";
    if (ext == "json")
        return "application/json";
    if (ext == "pdf")
        return "application/pdf";
    return "application/octet-stream";
}

HttpResponse RequestHandler::handleGet(const HttpRequest &request, const std::string &root)
{
    // temporary protection
    if (hasParentTraversal(request.getPath()))
        return forbidden();
    std::string path = root + request.getPath();
    struct stat info;
    if (stat(path.c_str(), &info) == -1)
        return notFound();
    if (S_ISDIR(info.st_mode))
    {
        if (!path.empty() && path[path.size() - 1] != '/')
            path += "/";
        path += "index.html";
        struct stat indexInfo;
        if (stat(path.c_str(), &indexInfo) == -1)
            return forbidden();
        if (!S_ISREG(indexInfo.st_mode))
            return forbidden();
    }
    else if (!S_ISREG(info.st_mode))
        return forbidden();
    std::ifstream file(path.c_str());
    if (!file.is_open())
        return forbidden();

    HttpResponse response;
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string mimeType = getMimeType(path);
    response.setStatus(HTTP_OK);
    response.setHeader("Content-Type", mimeType);
    response.setHeader("Connection", "close");
    response.setBody(buffer.str());
    return response;
}

HttpResponse RequestHandler::handlePost(const HttpRequest &request, const std::string &root)
{
    HttpResponse response;
    std::string path = root + request.getPath();
    struct stat info;
    bool existed = (stat(path.c_str(), &info) == 0);
    if (existed && !S_ISREG(info.st_mode))
        return forbidden();
    std::ofstream file(
        path.c_str(),
        std::ios::out | std::ios::binary
    );
    if (!file.is_open())
        return forbidden();
    file.write(
        request.getBody().data(),
        request.getBody().size()
    );
    if (!file)
    {
        response.setStatus(HTTP_INTERNAL_SERVER_ERROR);
        response.setHeader("Content-Type", "text/plain");
        response.setHeader("Connection", "close");
        response.setBody("500 Internal Server Error");
        return response;
    }
    if (existed)
        response.setStatus(HTTP_OK);
    else
        response.setStatus(HTTP_CREATED);
    response.setHeader("Content-Type", "text/plain");
    response.setHeader("Connection", "close");
    response.setBody("Upload successful");
    return response;
}

HttpResponse RequestHandler::handle(const HttpRequest &request)
{
    if (request.getMethod() == "GET")
        return handleGet(request, "./www");
    else if (request.getMethod() == "POST")
        return handlePost(request, "./www");
    // else if (request.getMethod() == "DELETE")
    // {}
    HttpResponse response;
    response.setStatus(HTTP_METHOD_NOT_ALLOWED);
    response.setHeader("Content-Type", "text/plain");
    response.setHeader("Connection", "close");
    response.setBody("405 Method Not Allowed");
    return response;
}