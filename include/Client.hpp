#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>
# include <cstddef>

class Client
{
private:
    Client();

    int _fd;
    std::string _readBuffer;
    std::string _writeBuffer;
    std::size_t _bytesSent;
    std::size_t _serverIndex;
public:
    Client(int fd, std::size_t serverIndex);
    Client(const Client &other);
    Client &operator=(const Client &other);
    ~Client();

    int getFd() const;
    const std::string &getReadBuffer() const;
    const std::string &getWriteBuffer() const;
    std::size_t getBytesSent() const;
    std::size_t getServerIndex() const;

    void appendToReadBuffer(const char *data, std::size_t length);
    void setWriteBuffer(const std::string &data);
    void addBytesSent(std::size_t amount);
};

#endif