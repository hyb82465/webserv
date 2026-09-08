#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "RequestState.hpp"
#include <string>
#include <cstddef>
#include <ctime>

class Client
{
private:
    Client();

    int _fd;
    std::string _readBuffer;
    std::string _writeBuffer;
    std::size_t _bytesSent;
    std::size_t _serverIndex;
    int _serverPort;
    std::string _remoteAddr;
    std::string _serverName;
    std::time_t _lastActivity;

    RequestState _requestState;

public:
    Client(int fd, std::size_t serverIndex, int serverPort, const std::string &remoteAddr,const std::string &serverName);
    Client(const Client &other);
    Client &operator=(const Client &other);
    ~Client();

    int getFd() const;
    const std::string &getReadBuffer() const;
    const std::string &getWriteBuffer() const;
    std::size_t getBytesSent() const;
    std::size_t getServerIndex() const;
    int getServerPort() const;
    const std::string &getRemoteAddr() const;
    const std::string &getServerName() const;
    RequestState &getRequestState();
    const RequestState &getRequestState() const;
    void resetRequestState();
    void clearWriteBuffer();
    void resetBytesSent();
    void consumeReadBuffer(std::size_t count);

    void appendToReadBuffer(const char *data, std::size_t length);
    void swapWriteBuffer(std::string &data);
    void addBytesSent(std::size_t amount);

    std::time_t getLastActivity() const;
    void updateLastActivity();
};

#endif