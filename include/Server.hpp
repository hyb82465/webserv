#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include "ServerConfig.hpp"
#include "HttpResponse.hpp"
#include "RequestHandler.hpp"
#include <map>
#include <vector>
#include <string>
#include <poll.h>

class CgiHandler;

class Server
{
private:
    std::vector<int> _listenFds;
    std::map<int, std::size_t> _listenServerMap;
    std::map<int, int> _listenPortMap;
    std::map<int, Client> _clients;
    std::vector<struct pollfd> _pollFds;

    std::map<int, CgiHandler*> _cgiFds;
    std::vector<CgiHandler*> _cgiHandlers;

    std::vector<ServerConfig> _configs;

    Server();
    Server(const Server& other);
    Server& operator=(const Server& other);

    bool isListenFd(int fd) const;
    bool setNonBlocking(int fd);
    int setupListenSocket(const ListenConfig& listenConfig);
    void removeClient(int fd, std::size_t i);
    void queueResponse(Client &client, std::size_t pollIndex, HttpResponse &response);
    ParseResult parseClientRequest(
        Client &client,
        const ServerConfig &config,
        RequestHandler &handler,
        const LocationConfig *&location);
    bool tryStartCgi(
        int clientFd,
        std::size_t pollIndex,
        Client &client,
        const LocationConfig *location,
        RequestHandler &handler);
    void processRequest(int fd, std::size_t& i);
    void acceptClient(int listenFd);
    void handleRead(int fd, std::size_t& i);
    void handleWrite(int fd, std::size_t& i);

    void startCgi(int clientFd,
                  HttpRequest& request,
                  const std::string& scriptPath,
                  const std::string& executable);
    void handleCgiWrite(int fd, std::size_t& i);
    void handleCgiRead(int fd, std::size_t& i);
    CgiHandler* getCgiByFd(int fd);
    void addCgi(CgiHandler* cgi);
    void removeCgi(CgiHandler* cgi);
    void finishCgi(CgiHandler* cgi);
    void removePollFd(int fd);
    void addCgiPollFds(CgiHandler* cgi);
    void checkCgiChildren();
    void removeCgiByClientFd(int clientFd);

    std::string findCgiExecutable(const std::string& path, const LocationConfig& location) const;
    void buildCgiResponse(std::string &response, bool keepAlive) const;

    void checkCgiTimeouts();
    void checkClientTimeouts();

public:
    Server(const std::vector<ServerConfig>& _configs);
    ~Server();

    void run();
};

#endif