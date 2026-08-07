#include "Server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <utility>
#include <unistd.h>
#include <cstring>
#include <cstddef>
#include <fcntl.h>
#include <cerrno>
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

void Server::removeClient(int fd, std::vector<struct pollfd> &pollFds, std::size_t i)
{
    close(fd);
    _clients.erase(fd);
    pollFds.erase(pollFds.begin() + i);
}

bool Server::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return false;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        return false;
    return true;
}

bool Server::setupServer()
{
    // socket
    // int socket(int domain, int type, int protocol);
    _listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenFd == -1)
    {
        std::cerr << "socket failed" << std::endl;
        return false;
    }
    std::cout << "socket created, fd = " << _listenFd << std::endl;
    
    // non-blocking
    if (!setNonBlocking(_listenFd))
    {
        std::cerr << "failed to set listen socket non-blocking" << std::endl;
        return false;
    }

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
        return false;
    }

    // listen
    // int listen(int socketFd, int backlog);
    if (listen(_listenFd, SOMAXCONN) == -1)
    {
        std::cerr << "listen failed" << std::endl;
        return false;
    }
    return true;
}

void Server::acceptClient(std::vector<struct pollfd> &pollFds)
{
    // accept
    /* int accept(int socketFd,
                struct sockaddr *clientAddress,
                socklen_t *clientAddressLength); */
    // struct sockaddr_in clientAddr;
    // socklen_t clientAddrLen = sizeof(clientAddr);
    // std::memset(&clientAddr, 0, sizeof(clientAddr));
    // int clientFd = accept(
    //     _listenFd,
    //     reinterpret_cast<struct sockaddr *>(&clientAddr),
    //     &clientAddrLen
    // );
    int clientFd = accept(_listenFd, NULL, NULL);
    if (clientFd == -1)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            std::cerr << "accept failed" << std::endl;
        return ;
    }

    // non-blocking
    if (!setNonBlocking(clientFd))
    {
        std::cerr << "failed to set client socket non-blocking" << std::endl;
        close(clientFd);
        return ;
    }

    struct pollfd clientPollFd;
    clientPollFd.fd = clientFd;
    clientPollFd.events = POLLIN;
    clientPollFd.revents = 0;
    pollFds.push_back(clientPollFd);
    _clients.insert(std::make_pair(clientFd, Client(clientFd)));
     std::cout << "client connected, fd = " << clientFd << std::endl;
}

void Server::handleRead(int fd, std::vector<struct pollfd> &pollFds, std::size_t &i)
{
    // recv
    /* ssize_t recv(int socketFd,
                    void *buffer,
                    size_t length,
                    int flags); */
    char buffer[4096];
    ssize_t byteRead = recv(fd, buffer, sizeof(buffer), 0);
    if (byteRead == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            ++i;
        else
            removeClient(fd, pollFds, i);
        return ;
    }
    else if (byteRead == 0)
    {
        std::cout << "client disconnected" << std::endl;
        removeClient(fd, pollFds, i);
        return ;
    }
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end()) // should not happen
    {
        std::cerr << "client not found" << std::endl;
        close(fd);
        pollFds.erase(pollFds.begin() + i);
        return ;
    }
    it->second.appendToReadBuffer(buffer, static_cast<std::size_t>(byteRead));
    if (it->second.getReadBuffer().find("\r\n\r\n") == std::string::npos)
    {
        std::cout << "request not complete yet" << std::endl;
        ++i;
        return ;
    }
    std::cout << it->second.getReadBuffer() << std::endl;
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 13\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "\r\n"
        "Hello World!\n";
    it->second.setWriteBuffer(response);
    pollFds[i].events = POLLOUT;
    ++i;
}

void Server::handleWrite(int fd, std::vector<struct pollfd> &pollFds, std::size_t &i)
{
    // send
    /* ssize_t send(int socketFd,
                    const void *buffer,
                    size_t length,
                    int flags); */
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end()) // should not happen
    {
        std::cerr << "client not found" << std::endl;
        close(fd);
        pollFds.erase(pollFds.begin() + i);
        return ;
    }
    const std::string &response = it->second.getWriteBuffer();
    ssize_t bytesSent = send(
        fd,
        response.c_str() + it->second.getBytesSent(),
        response.size() - it->second.getBytesSent(),
        0
    );
    if (bytesSent == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            ++i;
        else
        {
            std::cerr << "send failed" << std::endl;
            removeClient(fd, pollFds, i);
        }
        return ;
    }
    it->second.addBytesSent(static_cast<std::size_t>(bytesSent));
    if (it->second.getBytesSent() >= response.size())
    {
        std::cout << "totally sent " << it->second.getBytesSent() << " bytes" << std::endl;
        removeClient(fd, pollFds, i);
        return ;
    }
    ++i;
}

void Server::run()
{
    if (!setupServer())
        return ;
    std::vector<struct pollfd> pollFds;
    struct pollfd listenPollFd;
    listenPollFd.fd = _listenFd;
    listenPollFd.events = POLLIN;
    listenPollFd.revents = 0;
    pollFds.push_back(listenPollFd);

    while (true)
    {
        // poll
        // int poll(struct pollfd *fds, nfds_t nfds, int timeout);
        int readyCount = poll(&pollFds[0], pollFds.size(), -1);
        if (readyCount == -1)
        {
            std::cerr << "poll failed" << std::endl;
            return ;
        }
        for (std::size_t i = 0; i < pollFds.size(); )
        {
            if (pollFds[i].revents == 0)
            {
                ++i;
                continue ;
            }
                
            int fd = pollFds[i].fd;
            if (fd == _listenFd
                && (pollFds[i].revents & POLLIN))
            {
                acceptClient(pollFds);
                ++i;
            }
            else if (pollFds[i].revents & POLLIN)
            {
                handleRead(fd, pollFds, i);
            }
            else if (pollFds[i].revents & POLLOUT)
            {
                handleWrite(fd, pollFds, i);
            }
        }
    }
}