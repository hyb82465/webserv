#include "LocationConfig.hpp"

LocationConfig::LocationConfig()
: path(""), 
    root(""), 
    methods(),
    autoindex(false), 
    upload_store(""), 
    redirectCode(0), 
    redirectUrl(""), 
    index(""), 
    cgi() 
{
    
}
LocationConfig::~LocationConfig(){}
LocationConfig::LocationConfig(const LocationConfig &other)
:path(other.path),
 root(other.root),
 methods(other.methods),
 autoindex(other.autoindex),
 upload_store(other.upload_store),
 redirectCode(other.redirectCode),
 redirectUrl(other.redirectUrl),
 index(other.index),
 cgi(other.cgi)
{

}
LocationConfig &LocationConfig::operator=(const LocationConfig &other)
{
    if (this != &other)
    {
        path = other.path;
        root = other.root;
        methods = other.methods;
        autoindex = other.autoindex;
        upload_store = other.upload_store;
        redirectCode = other.redirectCode;
        redirectUrl = other.redirectUrl;
        index = other.index;
        cgi = other.cgi;
    }
    return *this;
}
const std::string &LocationConfig::getPath() const
{
    return path;
}

const std::string &LocationConfig::getRoot() const
{
    return root;
}

const std::vector<std::string> &
LocationConfig::getMethods() const
{
    return methods;
}

bool LocationConfig::getAutoindex() const
{
    return autoindex;
}

const std::string &LocationConfig::getUploadStore() const
{
    return upload_store;
}

int LocationConfig::getRedirectCode() const
{
    return redirectCode;
}

const std::string &LocationConfig::getRedirectUrl() const
{
    return redirectUrl;
}

const std::string &LocationConfig::getIndex() const
{
    return index;
}

const std::map<std::string, std::string> &
LocationConfig::getCgi() const
{
    return cgi;
}

void LocationConfig::setPath(const std::string &path)
{
    this->path = path;
}

void LocationConfig::setRoot(const std::string &root)
{
    this->root = root;
}

void LocationConfig::setIndex(const std::string &index)
{
    this->index = index;
}

void LocationConfig::setAutoindex(bool autoindex)
{
    this->autoindex = autoindex;
}

void LocationConfig::setUploadStore(
    const std::string &upload_store)
{
    this->upload_store = upload_store;
}

void LocationConfig::setRedirectCode(int code)
{
    this->redirectCode = code;
}

void LocationConfig::setRedirectUrl(
    const std::string &url)
{
    this->redirectUrl = url;
}

void LocationConfig::addMethod(
    const std::string &method)
{
    this->methods.push_back(method);
}

void LocationConfig::addCgi(
    const std::string &extension,
    const std::string &executable)
{
    this->cgi[extension] = executable;
}