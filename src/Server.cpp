#include "Server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

Server::Server() : _listenFd(-1)
{
}

Server::~Server()
{
    if (_listenFd != -1)
    {
        close(_listenFd);
        std::cout << "socket closed" << std::endl;
    } 
}

void Server::run()
{
    // socket
    // int socket(int domain, int type, int protocol);
    _listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenFd == -1)
    {
        std::cerr << "socket failed" << std::endl;
        return ;
    }
    std::cout << "socket created, fd = " << _listenFd << std::endl;
    
    // bind
    /* int bind(int socketFd,
                const struct sockaddr *address,
                socklen_t addressLength); */
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);
    if (bind(
            _listenFd,
            reinterpret_cast<struct sockaddr *>(&addr),
            sizeof(addr)
        ) == -1)
    {
        std::cerr << "bind failed" << std::endl;
        return ;
    }
    std::cout << "socket bound to port 8080" << std::endl;

    // listen
    // int listen(int socketFd, int backlog);
    if (listen(_listenFd, SOMAXCONN) == -1)
    {
        std::cerr << "listen failed" << std::endl;
        return ;
    }
    std::cout << "server is listening on port 8080" << std::endl;

    // accept
    /* int accept(int socketFd,
                  struct sockaddr *clientAddress,
                  socklen_t *clientAddressLength); */
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    std::memset(&clientAddr, 0, sizeof(clientAddr));
    int clientFd = accept(
        _listenFd,
        reinterpret_cast<struct sockaddr *>(&clientAddr),
        &clientAddrLen
    );
    if (clientFd == -1)
    {
        std::cerr << "accept failed" << std::endl;
        return ;
    }
    std::cout << "client connected, fd = " << clientFd << std::endl;
    
    // recv
    /* ssize_t recv(int socketFd,
                    void *buffer,
                    size_t length,
                    int flags); */
    char buffer[4096];
    std::memset(buffer, 0, sizeof(buffer));
    ssize_t byteRead = recv(
        clientFd,
        buffer,
        sizeof(buffer) - 1,
        0
    );
    if (byteRead == -1)
    {
        std::cerr << "recv failed" << std::endl;
        close(clientFd);
        return ;
    }
    if (byteRead == 0)
    {
        std::cout << "client disconnected" << std::endl;
        close(clientFd);
        return ;
    }
    buffer[byteRead] = '\0';
    std::cout << "received " << byteRead << " bytes:" << std::endl;
    std::cout << buffer << std::endl;

    // send
    /* ssize_t send(int socketFd,
                    const void *buffer,
                    size_t length,
                    int flags); */
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 13\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "\r\n"
        "Hello World!\n";

    ssize_t bytesSent = send(
        clientFd,
        response.c_str(),
        response.size(),
        0
    );
    if (bytesSent == -1)
        std::cerr << "send failed" << std::endl;
    else
        std::cout << "sent " << bytesSent << " bytes" << std::endl;
    close(clientFd);
}