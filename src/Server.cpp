#include "Server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <vector>
#include <utility>
#include <unistd.h>
#include <cstring>
#include <cstddef>
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
                    std::cerr << "accept failed" << std::endl;
                    ++i;
                    continue ;
                }
                std::cout << "client connected, fd = " << clientFd << std::endl;

                struct pollfd clientPollFd;
                clientPollFd.fd = clientFd;
                clientPollFd.events = POLLIN;
                clientPollFd.revents = 0;
                pollFds.push_back(clientPollFd);
                _clients.insert(std::make_pair(clientFd, Client(clientFd)));
                ++i;
            }
            else if (pollFds[i].revents & POLLIN)
            {
                // recv
                /* ssize_t recv(int socketFd,
                                void *buffer,
                                size_t length,
                                int flags); */
                char buffer[4096];
                std::memset(buffer, 0, sizeof(buffer));
                ssize_t byteRead = recv(
                    fd,
                    buffer,
                    sizeof(buffer) - 1,
                    0
                );
                if (byteRead <= 0)
                {
                    if (byteRead == 0)
                        std::cout << "client disconnected" << std::endl;
                    else
                        std::cerr << "recv failed" << std::endl;
                    close(fd);
                    _clients.erase(fd);
                    pollFds.erase(pollFds.begin() + i);
                    continue ;
                }
                std::map<int, Client>::iterator it = _clients.find(fd);
                if (it == _clients.end()) // should not happen
                {
                    std::cerr << "client not found" << std::endl;
                    close(fd);
                    pollFds.erase(pollFds.begin() + i);
                    continue ;
                }
                it->second.appendToReadBuffer(
                    buffer,
                    static_cast<std::size_t>(byteRead)
                );
                if (it->second.getReadBuffer().find("\r\n\r\n") == std::string::npos)
                {
                    std::cout << "request not complete yet" << std::endl;
                    ++i;
                    continue ;
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
            else if (pollFds[i].revents & POLLOUT)
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
                    continue ;
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
                    close(fd);
                    _clients.erase(fd);
                    pollFds.erase(pollFds.begin() + i);
                    continue ;
                }
                it->second.addBytesSent(static_cast<std::size_t>(bytesSent));
                if (it->second.getBytesSent() >= response.size())
                {
                    std::cout << "totally sent " << it->second.getBytesSent() << " bytes" << std::endl;
                    close(fd);
                    _clients.erase(fd);
                    pollFds.erase(pollFds.begin() + i);
                    continue ;
                }
                ++i;
            }
        }
    }
}