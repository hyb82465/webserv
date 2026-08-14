#ifndef LOCATIONCONFIG_HPP
# define LOCATIONCONFIG_HPP

#include <string>
#include <vector>

class LocationConfig
{
public:
    LocationConfig();

    std::string path;
    std::string root;
    std::vector<std::string> methods;
    bool autoindex;
    std::string upload_store;
    int redirectCode;
    std::string redirectUrl;
};

#endif