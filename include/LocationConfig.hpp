#ifndef LOCATIONCONFIG_HPP
# define LOCATIONCONFIG_HPP

#include <string>
#include <vector>
#include <map>

class LocationConfig
{
private:
    std::string path;
    std::string root;
    std::vector<std::string> methods;
    bool autoindex;
    std::string upload_store;
    int redirectCode;
    std::string redirectUrl;
    std::string index;
    std::map<std::string, std::string> cgi;
public:
    LocationConfig();
    ~LocationConfig();
    LocationConfig(const LocationConfig &other);
    LocationConfig &operator=(const LocationConfig &other);
    
    const std::string &getPath() const;
    const std::string &getRoot() const;
    const std::vector<std::string> &getMethods() const;
    bool getAutoindex() const;
    const std::string &getUploadStore() const;
    int getRedirectCode() const;
    const std::string &getRedirectUrl() const;
    const std::string &getIndex() const;
    const std::map<std::string, std::string> &getCgi() const;

    void setPath(const std::string &path);
    void setRoot(const std::string &root);
    void setIndex(const std::string &index);
    void setAutoindex(bool autoindex);
    void setUploadStore(const std::string &upload_store);
    void setRedirectCode(int code);
    void setRedirectUrl(const std::string &url);

    void addMethod(const std::string &method);
    void addCgi(const std::string &extension,
                const std::string &executable);
};

#endif