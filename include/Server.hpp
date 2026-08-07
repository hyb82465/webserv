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
public:
    Server();
    ~Server();

    void run();
};

#endif