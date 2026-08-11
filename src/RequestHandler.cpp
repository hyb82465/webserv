#include "RequestHandler.hpp"
#include "HttpStatus.hpp"

RequestHandler::RequestHandler()
{}

RequestHandler::~RequestHandler()
{}

HttpResponse RequestHandler::handle(const HttpRequest &request)
{
    HttpResponse response;
    if (request.getMethod() == "GET")
    {
        if (request.getPath() == "/")
        {
            response.setStatus(HTTP_OK);
            response.setHeader("Content-Type", "text/plain");
            response.setHeader("Connection", "close");
            response.setBody("Hello World");   
        }
        else
        {
            response.setStatus(HTTP_NOT_FOUND);
            response.setHeader("Content-Type", "text/plain");
            response.setHeader("Connection", "close");
            response.setBody("404 Not Found");
        }
    }
    // else if (request.getMethod() == "POST")
    // {}
    // else if (request.getMethod() == "DELETE")
    // {}
    else
    {
        response.setStatus(HTTP_METHOD_NOT_ALLOWED);
        response.setHeader("Content-Type", "text/plain");
        response.setHeader("Connection", "close");
        response.setBody("405 Method Not Allowed");
    }
    return response;
}