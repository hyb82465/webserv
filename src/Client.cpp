#include "Client.hpp"

Client::Client(int fd, std::size_t serverIndex)
    : _fd(fd), _readBuffer(""), _writeBuffer(""), _bytesSent(0), _serverIndex(serverIndex)
{}

Client::Client(const Client &other)
    : _fd(other._fd),
      _readBuffer(other._readBuffer),
      _writeBuffer(other._writeBuffer),
      _bytesSent(other._bytesSent),
      _serverIndex(other._serverIndex)
{}

Client &Client::operator=(const Client &other)
{
    if (this != &other)
    {
        _fd = other._fd;
        _readBuffer = other._readBuffer;
        _writeBuffer = other._writeBuffer;
        _bytesSent = other._bytesSent;
        _serverIndex = other._serverIndex;
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

const std::string &Client::getWriteBuffer() const
{
    return _writeBuffer;
}

std::size_t Client::getBytesSent() const
{
    return _bytesSent;
}

std::size_t Client::getServerIndex() const
{
    return _serverIndex;
}

void Client::appendToReadBuffer(const char *data, std::size_t length)
{
    _readBuffer.append(data, length);
}

void Client::setWriteBuffer(const std::string &data)
{
    _writeBuffer = data;
    _bytesSent = 0;
}

void Client::addBytesSent(std::size_t amount)
{
    _bytesSent += amount;
}