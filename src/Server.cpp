#include "Server.hpp"
#include "HttpRequest.hpp"
#include "RequestParser.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <utility>
#include <unistd.h>
#include <string>
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
    for (std::size_t i = 0; i < _pollFds.size(); ++i)
    {
        if (_pollFds[i].fd != -1)
            close(_pollFds[i].fd);
    }
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
    struct pollfd listenPollFd;
    listenPollFd.fd = _listenFd;
    listenPollFd.events = POLLIN;
    listenPollFd.revents = 0;
    _pollFds.push_back(listenPollFd);
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

void Server::removeClient(int fd, std::size_t i)
{
    close(fd);
    _clients.erase(fd);
    _pollFds.erase(_pollFds.begin() + i);
}

void Server::acceptClient()
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

    _pollFds.push_back(clientPollFd);
    _clients.insert(std::make_pair(clientFd, Client(clientFd)));
     std::cout << "client connected, fd = " << clientFd << std::endl;
}

void Server::handleRead(int fd, std::size_t &i)
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
        std::cerr << "recv failed" << std::endl;
        removeClient(fd, i);
        return ;
    }
    else if (byteRead == 0)
    {
        std::cout << "client disconnected" << std::endl;
        removeClient(fd, i);
        return ;
    }
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end()) // should not happen
    {
        std::cerr << "client not found" << std::endl;
        removeClient(fd, i);
        return ;
    }
    it->second.appendToReadBuffer(buffer, static_cast<std::size_t>(byteRead));
    HttpRequest request;
    RequestParser parser;
    ParseResult result = parser.parse(it->second.getReadBuffer(), request);
    if (result == PARSE_INCOMPLETE)
    {
        std::cout << "request incomplete" << std::endl;
        ++i;
        return ;
    }
    else if (result == PARSE_ERROR)
    {
        std::cout << "parse error, status = "
                  << request.getStatus()
                  << std::endl;
        ++i;
        return ;
    }
    
    std::cout << "method: "
              << request.getMethod()
              << std::endl;
    std::cout << "path: "
              << request.getPath()
              << std::endl;
    std::cout << "body: "
              << request.getBody()
              << std::endl;

    std::cout << it->second.getReadBuffer() << std::endl;

    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 13\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "\r\n"
        "Hello World!\n";
    it->second.setWriteBuffer(response);
    _pollFds[i].events = POLLOUT;
    ++i;
}

void Server::handleWrite(int fd, std::size_t &i)
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
        removeClient(fd, i);
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
        std::cerr << "send failed" << std::endl;
        removeClient(fd, i);
        return ;
    }
    it->second.addBytesSent(static_cast<std::size_t>(bytesSent));
    if (it->second.getBytesSent() >= response.size())
    {
        std::cout << "totally sent " << it->second.getBytesSent() << " bytes" << std::endl;
        removeClient(fd, i);
        return ;
    }
    ++i;
}

void Server::run()
{
    if (!setupServer())
        return ;
    while (true)
    {
        // poll
        // int poll(struct pollfd *fds, nfds_t nfds, int timeout);
        int readyCount = poll(&_pollFds[0], _pollFds.size(), -1);
        if (readyCount == -1)
        {
            std::cerr << "poll failed" << std::endl;
            return ;
        }
        for (std::size_t i = 0; i < _pollFds.size(); )
        {
            if (_pollFds[i].revents == 0)
            {
                ++i;
                continue ;
            }    
            int fd = _pollFds[i].fd;
            if (fd == _listenFd
                && _pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                std::cerr << "listen socket error" << std::endl;
                return ;
            }
            if (fd != _listenFd
                && _pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                std::cerr << "client connection closed or invalid" << std::endl;
                removeClient(fd, i);
                continue ;
            }
            if (fd == _listenFd
                && (_pollFds[i].revents & POLLIN))
            {
                acceptClient();
                ++i;
            }
            else if (_pollFds[i].revents & POLLIN)
                handleRead(fd, i);
            else if (_pollFds[i].revents & POLLOUT)
                handleWrite(fd, i);
            else
                ++i;
        }
    }
}