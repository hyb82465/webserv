#ifndef CONFIGVALIDATOR_HPP
#define CONFIGVALIDATOR_HPP

#include <vector>
#include <string>

#include "ServerConfig.hpp"
#include "LocationConfig.hpp"

class ConfigValidator
{
public:
    ConfigValidator();
    ~ConfigValidator();

    void validate(const std::vector<ServerConfig> &servers) const;

private:
    void validateServer(const ServerConfig &server) const;
    void validateListens(const ServerConfig &server) const;
    void validateRoot(const ServerConfig &server) const;
    void validateIndex(const ServerConfig &server) const;
    void validateErrorPages(const ServerConfig &server) const;
    void validateLocations(const ServerConfig &server) const;
    void validateLocation(const LocationConfig &location) const;
    void validateMethods(const LocationConfig &location) const;
    void validateRedirect(const LocationConfig &location) const;
    void validateUpload(const LocationConfig &location) const;
    void validateCgi(const LocationConfig &location) const;

    bool isValidMethod(const std::string &method) const;
};
#endif