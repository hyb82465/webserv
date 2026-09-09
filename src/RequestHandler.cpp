#include "RequestHandler.hpp"
#include "HttpStatus.hpp"
#include "Utils.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <cstddef>
#include <cstdio>
#include <dirent.h>
#include <iostream>

RequestHandler::RequestHandler(const ServerConfig &config) : _server(config)
{
}

RequestHandler::~RequestHandler()
{
}

HttpResponse RequestHandler::badRequest()
{
    return errorResponse(
        HTTP_BAD_REQUEST,
        "400 Bad Request");
}

HttpResponse RequestHandler::forbidden()
{
    return errorResponse(
        HTTP_FORBIDDEN,
        "403 Forbidden");
}

HttpResponse RequestHandler::notFound()
{
    return errorResponse(
        HTTP_NOT_FOUND,
        "404 Not Found");
}

HttpResponse RequestHandler::methodNotAllowed(const LocationConfig *location)
{
    HttpResponse response = errorResponse(
        HTTP_METHOD_NOT_ALLOWED,
        "405 Method Not Allowed");
    if (location != NULL)
    {
        const std::vector<std::string> &methods = location->getMethods();
        std::string allow;
        for (std::size_t i = 0; i < methods.size(); ++i)
        {
            if (!allow.empty())
                allow += ", ";
            allow += methods[i];
        }
        if (!allow.empty())
            response.setHeader("Allow", allow);
    }
    return response;
}

HttpResponse RequestHandler::payloadTooLarge()
{
    return errorResponse(
        HTTP_PAYLOAD_TOO_LARGE,
        "413 Payload Too Large");
}

HttpResponse RequestHandler::uriTooLong()
{
    return errorResponse(
        HTTP_URI_TOO_LONG,
        "414 URI Too Long");
}

HttpResponse RequestHandler::requestHeadersTooLarge()
{
    return errorResponse(
        HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE,
        "431 Request Header Fields Too Large");
}

HttpResponse RequestHandler::internalServerError()
{
    return errorResponse(
        HTTP_INTERNAL_SERVER_ERROR,
        "500 Internal Server Error");
}

HttpResponse RequestHandler::notImplemented()
{
    return errorResponse(
        HTTP_NOT_IMPLEMENTED,
        "501 Not Implemented");
}

HttpResponse RequestHandler::versionNotSupported()
{
    return errorResponse(
        HTTP_VERSION_NOT_SUPPORTED,
        "505 Version Not Supported");
}

HttpResponse RequestHandler::errorResponse(
    HttpStatus status,
    const std::string &defaultBody)
{
    HttpResponse response;
    const std::map<int, std::string> &errorPages =
        _server.getErrorPages();
    std::map<int, std::string>::const_iterator it =
        errorPages.find(static_cast<int>(status));
    if (it != errorPages.end())
    {
        std::ifstream file(it->second.c_str());
        if (!file.is_open())
        {
            response.setHeader("Content-Type", "text/plain");
            response.setBody(defaultBody);
        }
        else
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            response.setHeader("Content-Type", "text/html");
            response.setBody(buffer.str());
        }
    }
    else
    {
        response.setHeader("Content-Type", "text/plain");
        response.setBody(defaultBody);
    }
    response.setStatus(status);
    return response;
}

HttpResponse RequestHandler::autoindexResponse(
    const std::string &path,
    const std::string &requestPath)
{
    HttpResponse response;
    response.setStatus(HTTP_OK);
    response.setHeader("Content-Type", "text/html");
    response.setBody(generateAutoindex(path, requestPath));
    return response;
}

HttpResponse RequestHandler::redirect(
    HttpStatus status,
    const std::string &url)
{
    HttpResponse response;
    response.setStatus(status);
    response.setHeader("Location", url);
    response.setBody("");
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
    std::size_t start = 0;
    bool found = false;
    std::string boundary;
    while (start < contentType.size())
    {
        std::size_t end = contentType.find(';', start);
        std::string parameter;
        if (end == std::string::npos)
            parameter = contentType.substr(start);
        else
            parameter = contentType.substr(start, end - start);
        parameter = Utils::trim(parameter);
        std::size_t equal = parameter.find('=');
        if (equal != std::string::npos)
        {
            std::string name = Utils::toLower(Utils::trim(parameter.substr(0, equal)));
            std::string value = Utils::trim(parameter.substr(equal + 1));
            if (name == "boundary")
            {
                if (found)
                    return "";
                found = true;
                if (value.size() >= 2 && value[0] == '"' && value[value.size() - 1] == '"')
                    value = value.substr(1, value.size() - 2);
                else if (value.find('"') != std::string::npos)
                    return "";
                if (value.empty() || value.size() > 70)
                    return "";
                if (value.find('\r') != std::string::npos || value.find('\n') != std::string::npos)
                    return "";
                boundary = value;
            }
        }
        if (end == std::string::npos)
            break;
        start = end + 1;
    }
    return boundary;
}

