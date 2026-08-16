#ifndef LISTENCONFIG_HPP
#define LISTENCONFIG_HPP

#include <string>

class ListenConfig
{
    private:
        std::string host;
        int port;
    
    public:
        ListenConfig();
        ListenConfig(const std::string &host, int port);
        ~ListenConfig();

        ListenConfig(const ListenConfig &other);
        ListenConfig &operator=(const ListenConfig &other);

        const std::string &getHost()const;
        int getPort() const;

        void setHost(const std::string &host);
        void setPort(int port);

};
#endif