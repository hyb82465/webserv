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
    HttpResponse response;
    std::string path = root + request.getPath();
    struct stat info;
    if (stat(path.c_str(), &info) == -1)
    {
        response.setStatus(HTTP_NOT_FOUND);
        response.setHeader("Content-Type", "text/plain");
        response.setHeader("Connection", "close");
        response.setBody("404 Not Found");
        return response;
    }
    if (S_ISDIR(info.st_mode))
    {
        if (!path.empty() && path[path.size() - 1] != '/')
            path += "/";
        path += "index.html";
    }
    else if (!S_ISREG(info.st_mode))
    {
        response.setStatus(HTTP_FORBIDDEN);
        response.setHeader("Content-Type", "text/plain");
        response.setHeader("Connection", "close");
        response.setBody("403 Forbidden");
        return response;
    }
    std::ifstream file(path.c_str());
    if (!file.is_open())
    {
        response.setStatus(HTTP_FORBIDDEN);
        response.setHeader("Content-Type", "text/plain");
        response.setHeader("Connection", "close");
        response.setBody("403 Forbidden");
        return response;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string mimeType = getMimeType(path);
    response.setStatus(HTTP_OK);
    response.setHeader("Content-Type", mimeType);
    response.setHeader("Connection", "close");
    response.setBody(buffer.str());
    return response;
}

HttpResponse RequestHandler::handle(const HttpRequest &request)
{
    if (request.getMethod() == "GET")
        return handleGet(request, "./www");
    // else if (request.getMethod() == "POST")
    // {}
    // else if (request.getMethod() == "DELETE")
    // {}
    HttpResponse response;
    response.setStatus(HTTP_METHOD_NOT_ALLOWED);
    response.setHeader("Content-Type", "text/plain");
    response.setHeader("Connection", "close");
    response.setBody("405 Method Not Allowed");
    return response;
}