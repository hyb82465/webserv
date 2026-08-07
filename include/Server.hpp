#ifndef SERVER_HPP
# define SERVER_HPP

#include "Client.hpp"
#include <map>
#include <vector>

class Server
{
private:
    int _listenFd;
    std::map<int, Client> _clients;

    void removeClient(int fd, std::vector<struct pollfd> &pollFds, std::size_t i);
    bool setNonBlocking(int fd);
    bool setupServer();
    void acceptClient(std::vector<struct pollfd> &pollFds);
    void handleRead(int fd, std::vector<struct pollfd> &pollFds, std::size_t &i);
    void handleWrite(int fd, std::vector<struct pollfd> &pollFds, std::size_t &i);
public:
    Server();
    ~Server();

    void run();
};

#endif