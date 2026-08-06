#include "Client.hpp"

Client::Client() : _fd(-1)
{}

Client::Client(int fd) : _fd(fd)
{}

Client::Client(const Client &other) : _fd(other._fd)
{}

Client &Client::operator=(const Client &other)
{
    if (this != &other)
        _fd = other._fd;
    return *this;
}

Client::~Client()
{}

int Client::getFd() const
{
    return _fd;
}
