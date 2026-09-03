#include "ConfigValidator.hpp"

#include <stdexcept>
#include <map>

ConfigValidator::ConfigValidator() {}
ConfigValidator::~ConfigValidator() {}

void ConfigValidator::validate(const std::vector<ServerConfig> &servers) const
{
    if (servers.empty())
        throw std::runtime_error("Configuration mest contain at least one server");

    for (size_t i = 0; i < servers.size(); ++i)
        validateServer(servers[i]);
}

void ConfigValidator::validateServer(const ServerConfig &server) const
{
    validateListens(server);
    validateRoot(server);
    validateIndex(server);
    validateErrorPages(server);
    validateLocations(server);
}

void ConfigValidator::validateListens(const ServerConfig &server) const
{
    const std::vector<ListenConfig> &listens = server.getListens();

    if (listens.empty())
        throw std::runtime_error("server mest contain at least one listen");
    for (size_t i = 0; i < listens.size(); ++i)
    {
        const ListenConfig &listen = listens[i];

        if (listen.getHost().empty())
            throw std::runtime_error("Listen interface cannot be empty");
        if (listen.getPort() < 1 || listen.getPort() > 65535)
            throw std::runtime_error("listen port is out of range");
    }

    for (size_t i = 0; i < listens.size(); ++i)
    {
        for (size_t j = i + 1; j < listens.size(); ++j)
        {
            if (listens[i].getHost() == listens[j].getHost() && listens[i].getPort() == listens[j].getPort())
                throw std::runtime_error("Duplicate listen address");
        }
    }
}

void ConfigValidator::validateRoot(const ServerConfig &server) const
{
    if (server.getRoot().empty())
        throw std::runtime_error("Server root cannot be empty");
    // if (server.getRoot()[0] != '/')
    //     throw std::runtime_error("Server root must start with '/'");
}

void ConfigValidator::validateIndex(const ServerConfig &server) const
{
    if (server.getIndex().empty())
        throw std::runtime_error("Server index cannot be empty");
}

void ConfigValidator::validateErrorPages(const ServerConfig &server) const
{
    const std::map<int, std::string> &errorPages = server.getErrorPages();

    std::map<int, std::string>::const_iterator it;
    for (it = errorPages.begin(); it != errorPages.end(); ++it)
    {
        int code = it->first;
        if (code < 400 || code > 599)
            throw std::runtime_error("Invalid error page status code");
        if (it->second.empty())
            throw std::runtime_error("Error page path cannot be empty");
    }
}

void ConfigValidator::validateLocations(const ServerConfig &server) const
{
    const std::vector<LocationConfig> &locations = server.getLocations();

    for (size_t i = 0; i < locations.size(); ++i)
    {
        validateLocation(locations[i]);
        for (size_t j = i + 1; j < locations.size(); ++j)
        {
            if (locations[i].getPath() == locations[j].getPath())
            {
                throw std::runtime_error(
                    "Duplicate location path: " + locations[i].getPath());
            }
        }
    }
        
}

void ConfigValidator::validateLocation(const LocationConfig &location) const
{
    if (location.getPath().empty())
        throw std::runtime_error("Location path cannot be empty");
    if (location.getPath()[0] != '/')
        throw std::runtime_error("Location path must start with'/'");

    validateMethods(location);
    validateRedirect(location);
    validateUpload(location);
    validateCgi(location);
}

void ConfigValidator::validateMethods(const LocationConfig &location) const
{
    const std::vector<std::string> &methods = location.getMethods();

    for (size_t i = 0; i < methods.size(); ++i)
    {
        if (!isValidMethod(methods[i]))
            throw std::runtime_error("Invalide HTTP method");
        for (size_t j = i + 1; j < methods.size(); ++j)
        {
            if (methods[i] == methods[j])
                throw std::runtime_error("Duplicate HTTP method");
        }
    }
}

void ConfigValidator::validateRedirect(const LocationConfig &location) const
{
    int code = location.getRedirectCode();

    const std::string &url = location.getRedirectUrl();

    if (code == 0)
    {
        if (!url.empty())
            throw std::runtime_error("Redirect URL existd without redirect code");
        return;
    }
    if (code < 300 || code > 399)
        throw std::runtime_error("Invalide redirect status code");
    if (url.empty())
        throw std::runtime_error("Redirect URL cannot be empty");
}

void ConfigValidator::validateUpload(const LocationConfig &location) const
{
    const std::string &uploadStore = location.getUploadStore();

    if (!uploadStore.empty() && location.getMethods().empty())
        throw std::runtime_error("Upload location must define accepted methods");
}

void ConfigValidator::validateCgi(const LocationConfig &location) const
{
    const std::map<std::string, std::string> &cgi = location.getCgi();
    std::map<std::string, std::string>::const_iterator it;
    for (it = cgi.begin(); it != cgi.end(); ++it)
    {
        const std::string &extension = it->first;
        const std::string &executable = it->second;

        if (extension.empty() || extension[0] != '.')
            throw std::runtime_error("Invalid CGI extension");
        if (executable.empty())
            throw std::runtime_error("CGI executable cannot be empty");
    }
}

bool ConfigValidator::isValidMethod(const std::string &method) const
{
    return method == "GET" || method == "POST" || method == "DELETE";
}