bool RequestHandler::parseMultipart(
    const std::string &body,
    const std::string &boundary,
    std::vector<MultipartPart> &parts)
{
    parts.clear();
    std::string delimiter = "--" + boundary;
    std::size_t pos = body.find(delimiter);
    while (pos != std::string::npos)
    {
        pos += delimiter.size();
        if (body.compare(pos, 2, "--") == 0)
        {
            pos += 2;
            if (pos == body.size())
                return true;
            if (body.compare(pos, 2, "\r\n") == 0
                && pos + 2 == body.size())
                return true;
            return false;
        }
        if (body.compare(pos, 2, "\r\n") != 0)
            return false;
        pos += 2;
        std::size_t next = body.find(delimiter, pos);
        if (next == std::string::npos)
            return false;

        std::size_t partEnd = next;
        if (partEnd < 2)
            return false;
        if (body[partEnd - 2] != '\r' || body[partEnd - 1] != '\n')
            return false;
        partEnd -= 2;
        std::string part = body.substr(pos, partEnd - pos);
        std::size_t headerEnd = part.find("\r\n\r\n");
        if (headerEnd == std::string::npos)
            return false;
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
                return false;
            std::string key = line.substr(0, colon);
            if (key.empty())
                return false;
            for (std::size_t i = 0; i < key.size(); i++)
            {
                if (key[i] == ' ' || key[i] == '\t')
                    return false;
            }
            key = Utils::toLower(key);
            std::string value = Utils::trim(line.substr(colon + 1));
            current.headers[key] = value;
            if (endHeader == std::string::npos)
                break;
            startHeader = endHeader + 2;
        }
        std::map<std::string, std::string>::const_iterator it =
            current.headers.find("content-disposition");
        if (it == current.headers.end())
            return false;

        std::string disposition = it->second;
        std::string nameKey = "name=\"";
        std::string filenameKey = "filename=\"";

        std::size_t namePos = disposition.find(nameKey);
        if (namePos != std::string::npos)
        {
            std::size_t nameStart = namePos + nameKey.size();
            std::size_t nameEnd = disposition.find('"', nameStart);
            if (nameEnd == std::string::npos)
                return false;
            current.name = disposition.substr(nameStart, nameEnd - nameStart);
        }

        std::size_t filenamePos = disposition.find(filenameKey);
        if (filenamePos != std::string::npos)
        {
            std::size_t filenameStart = filenamePos + filenameKey.size();
            std::size_t filenameEnd = disposition.find('"', filenameStart);
            if (filenameEnd == std::string::npos)
                return false;
            current.filename = disposition.substr(filenameStart, filenameEnd - filenameStart);
        }
        parts.push_back(current);
        pos = next;
    }
    return false;
}

bool RequestHandler::writeFile(const std::string &path, const std::string &data)
{
    std::ofstream file(
        path.c_str(),
        std::ios::out | std::ios::binary);
    if (!file.is_open())
        return false;
    file.write(data.data(), data.size());
    if (!file)
        return false;
    return true;
}

std::string RequestHandler::generateAutoindex(const std::string &path, const std::string &requestPath)
{
    DIR *dir = opendir(path.c_str());
    if (dir == NULL)
        return "";
    struct dirent *entry;
    std::stringstream html;
    html << "<html>\n";
    html << "<body>\n";
    html << "<h1> Index of " << htmlEscape(requestPath) << "</h1>\n";
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name == "." || name == "..")
            continue;
        std::string href = requestPath;
        if (!href.empty() && href[href.size() - 1] != '/')
            href += "/";
        href += name;
        std::string entryPath = path + name;
        struct stat info;
        if (stat(entryPath.c_str(), &info) == -1)
        {
            closedir(dir);
            return "";
        }
        if (S_ISDIR(info.st_mode))
        {
            href += "/";
            name += "/";
        }
        html << "<a href=\""
             << urlEncodePath(href)
             << "\">"
             << htmlEscape(name)
             << "</a><br>\n";
    }
    html << "</body>\n";
    html << "</html>\n";
    closedir(dir);
    return html.str();
}

const LocationConfig *RequestHandler::findLocation(
    const ServerConfig &server,
    const std::string &requestPath) const
{
    const std::vector<LocationConfig> &locations = server.getLocations();
    const LocationConfig *best = NULL;
    for (std::size_t i = 0; i < locations.size(); ++i)
    {
        const std::string &locationPath = locations[i].getPath();
        if (locationPath.empty())
            continue;
        bool match = false;
        if (requestPath == locationPath)
            match = true;
        else if (requestPath + "/" == locationPath)
            match = true;
        else if (locationPath == "/")
            match = true;
        else if (requestPath.size() > locationPath.size() && requestPath.compare(0, locationPath.size(), locationPath) == 0 && (requestPath[locationPath.size()] == '/' || locationPath[locationPath.size() - 1] == '/'))
            match = true;
        if (match)
        {
            if (best == NULL || locationPath.size() > best->getPath().size())
                best = &locations[i];
        }
    }
    return best;
}

