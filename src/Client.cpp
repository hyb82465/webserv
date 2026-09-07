#include "Client.hpp"

Client::Client(int fd, std::size_t serverIndex, int serverPort, const std::string &remoteAddr, const std::string &serverName)
    : _fd(fd),
    _readBuffer(""),
    _writeBuffer(""),
    _bytesSent(0),
    _serverIndex(serverIndex),
    _serverPort(serverPort),
    _remoteAddr(remoteAddr),
    _serverName(serverName),
    _lastActivity(std::time(NULL))
{   
}

Client::Client(const Client &other)
    : _fd(other._fd),
      _readBuffer(other._readBuffer),
      _writeBuffer(other._writeBuffer),
      _bytesSent(other._bytesSent),
      _serverIndex(other._serverIndex),
      _serverPort(other._serverPort),
      _remoteAddr(other._remoteAddr),
      _serverName(other._serverName),   
      _lastActivity(other._lastActivity),
      _requestState(other._requestState)
{
}

Client &Client::operator=(const Client &other)
{
    if (this != &other)
    {
        _fd = other._fd;
        _readBuffer = other._readBuffer;
        _writeBuffer = other._writeBuffer;
        _bytesSent = other._bytesSent;
        _serverIndex = other._serverIndex;
        _serverPort = other._serverPort;
        _remoteAddr = other._remoteAddr;
        _serverName = other._serverName;
        _lastActivity = other._lastActivity;
        _requestState = other._requestState;
    }
    return *this;
}

Client::~Client()
{
}

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

int Client::getServerPort() const
{
    return _serverPort;
}

const std::string &Client::getRemoteAddr() const
{
    return _remoteAddr;
}

const std::string &Client::getServerName() const
{
    return _serverName;
}

RequestState &Client::getRequestState()
{
    return _requestState;
}

const RequestState &Client::getRequestState() const
{
    return _requestState;
}

void Client::resetRequestState()
{
    _requestState.reset();
    updateLastActivity();
}

void Client::clearWriteBuffer()
{
    std::string empty;
    _writeBuffer.swap(empty);
}

void Client::resetBytesSent()
{
    _bytesSent = 0;
}

void Client::consumeReadBuffer(std::size_t count)
{
    std::string remaining;
    if (count < _readBuffer.size())
        remaining = _readBuffer.substr(count);
    _readBuffer.swap(remaining);
}

void Client::appendToReadBuffer(const char *data, std::size_t length)
{
    _readBuffer.append(data, length);
    updateLastActivity();
}

void Client::setWriteBuffer(const std::string &data)
{
    _writeBuffer = data;
    _bytesSent = 0;
    updateLastActivity();
}

void Client::addBytesSent(std::size_t amount)
{
    _bytesSent += amount;
    updateLastActivity();
}

std::time_t Client::getLastActivity() const
{
    return _lastActivity;
}

void Client::updateLastActivity()
{
    _lastActivity = std::time(NULL);
}

void Client::swapWriteBuffer(std::string &data)
{
    _writeBuffer.swap(data);
    _bytesSent = 0;
    updateLastActivity();
}