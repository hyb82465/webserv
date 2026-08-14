#include "RequestHandler.hpp"
#include "HttpStatus.hpp"
#include "Utils.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <cstddef>
#include <iostream>

RequestHandler::RequestHandler()
{}

RequestHandler::~RequestHandler()
{}

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

std::string RequestHandler::getBoundary(const HttpRequest &request)
{
    std::string contentType = request.getHeader("content-type");
    std::string key = "boundary=";
    std::size_t pos = contentType.find(key);
    if (pos == std::string::npos)
    {
        // error
    }
    std::string boundary = contentType.substr(pos + key.size());
    return boundary;
}

std::vector<MultipartPart> RequestHandler::parseMultipart(const std::string &body,const std::string &boundary)
{
    std::vector<MultipartPart> parts;
    std::string delimiter = "--" + boundary;
    std::size_t pos = body.find(delimiter);
    while (pos != std::string::npos)
    {
        pos += delimiter.size();
        if (body.compare(pos, 2, "--") == 0)
            break ;
        if (body.compare(pos, 2, "\r\n") != 0)
            return parts;
        pos += 2;
        std::size_t next = body.find(delimiter, pos);
        if (next == std::string::npos)
            return parts;

        std::size_t partEnd = next;
        if (partEnd < 2)
            return parts;
        if (body[partEnd - 2] != '\r'
            || body[partEnd - 1] != '\n')
            return parts;
        partEnd -= 2;
        std::string part = body.substr(pos, partEnd - pos);
        std::size_t headerEnd = part.find("\r\n\r\n");
        if (headerEnd == std::string::npos)
            return parts;
        std::string headerBlock = part.substr(0, headerEnd);
        std::string data = part.substr(headerEnd + 4);
        MultipartPart current;
        current.data = data;
        std::size_t startHeader = 0;
        while (startHeader < headerBlock.size())
        {
            std::size_t endHeader = headerBlock.find("\r\n", startHeader);
            std::string line;
            if (endHeader == std::string::npos)
                line = headerBlock.substr(startHeader);
            else
                line = headerBlock.substr(startHeader, endHeader - startHeader);
            std::size_t colon = line.find(':');
            if (colon == std::string::npos)
                return parts;
            std::string key = line.substr(0, colon);
            if (key.empty())
                return parts;
            for (std::size_t i = 0; i < key.size(); i++)
            {
                if (key[i] == ' ' || key[i] == '\t')
                    return parts;
            }
            key = Utils::toLower(key);
            std::string value = Utils::trim(line.substr(colon + 1));
            current.headers[key] = value;
            if (endHeader == std::string::npos)
                break ;
            startHeader = endHeader + 2;
        }
        std::map<std::string, std::string>::const_iterator it =
            current.headers.find("content-disposition");
        if (it == current.headers.end())
            return parts;

        std::string disposition = it->second;
        std::string nameKey = "name=\"";
        std::string filenameKey = "filename=\"";

        std::size_t namePos = disposition.find(nameKey);
        if (namePos != std::string::npos)
        {
            std::size_t nameStart = namePos + nameKey.size();
            std::size_t nameEnd = disposition.find('"', nameStart);
            if (nameEnd == std::string::npos)
                return parts;
            current.name = disposition.substr(nameStart, nameEnd - nameStart);
        }
        
        std::size_t filenamePos = disposition.find(filenameKey);
        if (filenamePos != std::string::npos)
        {
            std::size_t filenameStart = filenamePos + filenameKey.size();
            std::size_t filenameEnd = disposition.find('"', filenameStart);
            if (filenameEnd == std::string::npos)
                return parts;
            current.filename = disposition.substr(filenameStart, filenameEnd - filenameStart);
        }
        parts.push_back(current);
        pos = next;
    }
    return parts;
}

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
    std::string contentType = request.getHeader("content-type");
    if (contentType.find("multipart/form-data") != std::string::npos)
    {
        std::string boundary = getBoundary(request);
        std::vector<MultipartPart> parts = parseMultipart(request.getBody(), boundary);
        std::cout << "parts size: "
                  << parts.size()
                  << std::endl;
        for (std::size_t i = 0; i < parts.size(); ++i)
        {
            std::cout << "name: "
                      << parts[i].name
                      << std::endl;
            std::cout << "filename: "
                      << parts[i].filename
                      << std::endl;
            std::cout << "data size: "
                      << parts[i].data.size()
                      << std::endl;
        }
    }
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