std::string RequestHandler::getRoot(const LocationConfig *location) const
{
    if (location != NULL)
        return location->getRoot();
    return _server.getRoot();
}

std::string RequestHandler::getIndex(const LocationConfig *location) const
{
    if (location != NULL)
        return location->getIndex();
    return _server.getIndex();
}

bool RequestHandler::getAutoindex(const LocationConfig *location) const
{
    if (location != NULL)
        return location->getAutoindex();
    return false;
}

bool RequestHandler::isMethodAllowed(
    const LocationConfig *location,
    const std::string &requestMethod)
{
    if (location == NULL)
        return true;
    const std::vector<std::string> &methods = location->getMethods();
    if (methods.empty())
        return true;
    for (std::size_t i = 0; i < methods.size(); ++i)
    {
        if (methods[i] == requestMethod)
            return true;
    }
    return false;
}

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
            break;
        start = end + 1;
    }
    return false;
}

bool RequestHandler::isSafeFilename(const std::string &filename)
{
    if (filename.empty())
        return false;
    if (filename == "." || filename == "..")
        return false;
    if (filename.find('/') != std::string::npos)
        return false;
    if (filename.find('\\') != std::string::npos)
        return false;
    for (std::size_t i = 0; i < filename.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(filename[i]);
        if (c < 32 || c == 127)
            return false;
    }
    return true;
}

std::string RequestHandler::htmlEscape(const std::string &value)
{
    std::string result;
    for (std::size_t i = 0; i < value.size(); ++i)
    {
        switch (value[i])
        {
            case '&':
                result += "&amp;";
                break;
            case '<':
                result += "&lt;";
                break;
            case '>':
                result += "&gt;";
                break;
            case '"':
                result += "&quot";
                break;
            case '\'':
                result += "&#39;";
                break;
            default:
                result += value[i];
                break;
        }
    }
    return result;
}

