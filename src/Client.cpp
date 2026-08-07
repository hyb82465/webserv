#include "Client.hpp"

Client::Client() : _fd(-1)
{}

Client::Client(int fd) : _fd(fd), _readBuffer("")
{}

Client::Client(const Client &other) : _fd(other._fd), _readBuffer("")
{}

Client &Client::operator=(const Client &other)
{
    if (this != &other)
    {
        _fd = other._fd;
        _readBuffer = other._readBuffer;
    }
    return *this;
}

Client::~Client()
{}

int Client::getFd() const
{
    return _fd;
}

const std::string &Client::getReadBuffer() const
{
    return _readBuffer;
}

void Client::appendToReadBuffer(const char *data, std::size_t length)
{
    _readBuffer.append(data, length);
}
