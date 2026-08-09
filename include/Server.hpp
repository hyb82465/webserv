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
    std::vector<struct pollfd> _pollFds;

    Server(const Server &other);
    Server &operator=(const Server &other);

    bool setNonBlocking(int fd);
    bool setupServer();
    void removeClient(int fd, std::size_t i);
    void acceptClient();
    void handleRead(int fd, std::size_t &i);
    void handleWrite(int fd, std::size_t &i);
public:
    Server();
    ~Server();

    void run();
};

#endif