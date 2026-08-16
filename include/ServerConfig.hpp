#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

#include <string>
#include "LocationConfig.hpp"
#include "ListenConfig.hpp"
#include <vector>
#include <map>

class ServerConfig
{
    private:
        std::vector<ListenConfig> listens;
        std::string root;
        std::string index;
        size_t client_max_body_size;
        std::map<int, std::string> error_pages;
        std::vector<LocationConfig> locations;

    public:
        ServerConfig();
        ~ServerConfig();
        ServerConfig(const ServerConfig &other);
        ServerConfig &operator=(const ServerConfig &other);

        // Getters
        const std::vector<ListenConfig> &getListens() const;
        const std::string &getRoot() const;
        const std::string &getIndex() const;
        size_t getClientMaxBodySize() const;
        const std::map<int, std::string> &getErrorPages() const;
        const std::vector<LocationConfig> &getLocations() const;

        // Setters
        void setRoot(const std::string &root);
        void setIndex(const std::string &index);
        void setClientMaxBodySize(size_t size);

        // Add configuration
        void addListen(const ListenConfig &listen);
        void addLocation(const LocationConfig &location);
        void addErrorPage(int code, const std::string &path);

        void applyDefaultsToLocations();
};

#endif 