std::string RequestHandler::urlEncodePath(const std::string &value)
{
    static const char hex[] = "0123456789ABCDEF";
    std::string result;
    for (std::size_t i = 0; i < value.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(value[i]);
        bool unreserved =
            (c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9')
            || c == '-'
            || c == '.'
            || c == '_'
            || c == '~';
        if (unreserved || c == '/')
            result += static_cast<char>(c);
        else
        {
            result += '%';
            result += hex[(c >> 4) & 0x0F];
            result += hex[c & 0x0F];
        }
    }
    return result;
}

HttpResponse RequestHandler::handleGet(const HttpRequest &request, const LocationConfig *location)
{
    std::string index = getIndex(location);
    bool autoindex = getAutoindex(location);
    std::string path = buildPath(location, request.getPath());
    struct stat info;
    if (stat(path.c_str(), &info) == -1)
        return notFound();
    if (S_ISDIR(info.st_mode))
    {
        if (!path.empty() && path[path.size() - 1] != '/')
            path += "/";
        if (!index.empty())
        {
            std::string indexPath = path + index;
            struct stat indexInfo;
            if (stat(indexPath.c_str(), &indexInfo) == 0)
            {
                if (!S_ISREG(indexInfo.st_mode))
                    return forbidden();
                path = indexPath;
            }
            else
            {
                if (!autoindex)
                    return notFound();
                return autoindexResponse(path, request.getPath());
            }
        }
        else
        {
            if (!autoindex)
                return forbidden();
            return autoindexResponse(path, request.getPath());
        }
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
    response.setBody(buffer.str());
    return response;
}

HttpResponse RequestHandler::handlePost(const HttpRequest &request, const LocationConfig *location)
{
    std::string contentType = request.getHeader("content-type");
    std::size_t semicolon = contentType.find(';');
    std::string mediaType;
    if (semicolon == std::string::npos)
        mediaType = contentType;
    else
        mediaType = contentType.substr(0, semicolon);
    mediaType = Utils::toLower(Utils::trim(mediaType));
    if (mediaType == "multipart/form-data")
    {
        if (location == NULL)
            return forbidden();
        const std::string &uploadStore = location->getUploadStore();
        if (uploadStore.empty())
            return forbidden();
        std::string boundary = getBoundary(request);
        if (boundary.empty())
            return badRequest();
        std::vector<MultipartPart> parts;
        if (!parseMultipart(request.getBody(), boundary, parts))
            return badRequest();
        bool created = false;
        for (std::size_t i = 0; i < parts.size(); ++i)
        {
            if (!parts[i].filename.empty())
            {
                if (!isSafeFilename(parts[i].filename))
                    return badRequest();
                std::string filePath = uploadStore;
                if (!filePath.empty() && filePath[filePath.size() - 1] != '/')
                    filePath += "/";
                filePath += parts[i].filename;
                struct stat info;
                bool existed = (stat(filePath.c_str(), &info) == 0);
                if (existed && !S_ISREG(info.st_mode))
                    return forbidden();
                if (!writeFile(filePath, parts[i].data))
                    return internalServerError();
                if (!existed)
                    created = true;
            }
        }
        HttpResponse response;
        if (created)
            response.setStatus(HTTP_CREATED);
        else
            response.setStatus(HTTP_OK);
        response.setHeader("Content-Type", "text/plain");
        response.setBody("Upload successful");
        return response;
    }
    std::string path = buildPath(location, request.getPath());
    struct stat info;
    bool existed = (stat(path.c_str(), &info) == 0);
    if (existed && !S_ISREG(info.st_mode))
        return forbidden();
    if (!writeFile(path, request.getBody()))
        return internalServerError();
    HttpResponse response;
    if (existed)
        response.setStatus(HTTP_OK);
    else
        response.setStatus(HTTP_CREATED);
    response.setHeader("Content-Type", "text/plain");
    response.setBody("Upload successful");
    return response;
}

HttpResponse RequestHandler::handleDelete(const HttpRequest &request, const LocationConfig *location)
{
    std::string path = buildPath(location, request.getPath());
    struct stat info;
    if (stat(path.c_str(), &info) == -1)
        return notFound();
    if (!S_ISREG(info.st_mode))
        return forbidden();
    if (std::remove(path.c_str()) != 0)
        return internalServerError();
    HttpResponse response;
    response.setStatus(HTTP_OK);
    response.setHeader("Content-Type", "text/plain");
    response.setBody("Delete successful");
    return response;
}

const LocationConfig *RequestHandler::getLocation(
    const HttpRequest &request) const
{
    return findLocation(_server, request.getPath());
}

std::string RequestHandler::buildPath(const LocationConfig *location, const std::string &requestPath)
{
    std::string root;
    std::string relativePath;
    if (location == NULL)
    {
        root = _server.getRoot();
        relativePath = requestPath;
    }
    else
    {
        root = location->getRoot();
        const std::string &locationPath = location->getPath();
        if (locationPath.empty())
            return "";
        if (requestPath.size() < locationPath.size())
            return "";
        if (locationPath == "/")
            relativePath = requestPath;
        else if (locationPath[locationPath.size() - 1] == '/')
            relativePath = "/" + requestPath.substr(locationPath.size());
        else
            relativePath = requestPath.substr(locationPath.size());
    }
    if (!root.empty() && root[root.size() - 1] == '/' && !relativePath.empty() && relativePath[0] == '/')
        root.erase(root.size() - 1);
    return root + relativePath;
}

bool RequestHandler::preCheck(
    const HttpRequest &request,
    const LocationConfig *location,
    HttpResponse &response)
{
    const std::string &requestPath = request.getPath();
    if (hasParentTraversal(requestPath))
    {
        response = forbidden();
        return true;
    }
    if (location != NULL && location->getPath() == requestPath + "/")
    {
        response = redirect(
            HTTP_MOVED_PERMANENTLY,
            requestPath + "/");
        return true;
    }
    const std::string &requestMethod = request.getMethod();
    if (!isMethodAllowed(location, requestMethod))
    {
        response = methodNotAllowed(location);
        return true;
    }
    if (location != NULL && location->getRedirectCode() != 0)
    {
        response = redirect(
            static_cast<HttpStatus>(location->getRedirectCode()),
            location->getRedirectUrl());
        return true;
    }
    return false;
}

HttpResponse RequestHandler::handleResolved(const HttpRequest &request)
{
    const LocationConfig *location = findLocation(_server, request.getPath());

    const std::string &requestMethod = request.getMethod();
    if (requestMethod == "GET")
        return handleGet(request, location);
    if (requestMethod == "POST")
        return handlePost(request, location);
    if (requestMethod == "DELETE")
        return handleDelete(request, location);
    return notImplemented();
}

HttpResponse RequestHandler::handleError(HttpStatus status)
{
    switch (status)
    {
    case HTTP_BAD_REQUEST:
        return badRequest();
    case HTTP_FORBIDDEN:
        return forbidden();
    case HTTP_NOT_FOUND:
        return notFound();
    case HTTP_PAYLOAD_TOO_LARGE:
        return payloadTooLarge();
    case HTTP_URI_TOO_LONG:
        return uriTooLong();
    case HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE:
        return requestHeadersTooLarge();
    case HTTP_VERSION_NOT_SUPPORTED:
        return versionNotSupported();
    default:
        return internalServerError();
    }
}