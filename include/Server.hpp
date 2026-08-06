#ifndef SERVER_HPP
# define SERVER_HPP

#include "Client.hpp"
#include <map>

class Server
{
private:
    int _listenFd;
    std::map<int, Client> _clients;
public:
    Server();
    ~Server();

    void run();
};

#endif