#include "RequestHandler.hpp"
#include "HttpStatus.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>

RequestHandler::RequestHandler()
{}

RequestHandler::~RequestHandler()
{}

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
    response.setStatus(HTTP_OK);
    response.setHeader("Content-Type", "text/plain");
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